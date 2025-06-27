#include "meshRegistry.h"
#include "vulkan/meshVK.h"

using namespace MiniEngine;


namespace 
{
    bool loadOBJ( const std::string& i_path, std::vector<Vertex>& o_vertices, std::vector<uint32>& o_indices )
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, i_path.c_str() );

        if (!err.empty()) 
        {
            throw MiniEngineException( err.c_str() );
        }

        if(!ret) 
        {
            throw MiniEngineException( "Failed to load/parse .obj.\n");
        }
        
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<glm::vec3, uint32_t> unique_vertices;

        for( const auto& shape : shapes )
        {
            for( const auto& index : shape.mesh.indices )
            {   
                Vertex vertex;
                vertex.m_position =
                {
                     attrib.vertices[ 3 * index.vertex_index + 0 ],
                     attrib.vertices[ 3 * index.vertex_index + 1 ],
                     attrib.vertices[ 3 * index.vertex_index + 2 ] 
                };

                if( index.normal_index >= 0 )
                {
                    vertex.m_normal =
                    {
                         attrib.normals[ 3 * index.normal_index + 0 ],
                         attrib.normals[ 3 * index.normal_index + 1 ],
                         attrib.normals[ 3 * index.normal_index + 2 ]
                    };
                }
                
                if( index.texcoord_index >= 0 )
                {
                    vertex.m_uv =
                    {
                        attrib.texcoords[ 2 * index.texcoord_index + 0 ],
                        attrib.texcoords[ 2 * index.texcoord_index + 1 ]
                    };
                }

                if( unique_vertices.count( vertex.m_position ) == 0 )
                {
                    unique_vertices[ vertex.m_position ] = static_cast< uint32_t >( vertices.size() );
                    vertices.push_back( vertex );
#ifdef PRINT_VERTICES
                    std::cout << "NEW VERTEX" << std::endl;
                    std::cout << "pos: x: " << vertex.m_position.x << ", y: " << vertex.m_position.y << ", z: " << vertex.m_position.z << std::endl;
                    std::cout << "normal: x: " << vertex.m_position.x << ", y: " << vertex.m_position.y << ", z: " << vertex.m_position.z << std::endl;
                    std::cout << "uv: x: " << vertex.m_uv.x << ", y: " << vertex.m_uv.y << std::endl;
#endif
                }

                indices.push_back( unique_vertices[ vertex.m_position ] );
            }
        }

        o_indices = indices;
        o_vertices = vertices;
        return true;
    }
}



std::shared_ptr<MeshVK> MeshRegistry::loadMesh( const std::string& i_path )
{
    auto mesh = m_meshes.find( i_path );

    //handle already exist
    if( mesh != m_meshes.end() )
    {
        return mesh->second;
    }

    //new handle
    std::vector<uint32> indices;
    std::vector<Vertex> vertices;

    if( !::loadOBJ( i_path, vertices, indices ) )
    {
        throw MiniEngineException( "Error while loading obj" );
    }

    std::shared_ptr<MeshVK> new_mesh = std::make_shared<MeshVK>( m_runtime, i_path, indices, vertices );
    new_mesh->initialize();

    m_meshes.insert( { i_path, new_mesh } );

    return new_mesh;
}


MeshRegistry::MeshRegistry( const Runtime& i_runtime ) :
    m_runtime( i_runtime ),
    m_meshes({})
{
}


bool MeshRegistry::initialize()
{
    return true;
}
 

void MeshRegistry::shutdown()
{
    for( auto mesh_block : m_meshes )
    {
        mesh_block.second->shutdown();
    }

    m_meshes.clear();
}



