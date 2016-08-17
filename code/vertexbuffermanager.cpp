
/************************************************************************
*    FILE NAME:       vertexbuffermanager.cpp
*
*    DESCRIPTION:     vertex buffer manager class singleton
************************************************************************/

#if !(defined(__IPHONEOS__) || defined(__ANDROID__))
// Glew dependencies (have to be defined first)
#include <GL/glew.h>
#endif

// Physical component dependency
#include <managers/vertexbuffermanager.h>

// Game lib dependencies
#include <common/quad2d.h>
#include <common/shaderdata.h>
#include <common/scaledframe.h>
#include <common/uv.h>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CVertBufMgr::CVertBufMgr()
    : m_currentVBOID(0),
      m_currentIBOID(0),
      currentMaxFontIndices(0)
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CVertBufMgr::~CVertBufMgr()
{
    // Free all vertex buffers in all groups
    for( auto & mapMapIter : m_vertexBuf2DMapMap )
    {
        for( auto & mapIter : mapMapIter.second )
        {
            glDeleteBuffers(1, &mapIter.second);
        }
    }

    // Free all index buffers in all groups
    for( auto & mapMapIter : m_indexBuf2DMapMap )
    {
        for( auto & mapIter : mapMapIter.second )
        {
            glDeleteBuffers(1, &mapIter.second);
        }
    }

}   // destructer


/************************************************************************
*    desc: Create VBO
************************************************************************/
GLuint CVertBufMgr::CreateVBO( 
    const std::string & group,
    const std::string & name,
    const std::vector<CVertex2D> & vertVec )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_vertexBuf2DMapMap.find( group );
    if( mapMapIter == m_vertexBuf2DMapMap.end() )
        mapMapIter = m_vertexBuf2DMapMap.emplace( group, std::map<const std::string, GLuint>() ).first;

    // See if this vertex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the vertex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        GLuint vboID = 0;
        glGenBuffers( 1, &vboID );
        glBindBuffer( GL_ARRAY_BUFFER, vboID );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CVertex2D)*vertVec.size(), vertVec.data(), GL_STATIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        // Insert the new vertex buffer info
        mapIter = mapMapIter->second.emplace( name, vboID ).first;
    }

    return mapIter->second;

}   // CreateVBO


/************************************************************************
*    desc:  Create a IBO buffer
************************************************************************/
GLuint CVertBufMgr::CreateIBO( const std::string & group, const std::string & name, GLubyte indexData[], int sizeInBytes )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_indexBuf2DMapMap.find( group );
    if( mapMapIter == m_indexBuf2DMapMap.end() )
            mapMapIter = m_indexBuf2DMapMap.emplace( group, std::map<const std::string, GLuint>() ).first;

    // See if this intex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the intex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        GLuint iboID = 0;
        glGenBuffers( 1, &iboID );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboID );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeInBytes, indexData, GL_STATIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        // Insert the new intex buffer info
        mapIter = mapMapIter->second.emplace( name, iboID ).first;
    }

    return mapIter->second;

}   // CreateIBO


/************************************************************************
*    desc:  Create a dynamic font IBO buffer
************************************************************************/
GLuint CVertBufMgr::CreateDynamicFontIBO( const std::string & group, const std::string & name, GLushort * pIndexData, int maxIndicies )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_indexBuf2DMapMap.find( group );
    if( mapMapIter == m_indexBuf2DMapMap.end() )
        mapMapIter = m_indexBuf2DMapMap.emplace( group, std::map<const std::string, GLuint>() ).first;

    // See if this intex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the intex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        GLuint iboID = 0;
        glGenBuffers( 1, &iboID );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboID );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * maxIndicies, pIndexData, GL_DYNAMIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        // Insert the new intex buffer info
        mapIter = mapMapIter->second.emplace( name, iboID ).first;

        // Save the number of indices for later to compair and expand this size of this IBO
        currentMaxFontIndices = maxIndicies;
    }
    else
    {
        // If the new indices are greater then the current, init the IBO with the newest
        if( maxIndicies > currentMaxFontIndices )
        {
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mapIter->second );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * maxIndicies, pIndexData, GL_DYNAMIC_DRAW );

            currentMaxFontIndices = maxIndicies;
        }
    }

    return mapIter->second;

}   // CreateDynamicFontIBO


/************************************************************************
*    desc:  Create a scaled frame
*    NOTE: This is a bit of a brute force implementation but writing an
*          algorithm that takes into account an index buffer is difficult
************************************************************************/
GLuint CVertBufMgr::CreateScaledFrame( 
    const std::string & group,
    const std::string & name,
    const CScaledFrame & scaledFrame,
    const CSize<int> & textSize,
    const CSize<int> & size,
    const std::vector<CVertex2D> & vertVec )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_vertexBuf2DMapMap.find( group );
    if( mapMapIter == m_vertexBuf2DMapMap.end() )
        mapMapIter = m_vertexBuf2DMapMap.emplace( group, std::map<const std::string, GLuint>() ).first;

    // See if this vertex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the vertex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        std::vector<CVertex2D> vertVecTmp;
        vertVecTmp.reserve(16);

        // Generate the scaled frame
        GenerateScaledFrame( vertVecTmp, scaledFrame, textSize, size );
        
        // Add in any additional verts
        if( !vertVecTmp.empty() )
            vertVecTmp.insert( vertVecTmp.end(), vertVec.begin(), vertVec.end() );

        GLuint vboID = 0;
        glGenBuffers( 1, &vboID );
        glBindBuffer( GL_ARRAY_BUFFER, vboID );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CVertex2D)*vertVecTmp.size(), vertVecTmp.data(), GL_STATIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        // Insert the new vertex buffer info
        mapIter = mapMapIter->second.emplace( name, vboID ).first;
    }

    return mapIter->second;

}   // CreateScaledFrame


/************************************************************************
*    desc: Generate a scaled frame
*    NOTE: This is a bit of a brute force implementation but writing an
*          algorithm that takes into account an index buffer is difficult
************************************************************************/
void CVertBufMgr::GenerateScaledFrame(
    std::vector<CVertex2D> & vertVec,
    const CScaledFrame & scaledFrame,
    const CSize<int> & textSize,
    const CSize<int> & size )
{
    // Offsets to center the mesh
    const CPoint<float> center((size.w / 2.f), (size.h / 2.f));
    const CSize<float> frameLgth( (float)size.w - (scaledFrame.m_frame.w * 2.f), (float)size.h - (scaledFrame.m_frame.h * 2.f) );
    const CSize<float> uvLgth( textSize.w - (scaledFrame.m_frame.w * 2.f), textSize.h - (scaledFrame.m_frame.h * 2.f) );
    
    CQuad2D quadBuf[8];
    
    // Left frame
    CreateQuad( CPoint<float>(-center.x, center.y-scaledFrame.m_frame.h),
                CSize<float>(scaledFrame.m_frame.w, -frameLgth.h),
                CUV(0, scaledFrame.m_frame.h),
                CSize<float>(scaledFrame.m_frame.w, uvLgth.h),
                textSize,
                size,
                quadBuf[0] );

    // top left
    CreateQuad( CPoint<float>(-center.x, center.y),
                CSize<float>(scaledFrame.m_frame.w, -scaledFrame.m_frame.h),
                CUV(0, 0),
                CSize<float>(scaledFrame.m_frame.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[1] );

    // top
    CreateQuad( CPoint<float>(-(center.x-scaledFrame.m_frame.w), center.y),
                CSize<float>(frameLgth.w, -scaledFrame.m_frame.h),
                CUV(scaledFrame.m_frame.w, 0),
                CSize<float>(uvLgth.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[2] );

    // top right
    CreateQuad( CPoint<float>(center.x-scaledFrame.m_frame.w, center.y),
                CSize<float>(scaledFrame.m_frame.w, -scaledFrame.m_frame.h),
                CUV(scaledFrame.m_frame.w + uvLgth.w,0),
                CSize<float>(scaledFrame.m_frame.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[3] );

    // right frame
    CreateQuad( CPoint<float>(center.x-scaledFrame.m_frame.w, center.y-scaledFrame.m_frame.h),
                CSize<float>(scaledFrame.m_frame.w, -frameLgth.h),
                CUV(scaledFrame.m_frame.w + uvLgth.w, scaledFrame.m_frame.h),
                CSize<float>(scaledFrame.m_frame.w, uvLgth.h),
                textSize,
                size,
                quadBuf[4] );

    // bottom right
    CreateQuad( CPoint<float>(center.x-scaledFrame.m_frame.w, -(center.y-scaledFrame.m_frame.h)),
                CSize<float>(scaledFrame.m_frame.w, -scaledFrame.m_frame.h),
                CUV(scaledFrame.m_frame.w + uvLgth.w, scaledFrame.m_frame.h + uvLgth.h),
                CSize<float>(scaledFrame.m_frame.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[5] );

    // bottom frame
    CreateQuad( CPoint<float>(-(center.x-scaledFrame.m_frame.w), -(center.y-scaledFrame.m_frame.h)),
                CSize<float>(frameLgth.w, -scaledFrame.m_frame.h),
                CUV(scaledFrame.m_frame.w, scaledFrame.m_frame.h + uvLgth.h),
                CSize<float>(uvLgth.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[6] );

    // bottom left
    CreateQuad( CPoint<float>(-center.x, -(center.y-scaledFrame.m_frame.h)),
                CSize<float>(scaledFrame.m_frame.w, -scaledFrame.m_frame.h),
                CUV(0, scaledFrame.m_frame.h + uvLgth.h),
                CSize<float>(scaledFrame.m_frame.w, scaledFrame.m_frame.h),
                textSize,
                size,
                quadBuf[7] );

    // Piece together the needed unique verts
    vertVec.push_back( quadBuf[0].vert[0] );
    vertVec.push_back( quadBuf[0].vert[1] );
    vertVec.push_back( quadBuf[0].vert[2] );
    vertVec.push_back( quadBuf[0].vert[3] );
    vertVec.push_back( quadBuf[1].vert[1] );
    vertVec.push_back( quadBuf[1].vert[2] );
    vertVec.push_back( quadBuf[2].vert[1] );
    vertVec.push_back( quadBuf[2].vert[3] );
    vertVec.push_back( quadBuf[3].vert[1] );
    vertVec.push_back( quadBuf[3].vert[3] );
    vertVec.push_back( quadBuf[4].vert[0] );
    vertVec.push_back( quadBuf[4].vert[3] );
    vertVec.push_back( quadBuf[5].vert[0] );
    vertVec.push_back( quadBuf[5].vert[3] );
    vertVec.push_back( quadBuf[6].vert[0] );
    vertVec.push_back( quadBuf[7].vert[0] );
}


/************************************************************************
*    desc:  Create a quad
************************************************************************/
void CVertBufMgr::CreateQuad( 
    const CPoint<float> & vert,
    const CSize<float> & vSize,
    const CUV & uv,
    const CSize<float> & uvSize,
    const CSize<float> & textSize,
    const CSize<float> & size,
    CQuad2D & quadBuf )
{
    // For OpenGL pixel perfect rendering is an even size graphic,
    // for DirectX, it's an odd size graphic.
    
    // Check if the width or height is odd. If so, we offset 
    // by 0.5 for proper orthographic rendering
    float additionalOffsetX = 0;
    if( (int)size.w % 2 != 0 )
        additionalOffsetX = 0.5f;

    float additionalOffsetY = 0;
    if( (int)size.h % 2 != 0 )
        additionalOffsetY = 0.5f;

    // Calculate the third vertex of the first face
    quadBuf.vert[0].vert.x = vert.x + additionalOffsetX;
    quadBuf.vert[0].vert.y = vert.y + additionalOffsetY + vSize.h;
    quadBuf.vert[0].uv.u = uv.u / textSize.w;
    quadBuf.vert[0].uv.v = (uv.v + uvSize.h) / textSize.h;

    // Calculate the second vertex of the first face
    quadBuf.vert[1].vert.x = vert.x + additionalOffsetX + vSize.w;
    quadBuf.vert[1].vert.y = vert.y + additionalOffsetY;
    quadBuf.vert[1].uv.u = (uv.u + uvSize.w) / textSize.w;
    quadBuf.vert[1].uv.v = uv.v / textSize.h;

    // Calculate the first vertex of the first face
    quadBuf.vert[2].vert.x = vert.x + additionalOffsetX;
    quadBuf.vert[2].vert.y = vert.y + additionalOffsetY;
    quadBuf.vert[2].uv.u = uv.u / textSize.w;
    quadBuf.vert[2].uv.v = uv.v / textSize.h;

    // Calculate the second vertex of the second face
    quadBuf.vert[3].vert.x = vert.x + additionalOffsetX + vSize.w;
    quadBuf.vert[3].vert.y = vert.y + additionalOffsetY + vSize.h;
    quadBuf.vert[3].uv.u = (uv.u + uvSize.w) / textSize.w;
    quadBuf.vert[3].uv.v = (uv.v + uvSize.h) / textSize.h;

}   // CreateScaledFrameSegment


/************************************************************************
*    desc: See if a VBO already exists
************************************************************************/
GLuint CVertBufMgr::IsVBO( const std::string & group, const std::string & name )
{
    // See if the group exists
    auto mapMapIter = m_vertexBuf2DMapMap.find( group );
    if( mapMapIter == m_vertexBuf2DMapMap.end() )
        return 0;

    // See if this vertex buffer ID has already been created
    auto mapIter = mapMapIter->second.find( name );
    if( mapIter == mapMapIter->second.end() )
        return 0;

    return mapIter->second;
}


/************************************************************************
*    desc:  Function call used to manage what buffer is currently bound.
*           This insures that we don't keep rebinding the same buffer
************************************************************************/
void CVertBufMgr::BindBuffers( GLuint vboID, GLuint iboID )
{
    if( m_currentVBOID != vboID )
    {
        // save the current binding
        m_currentVBOID = vboID;

        // Have OpenGL bind this buffer now
        glBindBuffer( GL_ARRAY_BUFFER, vboID );
    }

    if( m_currentIBOID != iboID )
    {
        // save the current binding
        m_currentIBOID = iboID;

        // Have OpenGL bind this buffer now
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, iboID );
    }

}   // BindBuffers


/************************************************************************
*    desc:  Unbind the buffers and reset the flag
************************************************************************/
void CVertBufMgr::UnbindBuffers()
{
    m_currentVBOID = 0;
    m_currentIBOID = 0;
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

}   // UnbindTexture


/************************************************************************
*    desc:  Delete buffer group
************************************************************************/
void CVertBufMgr::DeleteBufferGroupFor2D( const std::string & group )
{
    {
        auto mapMapIter = m_vertexBuf2DMapMap.find( group );
        if( mapMapIter != m_vertexBuf2DMapMap.end() )
        {
            // Delete all the buffers in this group
            for( auto & mapIter : mapMapIter->second )
            {
                glDeleteBuffers(1, &mapIter.second);
            }

            // Erase this group
            m_vertexBuf2DMapMap.erase( mapMapIter );
        }
    }

    {
        auto mapMapIter = m_indexBuf2DMapMap.find( group );
        if( mapMapIter != m_indexBuf2DMapMap.end() )
        {
            // Delete all the buffers in this group
            for( auto & mapIter : mapMapIter->second )
            {
                glDeleteBuffers(1, &mapIter.second);
            }

            // Erase this group
            m_indexBuf2DMapMap.erase( mapMapIter );
        }
    }

}   // DeleteVertexBufGroupFor2D

