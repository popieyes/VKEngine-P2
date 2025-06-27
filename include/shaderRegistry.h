#pragma once

#include "common.h"

namespace MiniEngine
{
    struct Runtime;
    class ShaderSolutionVK;

    class ShaderRegistry final
    {
    public:
        ShaderRegistry ( const Runtime& i_runtime );
        ~ShaderRegistry();

        bool initialize();
        void shutdown();

        VkShaderModule ShaderRegistry::loadShader( const std::string& i_filename, VkShaderStageFlagBits i_stage );

    private:
        ShaderRegistry( const ShaderRegistry& ) = delete;
        ShaderRegistry& operator=(const ShaderRegistry& ) = delete;

        std::unordered_map<std::string, VkShaderModule> m_shader_programs; 

        const Runtime& m_runtime;
    };
};