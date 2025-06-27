#include "scene.h"
#include "camera.h"
#include "light.h"
#include "entity.h"
#include "common.h"


using namespace MiniEngine;

namespace 
{
     /* Set of supported XML tags */
    enum ETag
    {
        /* Object classes */
        EScene,                 
        EMesh,                 
        EBSDF,                  
        EPhaseFunction,         
        EEmitter,               
        EMedium,                
        ECamera,                
        EIntegrator,            
        ESampler,               
        ETest,                  
        EReconstructionFilter,  

        /* Properties */
        EBoolean,
        EInteger,
        EFloat,
        EString,
        EPoint,
        EVector,
        EColor,
        ETransform,
        ETranslate,
        EMatrix,
        ERotate,
        EScale,
        ELookAt,

        EInvalid
    };

    /* Create a mapping from tag names to tag IDs */
    static std::unordered_map<std::string, ETag> tags
    {
        { "scene"     , EScene                  },
        { "mesh"      , EMesh                   },
        { "bsdf"      , EBSDF                   },
        { "emitter"   , EEmitter                },
        { "camera"    , ECamera                 },
        { "medium"    , EMedium                 },
        { "phase"     , EPhaseFunction          },
        { "integrator", EIntegrator             },
        { "sampler"   , ESampler                },
        { "rfilter"   , EReconstructionFilter   },
        { "test"      , ETest                   },
        { "boolean"   , EBoolean                },
        { "integer"   , EInteger                },
        { "float"     , EFloat                  },
        { "string"    , EString                 },
        { "point"     , EPoint                  },
        { "vector"    , EVector                 },
        { "color"     , EColor                  },
        { "transform" , ETransform              },
        { "translate" , ETranslate              },
        { "matrix"    , EMatrix                 },
        { "rotate"    , ERotate                 },
        { "scale"     , EScale                  },
        { "lookat"    , ELookAt                 }
    };                 
};


Scene::~Scene()
{
}

       
bool Scene::initialize()
{


    return true;
}
        

void Scene::shutdown()
{
    for( auto light : m_lights )
    {
        light->shutdown();
    }

    for( auto entity : m_entities )
    {
        entity->shutdown();
    }

    m_camera->shutdown();

}



std::shared_ptr<Scene> Scene::loadScene( const Runtime& i_runtime, const std::string& i_path )
{
     /* Load the XML file using 'pugi' (a tiny self-contained XML parser implemented in C++) */
     pugi::xml_document doc;
     pugi::xml_parse_result result = doc.load_file( i_path.c_str() );

     if( !result ) 
     {
         throw MiniEngineException( "Error while parsing %s\n", i_path );
     }

     size_t count = std::distance( doc.children("scene").begin(), doc.children("scene").end() );

     if( 0 == count )
     {
         throw MiniEngineException("No scene node");
     }

     auto scene_node = doc.child("scene");
     std::shared_ptr<Scene> scene = std::make_shared<Scene>( i_runtime, i_path );

     count = std::distance( scene_node.children("camera").begin(), scene_node.children("camera").end() );

     if( 0 == count )
     {
         throw MiniEngineException("No camera node");
     }

     //parse the camera
     CameraPtr camera = Camera::createCamera( i_runtime, scene_node.child("camera") );
     scene->m_camera = camera;
     
     uint32_t entity_id = 0;
     //parse objects
     for(pugi::xml_node node = scene_node.child("mesh"); node; node = node.next_sibling("mesh"))
     {
         if( !node.attribute("type") )
         {
             throw MiniEngineException( "node without type attribute" );
         }

         if(node.child("emitter") && strcmp( node.attribute("type").value(), "obj" ) == 0  )
         {
             LightPtr light = Light::createLight( i_runtime, node );
             scene->m_lights.push_back( light );
             continue;
         }

         if( node.child("medium") || node.child("phaseFunction") || node.child("test") || strcmp( node.attribute("type").value(), "obj" ) != 0 )
         {
             std::cout << "No supported by this engine" << std::endl;
             continue;
         }

         if( strcmp( node.attribute("type").value(), "obj" ) == 0 )
         {
             EntityPtr entity = Entity::createEntity( i_runtime, node, entity_id++ );
             scene->m_entities.push_back( entity );
         }

         assert( scene->m_entities.size() <= kMAX_NUMBER_OF_OBJECTS );
     }

     for(pugi::xml_node node = scene_node.child("emitter"); node; node = node.next_sibling("emitter"))
     {
         LightPtr light = Light::createLight( i_runtime, node );
         scene->m_lights.push_back( light );
         
         assert( scene->m_entities.size() <= kMAX_NUMBER_OF_OBJECTS );
     }

     scene->initialize();

     return scene;
}
