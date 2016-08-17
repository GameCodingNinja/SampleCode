
/************************************************************************
*    FILE NAME:       megatexture.cpp
*
*    DESCRIPTION:     Class to hold a mega texture
************************************************************************/

// Physical component dependency
#include <common/megatexture.h>

// Standard lib dependencies

// Boost lib dependencies
#include <boost/format.hpp>
#include <boost/container/vector.hpp>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <utilities/sortfunc.h>
#include <utilities/deletefuncs.h>
#include <utilities/collisionfunc2d.h>
#include <utilities/genfunc.h>
#include <system/xdevice.h>
#include <managers/texturemanager.h>
#include <managers/shader.h>
#include <common/texture.h>
#include <common/vertex2d.h>
#include <common/megatexturecomponent.h>
#include <common/texturepartition.h>
#include <3d/worldcamera.h>

// Vertex data to pass to the shader
const D3DVERTEXELEMENT9 vertexElement[] =
{
    // Position of the vertex
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    
    // UV index of the vertex. This is used to determine a vertex's UVs from a group of 4 values
    { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

    D3DDECL_END()
};

/************************************************************************
*    desc:  Constructer
************************************************************************/
CMegaTexture::CMegaTexture()
            : VERTEX_COUNT(4),
              FACE_COUNT(2),
              INDEX_COUNT(FACE_COUNT * 3)
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CMegaTexture::~CMegaTexture()
{
}	// destructer


/************************************************************************
*    desc:  Initialize the mega texture's buffers. This only matters when
*			we want to render the texture, so it's only called in the
*			render function
************************************************************************/
void CMegaTexture::InitBuffers()
{
    HRESULT hr;

    // Create the vertex declaration
    if( spVertexDeclaration == NULL )
        CXDevice::Instance().GetXDevice()->CreateVertexDeclaration( vertexElement, &spVertexDeclaration );

    // Create the vertex buffer
    if( spVertexBuffer == NULL )
    {
        if( FAILED( hr = CXDevice::Instance().GetXDevice()->CreateVertexBuffer( 
                    VERTEX_COUNT * sizeof( CVertex2D ),
                    0, 
                    0,
                    D3DPOOL_MANAGED, 
                    &spVertexBuffer, 
                    NULL ) ) )
        {
            DisplayError( hr, __FUNCTION__, __LINE__ );
        }
    }

    // Create the index buffer
    if( spIndexBuffer == NULL )
    {
        if( FAILED( hr = CXDevice::Instance().GetXDevice()->CreateIndexBuffer( 
                    INDEX_COUNT * sizeof( WORD ), 
                    0,
                    D3DFMT_INDEX16, 
                    D3DPOOL_MANAGED, 
                    &spIndexBuffer, 
                    NULL ) ) )
        {
            DisplayError( hr, __FUNCTION__, __LINE__ );
        }

        // Lock the index buffer for copying
        WORD * pIndex;
        if( FAILED( spIndexBuffer->Lock( 0, 0, (void **)&pIndex, 0 ) ) )
            throw NExcept::CCriticalException( "Mega Texture Error!", 
                boost::str( boost::format("Failed to lock the index buffer.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

        // The indexes are set up like so:
        // Vert Index       Index index
        // 0----1			0----1  3
        // |   /|			|   /  /|    
        // |  / |			|  /  / |
        // | /  |			| /  /  |
        // |/   |			|/  /   |
        // 2----3			2  5----4
        pIndex[0] = 0;
        pIndex[1] = 1;
        pIndex[2] = 2;
        pIndex[3] = 1;
        pIndex[4] = 3;
        pIndex[5] = 2;

        // Unlock the index buffer so it can be used
        spIndexBuffer->Unlock();
    }

}	// Init


/************************************************************************
*    desc:  Get a mega texture
*
*	 ret:	NText::CTextureFor2D * - mega texture to return
************************************************************************/
NText::CTextureFor2D * CMegaTexture::GetTexture()
{
    if( spMegaTexture == NULL )
        throw NExcept::CCriticalException( "Mega Texture Error!", 
                boost::str( boost::format("Trying to get a texture that hasn't been created.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    return spMegaTexture.get();

}	// GetTexture


/************************************************************************
*    desc:  Get the UVs of a texture
*  
*    param: NText::CTextureFor2D * pTex - texture whose UVs to get
*
*	 ret:	float * - UVs of the texture
************************************************************************/
float * CMegaTexture::GetUVs( NText::CTextureFor2D * pTex )
{
    spComponentMapIter = spComponentMap.find( pTex );

    if( spComponentMapIter == spComponentMap.end() )
        throw NExcept::CCriticalException( "Mega Texture Error!", 
                boost::str( boost::format("Texture component missing.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    return spComponentMapIter->second->uv;

}	// GetUVs


/************************************************************************
*    desc:  Render the mega texture
************************************************************************/
void CMegaTexture::Render()
{
    // Initialize the buffers. If the buffers are already made, nothing happens
    // in here
    InitBuffers();
        
    // Lock the vertex buffer for copying
    CVertex2D * pVertex;
    if( FAILED( spVertexBuffer->Lock( 0, 0, (void **)&pVertex, 0 ) ) )
        throw NExcept::CCriticalException( "Mega Texture Error!", 
                boost::str( boost::format("Failed to lock the vertex buffer.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    // Get the size of the whole mega texture
    CSize<float> tmpHalfSize = spMegaTexture->size / 2;

    // The vertices are positioned in a way that they they make a quad with side sizes of 1
    pVertex[0].vert = CPoint(-tmpHalfSize.w, tmpHalfSize.h,0) + CWorldCamera::Instance().GetPos();
    pVertex[1].vert = CPoint( tmpHalfSize.w, tmpHalfSize.h,0) + CWorldCamera::Instance().GetPos();
    pVertex[2].vert = CPoint(-tmpHalfSize.w,-tmpHalfSize.h,0) + CWorldCamera::Instance().GetPos();
    pVertex[3].vert = CPoint( tmpHalfSize.w,-tmpHalfSize.h,0) + CWorldCamera::Instance().GetPos();
    pVertex[0].uv = CUV(0,1);
    pVertex[1].uv = CUV(1,1);
    pVertex[2].uv = CUV(0,0);
    pVertex[3].uv = CUV(1,0);

    // Unlock the vertex buffer so it can be used
    spVertexBuffer->Unlock();

    // Set the vertex declaration
    CXDevice::Instance().GetXDevice()->SetVertexDeclaration( spVertexDeclaration );

    // Set up stream zero with our vertex buffer and set the indexes
    CXDevice::Instance().GetXDevice()->SetStreamSource( 0, spVertexBuffer, 0, sizeof( CVertex2D ) );
    CXDevice::Instance().GetXDevice()->SetIndices( spIndexBuffer );

    // Set up the shader before the rendering
    CEffectData * pEffectData = CShader::Instance().SetEffectAndTechnique( "shader_2d", "linearFilter" );

    // Copy the matrix to the shader
    CShader::Instance().SetEffectValue( pEffectData, "cameraViewProjMatrix", 
                                        CXDevice::Instance().GetProjectionMatrix( CSettings::EPT_ORTHOGRAPHIC ) );

    // Set the material color
    CShader::Instance().SetEffectValue( pEffectData, "materialColor", D3DXVECTOR4(1,1,1,1) );

    // Set the active texture to the first sprite's first texture
    CTextureMgr::Instance().SelectTexture( spMegaTexture->spTexture );
    
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

}	// Render


/************************************************************************
*    desc:  Create a mega texture using the group name passed in
*  
*    param: string & group - group of textures to combine
*			int wlimit	   - limit the width the texture will fit into
************************************************************************/
void CMegaTexture::CreateMegaTexture( const std::string & group, uint wLimit )
{
    // Make sure we don't go over the max size
    if( wLimit > CXDevice::Instance().GetMaxTextureWidth() )
        wLimit = CXDevice::Instance().GetMaxTextureWidth();

    // Make sure the hardware can handles this texture size
    if( CXDevice::Instance().GetMaxTextureWidth() < wLimit )
        throw NExcept::CCriticalException("Max texture width too small!",
                    boost::str( boost::format("Max texture width needed (%d) but was found (%d) (%s).\n\n%s\nLine: %s") % wLimit % CXDevice::Instance().GetMaxTextureWidth() % group % __FUNCTION__ % __LINE__ ));

    // The size of the mega texture;
    CSize<int> megaTextureSize;

    // Get the textures from the texture manager
    std::vector<NText::CTextureFor2D *> pTextureVector;
    CTextureMgr::Instance().GetGroupTextures( group, pTextureVector );

    // We don't want to create a mega texture if there's no textures in the texture manager
    if( !pTextureVector.empty() )
    {
        // Sort the textures by largest height to smallest height
        sort( pTextureVector.begin(), pTextureVector.end(), NSortFunc::Texture2DSort );

        // Create a vector to hold the sorted component data
        boost::container::vector<CMegaTextureComponent *> pTmpSortedComponentVec;
        boost::container::vector<CMegaTextureComponent *>::iterator sortedComponentVecIter;

        // Add the textures into the component containers
        for( size_t i = 0; i < pTextureVector.size(); ++i )
        {
            CMegaTextureComponent * pTmpComponent = new CMegaTextureComponent( pTextureVector[i] );
            spComponentMap.insert( pTextureVector[i], pTmpComponent );
            pTmpSortedComponentVec.push_back( pTmpComponent );
        }

        // Two dimensional pointer vector of partitions and add the first partition to it
        SPPartitionVecVec spPartitionVecVec;
        spPartitionVecVec.push_back( new boost::ptr_vector<CTexturePartition> );
        spPartitionVecVec[0].push_back( new CTexturePartition );
        spPartitionVecVec[0][0].size = CSize<int>( wLimit, CXDevice::Instance().GetMaxTextureHeight() );

        for( sortedComponentVecIter  = pTmpSortedComponentVec.begin();
             sortedComponentVecIter != pTmpSortedComponentVec.end();
           ++sortedComponentVecIter )
        {
            // Loop through each row of partitions
            for( size_t i = 0; i < spPartitionVecVec.size(); ++i )
            {
                bool nextComponent = false;

                // Loop through each partition in a row
                for( size_t j = 0; j < spPartitionVecVec[i].size(); ++j )
                {
                    // Get the far position. The far position is the point diagnally across from the position
                    CPointInt farPos;
                    farPos.x = spPartitionVecVec[i][j].pos.x + (*sortedComponentVecIter)->pTexture->size.w;
                    farPos.y = spPartitionVecVec[i][j].pos.y + (*sortedComponentVecIter)->pTexture->size.h;

                    // If we get in here, we can't fit the texture in this row by any means, so we'll move
                    // onto the next row
                    if( farPos.x > static_cast<int>(wLimit) )
                        break;

                    // If we get into here, then we can't fit all of our textures into one
                    else if( farPos.y > static_cast<int>(CXDevice::Instance().GetMaxTextureHeight()) )
                        throw NExcept::CCriticalException( "Mega Texture Error!", 
                            boost::str( boost::format("Cannot fit all textures of the group with a 4048x4048 space.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));
                        
                    // If the partition is vacant, we're going to see if we can fit the texture in it
                    if( spPartitionVecVec[i][j].vacant )
                        nextComponent = FitTextureToPartition( 
                            static_cast<int>(i), static_cast<int>(j), *sortedComponentVecIter, spPartitionVecVec );

                    if( nextComponent )
                        break;
                }

                if( nextComponent )
                    break;
            }

            // Update the mega texture's size
            if( (*sortedComponentVecIter)->pos.x + (*sortedComponentVecIter)->pTexture->size.w > megaTextureSize.w )
                megaTextureSize.w = (*sortedComponentVecIter)->pos.x + (*sortedComponentVecIter)->pTexture->size.w;

            if( (*sortedComponentVecIter)->pos.y + (*sortedComponentVecIter)->pTexture->size.h > megaTextureSize.h )
                megaTextureSize.h = (*sortedComponentVecIter)->pos.y + (*sortedComponentVecIter)->pTexture->size.h;
        }

        // Make sure no textures are overlapping
        CheckTextureOverlap();

        // Create the mega texture and set its size
        spMegaTexture.reset( new NText::CTextureFor2D() );
        spMegaTexture->size.w = megaTextureSize.w;
        spMegaTexture->size.h = megaTextureSize.h;

        NGenFunc::PostDebugMsg( "Mega Texture Create: %s - %d x %d", group.c_str(), megaTextureSize.w, megaTextureSize.h );

        // Create the texture we're going to give to the shader
        CopyToMegaTexture( group );

        // Calculate the UVs
        CalculateGroupUVs();
    }

}	// CreateMegaTexture


/************************************************************************
*    desc:  Try to fit a texture into partitions
*  
*    param: int row                            - the row index of the initial 
*											     vacant partition
*			int column                         - the column index of the initial 
*											     vacant partition
*			CMegaTextureComponent * pComponent - the texture we're trying to fit
*			SPPartitionVecVec & spPartVecVec   - 2d vector of partitions
************************************************************************/
bool CMegaTexture::FitTextureToPartition( int row, int column, 
                                          CMegaTextureComponent * pComponent,
                                          SPPartitionVecVec & spPartVecVec  )
{
    // The resize object holds the height and width of the current partitions' resized
    // heights and widths. The newEntry size holds the height and width of the new
    // partitions' heights and widths
    CSize<int> resize, newEntry;

    // The total size of the partitions used to fit the width of the texture
    CSize<int> totalSize;

    // Between the values row and endRow, and column and endColumn, is where we will
    // try and fit our texture
    int endRow = 0;
    int endColumn = 1;

    // The new row and new column index will be one after the end row and end column indexes
    int newRow = 0;
    int newColumn = 1;

    // Check if we can fit the texture's width into nearby partitions
    for( size_t i = column; i < spPartVecVec[row].size(); ++i )
    {
        // If we run into a non-vacant partition before fitting the texture, the texture
        // can't fit here, so we return false
        if( !spPartVecVec[row][i].vacant )
            return false;

        totalSize.w += spPartVecVec[row][i].size.w;

        // Once the texture width is less or equal to the total width, we know how
        // many partitions will be used to fit the texture's width
        if( pComponent->pTexture->size.w <= totalSize.w )
        {
            endColumn = static_cast<int>(i);
            newColumn = static_cast<int>(i + 1);
            newEntry.w = totalSize.w - pComponent->pTexture->size.w;
            resize.w = spPartVecVec[row][i].size.w - newEntry.w;
            break;
        }
    }

    // Check if we can fit the texture's height into nearby partitions
    for( size_t i = row; i < spPartVecVec.size(); ++i )
    {
        // If we run into a non-vacant partition before fitting the texture, the texture
        // can't fit here, so we return false
        if( !spPartVecVec[i][column].vacant )
            return false;

        totalSize.h += spPartVecVec[i][column].size.h;

        // Once the texture width is less or equal to the total width, we know how
        // many partitions will be used to fit the texture's width
        if( pComponent->pTexture->size.h <= totalSize.h )
        {
            endRow = static_cast<int>(i);
            newRow = static_cast<int>(i + 1);
            newEntry.h = totalSize.h - pComponent->pTexture->size.h;
            resize.h = spPartVecVec[i][column].size.h - newEntry.h;
            break;
        }
    }

    // All we need to check before inserting is that all partitions within the
    // partCount are vacant
    for( int i = row + 1; i <= endRow; ++i )
    {
        for( int j = column + 1; j <= endColumn; ++j )
        {
            if( !spPartVecVec[i][j].vacant )
                return false;
        }
    }

    // Once we've gotten this far, it means our texture will fit here. So we
    // begin subdividing partitions. We start by adding a new column and resizing
    // the width of the column before it if the texture doesn't fit completely into
    // the width of the partitions
    if( newEntry.w > 0 )
    {
        for( size_t i = 0; i < spPartVecVec.size(); ++i )
        {
            // Resize the widths of the partitions in the end column
            spPartVecVec[i][endColumn].size.w = resize.w;

            // Create a new partition
            CTexturePartition * pTmpPart = new CTexturePartition();
            pTmpPart->size.w = newEntry.w;
            pTmpPart->size.h = spPartVecVec[i][endColumn].size.h;
            pTmpPart->pos.x = spPartVecVec[i][endColumn].pos.x + resize.w;
            pTmpPart->pos.y = spPartVecVec[i][endColumn].pos.y;
            pTmpPart->vacant = spPartVecVec[i][endColumn].vacant;

            // Add the new partition after the current one
            if( newColumn < static_cast<int>(spPartVecVec[i].size()) )
                spPartVecVec[i].insert( spPartVecVec[i].begin()+newColumn, pTmpPart );
            else
                spPartVecVec[i].push_back( pTmpPart );
        }
    }

    // We add the new row if the texture doesn't fit completely into the height of the partitions
    if( newEntry.h > 0 )
    {
        if( newRow < static_cast<int>(spPartVecVec.size()) )
            spPartVecVec.insert( spPartVecVec.begin()+newRow, new boost::ptr_vector<CTexturePartition> );
        else
            spPartVecVec.push_back( new boost::ptr_vector<CTexturePartition> );

        // Now we add in the new row and resize the height of the row before it
        for( size_t i = 0; i < spPartVecVec[endRow].size(); ++i )
        {
            // Resize the heights of the partitions in the end row
            spPartVecVec[endRow][i].size.h = resize.h;

            // Create a new partition
            CTexturePartition * pTmpPart = new CTexturePartition();
            pTmpPart->size.h = newEntry.h;
            pTmpPart->size.w = spPartVecVec[endRow][i].size.w;
            pTmpPart->pos.y = spPartVecVec[endRow][i].pos.y + resize.h;
            pTmpPart->pos.x = spPartVecVec[endRow][i].pos.x;
            pTmpPart->vacant = spPartVecVec[endRow][i].vacant;

            // Add the new partition to the new row
            spPartVecVec[newRow].push_back( pTmpPart );
        }
    }

    // Go through all of the partitions used to store this texture and
    // mark them as unvacant
    for( int i = row; i <= endRow; ++i )
    {
        for( int j = column; j <= endColumn; ++j )
            spPartVecVec[i][j].vacant = false;
    }

    // Lastly, we set the passed in component's position
    pComponent->pos = spPartVecVec[row][column].pos;

    return true;

}	// FitTextureToPartition


/************************************************************************
*    desc:  If any textures are overlapping, throw an exception
************************************************************************/
void CMegaTexture::CheckTextureOverlap()
{
    spComponentMapIter = spComponentMap.begin();

    // Let's double check that no textures are overlapping
    while( spComponentMapIter != spComponentMap.end() )
    {
        // Set the four points of the texture quad
        CPointInt p[4];
        p[0]   = spComponentMapIter->second->pos;
        p[1].x = spComponentMapIter->second->pos.x + spComponentMapIter->second->pTexture->size.w;
        p[1].y = spComponentMapIter->second->pos.y;
        p[2].x = spComponentMapIter->second->pos.x;
        p[2].y = spComponentMapIter->second->pos.y + spComponentMapIter->second->pTexture->size.h;
        p[3].x = spComponentMapIter->second->pos.x + spComponentMapIter->second->pTexture->size.w;
        p[3].y = spComponentMapIter->second->pos.y + spComponentMapIter->second->pTexture->size.h;

        // Get another iterator so that we can compare every texture's placement against every other
        // texture's placement
        SPComponentMapIter tmpComponentMapIter = spComponentMap.begin();

        while( tmpComponentMapIter != spComponentMap.end() )
        {
            // We don't want to compare a texture against itself
            if( tmpComponentMapIter != spComponentMapIter )
            {
                // Set the bounds to check against
                uint right, left, top, bottom;
                right  = tmpComponentMapIter->second->pos.x + tmpComponentMapIter->second->pTexture->size.w;
                left   = tmpComponentMapIter->second->pos.x;
                top    = tmpComponentMapIter->second->pos.y + tmpComponentMapIter->second->pTexture->size.h;
                bottom = tmpComponentMapIter->second->pos.y;

                // Check each point against each bound
                for( int k = 0; k < 4; ++k )
                {
                    if( NCollisionFunc2D::PointInRect( p[k], top, bottom, left, right ) )
                        throw NExcept::CCriticalException( "Mega Texture Error!", 
                            boost::str( boost::format("Error creating a mega texture due to texture overlap.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));
                }
            }

            ++tmpComponentMapIter;
        }

        ++spComponentMapIter;
    }

}	// CheckTextureOverlap


/************************************************************************
*    desc:  Calculate the UVs of a mega texture
************************************************************************/
void CMegaTexture::CalculateGroupUVs()
{
    spComponentMapIter = spComponentMap.begin();
    while( spComponentMapIter != spComponentMap.end() )
    {
        // Set the four points of the texture quad
        CPoint p[2];
        p[0].x = spComponentMapIter->second->pos.x + 0.5f;
        p[0].y = spComponentMapIter->second->pos.y + 0.5f;
        p[1].x = spComponentMapIter->second->pos.x + spComponentMapIter->second->pTexture->size.w - 0.5f;
        p[1].y = spComponentMapIter->second->pos.y + spComponentMapIter->second->pTexture->size.h - 0.5f;

        // We flip the v's because the textures in our mega texture are upside-down for some reason
        spComponentMapIter->second->uv[0] = p[0].x / spMegaTexture->size.w;
        spComponentMapIter->second->uv[1] = p[0].y / spMegaTexture->size.h;
        spComponentMapIter->second->uv[2] = p[1].x / spMegaTexture->size.w;
        spComponentMapIter->second->uv[3] = p[1].y / spMegaTexture->size.h;

        ++spComponentMapIter;
    }
        
}	// CalculateGroupUVs


/************************************************************************
*    desc:  Render the textures to a single texture surface
*
*	 param:	const string & group - group from the texture manager to use
************************************************************************/
void CMegaTexture::CopyToMegaTexture( const std::string & group )
{
    HRESULT hresult;

    CComPtr< IDirect3DSurface9 > spTmpMegaSurface;
    
    // Create the mega texture
    if( FAILED( hresult = CXDevice::Instance().GetXDevice()->CreateTexture( 
            spMegaTexture->size.w,
            spMegaTexture->size.h,
            1, 
            0, 
            D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, 
            &spMegaTexture->spTexture,
            NULL ) ) )
    {
        DisplayError( hresult, __FUNCTION__, __LINE__ );
    }

    // Grab the surface of the mega texture
    if( FAILED( hresult = spMegaTexture->spTexture->GetSurfaceLevel( 0, &spTmpMegaSurface ) ) )
        DisplayError( hresult, __FUNCTION__, __LINE__ );

    spComponentMapIter = spComponentMap.begin();
    while( spComponentMapIter != spComponentMap.end() )
    {
        // Temporary texture and surface of the texture
        CComPtr< IDirect3DSurface9 > spTmpSurface;

        if( FAILED( hresult = spComponentMapIter->second->pTexture->spTexture->GetSurfaceLevel( 0, &spTmpSurface ) ) )
            DisplayError( hresult, __FUNCTION__, __LINE__ );

        // Set up the source and destination rects. Both rects are cropped by half a pixel on 
        // each edge to preserve the texture as best as possible
        RECT srcRect, destRect;
        srcRect.left = 0.5f;
        srcRect.top = 0.5f;
        srcRect.right = spComponentMapIter->second->pTexture->size.w - 0.5f;
        srcRect.bottom = spComponentMapIter->second->pTexture->size.h - 0.5f;
        destRect.left = spComponentMapIter->second->pos.x;
        destRect.top = spComponentMapIter->second->pos.y;
        destRect.right = destRect.left + srcRect.right;
        destRect.bottom = destRect.top + srcRect.bottom;

        // Load the temporary surface of the single texture into the surface of the mega texture
        if( FAILED( hresult = D3DXLoadSurfaceFromSurface( 
                spTmpMegaSurface,
                NULL,
                &destRect,
                spTmpSurface,
                NULL,
                &srcRect,
                D3DX_FILTER_NONE,
                0 ) ) )
        {
            DisplayError( hresult, __FUNCTION__, __LINE__ );
        }

        ++spComponentMapIter;
    }

}	// CopyToMegaTexture


/************************************************************************
*    desc:  Display error information
*
*    param: HRESULT hr - return result from function call
************************************************************************/
void CMegaTexture::DisplayError( HRESULT hr, const std::string & functionStr, int lineValue )
{
    switch( hr )
    {
        case D3DERR_NOTAVAILABLE:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error creating texture. This device does not support the queried technique.\n\n%s\nLine: %d") % functionStr % lineValue ));

            break;
        }
        case D3DERR_OUTOFVIDEOMEMORY:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error creating texture. Does not have enough display memory to load texture.\n\n%s\nLine: %d") % functionStr % lineValue ));
    
            break;
        }
        case D3DERR_INVALIDCALL:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error creating texture. The method call is invalid.\n\n%s\nLine: %d") % functionStr % lineValue ));
    
            break;
        }
        case D3DXERR_INVALIDDATA:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error creating texture. The data is invalid.\n\n%s\nLine: %d") % functionStr % lineValue ));
    
            break;
        }
        case E_OUTOFMEMORY:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error creating texture. Direct3D could not allocate sufficient memory to load texture.\n\n%s\nLine: %d") % functionStr % lineValue ));
    
            break;
        }
        default:
        {
            throw NExcept::CCriticalException("Mega Texture Load Error!", 
                    boost::str( boost::format("Error loading material. Unknow error.\n\n%s\nLine: %d") % functionStr % lineValue ));
    
            break;
        }
    }

}	// DisplayError