/************************************************************************
*    FILE NAME:		instancemesh2d.cpp
*
*    DESCRIPTION:   Class to render multiple 2D sprites using a single
*					draw call
************************************************************************/

// Physical component dependency
#include <2d/instancemesh2d.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <2d/actorsprite2d.h>
#include <2d/spritegroup2d.h>
#include <2d/visualsprite2d.h>
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <utilities/sortfunc.h>
#include <utilities/statcounter.h>
#include <system/xdevice.h>
#include <managers/shader.h>
#include <managers/texturemanager.h>
#include <managers/megatexturemanager.h>
#include <common/matrix.h>
#include <common/texture.h>
#include <common/megatexturecomponent.h>
#include <common/megatexture.h>


// The vertex element. It shows us what data we'll be sending up to the shader. All rows with a 0 in
// the first column are for stream 0. They are the data elements of the vertices in the vertex buffer.
// All rows with a 1 in the fist column are for stream 1. They are the data elements for each instance
// of the render. The data in stream 0 never changes, and the data in stream 1 changes every frame
const D3DVERTEXELEMENT9 vertexElement[] =
{
    // Position of the vertex
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0 },
    
    // UV index of the vertex. This is used to determine a vertex's UVs from a group of 4 values
    { 0, 12, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },

    
    // A transformation matrix represented as 16 values
    { 1, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
    { 1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
    { 1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
    { 1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 },

    // The color of the instance
    { 1, 64, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
    
    // The UVs of the instance represented as 4 values instead of 8. The vertex's index value
    // is used to determine which two UV values make up its UVs
    { 1, 80, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5 },

    D3DDECL_END()
};

/************************************************************************
*    desc:  Constructer                                                             
************************************************************************/
CInstanceMesh2D::CInstanceMesh2D()
               : instanceCount(0),
                 VERTEX_COUNT(4),
                 FACE_COUNT(2),
                 INDEX_COUNT(FACE_COUNT * 3)
{
}   // Constructer


/************************************************************************
*    desc:  Initialize the passed in sprite to the instance mesh
*
*	 param:	CSpriteGroup2D/CVisualSprite2D/CActorSprite2D * pSprite
*				- sprite to initialize
************************************************************************/
void CInstanceMesh2D::InitInstanceSprite( CActorSprite2D * pSprite )
{
    if( pSprite != NULL )
    {
        for( uint i = 0; i < pSprite->GetSpriteGroupCount(); ++i )
            InitInstanceSprite( pSprite->GetSpriteGroup(i) );
    }

}	// InitInstanceSprite

void CInstanceMesh2D::InitInstanceSprite( CSpriteGroup2D * pSprite )
{
    if( pSprite != NULL )
        pSprite->SetInstanceMesh( this );

}	// InitInstanceSprite


/************************************************************************
*    desc:  Add a sprite group to the instance mesh
*
*	 param:	CSpriteGroup2D * pSprite - Sprite to add to the instance mesh
************************************************************************/
void CInstanceMesh2D::AddSprite( CSpriteGroup2D * pSprite )
{
    pRenderMultiMap.insert( std::make_pair(pSprite->GetPos().z, SpriteGrp(pSprite, pSprite->GetCurrentFrame())) );

}	// AddSprite


/************************************************************************
*    desc:  Initialize the mesh
************************************************************************/
void CInstanceMesh2D::Init( const std::string & megatextureName )
{
    HRESULT hr;

    // Create the vertex declaration
    CXDevice::Instance().GetXDevice()->CreateVertexDeclaration( vertexElement, &spVertexDeclaration );

    // Create the vertex buffer
    if( spVertexBuffer == NULL )
    {
        if( FAILED( hr = CXDevice::Instance().GetXDevice()->CreateVertexBuffer( 
                    VERTEX_COUNT * sizeof( CVertexData ),
                    D3DUSAGE_WRITEONLY, 
                    0,
                    D3DPOOL_MANAGED, 
                    &spVertexBuffer, 
                    NULL ) ) )
        {
            DisplayError( hr );
        }
    }

    // Create the index buffer
    if( spIndexBuffer == NULL )
    {
        if( FAILED( hr = CXDevice::Instance().GetXDevice()->CreateIndexBuffer( 
                    INDEX_COUNT * sizeof( WORD ), 
                    D3DUSAGE_WRITEONLY,
                    D3DFMT_INDEX16, 
                    D3DPOOL_MANAGED, 
                    &spIndexBuffer, 
                    NULL ) ) )
        {
            DisplayError( hr );
        }
    }


    // Lock the vertex buffer for copying
    CVertexData * pVertex;
    if( FAILED( spVertexBuffer->Lock( 0, 0, (void **)&pVertex, 0 ) ) )
    {
        throw NExcept::CCriticalException( "Instance Mesh Error!", 
                                           "An instance mesh failed to lock its vertex buffer." );
    }

    // The vertices are positioned in a way that they they make a quad with side sizes of 1
    pVertex[0].vert = CPoint(-0.5f,  0.5f, 0);
    pVertex[1].vert = CPoint( 0.5f,  0.5f, 0);
    pVertex[2].vert = CPoint(-0.5f, -0.5f, 0);
    pVertex[3].vert = CPoint( 0.5f, -0.5f, 0);

    // The uv index is used to determine the uv values of a vertex in the shader
    pVertex[0].uvIndex = 0;
    pVertex[1].uvIndex = 1;
    pVertex[2].uvIndex = 2;
    pVertex[3].uvIndex = 3;

    // Unlock the vertex buffer so it can be used
    spVertexBuffer->Unlock();


    // Lock the index buffer for copying
    WORD * pIndex;
    if( FAILED( spIndexBuffer->Lock( 0, 0, (void **)&pIndex, 0 ) ) )
    {
        throw NExcept::CCriticalException( "Instance Mesh Error!", 
                                            "An instance mesh failed to lock its index buffer." );
    }

    // The indexes and verts are set up like so:
    // Indexes			Verts            
    // 0----1  3		0----1			
    // |   /  /|		|   /|			    
    // |  /  / |		|  / |			
    // | /  /  |		| /  |			
    // |/  /   |		|/   |			
    // 2  5----4		2----3			
    pIndex[0] = 0;
    pIndex[1] = 1;
    pIndex[2] = 2;
    pIndex[3] = 1;
    pIndex[4] = 3;
    pIndex[5] = 2;

    // Unlock the index buffer so it can be used
    spIndexBuffer->Unlock();

    // Get the texture
    pMegaTexture = CMegaTextureManager::Instance().GetTexture( megatextureName );

}	// Init


/************************************************************************
*    desc:  Reset the instance buffer to the size of the group vector
************************************************************************/
void CInstanceMesh2D::ResetInstanceBuffer()
{
    // Set the new instance count
    instanceCount = pRenderMultiMap.size();

    HRESULT hr; 

    // Release the instance buffer and recreate it
    spInstanceBuffer.Release();

    if( FAILED( hr = CXDevice::Instance().GetXDevice()->CreateVertexBuffer( 
                static_cast<UINT>(instanceCount * sizeof( CInstanceData )), 
                D3DUSAGE_WRITEONLY,
                0, 
                D3DPOOL_MANAGED, 
                &spInstanceBuffer, 
                NULL ) ) )
    {
        DisplayError( hr );
    }

}	// ResetInstanceBuffer


/************************************************************************
*    desc:  Render the instance mesh
************************************************************************/
void CInstanceMesh2D::Render()
{
    // Only render if there is something to render
    if( !pRenderMultiMap.empty() )
    {
        // If our total sprite group count exceeds the instance count, we recreate the instance buffer
        if( pRenderMultiMap.size() > instanceCount )
            ResetInstanceBuffer();

        // Update the mesh
        Update();

        // Increment our stat counter to keep track of what is going on.
        CStatCounter::Instance().IncDisplayCounter( pRenderMultiMap.size() );

        // Set the vertex declaration
        CXDevice::Instance().GetXDevice()->SetVertexDeclaration( spVertexDeclaration );

        // Set up stream zero with our vertex buffer and however many instances of it we're rendering
        CXDevice::Instance().GetXDevice()->SetStreamSource( 0, spVertexBuffer, 0, sizeof( CVertexData ) );
        CXDevice::Instance().GetXDevice()->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | static_cast<uint>(pRenderMultiMap.size()) );

        // Set up stream one with our instance buffer
        CXDevice::Instance().GetXDevice()->SetStreamSource( 1, spInstanceBuffer, 0, sizeof( CInstanceData ) );
        CXDevice::Instance().GetXDevice()->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | 1 );

        // Give the indexes to DirectX
        CXDevice::Instance().GetXDevice()->SetIndices( spIndexBuffer );

        // Set up the shader before the rendering
        CEffectData * pEffectData = CShader::Instance().SetEffectAndTechnique( "shader_2d", "instance" );

        // Set the active texture to the first sprite's first texture
        CTextureMgr::Instance().SelectTexture( pMegaTexture->GetTexture()->spTexture );
    
        // Begin rendering
        UINT iPass, cPasses;

        CShader::Instance().GetActiveShader()->Begin( &cPasses, 0 );
        for( iPass = 0; iPass < cPasses; ++iPass )
        {
            CShader::Instance().GetActiveShader()->BeginPass( iPass );
            CXDevice::Instance().GetXDevice()->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, VERTEX_COUNT, 0, FACE_COUNT );
            CShader::Instance().GetActiveShader()->EndPass();
        }
        CShader::Instance().GetActiveShader()->End();

        // Reset the stream frequencies
        CXDevice::Instance().GetXDevice()->SetStreamSourceFreq(0,1);
        CXDevice::Instance().GetXDevice()->SetStreamSourceFreq(1,1);
    }

}	// Render


/************************************************************************
*    desc:  Update the mesh information
************************************************************************/
void CInstanceMesh2D::Update()
{
    // Set the instance buffer values
    CInstanceData * pInstance;
    if( FAILED( spInstanceBuffer->Lock( 0, 0, (void **)&pInstance, 0 ) ) )
        throw NExcept::CCriticalException( "Instance Mesh Error!", 
                                            "An instance mesh failed to lock its instance buffer." );

    // Begin copying the instance data
    renderIter = pRenderMultiMap.begin();
    uint instanceIndex = 0;

    while( renderIter != pRenderMultiMap.end() )
    {
        // Create a scale matrix so that the generic mesh in the vertex buffer will conform
        // to the size of the specific sprite
        int x = renderIter->second.GetSpriteGrp()->GetVisualSprite()->GetSize(false).w;
        int y = renderIter->second.GetSpriteGrp()->GetVisualSprite()->GetSize(false).h;
        D3DXMATRIX sizeMatrix( x, 0, 0, 0,
                                0, y, 0, 0,
                                0, 0, 1, 0,
                                0, 0, 0, 1 );

        CPoint finalPos = CWorldCamera::Instance().GetPos() + renderIter->second.GetSpriteGrp()->GetTransPos();

        // Copy it to the DirectX matrix
        D3DXMATRIX scalCameraMatrix( renderIter->second.GetSpriteGrp()->GetScaledMatrix()() );
        scalCameraMatrix._41 = finalPos.x;
        scalCameraMatrix._42 = finalPos.y;
        scalCameraMatrix._43 = finalPos.z;

        // Create the matrix to send to the shader
        D3DXMATRIX cameraViewProjectionMatrix = sizeMatrix * scalCameraMatrix * 
                                                CXDevice::Instance().GetProjectionMatrix( renderIter->second.GetSpriteGrp()->GetProjectionType() );

        // Set the instance data
        pInstance[instanceIndex].SetMatrix( cameraViewProjectionMatrix );
        pInstance[instanceIndex].SetColor( renderIter->second.GetSpriteGrp()->GetResultColor() );

        // Set the current frame
        renderIter->second.GetSpriteGrp()->SetCurrentFrame( renderIter->second.GetFrameIndex() );

        // Set the UVs using the mega texture component data
        pInstance[instanceIndex].SetUVs( pMegaTexture->GetUVs( renderIter->second.GetSpriteGrp()->GetActiveTexture() ) );

        // We reset the required transformations so we're not constantly recalculating matrices
        renderIter->second.GetSpriteGrp()->ResetTransformParameters();
        ++instanceIndex;
        ++renderIter;
    }
    
    spInstanceBuffer->Unlock();

}	// Update


/************************************************************************
*    desc:  Clear the render multi map
************************************************************************/
void CInstanceMesh2D::Clear()
{
    pRenderMultiMap.clear();

}	// ClearRenderVector


/************************************************************************
*    desc:  Display error information
************************************************************************/
void CInstanceMesh2D::DisplayError( HRESULT hr )
{
    switch( hr )
    {
        case D3DERR_OUTOFVIDEOMEMORY:
        {
            throw NExcept::CCriticalException( "Instance Mesh Error!",
                                               "Error creating vertex buffer. Does not have enough display memory to load texture." );
            break;
        }
        case D3DERR_INVALIDCALL:
        {
            throw NExcept::CCriticalException( "Instance Mesh Error!",
                                               "Error creating vertex buffer. The method call is invalid." );
            break;
        }
        case E_OUTOFMEMORY:
        {
            throw NExcept::CCriticalException( "Instance Mesh Error!",
                                               "Error creating vertex buffer. Direct3D could not allocate sufficient memory to load texture." );
            break;
        }
        default:
        {
            throw NExcept::CCriticalException( "Instance Mesh Error!",
                                               "Error creating vertex buffer. Unknow error." );
            break;
        }
    }

}	// DisplayError