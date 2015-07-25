
/************************************************************************
*    FILE NAME:       vertexbuffermanager.cpp
*
*    DESCRIPTION:     vertex buffer manager class singleton
************************************************************************/

// Glew dependencies
#include <GL/glew.h>

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
    for( auto mapMapIter = m_vertexBuf2DMapMap.begin();
         mapMapIter != m_vertexBuf2DMapMap.end();
         ++mapMapIter )
    {
        for( auto mapIter = mapMapIter->second.begin();
             mapIter != mapMapIter->second.end();
             ++mapIter )
        {
            glDeleteBuffers(1, &mapIter->second);
        }
    }

    // Free all index buffers in all groups
    for( auto mapMapIter = m_indexBuf2DMapMap.begin();
         mapMapIter != m_indexBuf2DMapMap.end();
         ++mapMapIter )
    {
        for( auto mapIter = mapMapIter->second.begin();
             mapIter != mapMapIter->second.end();
             ++mapIter )
        {
            glDeleteBuffers(1, &mapIter->second);
        }
    }

}   // destructer


/************************************************************************
*    desc:  Create a 2D quad VBO buffer
************************************************************************/
GLuint CVertBufMgr::CreateQuadVBO( const std::string & group, const std::string & name, const CRect<float> & uv )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_vertexBuf2DMapMap.find( group );
    if( mapMapIter == m_vertexBuf2DMapMap.end() )
            mapMapIter = m_vertexBuf2DMapMap.insert( std::make_pair(group, std::map<const std::string, GLuint>()) ).first;

    // See if this vertex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the vertex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        // Test VBO data
        // UV x starts from the top left. UV y starts from the bottom right. Clockwise
        // verts start from the bottom left. Counter clockwise
        GLfloat vertexData[] =
        {
            -0.5f, -0.5f, 0.0,  uv.x1, uv.y2,
             0.5f, -0.5f, 0.0,  uv.x2, uv.y2,
             0.5f,  0.5f, 0.0,  uv.x2, uv.y1,
            -0.5f,  0.5f, 0.0,  uv.x1, uv.y1
        };

        GLuint vboID = 0;
        glGenBuffers( 1, &vboID );
        glBindBuffer( GL_ARRAY_BUFFER, vboID );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CQuad2D), vertexData, GL_STATIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        // Insert the new vertex buffer info
        mapIter = mapMapIter->second.insert( std::make_pair(name, vboID) ).first;
    }

    return mapIter->second;

}   // CreateBasicQuadVBO


/************************************************************************
*    desc:  Create a IBO buffer
************************************************************************/
GLuint CVertBufMgr::CreateIBO( const std::string & group, const std::string & name, GLubyte indexData[], int sizeInBytes )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_indexBuf2DMapMap.find( group );
    if( mapMapIter == m_indexBuf2DMapMap.end() )
            mapMapIter = m_indexBuf2DMapMap.insert( std::make_pair(group, std::map<const std::string, GLuint>()) ).first;

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
        mapIter = mapMapIter->second.insert( std::make_pair(name, iboID) ).first;
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
            mapMapIter = m_indexBuf2DMapMap.insert( std::make_pair(group, std::map<const std::string, GLuint>()) ).first;

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
        mapIter = mapMapIter->second.insert( std::make_pair(name, iboID) ).first;

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

}   // CreateVertBuffer2D


/************************************************************************
*    desc:  Create a scaled frame
*    NOTE: This is a bit of a brute force implementation but writing an
*          algorithm that takes into account an index buffer is difficult
************************************************************************/
GLuint CVertBufMgr::CreateScaledFrame( const std::string & group,
                                       const std::string & name,
                                       const CScaledFrame & scaledFrame,
                                       const CSize<int> & textSize,
                                       const CSize<int> & size )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_vertexBuf2DMapMap.find( group );
    if( mapMapIter == m_vertexBuf2DMapMap.end() )
            mapMapIter = m_vertexBuf2DMapMap.insert( std::make_pair(group, std::map<const std::string, GLuint>()) ).first;

    // See if this vertex buffer ID has already been loaded
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, create the vertex buffer and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        // Offsets to center the mesh
        const CPoint<float> center((size.w / 2.f), (size.h / 2.f));
        const CSize<float> frameLgth( (float)size.w - (scaledFrame.m_frame.w * 2.f), (float)size.h - (scaledFrame.m_frame.h * 2.f) );
        const CSize<float> uvLgth( textSize.w - (scaledFrame.m_frame.w * 2.f), textSize.h - (scaledFrame.m_frame.h * 2.f) );

        CQuad2D quadBuf[8];
        CVertex2D vertBuf[16];

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
        vertBuf[0] = quadBuf[0].vert[0];
        vertBuf[1] = quadBuf[0].vert[1];
        vertBuf[2] = quadBuf[0].vert[2];
        vertBuf[3] = quadBuf[0].vert[3];
        vertBuf[4] = quadBuf[1].vert[1];
        vertBuf[5] = quadBuf[1].vert[2];
        vertBuf[6] = quadBuf[2].vert[1];
        vertBuf[7] = quadBuf[2].vert[3];
        vertBuf[8] = quadBuf[3].vert[1];
        vertBuf[9] = quadBuf[3].vert[3];
        vertBuf[10] = quadBuf[4].vert[0];
        vertBuf[11] = quadBuf[4].vert[3];
        vertBuf[12] = quadBuf[5].vert[0];
        vertBuf[13] = quadBuf[5].vert[3];
        vertBuf[14] = quadBuf[6].vert[0];
        vertBuf[15] = quadBuf[7].vert[0];

        GLuint vboID = 0;
        glGenBuffers( 1, &vboID );
        glBindBuffer( GL_ARRAY_BUFFER, vboID );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CVertex2D)*16, vertBuf, GL_STATIC_DRAW );

        // unbind the buffer
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        // Insert the new vertex buffer info
        mapIter = mapMapIter->second.insert( std::make_pair(name, vboID) ).first;
    }

    return mapIter->second;

}   // CreateScaledFrame


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

}	// UnbindTexture


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
            for( auto mapIter = mapMapIter->second.begin();
                 mapIter != mapMapIter->second.end();
                 ++mapIter )
            {
                glDeleteBuffers(1, &mapIter->second);
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
            for( auto mapIter = mapMapIter->second.begin();
                 mapIter != mapMapIter->second.end();
                 ++mapIter )
            {
                glDeleteBuffers(1, &mapIter->second);
            }

            // Erase this group
            m_indexBuf2DMapMap.erase( mapMapIter );
        }
    }

}   // DeleteVertexBufGroupFor2D

