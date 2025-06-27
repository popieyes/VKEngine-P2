#pragma once

#include "common.h"

namespace MiniEngine
{
    class Renderer
    {
    public:
        Renderer()
        {}
        
        virtual ~Renderer()
        {}

        virtual bool initialize() = 0;
        virtual void shutdown  () = 0;
    
    };

};

