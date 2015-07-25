
/************************************************************************
*    FILE NAME:       device.h
*
*    DESCRIPTION:     Class used for window and openGL management
************************************************************************/

// Physical component dependency
#include <system/device.h>

// Glew dependencies
#include <GL/glew.h>

// SDL lib dependencies
#include <SDL_opengl.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <utilities/settings.h>
#include <utilities/genfunc.h>
#include <common/size.h>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CDevice::CDevice()
    : m_pWindow(nullptr),
      m_context(nullptr),
      m_pScreenSurface(nullptr)
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CDevice::~CDevice()
{
}	// destructer


/***************************************************************************
*   desc:  Create the window and OpenGL context
 ****************************************************************************/
void CDevice::Create()
{
    // Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER ) < 0 )
        throw NExcept::CCriticalException("SDL could not initialize!", SDL_GetError() );

    // Use OpenGL 3.3 core
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, CSettings::Instance().GetMajorVersion() );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, CSettings::Instance().GetMinorVersion() );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, CSettings::Instance().GetProfile() );

    // Do we add stencil buffer
    if( CSettings::Instance().GetCreateStencilBuffer() )
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, CSettings::Instance().GetStencilBufferBitSize() );

    // Get the window size
    const CSize<int> size( CSettings::Instance().GetSize() );

    // Create window
    m_pWindow = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.GetW(), size.GetH(), SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
    if( m_pWindow == nullptr )
        throw NExcept::CCriticalException("Game window could not be created!", SDL_GetError() );

    // Create context
    m_context = SDL_GL_CreateContext( m_pWindow );
    if( m_context == nullptr )
        throw NExcept::CCriticalException("OpenGL context could not be created!", SDL_GetError() );

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if( glewError != GLEW_OK )
        throw NExcept::CCriticalException("Error initializing GLEW!", 
                    boost::str( boost::format("Error initializing GLEW (%s).\n\n%s\nLine: %s") % glewGetErrorString( glewError ) % __FUNCTION__ % __LINE__ ));

    // Enable/disable v-sync
    EnableVSync( CSettings::Instance().GetVSync() );

    // Set the full screen
    if( CSettings::Instance().GetFullScreen() )
        SetFullScreen( CSettings::Instance().GetFullScreen() );

    // Depth testing is off by default. Enable it?
    if( CSettings::Instance().GetEnableDepthBuffer() )
        glEnable(GL_DEPTH_TEST);

    // Create the projection matrixes
    CreateProjMatrix();
    
}   // Create


/***************************************************************************
*   desc:  Enable/disable v-sync
 ****************************************************************************/
void CDevice::EnableVSync( bool enable )
{
    if( SDL_GL_SetSwapInterval( (enable == true) ? 1 : 0 ) < 0 )
        NGenFunc::PostDebugMsg( boost::str( boost::format("Warning: Unable to set VSync! SDL GL Error: %s") % SDL_GetError() ) );

}   // EnableVSync


/************************************************************************
*    desc:  Create the projection matrixes
************************************************************************/
void CDevice::CreateProjMatrix()
{
    // Calc the aspect ratio
    float aspectRatio = CSettings::Instance().GetSize().GetW() / 
                        CSettings::Instance().GetSize().GetH();

    m_perspectiveMatrix.PerspectiveFovRH(
        CSettings::Instance().GetViewAngle(),
        aspectRatio,
        CSettings::Instance().GetMinZdist(),
        CSettings::Instance().GetMaxZdist() );

    m_orthographicMatrix.OrthographicRH(
        CSettings::Instance().GetDefaultSize().GetW(),
        CSettings::Instance().GetDefaultSize().GetH(),
        CSettings::Instance().GetMinZdist(),
        CSettings::Instance().GetMaxZdist() );

}   // CreateProjMatrix


/************************************************************************
*    desc:  Get the projection matrix
************************************************************************/
const CMatrix & CDevice::GetProjectionMatrix( NDefs::EProjectionType type ) const
{
    if( type == NDefs::EPT_PERSPECTIVE )
        return m_perspectiveMatrix;
    else
        return m_orthographicMatrix;

}   // GetProjectionMatrix


/***************************************************************************
*   desc:  Show/Hide the Window
 ****************************************************************************/
void CDevice::ShowWindow( bool visible )
{
    if( visible )
        SDL_ShowWindow( m_pWindow );
    else
        SDL_HideWindow( m_pWindow );

}   // hide


/***************************************************************************
*   desc:  Set full screen or windowed mode
 ****************************************************************************/
void CDevice::SetFullScreen( bool fullscreen )
{
    int flag(0);

    if( fullscreen )
        flag = SDL_WINDOW_FULLSCREEN;

    if( SDL_SetWindowFullscreen( m_pWindow, flag ) < 0 )
        NGenFunc::PostDebugMsg( boost::str( boost::format("Warning: Unable to set full screen! SDL GL Error: %s") % SDL_GetError() ) );

}   // SetFullScreen


/***************************************************************************
*   desc:  Get the OpenGl Context
 ****************************************************************************/
SDL_GLContext CDevice::GetContext()
{
    return m_context;

}   // GetContext


/***************************************************************************
*   desc:  Get the SDL window
 ****************************************************************************/
SDL_Window * CDevice::GetWindow()
{
    return m_pWindow;

}   // GetWindow


/***************************************************************************
*   desc:  Get the SDL window surface
 ****************************************************************************/
SDL_Surface * CDevice::GetSurface()
{
    // Create the window surface. We don't own the surface so DON'T FREE
    // This is for 2D software rendering for loading screens
    if( m_pScreenSurface == nullptr )
    {
        if( m_pWindow == nullptr )
            throw NExcept::CCriticalException("Game window has not be created!", SDL_GetError() );
        
        m_pScreenSurface = SDL_GetWindowSurface( m_pWindow );
        if( m_pScreenSurface == nullptr )
            throw NExcept::CCriticalException("Surface Creation error!", SDL_GetError());
    }
    
    return m_pScreenSurface;

}   // GetSurface


/***************************************************************************
*   desc:  Init current gamepads plugged in at startup
 ****************************************************************************/
void CDevice::InitStartupGamepads()
{
    #ifndef _WINDOWS
    int newMappings = SDL_GameControllerAddMappingsFromFile("data/settings/gamecontrollerdb.txt");
    NGenFunc::PostDebugMsg( boost::str( boost::format("New controller mappings found: %d") % newMappings ) );
    #endif
    
    for( int i = 0; i < SDL_NumJoysticks(); ++i )
        AddGamepad( i );

}   // InitStartupGamepads


/***************************************************************************
*   desc:  Add/Remove the game pad
 ****************************************************************************/
void CDevice::AddGamepad( int id )
{
    if( SDL_IsGameController( id ) )
    {
        SDL_GameController * pGamePad = SDL_GameControllerOpen(id);
        if( pGamePad != NULL )
        {
            m_pGamepadMap.insert( std::make_pair(id, pGamePad) );
        }
    }
    
}   // AddGamepad

void CDevice::RemoveGamepad( int id )
{
    auto iter = m_pGamepadMap.find( id );
    if( iter != m_pGamepadMap.end() )
    {
        SDL_GameControllerClose( iter->second );
        m_pGamepadMap.erase( iter );
    }

}   // RemoveGamepad
