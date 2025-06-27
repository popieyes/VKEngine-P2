#include "shaderRegistry.h"
#include "runtime.h"
#include "vulkan/rendererVK.h"
#include "vulkan/utilsVK.h"
#include "vulkan/deviceVK.h"


using namespace MiniEngine;


ShaderRegistry::ShaderRegistry ( const Runtime& i_runtime ) :
    m_runtime( i_runtime )
{
}


ShaderRegistry::~ShaderRegistry()
{
}


bool ShaderRegistry::initialize()
{
    return true;
}


void ShaderRegistry::shutdown()
{
    const RendererVK& renderer = *m_runtime.m_renderer;

    for( auto shader : m_shader_programs )
    {
        vkDestroyShaderModule( renderer.getDevice()->getLogicalDevice(), shader.second, nullptr );
    }

    m_shader_programs.clear();
}


VkShaderModule ShaderRegistry::loadShader( const std::string& i_filename, VkShaderStageFlagBits i_stage )
{
    auto stored_shader = m_shader_programs.find( i_filename );

    if( stored_shader != m_shader_programs.end() )
    {
        return stored_shader->second;
    }

    VkShaderModule new_shader = UtilsVK::loadShader( i_filename.c_str(), m_runtime.m_renderer->getDevice()->getLogicalDevice() );
    m_shader_programs.insert( { i_filename, new_shader } );

    return new_shader;
}