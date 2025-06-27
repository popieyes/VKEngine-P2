
//engine includes
#include "engine.h"
#include "frame.h"
#include "meshRegistry.h"
#include "shaderRegistry.h"
#include "scene.h"
#include "camera.h"
#include "light.h"
#include "entity.h"
#include "material.h"
#include "diffuse.h"
#include "microfacets.h"

// vulkan includes
#include "vulkan/rendererVK.h"
#include "vulkan/renderPassVK.h"
#include "vulkan/deferredPassVK.h"
#include "vulkan/compositionPassVK.h"
#include "vulkan/windowVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/utilsVK.h"



using namespace MiniEngine;


namespace
{
    Engine* m_instance = nullptr;
}


Engine& Engine::instance()
{
    if( !m_instance )
    {
        m_instance = new Engine();
    }

    return *m_instance;
}


Engine::Engine() : 
    m_current_frame( 0     ),
    m_close        ( false ),
    m_resize       ( false )
{

}


Engine::~Engine()
{

}


bool Engine::initialize()
{
    //init vulkan 
    m_runtime.m_renderer = std::make_unique<RendererVK>();
    RendererVK& renderer = *m_runtime.m_renderer;

    renderer.initialize();

    m_runtime.m_mesh_registry   = std::make_unique<MeshRegistry  >( m_runtime );
    m_runtime.m_shader_registry = std::make_unique<ShaderRegistry>( m_runtime );

    m_runtime.m_mesh_registry->initialize();
    m_runtime.m_shader_registry->initialize();

    createSyncObjects ();

    return true;
}


void Engine::run()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    bool loop = true;
    while( loop && m_scene ) 
    {
        uint32_t clamped_idx = m_current_frame % 3;
        renderer.getWindow().prepareFrame( m_frame_semaphore[ clamped_idx ].m_presentation_semaphore );
        
        vkWaitForFences( renderer.getDevice()->getLogicalDevice(), 1, &m_frame_fence[ clamped_idx ], VK_TRUE, 1000000000 );
        
        //update global uniforms buffers 
        updateGlobalBuffers(); 

        //prepare pipeline stages
        VkSubmitInfo submit_info{};
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext                   = nullptr;
        submit_info.pWaitDstStageMask       = &wait_stage;
        submit_info.waitSemaphoreCount      = 1;
        submit_info.pWaitSemaphores         = &m_frame_semaphore[ clamped_idx ].m_presentation_semaphore;
        submit_info.signalSemaphoreCount    = 1;
        submit_info.pSignalSemaphores       = &m_frame_semaphore[ clamped_idx ].m_render_semaphore;

        // draw render passes
        std::vector<VkCommandBuffer> cmds;
        for( auto& pass : m_render_passes )
        {
            cmds.push_back( pass->draw( {} ) );
        }

        submit_info.commandBufferCount = static_cast<uint32_t>(cmds.size());
        submit_info.pCommandBuffers    = cmds.data();

        vkResetFences  ( renderer.getDevice()->getLogicalDevice(), 1, &m_frame_fence[ clamped_idx ] );

        vkQueueSubmit( renderer.getDevice()->getGraphicsQueue(), 1, &submit_info, m_frame_fence[ clamped_idx ] );

        uint32_t result = renderer.getWindow().renderFrame( m_frame_semaphore[ clamped_idx ].m_render_semaphore );
        
        vkQueueWaitIdle( renderer.getDevice()->getGraphicsQueue() );

        //
        //check if we need to resize the window               
        // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
        if( ( result == VK_ERROR_OUT_OF_DATE_KHR ) || ( result == VK_SUBOPTIMAL_KHR ) )
        {
           renderer.getWindow().wait();

            vkDeviceWaitIdle( renderer.getDevice()->getLogicalDevice() );
                       
            destroySamplers    ();
            destroyRenderPasses();
            destroyAttachments ();
            destroySyncObjects ();
            renderer.getWindow ().resize();        

            createSyncObjects ();
            createSamplers    ();
            createAttachments ();
            createRenderPasses();
        }                   


        m_current_frame++;
        //check if the window is closed and poll input events
        loop = renderer.getWindow().loop();

    }
}


void Engine::shutdown()
{
    RendererVK& renderer = *m_runtime.m_renderer;


    vkDeviceWaitIdle( renderer.getDevice()->getLogicalDevice() );
    
    m_runtime.freeResources();

    if( m_scene )
    {
        m_scene->shutdown();
    }

    destroyRenderPasses();
    destroyAttachments ();
    destroySamplers    ();
    destroySyncObjects ();

    m_runtime.m_mesh_registry->shutdown();
    m_runtime.m_shader_registry->shutdown();

    m_runtime.m_renderer->shutdown();
}


void Engine::loadScene( const std::string& i_path )
{
    m_scene = Scene::loadScene( m_runtime, i_path );

    assert( m_scene );

    if( !m_render_passes.empty() )
    {
        destroySamplers    ();
        destroyAttachments ();
        destroyRenderPasses();
    }
    else //create uniform buffers just once
    {
        m_runtime.createResources();
    }

    createSamplers    ();
    createAttachments ();
    createRenderPasses();

    RendererVK& renderer = *m_runtime.m_renderer;
    renderer.getWindow().resize( m_scene->getCamera().getWidth(), m_scene->getCamera().getHeight() );

}


void Engine::createSyncObjects()                                  
{
    RendererVK& renderer = *m_runtime.m_renderer;

    //create sync objects
    for( uint32_t idx = 0; idx < 3; idx++ )
    {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if( vkCreateSemaphore( renderer.getDevice()->getLogicalDevice(), &semaphore_info, nullptr, &m_frame_semaphore[ idx ].m_render_semaphore ) )
        {
            throw MiniEngineException( "Cannot create render semaphore" );
        }
        
        if( vkCreateSemaphore( renderer.getDevice()->getLogicalDevice(), &semaphore_info, nullptr, &m_frame_semaphore[ idx ].m_presentation_semaphore ) )
        {
            throw MiniEngineException( "Cannot create presentation semaphore" );
        }

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if( vkCreateFence( renderer.getDevice()->getLogicalDevice(), &fence_info, nullptr, &m_frame_fence[ idx ] ) )
        {
            throw MiniEngineException( "Cannot create fence" );
        }
    }
}


void Engine::destroySyncObjects()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    for( uint32_t idx = 0; idx < 3; idx++ )
    {
        vkDestroySemaphore( renderer.getDevice()->getLogicalDevice(), m_frame_semaphore[ idx ].m_render_semaphore      , nullptr );
        vkDestroySemaphore( renderer.getDevice()->getLogicalDevice(), m_frame_semaphore[ idx ].m_presentation_semaphore, nullptr );
        vkDestroyFence    ( renderer.getDevice()->getLogicalDevice(), m_frame_fence[ idx ]                             , nullptr );
    }
}


void Engine::createRenderPasses ()
{ 
    //temp
    for( auto pass : m_render_passes )
    {
        pass->shutdown();
    }
    m_render_passes.clear();
    

    auto gbuffer_pass = std::make_shared<DeferredPassVK>(
        m_runtime, 
        m_render_target_attachments.m_depth_attachment, 
        m_render_target_attachments.m_color_attachment, 
        m_render_target_attachments.m_normal_attachment, 
        m_render_target_attachments.m_position_depth_attachment, 
        m_render_target_attachments.m_material_attachment );
    gbuffer_pass->initialize();

    m_render_passes.push_back( gbuffer_pass );


    auto composition_pass = std::make_shared<CompositionPassVK>( m_runtime, m_render_target_attachments.m_color_attachment, m_render_target_attachments.m_position_depth_attachment, m_render_target_attachments.m_normal_attachment, m_render_target_attachments.m_material_attachment, m_runtime.m_renderer->getWindow().getSwapChainImages() );
    composition_pass->initialize();

    m_render_passes.push_back( composition_pass );


    if( m_scene )
    {
        for( auto pass : m_render_passes )
        {
            for( auto entity : m_scene->getMeshes() )
            {
                pass->addEntityToDraw( entity );
            }
        }
    }
}


void Engine::destroyRenderPasses()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    for( auto pass : m_render_passes )
    {
        pass->shutdown();
        pass = nullptr;
    }

    m_render_passes.clear();
}


void Engine::updateGlobalBuffers()
{
    assert( m_runtime.m_per_frame_buffer[ m_current_frame % 3 ] );
    assert( m_scene );

    //global settings
    PerFrameData perframe_data;
    Vector3f cam_pos = m_scene->getCamera().getCameraPos();
    perframe_data.m_camera_pos          = Vector4f( cam_pos.x, cam_pos.y, cam_pos.z, 0.0f );
    perframe_data.m_projection          = const_cast< Camera& >( m_scene->getCamera() ).getProjection();
    perframe_data.m_view                = const_cast< Camera& >( m_scene->getCamera() ).getView();
    perframe_data.m_view_projection     = const_cast< Camera& >( m_scene->getCamera() ).getViewProjection();
    perframe_data.m_inv_projection      = glm::inverse( perframe_data.m_projection          );
    perframe_data.m_inv_view            = glm::inverse( perframe_data.m_view                );
    perframe_data.m_inv_view_projection = glm::inverse( perframe_data.m_inv_view_projection );
    perframe_data.m_clipping_planes     = Vector4f( m_scene->getCamera().getNearPlane(), m_scene->getCamera().getFarPlane(), 0.0f, 0.0f );
    perframe_data.m_number_of_lights    = 0;

    for( perframe_data.m_number_of_lights = 0; perframe_data.m_number_of_lights < m_scene->getLights().size() && perframe_data.m_number_of_lights < kMAX_NUMBER_LIGHTS; perframe_data.m_number_of_lights++ )
    {
        assert( perframe_data.m_number_of_lights < kMAX_NUMBER_LIGHTS );
       
        auto light = m_scene->getLights()[ perframe_data.m_number_of_lights ];

        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_light_pos    = Vector4f( light->m_data.m_position.x   , light->m_data.m_position.y   , light->m_data.m_position.z   , light->m_data.m_type );
        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_radiance     = Vector4f( light->m_data.m_radiance.x   , light->m_data.m_radiance.y   , light->m_data.m_radiance.z   , 0.0f                 );
        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_attenuattion = Vector4f( light->m_data.m_attenuation.x, light->m_data.m_attenuation.y, light->m_data.m_attenuation.z, 0.0f                 );
    }


    //material buffers
    void* data;
    vkMapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_frame_buffer_memory[ m_current_frame  % 3 ], 0, sizeof( PerFrameData ), 0, &data );

    memcpy( data, &perframe_data, sizeof( PerFrameData ) );

    vkUnmapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_frame_buffer_memory[ m_current_frame % 3 ] );
    
    for( uint32_t idx = 0; idx < m_scene->getMeshes().size(); idx++ )
    {
        PerObjectData* data_object;
        std::shared_ptr<Entity> entity = m_scene->getMeshes()[ idx ];
        
        vkMapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_object_buffer_memory[ m_current_frame % 3 ], sizeof( PerObjectData ) * idx, sizeof( PerObjectData ), 0, reinterpret_cast<void**>( &data_object ) );

        data_object->m_model = entity->getTransform().getTransform();

        switch( entity->getMaterial().getType() )
        {
            case Material::TMaterial::Diffuse:
            {
                Diffuse& diffuse = reinterpret_cast<Diffuse&>( entity->getMaterial() );
                data_object->m_albedo  = Vector4f( diffuse.getData().m_albedo.x, diffuse.getData().m_albedo.y, diffuse.getData().m_albedo.z, 0.0f );

                break;
            }
            case Material::TMaterial::Microfacets: 
            {
                Microfacets& microfacets = reinterpret_cast<Microfacets&>( entity->getMaterial() );
                data_object->m_albedo             = Vector4f( microfacets.getData().m_albedo.x, microfacets.getData().m_albedo.y , microfacets.getData().m_albedo.z, 0.0f );
                data_object->m_metallic_roughness = Vector4f( microfacets.getData().m_metallic, microfacets.getData().m_roughness,                             0.0f, 0.0f );
                break;
            }
        }        

        vkUnmapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_object_buffer_memory[ m_current_frame % 3 ] );
    }
    
}


void Engine::createAttachments()
{
    uint32_t width, height;

    m_runtime.m_renderer->getWindow().getWindowSize( width, height );

    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8G8B8A8_UNORM     , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_color_attachment          );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8G8B8A8_UNORM     , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_normal_attachment         );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_position_depth_attachment );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8G8B8A8_UNORM     , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_material_attachment       );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_D32_SFLOAT_S8_UINT , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height, m_render_target_attachments.m_depth_attachment          );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8_UNORM           , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_ssao_attachment           );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8_UNORM           , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_ssao_blur_attachment      );

    m_render_target_attachments.m_color_attachment.m_sampler            = m_global_samplers[ 0 ];         
    m_render_target_attachments.m_normal_attachment.m_sampler           = m_global_samplers[ 0 ];        
    m_render_target_attachments.m_position_depth_attachment.m_sampler   = m_global_samplers[ 0 ];
    m_render_target_attachments.m_material_attachment.m_sampler         = m_global_samplers[ 0 ];      
    m_render_target_attachments.m_depth_attachment.m_sampler            = m_global_samplers[ 0 ];         
    m_render_target_attachments.m_ssao_attachment.m_sampler             = m_global_samplers[ 0 ];          
    m_render_target_attachments.m_ssao_blur_attachment.m_sampler        = m_global_samplers[ 0 ]; 

    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_color_attachment.m_image          ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Color Attachment"    );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_normal_attachment.m_image         ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Normal Attachment "  );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_position_depth_attachment.m_image ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Position Attachment ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_material_attachment.m_image       ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Material Attachment ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_depth_attachment.m_image          ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Depth Buffer"        );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_ssao_attachment.m_image           ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image SSAO attachment"     );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_ssao_blur_attachment.m_image      ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image SSAO blur "          );
}


void Engine::destroyAttachments()
{
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_color_attachment          );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_normal_attachment         );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_position_depth_attachment );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_material_attachment       );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_depth_attachment          );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_ssao_attachment           );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_ssao_blur_attachment      );
}


void Engine::createSamplers()
{
    VkSamplerCreateInfo sampler{};
    sampler.sType           = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter       = VK_FILTER_NEAREST;
	sampler.minFilter       = VK_FILTER_NEAREST;
	sampler.mipmapMode      = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias      = 0.0f;
	sampler.maxAnisotropy   = 1.0f;
	sampler.minLod          = 0.0f;
	sampler.maxLod          = 1.0f;
	sampler.borderColor     = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	
    if( VK_SUCCESS != vkCreateSampler( m_runtime.m_renderer->getDevice()->getLogicalDevice(), &sampler, nullptr, &m_global_samplers[ 0 ] ) )
    {
        throw MiniEngineException( "Error creating sampler" );
    }

    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)m_global_samplers[ 0 ], VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "Global Sampler"  );
}


void Engine::destroySamplers()
{
    for( VkSampler sampler : m_global_samplers )
    {
        vkDestroySampler( m_runtime.m_renderer->getDevice()->getLogicalDevice(), sampler, nullptr );
    }
}

