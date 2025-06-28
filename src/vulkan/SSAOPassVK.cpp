#include "common.h"
#include "vulkan/utilsVK.h"
#include "vulkan/SSAOPassVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/windowVK.h"
#include "runtime.h"
#include "frame.h"
#include "shaderRegistry.h"
#include "entity.h"
#include "vulkan/meshVK.h"
#include "material.h"


using namespace MiniEngine;


SSAOPassVK::SSAOPassVK(
    const Runtime& i_runtime,
    const ImageBlock& i_depth_buffer,
    const ImageBlock& i_normals_attachment,
    const ImageBlock& i_position_attachment,
    const ImageBlock& i_ssao_attachment) :
    RenderPassVK(i_runtime),
    m_depth_buffer(i_depth_buffer),
    m_normals_attachment(i_normals_attachment),
    m_position_attachment(i_position_attachment),
    m_ssao_attachment(i_ssao_attachment)
{
    for (auto cmd : m_command_buffer)
    {
        cmd = VK_NULL_HANDLE;
    }
}


SSAOPassVK::~SSAOPassVK()
{
}


bool SSAOPassVK::initialize()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    m_entities_to_draw = {
                            { static_cast<uint32_t>(Material::TMaterial::Diffuse), {} },
                            { static_cast<uint32_t>(Material::TMaterial::Microfacets), {} }
    };

    //SHADER STAGES
    {
        VkShaderModule vert_module = m_runtime.m_shader_registry->loadShader("./shaders/ssao_v.spv", VK_SHADER_STAGE_VERTEX_BIT);
        VkShaderModule frag_module = m_runtime.m_shader_registry->loadShader("./shaders/ssao_f.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        {
            VkPipelineShaderStageCreateInfo vert_shader{};
            vert_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_shader.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vert_shader.module = vert_module;
            vert_shader.pName = "main";

            VkPipelineShaderStageCreateInfo frag_shader{};
            frag_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_shader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_shader.module = frag_module;
            frag_shader.pName = "main";

            m_pipelines[static_cast<uint32_t>(Material::TMaterial::Diffuse)].m_shader_stages[0] = vert_shader;
            m_pipelines[static_cast<uint32_t>(Material::TMaterial::Microfacets)].m_shader_stages[0] = vert_shader;
            m_pipelines[static_cast<uint32_t>(Material::TMaterial::Diffuse)].m_shader_stages[1] = frag_shader;
            m_pipelines[static_cast<uint32_t>(Material::TMaterial::Microfacets)].m_shader_stages[1] = frag_shader;
        }
    }

    generateKernelSamples();
    generateNoiseTexture();

    createRenderPass();
    createPipelines();



    createFbo();

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};

    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = renderer.getDevice()->getCommandPool();
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 3;

    vkAllocateCommandBuffers(renderer.getDevice()->getLogicalDevice(), &commandBufferAllocateInfo, m_command_buffer.data());

    return true;
}


void SSAOPassVK::shutdown()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    vkFreeCommandBuffers(renderer.getDevice()->getLogicalDevice(), renderer.getDevice()->getCommandPool(), m_command_buffer.size(), m_command_buffer.data());

    vkDestroyDescriptorPool(renderer.getDevice()->getLogicalDevice(), m_descriptor_pool, nullptr);

    for (auto& pipeline : m_pipelines)
    {
        vkDestroyDescriptorSetLayout(renderer.getDevice()->getLogicalDevice(), pipeline.m_descriptor_set_layout[0], nullptr);
        vkDestroyDescriptorSetLayout(renderer.getDevice()->getLogicalDevice(), pipeline.m_descriptor_set_layout[1], nullptr);
        vkDestroyPipeline(renderer.getDevice()->getLogicalDevice(), pipeline.m_pipeline, nullptr);
        vkDestroyPipelineLayout(renderer.getDevice()->getLogicalDevice(), pipeline.m_pipeline_layouts, nullptr);
    }


    for (uint32 id = 0; id < static_cast<uint32>(renderer.getWindow().getImageCount()); id++)
    {
        vkDestroyFramebuffer(renderer.getDevice()->getLogicalDevice(), m_fbos[id], nullptr);
    }

    vkDestroyBuffer(renderer.getDevice()->getLogicalDevice(), m_kernelBuffer, nullptr);
    vkFreeMemory(renderer.getDevice()->getLogicalDevice(), m_kernelMemory, nullptr);

    vkDestroyImage(renderer.getDevice()->getLogicalDevice(), m_noise.m_image, nullptr);
    vkDestroyImageView(renderer.getDevice()->getLogicalDevice(), m_noise.m_image_view, nullptr);
    vkDestroySampler(renderer.getDevice()->getLogicalDevice(), m_noise.m_sampler, nullptr);
    vkFreeMemory(renderer.getDevice()->getLogicalDevice(), m_noise.m_memory, nullptr);

    vkDestroyRenderPass(renderer.getDevice()->getLogicalDevice(), m_render_pass, nullptr);
}


VkCommandBuffer SSAOPassVK::draw(const Frame& i_frame)
{
    RendererVK& renderer = *m_runtime.m_renderer;

    VkCommandBuffer& current_cmd = m_command_buffer[renderer.getWindow().getCurrentImageId()];

    if (current_cmd != VK_NULL_HANDLE)
    {
        VkCommandBufferResetFlags flags{};
        vkResetCommandBuffer(current_cmd, flags);
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    uint32_t width = 0, height = 0;
    renderer.getWindow().getWindowSize(width, height);

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_pass;
    render_pass_info.framebuffer = m_fbos[renderer.getWindow().getCurrentImageId()];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = { width, height };

    std::array<VkClearValue, 4> clear_values;
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    //clear_values[ 4 ].depthStencil   = { 1.0f, 0 };

    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    if (vkBeginCommandBuffer(current_cmd, &begin_info) != VK_SUCCESS)
    {
        throw MiniEngineException("failed to begin recording command buffer!");
    }

    UtilsVK::beginRegion(current_cmd, "GBuffer Pass", Vector4f(0.0f, 0.5f, 0.0f, 1.0f));
    vkCmdBeginRenderPass(current_cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    for (uint32_t mat_id = static_cast<uint32_t>(Material::TMaterial::Diffuse); mat_id < static_cast<uint32_t>(m_pipelines.size()); mat_id++)
    {
        UtilsVK::beginRegion(current_cmd, mat_id == 0 ? "Diffuse GBuffer Pass" : mat_id == 1 ? "Dielectric GBuffer Pass" : "Microfacets GBuffer Pass", Vector4f(0.0f, 0.5f, 0.5f, 1.0f));

        vkCmdBindPipeline(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[mat_id].m_pipeline);
        //vkCmdBindDescriptorSets(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[mat_id].m_pipeline_layouts, 0, 2, &m_pipelines[mat_id].m_descriptor_sets[renderer.getWindow().getCurrentImageId()].m_per_frame_descriptor, 0, nullptr);
        VkDescriptorSet sets[] = {
    m_pipelines[mat_id].m_descriptor_sets[renderer.getWindow().getCurrentImageId()].m_per_frame_descriptor,
    m_pipelines[mat_id].m_descriptor_sets[renderer.getWindow().getCurrentImageId()].m_ssao_inputs_descriptor
        };
        vkCmdBindDescriptorSets(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelines[mat_id].m_pipeline_layouts, 0, 2, sets, 0, nullptr);

        for (auto entity : m_entities_to_draw[mat_id])
        {
            entity->draw(current_cmd, i_frame);
        }

        UtilsVK::endRegion(current_cmd);
    }

    vkCmdEndRenderPass(current_cmd);
    UtilsVK::endRegion(current_cmd);

    if (vkEndCommandBuffer(current_cmd) != VK_SUCCESS)
    {
        throw MiniEngineException("failed to record command buffer!");
    }

    return current_cmd;
}


void SSAOPassVK::addEntityToDraw(const EntityPtr i_entity)
{
    m_entities_to_draw[static_cast<uint32_t>(i_entity->getMaterial().getType())].push_back(i_entity);
}



void SSAOPassVK::createFbo()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    uint32_t width = 0, height = 0;
    renderer.getWindow().getWindowSize(width, height);

    for (size_t i = 0; i < m_fbos.size(); i++)
    {
        std::array<VkImageView, 4> attachments;
        attachments[0] = m_position_attachment.m_image_view;
        attachments[1] = m_normals_attachment.m_image_view;
        attachments[2] = m_depth_buffer.m_image_view;
        attachments[3] = m_ssao_attachment.m_image_view;

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

        framebuffer_create_info.renderPass = m_render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = width;
        framebuffer_create_info.height = height;
        framebuffer_create_info.layers = 1;


        if (vkCreateFramebuffer(renderer.getDevice()->getLogicalDevice(), &framebuffer_create_info, nullptr, &m_fbos[i]))
        {
            throw MiniEngineException("Failed to create fbos");
        }
    }
}



void SSAOPassVK::createRenderPass()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    std::array<VkAttachmentDescription, 4> attachments = {};

    // Position
    attachments[0].format = m_position_attachment.m_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Normal attachment
    attachments[1].format = m_normals_attachment.m_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth  attachment
    attachments[2].format = m_depth_buffer.m_format;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    // SSAO output
    attachments[3].format = m_ssao_attachment.m_format;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    VkAttachmentReference position_reference = {};
    position_reference.attachment = 0;
    position_reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference normal_reference = {};
    normal_reference.attachment = 1;
    normal_reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 2;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    std::array<VkAttachmentReference, 3> attachments_references = { position_reference, normal_reference, position_reference };

    VkAttachmentReference output_reference = {};
    output_reference.attachment = 3;
    output_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &output_reference;
    subpass_description.pDepthStencilAttachment = &depth_reference;
    subpass_description.inputAttachmentCount = attachments_references.size();;
    subpass_description.pInputAttachments = attachments_references.data();
    //subpass_description.preserveAttachmentCount = 0;
    //subpass_description.pPreserveAttachments = nullptr;
    //subpass_description.pResolveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> dependencies;
    // G-Buffer -> SSAO
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    //SSAO -> Next
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_description;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(renderer.getDevice()->getLogicalDevice(), &render_pass_info, nullptr, &m_render_pass))
    {
        throw MiniEngineException("Failed to create empty render pass");
    }
}

void SSAOPassVK::generateKernelSamples() {
    m_kernelSamples.resize(64);

    std::random_device random;
    std::mt19937 sampler(random());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (int i = 0; i < 64; ++i) {
        Vector3f sample(
            dist(sampler),
            dist(sampler),
            dist(sampler) * 0.5f + 0.5f
        );

        sample = normalize(sample);

        float scale = static_cast<float>(i) / 64.0f;
        scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
        sample *= scale;
        m_kernelSamples[i] = Vector4f(sample, 0.0f);

    }

    UtilsVK::createBuffer(
        *m_runtime.m_renderer->getDevice(),
        m_kernelSamples.size() * sizeof(Vector4f),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_kernelBuffer,
        m_kernelMemory
    );

    void* data;
    vkMapMemory(m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_kernelMemory, 0, sizeof(Vector4f) * 64, 0, &data);
    memcpy(data, m_kernelSamples.data(), sizeof(Vector4f) * 64);
    vkUnmapMemory(m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_kernelMemory);
}

void SSAOPassVK::generateNoiseTexture() {
    std::vector<Vector2f> noiseData(16);

    std::random_device random;
    std::default_random_engine sampler(random());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& v : noiseData) {
        v = Vector2f(dist(sampler), dist(sampler));
    }

    UtilsVK::TextureFromBuffer(
        *m_runtime.m_renderer->getDevice(),
        noiseData.data(),
        noiseData.size() * sizeof(Vector2f),
        VK_FORMAT_R32G32_SFLOAT,
        4,
        4,
        m_noise,
        VK_FILTER_NEAREST,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkCreateSampler(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &samplerInfo, nullptr, &m_noise.m_sampler);
}

void SSAOPassVK::createPipelines()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    VkVertexInputBindingDescription binding_vertex_descrition{};
    binding_vertex_descrition.binding = 0;
    binding_vertex_descrition.stride = sizeof(Vertex);
    binding_vertex_descrition.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, m_position);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, m_normal);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, m_uv);

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &binding_vertex_descrition;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    vertex_input_info.flags = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    input_assembly.flags = 0;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.flags = 0;


    VkPipelineRasterizationStateCreateInfo raster_info{};
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.pNext = VK_NULL_HANDLE;
    raster_info.flags = 0;
    raster_info.depthClampEnable = VK_FALSE;
    raster_info.rasterizerDiscardEnable = VK_FALSE;
    raster_info.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_info.depthBiasEnable = VK_FALSE;
    raster_info.depthBiasConstantFactor = 0.f;
    raster_info.depthBiasClamp = VK_FALSE;
    raster_info.depthBiasSlopeFactor = 0.f;
    raster_info.lineWidth = 1.f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;// | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    std::array<VkPipelineColorBlendAttachmentState, 4> blend_state =
    {
        color_blend_attachment,
        color_blend_attachment,
        color_blend_attachment,
        color_blend_attachment
    };

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = blend_state.size();
    color_blending.pAttachments = blend_state.data();
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;
    color_blending.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.flags = 0;


    uint32 width = 0, height = 0;
    renderer.getWindow().getWindowSize(width, height);
    VkExtent2D extend{ width, height };

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)width;
    viewport.height = (float)height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extend;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;
    viewport_state.flags = 0;

    //create unfiorms 
    createDescriptorLayout();

    for (auto& pipeline : m_pipelines)
    {

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = pipeline.m_descriptor_set_layout.size();
        pipeline_layout_info.pSetLayouts = pipeline.m_descriptor_set_layout.data();
        pipeline_layout_info.pPushConstantRanges = VK_NULL_HANDLE;
        pipeline_layout_info.pushConstantRangeCount = 0;
        pipeline_layout_info.flags = 0;

        std::vector<VkGraphicsPipelineCreateInfo> graphic_pipelines;


        if (vkCreatePipelineLayout(renderer.getDevice()->getLogicalDevice(), &pipeline_layout_info, nullptr, &pipeline.m_pipeline_layouts) != VK_SUCCESS)
        {
            throw MiniEngineException("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.layout = pipeline.m_pipeline_layouts;
        pipeline_info.renderPass = m_render_pass;
        pipeline_info.basePipelineIndex = -1;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pRasterizationState = &raster_info;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pDynamicState = VK_NULL_HANDLE;
        pipeline_info.stageCount = pipeline.m_shader_stages.size();
        pipeline_info.pStages = pipeline.m_shader_stages.data();
        pipeline_info.flags = 0;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.subpass = 0;

        graphic_pipelines.push_back(pipeline_info);


        if (vkCreateGraphicsPipelines(renderer.getDevice()->getLogicalDevice(), VK_NULL_HANDLE, 1, graphic_pipelines.data(), nullptr, &pipeline.m_pipeline))
        {
            throw MiniEngineException("Error creating the pipeline");
        }
    }
    createDescriptors();
}


void SSAOPassVK::createDescriptorLayout()
{
    // PER FRAME
    VkDescriptorSetLayoutBinding per_frame_binding = {};
    per_frame_binding.binding = 0;
    per_frame_binding.descriptorCount = 1;
    per_frame_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    per_frame_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo set_per_frame_info = {};
    set_per_frame_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_per_frame_info.pNext = nullptr;
    set_per_frame_info.bindingCount = 1;
    set_per_frame_info.flags = 0;
    set_per_frame_info.pBindings = &per_frame_binding;

    // PER OBJECT
    /*
    VkDescriptorSetLayoutBinding per_object_binding = {};
    per_object_binding.binding = 0;
    per_object_binding.descriptorCount = 1;
    per_object_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    per_object_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo set_per_object_info = {};
    set_per_object_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_per_object_info.pNext = nullptr;
    set_per_object_info.bindingCount = 1;
    set_per_object_info.flags = 0;
    set_per_object_info.pBindings = &per_object_binding;*/
    std::array<VkDescriptorSetLayoutBinding, 5> input_bindings = {};

    // Position input
    input_bindings[0].binding = 0;
    input_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input_bindings[0].descriptorCount = 1;
    input_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Normals input 
    input_bindings[1].binding = 1;
    input_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input_bindings[1].descriptorCount = 1;
    input_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth input 
    input_bindings[2].binding = 2;
    input_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    input_bindings[2].descriptorCount = 1;
    input_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Noise texture
    input_bindings[3].binding = 3;
    input_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    input_bindings[3].descriptorCount = 1;
    input_bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Kernel samples
    input_bindings[4].binding = 4;
    input_bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    input_bindings[4].descriptorCount = 1;
    input_bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo input_layout_info = {};
    input_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    input_layout_info.bindingCount = static_cast<uint32_t>(input_bindings.size());
    //input_layout_info.pNext = nullptr;
    //input_layout_info.flags = 0;
    input_layout_info.pBindings = input_bindings.data();

    for (auto& pipeline : m_pipelines)
    {
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &set_per_frame_info, nullptr, &pipeline.m_descriptor_set_layout[0]))
        {
            throw MiniEngineException("Error creating descriptor set");
        }

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &input_layout_info, nullptr, &pipeline.m_descriptor_set_layout[1]))
        {
            throw MiniEngineException("Error creating descriptor set");
        }
    }
}


void SSAOPassVK::createDescriptors()
{
    //create a descriptor pool that will hold 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes =
    {
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 30 * 3},
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 30 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 30 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 30 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 30;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    if (VK_SUCCESS != vkCreateDescriptorPool(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &pool_info, nullptr, &m_descriptor_pool))
    {
        throw MiniEngineException("Error creating descriptor pool");
    }

    //create descriptors for the global buffers
    for (auto& pipeline : m_pipelines)
    {
        for (uint32_t id = 0; id < m_runtime.m_renderer->getWindow().getImageCount(); id++)
        {

            //globals per frame
            //allocate one descriptor set for each frame
            VkDescriptorSetAllocateInfo alloc_per_frame_info = {};
            alloc_per_frame_info.pNext = nullptr;
            alloc_per_frame_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_per_frame_info.descriptorPool = m_descriptor_pool;
            alloc_per_frame_info.descriptorSetCount = 1;
            alloc_per_frame_info.pSetLayouts = &pipeline.m_descriptor_set_layout[0];
            vkAllocateDescriptorSets(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &alloc_per_frame_info, &pipeline.m_descriptor_sets[id].m_per_frame_descriptor);

            //objects
            //allocate one descriptor set for each frame
            VkDescriptorSetAllocateInfo alloc_input_info = {};
            alloc_input_info.pNext = nullptr;
            alloc_input_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_input_info.descriptorPool = m_descriptor_pool;
            alloc_input_info.descriptorSetCount = 1;
            alloc_input_info.pSetLayouts = &pipeline.m_descriptor_set_layout[1];
            vkAllocateDescriptorSets(m_runtime.m_renderer->getDevice()->getLogicalDevice(), &alloc_input_info, &pipeline.m_descriptor_sets[id].m_ssao_inputs_descriptor);

            //
            std::array<VkDescriptorImageInfo, 3> input_image_infos = {};

            // Position input
            input_image_infos[0].imageView = m_position_attachment.m_image_view;
            input_image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Normals input
            input_image_infos[1].imageView = m_normals_attachment.m_image_view;
            input_image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Depth input
            input_image_infos[2].imageView = m_depth_buffer.m_image_view;
            input_image_infos[2].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            //information about the buffer we want to point at in the descriptor
            VkDescriptorBufferInfo binfo;
            binfo.buffer = m_runtime.getPerFrameBuffer()[id];
            binfo.offset = 0;
            binfo.range = sizeof(PerFrameData);

            std::array<VkWriteDescriptorSet, 3> input_writes = {};

            for (uint32_t i = 0; i < 3; i++) {
                input_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                input_writes[i].dstSet = pipeline.m_descriptor_sets[id].m_per_frame_descriptor;
                input_writes[i].dstBinding = i;
                input_writes[i].descriptorCount = 1;
                input_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                input_writes[i].pImageInfo = &input_image_infos[i];
                input_writes[i].pBufferInfo = &binfo;
            }

            vkUpdateDescriptorSets(m_runtime.m_renderer->getDevice()->getLogicalDevice(), static_cast<uint32_t>(input_writes.size()), input_writes.data(), 0, nullptr);

            VkDescriptorImageInfo noiseImageInfo{};
            noiseImageInfo.sampler = m_noise.m_sampler;
            noiseImageInfo.imageView = m_noise.m_image_view;
            noiseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorBufferInfo kernelBufferInfo{};
            kernelBufferInfo.buffer = m_kernelBuffer;
            kernelBufferInfo.offset = 0;
            kernelBufferInfo.range = sizeof(Vector4f) * 64;

            std::array<VkWriteDescriptorSet, 2> additionalWrites = {};
            additionalWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            additionalWrites[0].dstSet = pipeline.m_descriptor_sets[id].m_per_frame_descriptor;
            additionalWrites[0].dstBinding = 4;//3; // matches shader binding
            additionalWrites[0].descriptorCount = 1;
            additionalWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            additionalWrites[0].pImageInfo = &noiseImageInfo;

            additionalWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            additionalWrites[1].dstSet = pipeline.m_descriptor_sets[id].m_per_frame_descriptor;
            additionalWrites[1].dstBinding = 3; // 4; // matches shader binding
            additionalWrites[1].descriptorCount = 1;
            additionalWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            additionalWrites[1].pBufferInfo = &kernelBufferInfo;

            vkUpdateDescriptorSets(m_runtime.m_renderer->getDevice()->getLogicalDevice(), static_cast<uint32_t>(additionalWrites.size()), additionalWrites.data(), 0, nullptr);

            /*
            VkWriteDescriptorSet set_write = {};
            set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            set_write.pNext = nullptr;
            set_write.dstBinding = 0;
            set_write.dstSet = pipeline.m_descriptor_sets[id].m_per_frame_descriptor;
            set_write.descriptorCount = 1;
            set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            set_write.pBufferInfo = &binfo;

            vkUpdateDescriptorSets(m_runtime.m_renderer->getDevice()->getLogicalDevice(), 2, &set_write, 0, nullptr);*/

        }
    }

}