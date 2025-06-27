#pragma once

#include "common.h"

namespace MiniEngine
{
    struct Runtime;
    class Entity;
    struct Frame;
    typedef std::shared_ptr<Entity> EntityPtr;

    class RenderPassVK
    {
    public:
        RenderPassVK( const Runtime& i_runtime, const std::shared_ptr<RenderPassVK> i_prev_pass = nullptr ) : 
            m_runtime( i_runtime ),
            m_prev_render_pass( i_prev_pass )
        {}
        virtual ~RenderPassVK() = default;

        virtual bool            initialize() = 0;
        virtual void            shutdown  () = 0;
        virtual VkCommandBuffer draw      ( const Frame& ) = 0;

        virtual void addEntityToDraw( const EntityPtr i_entity )
        {
        }

    protected:
        const Runtime& m_runtime;
        const std::shared_ptr<RenderPassVK> m_prev_render_pass;

    private:
        RenderPassVK( const RenderPassVK& ) = delete;
        RenderPassVK& operator=(const RenderPassVK& ) = delete;
    };
};