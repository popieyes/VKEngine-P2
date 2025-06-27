#include "common.h"
#include "engine.h"

using namespace MiniEngine;

int main( int argc, char* argv[] )
{
    
    if( argc == 2 )
    {
        Engine::instance().initialize();
        Engine::instance().loadScene( std::string(argv[1] ) );
        Engine::instance().run       ();
        Engine::instance().shutdown  ();
    }

    return 0;
}
