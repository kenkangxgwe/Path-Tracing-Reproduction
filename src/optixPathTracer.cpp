#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glew.h>
#  if defined( _WIN32 )
#    include <GL/wglew.h>
#    include <GL/freeglut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include "optixPathTracer.h"
#include <sutil.h>
#include <Arcball.h>
#include "Camera.h"
#include <algorithm>
#include <OptiXMesh.h>
#include <cstring>
#include <iostream>
#include <stdint.h>

using namespace optix;
const char* const PROGRAM_NAME = "optixPathTracer";

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------
Context        context = 0;
uint32_t       width  = 512;
uint32_t       height = 512;
bool           use_pbo = true;

int            frame_number = 1;
int            sqrt_num_samples = 1;
int            rr_begin_depth = 1;
Program        pgram_intersection = 0;
Program        pgram_bounding_box = 0;

// Camera state
Matrix4x4      lastFrameInvTransMat;
bool           camera_changed = true;
sutil::Arcball arcball;
const float fov = 35.0f;
Camera *myCam = new Camera(fov, width, height);
// field of view

// Mouse state
int2           mouse_prev_pos;
int            mouse_button;


//------------------------------------------------------------------------------
//
// Forward decls
//
//------------------------------------------------------------------------------

std::string ptxPath( const std::string& cuda_file );
Buffer getOutputBuffer();
void destroyContext();
void registerExitHandler();
void createContext();
void loadGeometry();
void loadGeometryFromFile(const std::string &filename);
void updateCamera();
void glutInitialize( int* argc, char** argv );
void glutRun();

void glutDisplay();
void glutKeyboardPress( unsigned char k, int x, int y );
void glutMousePress( int button, int state, int x, int y );
void glutMouseMotion( int x, int y);
void glutResize( int w, int h );


//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------

void updateCamera()
{
	myCam->updateCamera(width, height);
	if (camera_changed)
		frame_number = 1;
	camera_changed = false;
	context["frame_number"]->setUint(frame_number++);
    context[ "eye"]->setFloat( myCam->getEye());
    context[ "U"  ]->setFloat( myCam->getU());
    context[ "V"  ]->setFloat( myCam->getV());
    context[ "W"  ]->setFloat( myCam->getW());
}

std::string ptxPath( const std::string& cuda_file )
{
    return
        std::string(sutil::samplesPTXDir()) +
        "/" + std::string(PROGRAM_NAME) + "_generated_" +
        cuda_file +
        ".ptx";
}


Buffer getOutputBuffer()
{
    return context[ "output_buffer" ]->getBuffer();
}


void destroyContext()
{
    if( context )
    {
        context->destroy();
        context = 0;
    }
}


void registerExitHandler()
{
    // register shutdown handler
#ifdef _WIN32
    glutCloseFunc( destroyContext );  // this function is freeglut-only
#else
    atexit( destroyContext );
#endif
}


void setMaterial(
        GeometryInstance& gi,
        Material material,
        const std::string& color_name,
        const float3& color)
{
    gi->addMaterial(material);
    gi[color_name]->setFloat(color);
}


GeometryInstance createParallelogram(
        const float3& anchor,
        const float3& offset1,
        const float3& offset2)
{
    Geometry parallelogram = context->createGeometry();
    parallelogram->setPrimitiveCount( 1u );
    parallelogram->setIntersectionProgram( pgram_intersection );
    parallelogram->setBoundingBoxProgram( pgram_bounding_box );

    float3 normal = normalize( cross( offset1, offset2 ) );
    float d = dot( normal, anchor );
    float4 plane = make_float4( normal, d );

    float3 v1 = offset1 / dot( offset1, offset1 );
    float3 v2 = offset2 / dot( offset2, offset2 );

    parallelogram["plane"]->setFloat( plane );
    parallelogram["anchor"]->setFloat( anchor );
    parallelogram["v1"]->setFloat( v1 );
    parallelogram["v2"]->setFloat( v2 );

    GeometryInstance gi = context->createGeometryInstance();
    gi->setGeometry(parallelogram);
    return gi;
}


void createContext()
{
    context = Context::create();
    context->setRayTypeCount( 2 );

    context->setEntryPointCount( 1 );
    context->setStackSize( 1800 );

	context->setPrintEnabled(1);
    context[ "scene_epsilon"                  ]->setFloat( 1.e-3f );
    context[ "pathtrace_ray_type"             ]->setUint( 0u );
    context[ "pathtrace_shadow_ray_type"      ]->setUint( 1u );
    context[ "rr_begin_depth"                 ]->setUint( rr_begin_depth );

    Buffer buffer = sutil::createOutputBuffer( context, RT_FORMAT_FLOAT4, width, height, use_pbo );
    context["output_buffer"]->set( buffer );

    Buffer positionbuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, use_pbo);
    context["position_buffer"]->set(positionbuffer);

    Buffer normalbuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, use_pbo);
    context["normal_buffer"]->set(normalbuffer);

    Buffer historybuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT4, width, height, use_pbo);
    context["history_buffer"]->set(historybuffer);

    // Setup programs
    const std::string cuda_file = std::string( PROGRAM_NAME ) + ".cu";
    const std::string ptx_path = ptxPath( cuda_file );
    context->setRayGenerationProgram( 0, context->createProgramFromPTXFile( ptx_path, "pathtrace_camera" ) );
    context->setExceptionProgram( 0, context->createProgramFromPTXFile( ptx_path, "exception" ) );
    context->setMissProgram( 0, context->createProgramFromPTXFile( ptx_path, "miss" ) );

    context[ "sqrt_num_samples" ]->setUint( sqrt_num_samples );
    context[ "bad_color"        ]->setFloat( 1000000.0f, 0.0f, 1000000.0f ); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
    context[ "bg_color"         ]->setFloat( make_float3(0.0f) );
}


void loadGeometryFromFile(const std::string &filename)
{
	//TODO: make it a class and be able to load a different scene.
    // Light buffer
    ParallelogramLight light;
    light.corner   = make_float3( -0.24f, 1.88f, -0.22f);
    light.v1       = make_float3( 0.0f, 0.0f, 0.38f);
    light.v2       = make_float3( 0.47f, 0.0f, 0.0f);
    light.normal   = normalize( cross(light.v1, light.v2) );
    light.emission = make_float3( 17.0f, 12.0f, 4.0f );
 //   ParallelogramLight light;
 //   light.corner   = make_float3( -0.24f, 5.88f, -0.22f);
 //   light.v1       = make_float3( 0.0f, 0.0f, 0.38f);
 //   light.v2       = make_float3( 0.47f, 0.0f, 0.0f);
 //   light.normal   = normalize( cross(light.v1, light.v2) );
 //   light.emission = make_float3( 57.0f, 52.0f, 20.0f );

 //   ParallelogramLight sunlight;
 //   sunlight.corner   = make_float3( 8.24f, 9.88f, -0.22f);
 //   sunlight.v1       = make_float3( 0.0f, 0.0f, 0.38f);
 //   sunlight.v2       = make_float3( 0.47f, 0.47f, 0.0f);
 //   sunlight.normal   = normalize( cross(light.v1, light.v2) );
 //   sunlight.emission = make_float3( 80.0f, 80.0f, 50.0f );
	//ParallelogramLight lightArray[2] = { light, sunlight };

    Buffer light_buffer = context->createBuffer( RT_BUFFER_INPUT );
    light_buffer->setFormat( RT_FORMAT_USER );
    light_buffer->setElementSize( sizeof( ParallelogramLight ) );
    light_buffer->setSize( 1u );
    memcpy( light_buffer->map(), &light, sizeof( light) );
    light_buffer->unmap();
    context["lights"]->setBuffer( light_buffer );


    // Set up material
    const std::string cuda_file = std::string( PROGRAM_NAME ) + ".cu";
    std::string ptx_path = ptxPath( cuda_file );
    Program diffuse_ch = context->createProgramFromPTXFile( ptx_path, "diffuse" );
    Program diffuse_ah = context->createProgramFromPTXFile( ptx_path, "shadow" );

    Material diffuse_light = context->createMaterial();
    Program diffuse_em = context->createProgramFromPTXFile( ptx_path, "diffuseEmitter" );
    diffuse_light->setClosestHitProgram( 0, diffuse_em );

	OptiXMesh mesh;
	mesh.context = context;
	mesh.closest_hit = diffuse_ch;
	mesh.any_hit = diffuse_ah;
	loadMesh(filename, mesh);

    GeometryGroup shadow_group = context->createGeometryGroup();
    shadow_group->addChild( mesh.geom_instance );
    shadow_group->setAcceleration( context->createAcceleration( "Trbvh" ) );
	Transform transformedToWorld = context->createTransform();
	transformedToWorld->setMatrix(false, Matrix4x4::scale(make_float3(1.0f)).getData(), NULL);
	transformedToWorld->setChild(shadow_group);
    context[ "top_shadower" ]->set( transformedToWorld ); 
	transformedToWorld->validate();

    context["top_object"]->set(transformedToWorld);
}

void loadGeometry()
{
	//TODO: make it a class and be able to load a different scene.
    // Light buffer
    ParallelogramLight light;
    light.corner   = make_float3( 343.0f, 548.6f, 227.0f);
    light.v1       = make_float3( -130.0f, 0.0f, 0.0f);
    light.v2       = make_float3( 0.0f, 0.0f, 105.0f);
    light.normal   = normalize( cross(light.v1, light.v2) );
    light.emission = make_float3( 15.0f, 15.0f, 5.0f );

    Buffer light_buffer = context->createBuffer( RT_BUFFER_INPUT );
    light_buffer->setFormat( RT_FORMAT_USER );
	std::cout << sizeof(ParallelogramLight) << std::endl;
    light_buffer->setElementSize( sizeof( ParallelogramLight ) );
    light_buffer->setSize( 1u );
    memcpy( light_buffer->map(), &light, sizeof( light ) );
    light_buffer->unmap();
    context["lights"]->setBuffer( light_buffer );


    // Set up material
    const std::string cuda_file = std::string( PROGRAM_NAME ) + ".cu";
    std::string ptx_path = ptxPath( cuda_file );
    Material diffuse = context->createMaterial();
    Program diffuse_ch = context->createProgramFromPTXFile( ptx_path, "diffuse" );
    Program diffuse_ah = context->createProgramFromPTXFile( ptx_path, "shadow" );
    diffuse->setClosestHitProgram( 0, diffuse_ch );
    diffuse->setAnyHitProgram( 1, diffuse_ah );

    Material diffuse_light = context->createMaterial();
    Program diffuse_em = context->createProgramFromPTXFile( ptx_path, "diffuseEmitter" );
    diffuse_light->setClosestHitProgram( 0, diffuse_em );

    // Set up parallelogram programs
    ptx_path = ptxPath( "parallelogram.cu" );
    pgram_bounding_box = context->createProgramFromPTXFile( ptx_path, "bounds" );
    pgram_intersection = context->createProgramFromPTXFile( ptx_path, "intersect" );

    // create geometry instances
    std::vector<GeometryInstance> gis;

    const float3 white = make_float3( 0.8f, 0.8f, 0.8f );
    const float3 green = make_float3( 0.05f, 0.8f, 0.05f );
    const float3 red   = make_float3( 0.8f, 0.05f, 0.05f );
    const float3 light_em = make_float3( 15.0f, 15.0f, 5.0f );

    // Floor
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Ceiling
    gis.push_back( createParallelogram( make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Back wall
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 559.2f),
                                        make_float3( 0.0f, 548.8f, 0.0f),
                                        make_float3( 556.0f, 0.0f, 0.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Right wall
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", green);

    // Left wall
    gis.push_back( createParallelogram( make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", red);

    // Short block
    gis.push_back( createParallelogram( make_float3( 130.0f, 165.0f, 65.0f),
                                        make_float3( -48.0f, 0.0f, 160.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 290.0f, 0.0f, 114.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -50.0f, 0.0f, 158.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 130.0f, 0.0f, 65.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 82.0f, 0.0f, 225.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 48.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 240.0f, 0.0f, 272.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, -47.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Tall block
    gis.push_back( createParallelogram( make_float3( 423.0f, 330.0f, 247.0f),
                                        make_float3( -158.0f, 0.0f, 49.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 423.0f, 0.0f, 247.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 472.0f, 0.0f, 406.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, 50.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 314.0f, 0.0f, 456.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -49.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 265.0f, 0.0f, 296.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 158.0f, 0.0f, -49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Create shadow group (no light)
    GeometryGroup shadow_group = context->createGeometryGroup(gis.begin(), gis.end());
    shadow_group->setAcceleration( context->createAcceleration( "Trbvh" ) );
	Transform transformedShadow = context->createTransform();
	transformedShadow->setMatrix(false, Matrix4x4::scale(make_float3(0.5f)).getData(), NULL);
	transformedShadow->setChild(shadow_group);
    context["top_shadower"]->set( transformedShadow );
	transformedShadow->validate();

    // Light
    gis.push_back( createParallelogram( make_float3( 343.0f, 548.6f, 227.0f),
                                        make_float3( -130.0f, 0.0f, 0.0f),
                                        make_float3( 0.0f, 0.0f, 105.0f) ) );
    setMaterial(gis.back(), diffuse_light, "emission_color", light_em);

    // Create geometry group
    GeometryGroup geometry_group = context->createGeometryGroup(gis.begin(), gis.end());
    geometry_group->setAcceleration( context->createAcceleration( "Trbvh" ) );
	Transform transformedGeometry = context->createTransform();
	transformedGeometry->setMatrix(false, Matrix4x4::scale(make_float3(0.5f)).getData(), NULL);
	transformedGeometry->setChild(geometry_group);
    context["top_object"]->set( transformedGeometry );
	transformedGeometry->validate();
}


void glutInitialize( int* argc, char** argv )
{
    glutInit( argc, argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize( width, height );
    glutInitWindowPosition( 100, 100 );                                               
    glutCreateWindow( PROGRAM_NAME );
    glutHideWindow();                                                              
}


void glutRun()
{
    // Initialize GL state
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1 );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, width, height);

    glutShowWindow();
    glutReshapeWindow( width, height);

    // register glut callbacks
    glutDisplayFunc( glutDisplay );
    glutIdleFunc( glutDisplay );
    glutReshapeFunc( glutResize );
    glutKeyboardFunc( glutKeyboardPress );
    glutMouseFunc( glutMousePress );
    glutMotionFunc( glutMouseMotion );

    registerExitHandler();

    glutMainLoop();
}


//------------------------------------------------------------------------------
//
//  GLUT callbacks
//
//------------------------------------------------------------------------------

void glutDisplay()
{
    updateCamera();

    context->launch( 0, width, height );

    sutil::displayBufferGL( getOutputBuffer() );

    {
      static unsigned frame_count = 0;
      sutil::displayFps( frame_count++ );
    }
    glutSwapBuffers();

}


void glutKeyboardPress( unsigned char k, int x, int y )
{
	//TODO: keyboards on camera.

    switch( k )
    {
        case( 'x' ):
        case( 27 ): // ESC
        {
            destroyContext();
            exit(0);
        }
        case( 'u' ):
        {
            const std::string outputImage = std::string(PROGRAM_NAME) + ".ppm";
            std::cerr << "Saving current frame to '" << outputImage << "'\n";
            sutil::displayBufferPPM( outputImage.c_str(), getOutputBuffer() );
            break;
        }
		case ('w'):
		{
			myCam->keyIn(MOVE_DIR::FORWARD);
			camera_changed = true;
			break;
		}
		case ('s'):
		{
			myCam->keyIn(MOVE_DIR::BACKWARD);
			camera_changed = true;
			break;
		}
		case ('a'):
		{
			myCam->keyIn(MOVE_DIR::LEFT);
			camera_changed = true;
			break;
		}
		case ('d'):
		{
			myCam->keyIn(MOVE_DIR::RIGHT);
			camera_changed = true;
			break;
		}
		case ('q'):
		{
			myCam->keyIn(MOVE_DIR::UP);
			camera_changed = true;
			break;
		}
		case ('e'):
		{
			myCam->keyIn(MOVE_DIR::DOWN);
			camera_changed = true;
			break;
		}
    }
}


void glutMousePress( int button, int state, int x, int y )
{
    if( state == GLUT_DOWN )
    {
        mouse_button = button;
        mouse_prev_pos = make_int2( x, y );
    }
    else
    {
        // nothing
    }
}


void glutMouseMotion( int x, int y)
{
    if( mouse_button == GLUT_RIGHT_BUTTON )
    {
        const float dx = static_cast<float>( x - mouse_prev_pos.x ) /
                         static_cast<float>( width );
        const float dy = static_cast<float>( y - mouse_prev_pos.y ) /
                         static_cast<float>( height );
        const float dmax = fabsf( dx ) > fabs( dy ) ? dx : dy;
        const float scale = std::min<float>( dmax, 0.9f );
		myCam->setEye(myCam->getEye() + (myCam->getLookAt() - myCam->getEye()) * scale);
        camera_changed = true;
		lastFrameInvTransMat = myCam->getLastFrameInverseMat();
    }
    else if( mouse_button == GLUT_LEFT_BUTTON )
    {
        const float2 from = { static_cast<float>(mouse_prev_pos.x),
                              static_cast<float>(mouse_prev_pos.y) };
        const float2 to   = { static_cast<float>(x),
                              static_cast<float>(y) };

        const float2 a = { from.x / width, from.y / height };
        const float2 b = { to.x   / width, to.y   / height };

		myCam->setRotate(arcball.rotate(b, a));
        camera_changed = true;
		lastFrameInvTransMat = myCam->getLastFrameInverseMat();
    }

    mouse_prev_pos = make_int2( x, y );
}


void glutResize( int w, int h )
{
    if ( w == (int)width && h == (int)height ) return;

    camera_changed = true;
	lastFrameInvTransMat = myCam->getLastFrameInverseMat();

    width  = w;
    height = h;

    sutil::resizeBuffer( getOutputBuffer(), width, height );

    glViewport(0, 0, width, height);

    glutPostRedisplay();
}


//------------------------------------------------------------------------------
//
// Main
//
//------------------------------------------------------------------------------

void printUsageAndExit( const std::string& argv0 )
{
    std::cerr << "\nUsage: " << argv0 << " [options]\n";
    std::cerr <<
        "App Options:\n"
        "  -h | --help               Print this usage message and exit.\n"
        "  -f | --file               Save single frame to file and exit.\n"
        "  -n | --nopbo              Disable GL interop for display buffer.\n"
        "  -m | --mesh <mesh_file>   Specify path to mesh to be loaded.\n"
        "App Keystrokes:\n"
        "  q  Quit\n" 
        "  s  Save image to '" << PROGRAM_NAME << ".ppm'\n"
        << std::endl;

    exit(1);
}


int main( int argc, char** argv )
 {
    std::string out_file;
    std::string mesh_file = std::string( sutil::samplesDir() ) + "/data/cow.obj";
    for( int i=1; i<argc; ++i )
    {
        const std::string arg( argv[i] );

        if( arg == "-h" || arg == "--help" )
        {
            printUsageAndExit( argv[0] );
        }
        else if( arg == "-f" || arg == "--file"  )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << arg << "' requires additional argument.\n";
                printUsageAndExit( argv[0] );
            }
            out_file = argv[++i];
        }
        else if( arg == "-n" || arg == "--nopbo"  )
        {
            use_pbo = false;
        }
        else if( arg == "-m" || arg == "--mesh" )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << argv[i] << "' requires additional argument.\n";
                printUsageAndExit( argv[0] );
            }
            mesh_file = argv[++i];
        }
        else
        {
            std::cerr << "Unknown option '" << arg << "'\n";
            printUsageAndExit( argv[0] );
        }
    }

    try
    {
        glutInitialize( &argc, argv );

#ifndef __APPLE__
        glewInit();
#endif

        createContext();
        //setupCamera();
		//loadGeometry();
        loadGeometryFromFile("C:/Users/Anseren/Downloads/CornellBox/CornellBox-Original.obj");
        //loadGeometryFromFile("C:/Users/Anseren/Documents/GitHhub_Projects/Path-Tracing-Reproduction/breakfast_room/breakfast_room.obj");
        //loadGeometryFromFile("C:/ProgramData/NVIDIA Corporation/OptiX SDK 4.1.1/SDK/data/cow.obj");

        context->validate();

        if ( out_file.empty() )
        {
            glutRun();
        }
        else
        {
            updateCamera();

            context->launch( 0, width, height );
            sutil::displayBufferPPM( out_file.c_str(), getOutputBuffer() );
            destroyContext();
        }

        return 0;
    }
    SUTIL_CATCH( context->get() )
}
