
/************************************************************************
*    FILE NAME:       xdevice.cpp
*
*    DESCRIPTION:     For creating and managing a DirectX Device 
************************************************************************/

                             // Enables strict compiler error checking. Without this, the compiler doesn't
#define STRICT               // know the difference between many Windows data types. Very bad to leave out. Doesn't add to code size
#include <windows.h>         // Windows header file for creating windows programs. This is a must have.
#include <windowsx.h>        // Macros the makes windows programming easer and more readable. Doesn't add to code size

// Physical component dependency
#include <system/xdevice.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <common/xquad2d.h>

// Turn off the data type conversion warning (ie. int to float, float to int etc.)
// We do this all the time in 3D. Don't need to be bugged by it all the time.
#pragma warning(disable : 4244)

/************************************************************************
*    desc: Constructor                                                             
************************************************************************/
CXDevice::CXDevice()
        : maximumZDist(0.0f),
          minimumZDist(0.0f),
          squarePercentage(0.0f),
          frustrumYRatio(0.0f),
          zPassStencilBufferMode(false),
          bufferClearMask(0),
          depthStencilBufferClearMask(0),
          postProcBufIndex(0),
          hWnd(NULL)
{
}


/************************************************************************
*    desc: Destructor                                                             
************************************************************************/
CXDevice::~CXDevice()
{
}


/************************************************************************                                                            
*    desc:  Free all allocations
************************************************************************/
void CXDevice::Free()
{
    // function for releasing device created members 
    // for device lost reset.
    ReleaseDeviceCreatedMembers();

    spDXDevice.Release();
    spDXInstance.Release();

}   // Free


/************************************************************************                                                            
*    desc:  Release device created members
************************************************************************/
void CXDevice::ReleaseDeviceCreatedMembers()
{
    spShadowMapBufferTexture.Release();
    spShadowMapBufferSurface.Release();
    spDisplaySurface.Release();

    pPostProcSurfaceVec.clear();
    pPostProcTextVec.clear();
    spPostProcessVertBuf.Release();
    
}   // ReleaseAllDeviceCreatedMembers


/************************************************************************
*    desc:  Create the DirectX device for rendering
* 
*    return: bool - true on success or false on fail
************************************************************************/
void CXDevice::CreateXDevice( HWND hwnd )
{
    hWnd = hwnd;
    D3DDISPLAYMODE dxdm;
    HRESULT hresult;
    D3DFORMAT depthFormat[] = {D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16, (D3DFORMAT)0};
    D3DFORMAT selectedDepthFormat;
    int counter = 0;

    // Set the initial buffer clear mask
    if( CSettings::Instance().GetClearTargetBuffer() )
        bufferClearMask = D3DCLEAR_TARGET;

    // Recored the max and min z distances
    maximumZDist = CSettings::Instance().GetMaxZdist();
    minimumZDist = CSettings::Instance().GetMinZdist();
    viewAngle = D3DXToRadian( CSettings::Instance().GetViewAngle() );

    // Calc the aspect ratio
    float aspectRatio = CSettings::Instance().GetSize().w / 
                        CSettings::Instance().GetSize().h;

    // Calc the square percentage
    squarePercentage = CSettings::Instance().GetSize().h /
                       CSettings::Instance().GetSize().w;

    // calc the y ratio to the frustrum
    frustrumYRatio = squarePercentage / aspectRatio;

    // Create the DirectX 9 instance
    spDXInstance.Attach( Direct3DCreate9( D3D_SDK_VERSION ) );

    if( spDXInstance == NULL )
        throw NExcept::CCriticalException("DirectX Init Error",
            boost::str( boost::format("Error creating an instance of DirectX9.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // Get the caps to see if we can do hardware vertex processing
    if( FAILED( spDXInstance->GetDeviceCaps( D3DADAPTER_DEFAULT,
                                             D3DDEVTYPE_HAL,
                                             &d3dCaps ) ) )
        throw NExcept::CCriticalException("DirectX Init Error",
            boost::str( boost::format("Error getting device capabilities of video card.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // Get info about the video card
    if( FAILED( spDXInstance->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &dxdm ) ) )
        throw NExcept::CCriticalException("DirectX Init Error",
            boost::str( boost::format("Error getting adapter display mode of video card.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // Check for the best z buffer format
    do
    {
        selectedDepthFormat = depthFormat[counter];

        hresult = spDXInstance->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                                   D3DDEVTYPE_HAL,
                                                   dxdm.Format,
                                                   D3DUSAGE_DEPTHSTENCIL,
                                                   D3DRTYPE_SURFACE,
                                                   depthFormat[counter++] );
    }
    while( hresult != D3D_OK && depthFormat[counter] );

    // Tell them we couldn't secure a depth buffer
    if( hresult != D3D_OK )
        throw NExcept::CCriticalException("DirectX Init Error",
            boost::str( boost::format("Video card does not support depth buffering.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // clear out the structure
    memset( &dxpp, 0, sizeof(dxpp) );

    // Full screen or windowed mode
    dxpp.Windowed = !CSettings::Instance().GetFullScreen();

    // Fill out the remaining items of the structure
    dxpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    dxpp.BackBufferFormat       = dxdm.Format;
    dxpp.BackBufferWidth        = (int)CSettings::Instance().GetSize().w;
    dxpp.BackBufferHeight       = (int)CSettings::Instance().GetSize().h;
    dxpp.hDeviceWindow          = hWnd;
    dxpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
    dxpp.BackBufferCount        = 1;

    if( CSettings::Instance().GetTripleBuffering() )
        dxpp.BackBufferCount = 2;

    if( CSettings::Instance().GetVSync() )
        dxpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Do we need the depth stencil? Z buffer and stencil are both created at the same time in hardware
    if( CSettings::Instance().GetCreateDepthStencilBuffer() )
    {
        dxpp.EnableAutoDepthStencil = true;
        dxpp.AutoDepthStencilFormat = selectedDepthFormat;
        depthStencilBufferClearMask = D3DCLEAR_ZBUFFER;

        if( CSettings::Instance().GetClearStencilBuffer() )
            depthStencilBufferClearMask |= D3DCLEAR_STENCIL;

        // check for stencil buffer support
        if( spDXInstance->CheckDepthStencilMatch( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dxpp.BackBufferFormat, dxpp.BackBufferFormat, D3DFMT_D24FS8 ) != D3D_OK )
        {
            if( spDXInstance->CheckDepthStencilMatch( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dxpp.BackBufferFormat, dxpp.BackBufferFormat, D3DFMT_D24S8 ) != D3D_OK )
            {
                if( spDXInstance->CheckDepthStencilMatch( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dxpp.BackBufferFormat, dxpp.BackBufferFormat, D3DFMT_D24X4S4 ) != D3D_OK )
                {
                    if( spDXInstance->CheckDepthStencilMatch( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dxpp.BackBufferFormat, dxpp.BackBufferFormat, D3DFMT_D24FS8 ) != D3D_OK )
                    {
                        throw NExcept::CCriticalException("DirectX Init Error",
                            boost::str( boost::format("Video card does not support hardware stencil buffer.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));
                    }
                    else // support for a d15s1 buffer
                    {
                        dxpp.AutoDepthStencilFormat = D3DFMT_D15S1;
                    }
                }
                else // support for a d24x4s4 buffer
                {
                    dxpp.AutoDepthStencilFormat = D3DFMT_D24X4S4;
                }
            }
            else // support for a d24s8 buffer
            {
                dxpp.AutoDepthStencilFormat = D3DFMT_D24S8;
            }
        }
        else //support for a d24fs8 buffer
        {
            dxpp.AutoDepthStencilFormat = D3DFMT_D24FS8;
        }
    }

    DWORD dwBehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    if( d3dCaps.VertexProcessingCaps != 0 )    
        dwBehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    // Create the device
    if( FAILED( hresult = spDXInstance->CreateDevice( D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL,
                                         hWnd,
                                         dwBehaviorFlags | D3DCREATE_MULTITHREADED,
                                         &dxpp, 
                                         &spDXDevice ) ) )
    {
        DisplayError( hresult );
    }

    // Calculate the projection matrixes
    CalcProjMatrix( CSettings::Instance().GetSize(),
                    CSettings::Instance().GetDefaultSize() );

    // Get the initial texture memory amount
    initialVideoMemory = GetAvailableTextureMem();
    
    // Buffers and render states that need to be reset/recreated after a device rest
    ResetXDevice();

}   // CreateXDevice


/************************************************************************
*    desc:  Get the available texture memory
************************************************************************/
uint CXDevice::GetAvailableTextureMem()
{
    return spDXDevice->GetAvailableTextureMem() >> 20;

}	// GetAvailableTextureMem


/************************************************************************
*    desc:  Get used amount of memory used for video
************************************************************************/
uint CXDevice::GetVideoMemUsed()
{
    return initialVideoMemory - GetAvailableTextureMem();

}	// GetVideoMemUsed


/************************************************************************
*    desc:  Get the max texture width
************************************************************************/
uint CXDevice::GetMaxTextureWidth()
{
    return d3dCaps.MaxTextureWidth;

}	// GetMaxTextureWidth


/************************************************************************
*    desc:  Get the max texture height
************************************************************************/
uint CXDevice::GetMaxTextureHeight()
{
    return d3dCaps.MaxTextureHeight;

}	// GetMaxTextureWidth


/************************************************************************
*    desc:  Calculate the projection matrixes
************************************************************************/
void CXDevice::CalcProjMatrix( const CSize<float> & res, const CSize<float> & defSize )
{
    D3DXMatrixPerspectiveFovLH( &perspectiveMatrix,
                                viewAngle,
                                res.w / res.h,
                                minimumZDist,
                                maximumZDist );

    D3DXMatrixOrthoLH( &orthographicMatrix,
                       defSize.w,
                       defSize.h,
                       minimumZDist,
                       maximumZDist);

}	// CalcProjMatrix


/************************************************************************
*    desc:  Buffers and render states that need to be 
*           reset/recreated after a device rest
************************************************************************/
void CXDevice::ResetXDevice()
{
    HRESULT hresult;

    postProcBufIndex = 0;

    // Grab the surface of the display buffer
    if( FAILED( hresult = spDXDevice->GetRenderTarget( 0, &spDisplaySurface ) ) )
        DisplayError( hresult );

    // Create the shadow map buffer
    if( CSettings::Instance().GetCreateShadowMapBuffer() )
    {
        CSize<int> size = CSettings::Instance().GetShadowMapBufferSize();

        if( FAILED( hresult = spDXDevice->CreateTexture( size.w, size.h, 1,
                                                 D3DUSAGE_RENDERTARGET, D3DFMT_R32F,
                                                 D3DPOOL_DEFAULT, &spShadowMapBufferTexture,
                                                 NULL ) ) )
        {
           DisplayError( hresult );
        }

        // Grab the surface of the shadow map buffer
        if( FAILED( hresult = spShadowMapBufferTexture->GetSurfaceLevel( 0, &spShadowMapBufferSurface ) ) )
            DisplayError( hresult );
    }

    // Create post-process buffer
    if( CSettings::Instance().GetCreatePostProcBuf() )
    {
        for( size_t i = 0; i < CSettings::Instance().GetPostProcBufCount(); ++i )
        {
            CComPtr<IDirect3DTexture9> spPostProcessBufferTexture;
            CComPtr<IDirect3DSurface9> spPostProcessBufferSurface;

            // Get the info about this post process buffer
            const CSettings::CPostProcBuff & postProcInfo = CSettings::Instance().GetPostProcBufInfo(static_cast<int>(i));

            if( FAILED( hresult = spDXDevice->CreateTexture( (UINT)(GetBufferWidth() * postProcInfo.scale),
                                                             (UINT)(GetBufferHeight() * postProcInfo.scale),
                                                             1, D3DUSAGE_RENDERTARGET, dxpp.BackBufferFormat,
                                                             D3DPOOL_DEFAULT, &spPostProcessBufferTexture,
                                                             NULL ) ) )
            {
               DisplayError( hresult );
            }

            // Grab the surface of the post process buffer
            if( FAILED( hresult = spPostProcessBufferTexture->GetSurfaceLevel( 0, &spPostProcessBufferSurface ) ) )
                DisplayError( hresult );

            // Add the texture and surface to our vectors
            pPostProcTextVec.push_back( spPostProcessBufferTexture );
            pPostProcSurfaceVec.push_back( spPostProcessBufferSurface );
        }
    }

    // Create the post process vertex buffer
    CreatePostProcessVertexBuffer();

    // Turn on the zbuffer
    if( CSettings::Instance().GetCreateDepthStencilBuffer() )
    {
        spDXDevice->SetRenderState( D3DRS_ZWRITEENABLE, true );
        spDXDevice->SetRenderState( D3DRS_ZENABLE, CSettings::Instance().GetEnableDepthBuffer() );
    }

    // Disable lighting -  not needed because shader lighting is used
    spDXDevice->SetRenderState( D3DRS_LIGHTING, false );

    // Turn on/off point sprites
    spDXDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, false );

    // Allow sprites to be scaled with distance
    spDXDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  true );

    // Float value that specifies the minimum size of point primitives. Point primitives are clamped to this size during rendering.
    spDXDevice->SetRenderState( D3DRS_POINTSIZE_MIN, FtoDW(0.0) );
    // Not needed unless you want to specify a size smaller then video card max
    //spDXDevice->SetRenderState( D3DRS_POINTSIZE_MAX, FtoDW(32.0) );
    spDXDevice->SetRenderState( D3DRS_POINTSCALE_A,  FtoDW(0.0) );
    spDXDevice->SetRenderState( D3DRS_POINTSCALE_B,  FtoDW(0.0) );
    spDXDevice->SetRenderState( D3DRS_POINTSCALE_C,  FtoDW(1.0) );

    //////////////////////////////////////////////////
    // Set the alpha blend states but they are not 
    // used until the alpha rendering state is enable
    //////////////////////////////////////////////////

    // Turn on alpha blending
    spDXDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );

    // source blend factor		
    spDXDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ); // D3DBLEND_SRCALPHA D3DBLEND_ONE

    // destination blend factor
    spDXDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ); // D3DBLEND_INVSRCALPHA D3DBLEND_ONE

    // alpha from texture
    spDXDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

    spDXDevice->SetRenderState( D3DRS_ALPHATESTENABLE, true );
    spDXDevice->SetRenderState( D3DRS_ALPHAREF, 0x01 );
    spDXDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

    // Enable culling
    spDXDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

}	// ResetXDevice


/************************************************************************
*    desc:  Handle the device reset
************************************************************************/
void CXDevice::HandleDeviceReset( const CSize<float> & res, bool vSync, bool windowed )
{
    // function for releasing device created members for device
    // lost reset. The reset will error if these are not released
    ReleaseDeviceCreatedMembers();

    // Set the resolution
    dxpp.BackBufferWidth  = res.w;
    dxpp.BackBufferHeight = res.h;

    // Set the windowed/full screen mode
    dxpp.Windowed = windowed;

    // Set the vsync
    dxpp.PresentationInterval = vSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

    // All things created by the device that is not managed 
    // need to be Released before the device can be reset
    if( spDXDevice->Reset( &dxpp ) == D3DERR_INVALIDCALL )
        throw NExcept::CCriticalException("DirectX Reset Error",
            boost::str( boost::format("Call to Reset() failed with D3DERR_INVALIDCALL!\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // Buffers and render states that need to be reset/recreated after a device reset
    ResetXDevice();

}	// HandleDeviceReset


/************************************************************************
*    desc:  Clear the buffers
************************************************************************/
void CXDevice::ClearBuffers()
{
    if( bufferClearMask > 0 )
        spDXDevice->Clear( 0, NULL, bufferClearMask, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );

}	// ClearBuffers


/************************************************************************
*    desc:  Clear the z-buffer only
************************************************************************/
void CXDevice::ClearZBuffer()
{
    if( CSettings::Instance().GetCreateDepthStencilBuffer() )
        spDXDevice->Clear( 0, NULL, depthStencilBufferClearMask, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );

}	// ClearZBuffer


/************************************************************************
*    desc:  Clear the stencil buffer only
************************************************************************/
void CXDevice::ClearStencilBuffer()
{
    if( CSettings::Instance().GetClearStencilBuffer() )
        spDXDevice->Clear( 0, NULL, D3DCLEAR_STENCIL, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );

}	// ClearStencilBuffer


/************************************************************************
*    desc:  Get the texture filtering
************************************************************************/
int CXDevice::GetTextureFiltering( CSettings::ETextFilter textFilter )
{
    int result = D3DTEXF_POINT;

    if( textFilter == CSettings::ETF_LINEAR || d3dCaps.MaxAnisotropy < 2 )
        result = D3DTEXF_LINEAR;

    // Anisotropic filting for best possible quality
    else if( textFilter >= CSettings::ETF_ANISOTROPIC_2X )
        result = D3DTEXF_ANISOTROPIC;

    return result;

}	// GetTextureFiltering


/************************************************************************
*    desc:  Get the anisotropic filtering
************************************************************************/
int CXDevice::GetAnisotropicFiltering( CSettings::ETextFilter textFilter )
{
    if( textFilter >= CSettings::ETF_ANISOTROPIC_2X )
        return textFilter;
    else
        return 0;

}	// GetAnisotropicFiltering


/***************************************************************************
*    desc:  Create the post process vertex buffer
****************************************************************************/
void CXDevice::CreatePostProcessVertexBuffer()
{
    // Setup the 2d quad
    CXQuad2D quad2D( GetBufferWidth(), GetBufferHeight() );

    // create the vertex buffer for the 2d quad
    if( D3D_OK != spDXDevice->CreateVertexBuffer( sizeof( CXQuad2D ),
                                                  D3DUSAGE_WRITEONLY,
                                                  0,
                                                  D3DPOOL_MANAGED,
                                                  &spPostProcessVertBuf,
                                                  NULL ) )
    {
        throw NExcept::CCriticalException("Post Process Buffer Creation Error!",
            boost::str( boost::format("Error creating post process vertex buffer.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));
    }

    // Fill in the vertex buffer
    LPVOID pVBData;
    if( SUCCEEDED( spPostProcessVertBuf->Lock( 0, 0, &pVBData, D3DLOCK_DISCARD ) ) )
    {
        CopyMemory( pVBData, &quad2D, sizeof( CXQuad2D ) );
        spPostProcessVertBuf->Unlock();
    }
    else
        throw NExcept::CCriticalException("Post Process Buffer Creation Error!",
            boost::str( boost::format("Error Locking post process vertex buffer.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

}	// CreatePostProcessVertexBuffer


/************************************************************************
*    desc:  Setup the stencil buffer for shadows. The below is the default 
*           for two sided stencils
************************************************************************/
void CXDevice::SetStencilBufferForShadows()
{
    // general stencil configuration
    spDXDevice->SetRenderState( D3DRS_STENCILREF,	   0x1 );
    spDXDevice->SetRenderState( D3DRS_STENCILMASK,	   0xffffffff );
    spDXDevice->SetRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
    spDXDevice->SetRenderState( D3DRS_STENCILFAIL,	   D3DSTENCILOP_KEEP );
    spDXDevice->SetRenderState( D3DRS_CCW_STENCILFAIL,  D3DSTENCILOP_KEEP );

    if( zPassStencilBufferMode )
    {
        // configure stencil modes for z-pass method
        spDXDevice->SetRenderState( D3DRS_STENCILZFAIL,	   D3DSTENCILOP_KEEP );
        spDXDevice->SetRenderState( D3DRS_STENCILPASS,      D3DSTENCILOP_INCR );
        spDXDevice->SetRenderState( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP );
        spDXDevice->SetRenderState( D3DRS_CCW_STENCILPASS,  D3DSTENCILOP_DECR );
    }
    else
    {
        // configure stencil modes for z-fail method
        spDXDevice->SetRenderState( D3DRS_STENCILZFAIL,	   D3DSTENCILOP_INCR );
        spDXDevice->SetRenderState( D3DRS_STENCILPASS,      D3DSTENCILOP_KEEP );
        spDXDevice->SetRenderState( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_DECR );
        spDXDevice->SetRenderState( D3DRS_CCW_STENCILPASS,  D3DSTENCILOP_KEEP );
    }
}	// SetStencilBufferForShadows


/************************************************************************
*    desc:  Enable/Disable rendering to the stencil buffer
************************************************************************/
void CXDevice::EnableStencilRender( bool enable )
{
    if( enable )
    {
        spDXDevice->SetRenderState( D3DRS_ZWRITEENABLE, false );
        spDXDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0x0 );

        spDXDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT );
        spDXDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
    }

    spDXDevice->SetRenderState( D3DRS_STENCILENABLE, enable );

}	// EnableStencilRender


/************************************************************************
*    desc:  Get the stencil buffer ready to render through it
************************************************************************/
void CXDevice::InitRenderThroughStencil()
{
    spDXDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
    spDXDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_LESS );

    spDXDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0xf );
    spDXDevice->SetRenderState( D3DRS_ZWRITEENABLE, true );

}	// InitRenderThroughStencil


/************************************************************************
*    desc:  Get the width of the directX buffer
*
*    NOTE: Function needed for DirectX Reset() resolution change
*
*    ret: float - width of the buffer
************************************************************************/
float CXDevice::GetBufferWidth()
{
    return (float)dxpp.BackBufferWidth;
}


/************************************************************************
*    desc:  Get the back buffer format
*
*    ret: D3DFORMAT - format of the back buffer
************************************************************************/
D3DFORMAT CXDevice::GetBufferFormat()
{
    return dxpp.BackBufferFormat;
}


/************************************************************************
*    desc:  Get the height of the directX buffer
*
*    NOTE: Function needed for DirectX Reset() resolution change
*
*    ret: float - height of the buffer
************************************************************************/
float CXDevice::GetBufferHeight()
{
    return (float)dxpp.BackBufferHeight;
}


/************************************************************************
*    desc:  Get the window mode
*
*    NOTE: Function needed for DirectX Reset() resolution change
*
*    ret: bool - 
************************************************************************/
BOOL CXDevice::GetWindowed()
{
    return dxpp.Windowed;
}


/************************************************************************
*    desc:  Get the instance of DirectX object
*
*    ret: LPDIRECT3D9 - Returns pointer to DirectX Instance
************************************************************************/
LPDIRECT3D9 CXDevice::GetXInstance()
{
    return spDXInstance;
}


/************************************************************************
*    desc:  Get the pointer to DirectX device
*
*    ret: LPDIRECT3DDEVICE9 - Returns pointer to DirectX Device
************************************************************************/
LPDIRECT3DDEVICE9 CXDevice::GetXDevice()
{
    return spDXDevice;
}


/************************************************************************
*    desc:  Get the maximum Z distance
*
*    ret: float - max z distance
************************************************************************/
float CXDevice::GetMaxZDist()
{
    return maximumZDist;
}


/************************************************************************
*    desc:  Get the minimum Z distance
*
*    ret: float - min z distance
************************************************************************/
float CXDevice::GetMinZDist()
{
    return minimumZDist;
}


/************************************************************************
*    desc:  Get the view angle
*
*    ret: float - view angle
************************************************************************/
float CXDevice::GetViewAngle()
{
    return viewAngle;
}


/************************************************************************
*    desc:  Get the square percentage
*
*    ret: float - square percentage
************************************************************************/
float CXDevice::GetSquarePercentage()
{
    return squarePercentage;
}


/************************************************************************
*    desc:  Get the frustrum Y ratio
*
*    ret: float - frustrum Y ratio
************************************************************************/
float CXDevice::GetFrustrumYRatio()
{
    return frustrumYRatio;
}


/************************************************************************
*    desc:  Check for two sided stencil capabilities
*
*    ret: bool - true if supported
************************************************************************/
bool CXDevice::IsTwoSidedStencil()
{
    return ( ( d3dCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED ) != 0 );
}


/************************************************************************
*    desc:  Check if we are running zPass or zFail shadow method
*
*    ret: bool - true = zPass, false = zFail
************************************************************************/
bool CXDevice::IsUsingZPassShadowMethod()
{
    return zPassStencilBufferMode;
}


/************************************************************************
*    desc:  Set if we are running zPass or zFail shadow method
*
*    param: bool value - zPass or zFail method
************************************************************************/
void CXDevice::SetUsingZPassShadowMethod( bool value )
{
    zPassStencilBufferMode = value;
}


/************************************************************************
*    desc:  Enable Z Buffering
*
*    param: value
************************************************************************/
void CXDevice::EnableZBuffering( bool value, bool overrideSettings )
{
    if( overrideSettings )
        spDXDevice->SetRenderState( D3DRS_ZENABLE, value );

    else if( CSettings::Instance().GetEnableDepthBuffer() )
        spDXDevice->SetRenderState( D3DRS_ZENABLE, value );

}	// EnableZBuffering


/************************************************************************
*    desc:  Get the projection matrix
*
*    ret: D3DXMATRIX & - projection matrix
************************************************************************/
D3DXMATRIX & CXDevice::GetProjectionMatrix( CSettings::EProjectionType type )
{
    if( type == CSettings::EPT_ORTHOGRAPHIC )
        return orthographicMatrix;

    else
        return perspectiveMatrix;

}	// GetProjectionMatrix


/************************************************************************
*    desc:  Get the shadow map buffer surface
*
*    ret: LPDIRECT3DSURFACE9
************************************************************************/
LPDIRECT3DSURFACE9 CXDevice::GetShadowMapBufferSurface()
{
    return spShadowMapBufferSurface;
}


/************************************************************************
*    desc:  Get the shadow map buffer texture
*
*    ret: LPDIRECT3DSURFACE9
************************************************************************/
LPDIRECT3DTEXTURE9 CXDevice::GetShadowMapBufferTexture()
{
    return spShadowMapBufferTexture;
}


/************************************************************************
*    desc:  Get the display buffer surface
*
*    ret: LPDIRECT3DSURFACE9
************************************************************************/
LPDIRECT3DSURFACE9 CXDevice::GetDisplayBufferSurface()
{
    return spDisplaySurface;
}


/************************************************************************
*    desc:  Get the post process vertex buffer
*
*    ret: LPDIRECT3DVERTEXBUFFER9
************************************************************************/
LPDIRECT3DVERTEXBUFFER9 CXDevice::GetPostProcessVertexBuffer()
{
    return spPostProcessVertBuf;
}


/************************************************************************
*    desc:  Is the shadow buffer active?
*
*    ret: BOOL
************************************************************************/
bool CXDevice::IsShadowMapBufferActive()
{
    return (spShadowMapBufferSurface != NULL);
}


/************************************************************************
*    desc:  Is the post process buffer active?
*
*    ret: BOOL
************************************************************************/
bool CXDevice::IsPostProcessBufferActive()
{
    return !pPostProcTextVec.empty();
}


/************************************************************************
*    desc:  Set the shadow map buffer as the render target
************************************************************************/
void CXDevice::SetShadowMapBufferAsRenderTarget()
{
    spDXDevice->SetRenderTarget( 0, spShadowMapBufferSurface );
}


/************************************************************************
*    desc:  Set the display surface as the render target
************************************************************************/
void CXDevice::SetDisplaySurfaceAsRenderTarget()
{
    spDXDevice->SetRenderTarget( 0, spDisplaySurface );
}


/************************************************************************
*    desc:  Set the post process surface as the render target
************************************************************************/
void CXDevice::SetPostProcessSurfaceAsRenderTarget()
{
    spDXDevice->SetRenderTarget( 0, pPostProcSurfaceVec[postProcBufIndex] );
}


/************************************************************************
*    desc:  Get the post process buffer texture
*
*    ret: LPDIRECT3DSURFACE9
************************************************************************/
LPDIRECT3DTEXTURE9 CXDevice::GetPostProcessBufferTexture()
{
    return pPostProcTextVec[postProcBufIndex];
}


/************************************************************************
*    desc:  Flip the prost process buffers
************************************************************************/
void CXDevice::FlipPostProcBuffers()
{
    postProcBufIndex = (postProcBufIndex + 1) % pPostProcTextVec.size();
}


/************************************************************************
*    desc:  Get the post process buffer at index
************************************************************************/
LPDIRECT3DTEXTURE9 CXDevice::GetPostProcBufText( unsigned int index )
{
    return pPostProcTextVec[index];
}


/************************************************************************
*    desc:  Set the post process buffer index
************************************************************************/
void CXDevice::SetActivePostProcBufIndex( unsigned int index )
{
    postProcBufIndex = index;
}


/************************************************************************
*    desc:  Enable the culling
************************************************************************/
void CXDevice::EnableCulling( bool enable )
{
    if( enable )
        spDXDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    else
        spDXDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

}	// EnableCulling


/************************************************************************
*    desc:  Get the handle to the game window associated with the device
*
*    ret: HWND - Handle to game window
************************************************************************/
HWND CXDevice::GetWndHandle()
{
    return hWnd;
}


/************************************************************************
*    desc:  Display error information
*
*    param: HRESULT hr - return result from function call
************************************************************************/
void CXDevice::DisplayError( HRESULT hr )
{
    switch( hr )
    {
        case D3DERR_DEVICELOST:
        {
            throw NExcept::CCriticalException("DirectX Init Error", "Error creating DirectX9 device. Device has been lost.");
            break;
        }
        case D3DERR_INVALIDCALL:
        {
            throw NExcept::CCriticalException("DirectX Init Error", "Error creating DirectX9 device. Invalid paramter call.");
            break;
        }
        case D3DERR_NOTAVAILABLE:
        {
            throw NExcept::CCriticalException("DirectX Init Error", "Error creating DirectX9 device. Unsupported Queried Technique.");
            break;
        }
        case D3DERR_OUTOFVIDEOMEMORY:
        {
            throw NExcept::CCriticalException("DirectX Init Error", "Error creating DirectX9 device. Out of video memory.");
            break;
        }
        default:
        {
            throw NExcept::CCriticalException("DirectX Init Error", "Error creating DirectX9 device. Unknow error.");
            break;
        }
    }
}	// DisplayError