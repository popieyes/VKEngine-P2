#pragma once

#include "common.h"
#include "material.h"

namespace MiniEngine
{
    struct Runtime;

    class Diffuse final : public Material
    {
    public:
        struct Data
        {
            Vector3f m_albedo;
        };
        

        explicit Diffuse( const Runtime& i_runtime ) : 
            Material( i_runtime, Material::TMaterial::Diffuse ),
            m_data( { { 0.5f, 0.5f, 0.5f } } )
        {
        };

        ~Diffuse() = default;
        
        bool initialize   () override;
    
        static std::shared_ptr<Diffuse> createMaterial( const Runtime& i_runtime, const pugi::xml_node& i_node );

        inline const Data& getData() const
        {
            return m_data;
        }

    private:
        Diffuse( const Diffuse& ) = delete;
        Diffuse& operator=(const Diffuse& ) = delete;

        Data m_data;
    };
};