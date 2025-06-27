#pragma once

#include "common.h"
#include "transform.h"

namespace MiniEngine
{
    struct Runtime;

    class Light final 
    {
    public:
        enum class LightType : uint32_t
        {
            Directional,
            Point,
            Ambient,
            Count,
            Undefined
        };

        explicit Light( const Runtime& i_runtime ) : m_runtime( i_runtime ){};
        ~Light() = default;
        

        bool initialize()
        {
            return true;
        }

        void shutdown()
        {}

        static std::shared_ptr<Light> createLight(  const Runtime& i_runtime, const pugi::xml_node& emitter );

        // we use this structure to define the light uniform buffer
        struct LightData
        {
            LightType   m_type;
            Vector3f    m_radiance;
            Vector3f    m_position;
            Vector3f    m_attenuation;
        };
        
        LightData m_data;

    private:
        Light( const Light& ) = delete;
        Light& operator=(const Light& ) = delete;

        const Runtime& m_runtime;
    };
};