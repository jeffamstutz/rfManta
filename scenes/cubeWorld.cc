#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ArcAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Instances/Instance.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Primitives/Cube.h>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

using namespace Manta;
using namespace std;

void argumentError(
    int i,
    vector< string > const &arguments )
{
    cerr << "Valid options for scene cubeWorld:" << endl
         << " -model         - Required. The file to load each line of the model has 9 floats:" << endl
         << "                  <center> <lengths> pitch roll yaw." << endl
         << " -lightPosition - Location of light." << endl;
    throw IllegalArgument("scene cubeWorld", i, arguments );
}

MANTA_PLUGINEXPORT
Scene* make_scene(
    ReadContext const &/*context*/,
    vector< string > const &arguments )
{

    cout << "Make_scene arguments: " << arguments.size() << endl;
    string file_name;
    Vector light_position;
    bool file_set = false;
    bool light_set = false;

    for( size_t index = 0; index < arguments.size(); ++index )
    {
        string arg = arguments[ index ];
        if( arg == "-model" )
        {
            if( !getStringArg( index, arguments, file_name ) )
                throw IllegalArgument( "scene cubeWorld -model", index, arguments );
            file_set = true;
        }
        else if (arg == "-lightPosition")
        {
            if ( !getVectorArg( index, arguments, light_position ) )
                throw IllegalArgument( "scene cubeWorld -lightPosition", index, arguments );
            light_set = true;
        }
        else
            argumentError( index, arguments );
    }
    if ( arguments.empty() || !file_set )
        argumentError( 0, arguments );

    Group* group = new Group();
    Material *flat = new Lambertian( Color::white() * 0.8 );
    ifstream input( file_name.c_str() );
    Vector center, length, rotations;
    while( input >> center >> length >> rotations )
    {
        length *= 0.5;
        if ( rotations != Vector( 0.0, 0.0, 0.0 ) )
        {
            AffineTransform transform;
            transform.initWithIdentity();
            transform.rotate( Vector( 1.0, 0.0, 0.0 ), rotations.x() * M_PI / 180.0 );
            transform.rotate( Vector( 0.0, 1.0, 0.0 ), rotations.y() * M_PI / 180.0 );
            transform.rotate( Vector( 0.0, 0.0, 1.0 ), rotations.z() * M_PI / 180.0 );
            transform.translate( center );
            group->add( new Instance( new Cube( flat, -length, length ), transform ) );
        }
        else
            group->add( new Cube( flat, center - length, center + length ) );
    }

    LightSet* lights = new LightSet();
    if ( !light_set )
    {
        BBox bbox;
        PreprocessContext dummy_context;
        group->computeBounds( dummy_context, bbox );
        light_position = bbox[ 1 ] + bbox.diagonal() * 0.3;
    }
    lights->add( new PointLight( light_position, Color::white() ) );
    lights->setAmbientLight( new ArcAmbient( Color( RGB( 0.2, 0.2, 0.3 ) ),
                                             Color( RGB( 0.2, 0.1, 0.1 ) ),
                                             Vector( 0.0, 0.0, 1.0 ) ) );

    AccelerationStructure *world = new DynBVH();
    world->setGroup( group );
    world->rebuild();
    Scene* scene = new Scene();
    scene->setBackground( new ConstantBackground( Color::white() * 0.1 ) );
    scene->setObject( world );
    scene->setLights( lights );

    return scene;

}
