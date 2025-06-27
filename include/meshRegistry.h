#pragma once

#include "common.h"

namespace MiniEngine
{
    struct Runtime;
    class MeshVK;

    
    class MeshRegistry final
    {
    public:
        explicit MeshRegistry ( const Runtime& i_runtime );
        ~MeshRegistry() = default;

        bool initialize();
        void shutdown();

        std::shared_ptr<MeshVK> loadMesh( const std::string& i_path );


    private:
        MeshRegistry( const MeshRegistry& ) = delete;
        MeshRegistry& operator=(const MeshRegistry& ) = delete;

        const Runtime& m_runtime;
        std::unordered_map<std::string, std::shared_ptr<MeshVK>> m_meshes;
    };
};