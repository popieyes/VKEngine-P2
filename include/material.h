#pragma once

#include "common.h"

namespace MiniEngine
{
    struct Runtime;

    class Material 
    {
    public:
        enum class TMaterial
        {
            Diffuse,
            Microfacets,
            Count,
            Undefined
        };

        explicit Material( const Runtime& i_runtime, const TMaterial i_type ) : m_runtime( i_runtime ), m_material_type( i_type ){};

        ~Material() = default;
    
        virtual bool initialize()
        {
            return true;
        }

        virtual void shutdown()
        {}

        TMaterial getType() const
        {
            return m_material_type;
        }

        static std::shared_ptr<Material> createMaterial(  const Runtime& i_runtime, const pugi::xml_node& i_node );

    protected:
       const Runtime& m_runtime;
       const TMaterial m_material_type;

    private:
        Material( const Material& ) = delete;
        Material& operator=(const Material& ) = delete;
    };
};