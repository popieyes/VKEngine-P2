#pragma once

#include "common.h"
#include "material.h"

namespace MiniEngine
{
    struct Runtime;

    class Microfacets final : public Material
    {
    public:
        struct Data
        {
            float    m_roughness;
            float    m_metallic;
            Vector3f m_albedo;
        };

        explicit Microfacets( const Runtime& i_runtime ) : 
            Material( i_runtime, Material::TMaterial::Microfacets ),
            m_data( {
                    0.1f,
                    1.0f,
                    { 0.5f, 0.5f, 0.5f }
                  } )
        {}

        ~Microfacets() = default;

        bool initialize   () override;
      
        static std::shared_ptr<Microfacets> createMaterial( const Runtime& i_runtime, const pugi::xml_node& i_node, bool i_is_dielectric = false );

        inline const Data& getData() const
        {
            return m_data;
        }

    private:
        Microfacets( const Microfacets& ) = delete;
        Microfacets& operator=(const Microfacets& ) = delete;

        Data m_data;
    };
};