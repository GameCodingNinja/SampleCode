
/************************************************************************
*    FILE NAME:       ojectvisualdata2d.cpp
*
*    DESCRIPTION:     Class containing the 2D object's visual data
************************************************************************/

#if !(defined(__IPHONEOS__) || defined(__ANDROID__))
// Glew dependencies (have to be defined first)
#include <GL/glew.h>
#endif

// Physical component dependency
#include <objectdata/objectvisualdata2d.h>

// Game lib dependencies
#include <managers/texturemanager.h>
#include <managers/vertexbuffermanager.h>
#include <managers/spritesheetmanager.h>
#include <utilities/xmlParser.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/exceptionhandling.h>
#include <common/defs.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Standard lib dependencies
#include <cstring>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CObjectVisualData2D::CObjectVisualData2D() :
    m_vbo(0),
    m_ibo(0),
    m_genType(NDefs::EGT_NULL),
    m_textureSequenceCount(0),
    m_compressed(false),
    m_iboCount(0),
    m_vertexScale(1,1,1)
{
}   // constructor


/************************************************************************
*    desc:  Destructer                                                             
************************************************************************/
CObjectVisualData2D::~CObjectVisualData2D()
{
    // NOTE: Nothing should ever be deleted here
}   // Destructer


/************************************************************************
*    desc:  Load the object data from node
************************************************************************/
void CObjectVisualData2D::LoadFromNode( const XMLNode & objectNode )
{
    const XMLNode visualNode = objectNode.getChildNode( "visual" );
    if( !visualNode.isEmpty() )
    {
        // See if we have a texture to load
        const XMLNode textureNode = visualNode.getChildNode("texture");
        if( !textureNode.isEmpty() )
        {
            if( textureNode.isAttributeSet("count") )
                m_textureSequenceCount = std::atoi( textureNode.getAttribute( "count" ) );
            
            if( textureNode.isAttributeSet("file") )
                m_textureFilePath = textureNode.getAttribute( "file" );

            // Is this a compressed texture?
            if( textureNode.isAttributeSet("compressed") )
                m_compressed = (std::strcmp(textureNode.getAttribute( "compressed" ), "true") == 0);
        }

        // Get the mesh node
        const XMLNode meshNode = visualNode.getChildNode( "mesh" );
        if( !meshNode.isEmpty() )
        {
            if( meshNode.isAttributeSet("genType") )
            {
                std::string genTypeStr = meshNode.getAttribute( "genType" );

                if( genTypeStr == "quad" )
                    m_genType = NDefs::EGT_QUAD;
                
                else if( genTypeStr == "sprite_sheet" )
                    m_genType = NDefs::EGT_SPRITE_SHEET;
                
                else if( genTypeStr == "scaled_frame" )
                    m_genType = NDefs::EGT_SCALED_FRAME;

                else if( genTypeStr == "mesh_file" )
                    m_genType = NDefs::EGT_MESH_FILE;

                else if( genTypeStr == "font" )
                    m_genType = NDefs::EGT_FONT;
                
                else if( genTypeStr == "scaled_frame_mesh_file" )
                    m_genType = NDefs::EGT_SCALED_FRAME_MESH_FILE;
            }

            const XMLNode quadNode = meshNode.getChildNode( "quad" );
            if( !quadNode.isEmpty() )
            {
                m_uv.x1 = std::atof(quadNode.getAttribute( "uv.x1" ));
                m_uv.y1 = std::atof(quadNode.getAttribute( "uv.y1" ));
                m_uv.x2 = std::atof(quadNode.getAttribute( "uv.x2" ));
                m_uv.y2 = std::atof(quadNode.getAttribute( "uv.y2" ));
            }
            
            const XMLNode spriteSheetNode = meshNode.getChildNode("spriteSheet");
            if( !spriteSheetNode.isEmpty() )
            {
                if( spriteSheetNode.isAttributeSet("defIndex") )
                    m_spriteSheet.SetDefaultIndex( std::atoi( spriteSheetNode.getAttribute( "defIndex" ) ) );
                
                // Make sure all elements are defined for manually building the sprite sheet data
                if( spriteSheetNode.isAttributeSet("glyphCount") )
                {
                    const uint glyphCount = std::atoi( spriteSheetNode.getAttribute( "glyphCount" ) );

                    if( spriteSheetNode.isAttributeSet("columns") )
                    {
                        const uint columns = std::atoi( spriteSheetNode.getAttribute( "columns" ) );

                        // Init the manual build of the sprite sheet data
                        m_spriteSheet.InitBuild( glyphCount, columns );
                    }
                }
                
                // See if any glyph Id's have been defined
                for( int i = 0; i < spriteSheetNode.nChildNode(); ++i )
                    m_glyphIDs.push_back( spriteSheetNode.getChildNode(i).getAttribute( "id" ) );
            }

            const XMLNode scaledFrameNode = meshNode.getChildNode( "scaledFrame" );
            if( !scaledFrameNode.isEmpty() )
            {
                m_scaledFrame.m_frame.w = std::atof(scaledFrameNode.getAttribute( "thicknessWidth" ));
                m_scaledFrame.m_frame.h = std::atof(scaledFrameNode.getAttribute( "thicknessHeight" ));

                if( scaledFrameNode.isAttributeSet("centerQuad") )
                    m_scaledFrame.m_centerQuad = (std::strcmp(scaledFrameNode.getAttribute( "centerQuad" ), "false") != 0);
                
                if( scaledFrameNode.isAttributeSet("frameBottom") )
                    m_scaledFrame.m_bottomFrame = (std::strcmp(scaledFrameNode.getAttribute( "frameBottom" ), "false") != 0);
            }

            const XMLNode fileNode = meshNode.getChildNode( "file" );
            if( !fileNode.isEmpty() )
            {
                m_meshFile = fileNode.getAttribute( "name" );
            }
        }

        // Check for color
        m_color = NParseHelper::LoadColor( visualNode, m_color );

        // The shader node determines which shader to use
        const XMLNode shaderNode = visualNode.getChildNode( "shader" );
        if( !shaderNode.isEmpty() )
        {
            m_shaderID = shaderNode.getAttribute( "id" );
        }

        // Raise an exception if there's a vbo but no shader
        if( (m_genType != NDefs::EGT_NULL) && m_shaderID.empty() )
        {
            throw NExcept::CCriticalException("Shader effect or techique not set!",
                boost::str( boost::format("Shader object data missing.\n\n%s\nLine: %s")
                    % __FUNCTION__ % __LINE__ ));
        }
    }

}   // LoadFromNode


/************************************************************************
*    desc:  Create the object from data
************************************************************************/
void CObjectVisualData2D::CreateFromData( const std::string & group, CSize<int> & rSize )
{
    CTexture texture;

    // Try to load the texture if one exists
    LoadTexture( texture, group, rSize );

    if( m_genType == NDefs::EGT_QUAD )
    {
        // Generate a quad
        GenerateQuad( group );
        
        // For this generation type, the image size is the default scale
        m_vertexScale.x = rSize.w;
        m_vertexScale.y = rSize.h;
    }
    // Load object data defined as a sprite sheet
    else if( m_genType == NDefs::EGT_SPRITE_SHEET )
    {
        // Build the simple (grid) sprite sheet from XML data
        if( m_meshFile.empty() )
            m_spriteSheet.Build( rSize );
        
        // Load a sprite sheet XML that doesn't use string id's
        // This is assumed to be a simple sprite sheet
        else if( m_glyphIDs.empty() )
            m_spriteSheet.LoadFromXML( m_meshFile, rSize, false );

        // Load complex sprite sheet data to the manager. It's assumed
        // that string Id's are for complex sprite sheets that are shared
        // among many sprites
        else
        {
            // This will return the sprite sheet if it's been loaded or not, that uses string Id's
            auto rSpriteSheet = CSpriteSheetMgr::Instance().LoadFromXML( m_meshFile, rSize );
            
            // Copy the needed glyph data from the manager
            rSpriteSheet.CopyTo( m_spriteSheet, m_glyphIDs );
        }
        
        // Generate a quad
        GenerateQuad( group );
        
        // For this generation type, the glyph size is the default scale
        const auto rGlyphSize = m_spriteSheet.GetGlyph().GetSize();
        m_vertexScale.x = rGlyphSize.w;
        m_vertexScale.y = rGlyphSize.h;
    }
    else if( m_genType == NDefs::EGT_SCALED_FRAME )
    {
        // Generate a scaled frame
        GenerateScaledFrame( texture, group, rSize );
    }
    else if( m_genType == NDefs::EGT_SCALED_FRAME_MESH_FILE )
    {
        // Generate a scaled frame
        GenerateScaledFrameMeshFile( texture, group, rSize );
    }

}   // CreateFromData


/************************************************************************
*    desc:  Try to load the texture if one exists
************************************************************************/
void CObjectVisualData2D::LoadTexture( CTexture & rTexture, const std::string & group, CSize<int> & rSize )
{
    if( !m_textureFilePath.empty() )
    {
        if( m_textureSequenceCount > 0 )
        {
            for( int i = 0; i < m_textureSequenceCount; ++i )
            {
                std::string file = boost::str( boost::format(m_textureFilePath) % i );
                rTexture = CTextureMgr::Instance().LoadFor2D( group, file, m_compressed );
                m_textureIDVec.push_back( rTexture.GetID() );
            }
        }
        else
        {
            rTexture = CTextureMgr::Instance().LoadFor2D( group, m_textureFilePath, m_compressed );
            m_textureIDVec.push_back( rTexture.GetID() );
        }

        // If the passed in size reference is empty, set it to the texture size
        if( rSize.IsEmpty() )
            rSize = rTexture.GetSize();
    }
    
}   // LoadTexture


/************************************************************************
*    desc:  Generate a quad
************************************************************************/
void CObjectVisualData2D::GenerateQuad( const std::string & group )
{
    GLubyte indexData[] = {0, 1, 2, 3};

    std::string vboName = boost::str( boost::format("quad_%s_%s_%s_%s") % m_uv.x1 % m_uv.y1 % m_uv.x2 % m_uv.y2 );
    
    // VBO data
    // The order of the verts is counter clockwise
    // 1----0
    // |   /|
    // |  / |
    // | /  |
    // 2----3
    std::vector<CVertex2D> vertVec =
    {
        {{ 0.5f,  0.5f, 0.0},  {m_uv.x2, m_uv.y1}},
        {{-0.5f,  0.5f, 0.0},  {m_uv.x1, m_uv.y1}},
        {{-0.5f, -0.5f, 0.0},  {m_uv.x1, m_uv.y2}},
        {{ 0.5f, -0.5f, 0.0},  {m_uv.x2, m_uv.y2}}
    };

    m_vbo = CVertBufMgr::Instance().CreateVBO( group, vboName, vertVec );
    m_ibo = CVertBufMgr::Instance().CreateIBO( group, "quad_0123", indexData, sizeof(indexData) );

    // A quad has 4 ibos
    m_iboCount = 4;
        
}   // GenerateQuad


/************************************************************************
*    desc:  Generate a scaled frame
************************************************************************/
void CObjectVisualData2D::GenerateScaledFrame( 
    const CTexture & texture, const std::string & group, const CSize<int> & size )
{
    std::string vboName = boost::str( boost::format("scaled_frame_%d_%d_%d_%d_%d_%d") 
        % size.w % size.h % m_scaledFrame.m_frame.w % m_scaledFrame.m_frame.h % texture.m_size.w % texture.m_size.h );

    m_vbo = CVertBufMgr::Instance().CreateScaledFrame(
        group, vboName, m_scaledFrame, texture.GetSize(), size, std::vector<CVertex2D>() );

    GLubyte indexData[] = {
        0,1,2,     0,3,1,
        2,4,5,     2,1,4,
        1,6,4,     1,7,6,
        7,8,6,     7,9,8,
        10,9,7,    10,11,9,
        12,11,10,  12,13,11,
        14,10,3,   14,12,10,
        15,3,0,    15,14,3,
        3,7,1,     3,10,7 };

    // Create the reusable IBO buffer
    m_ibo = CVertBufMgr::Instance().CreateIBO( group, "scaled_frame", indexData, sizeof(indexData) );

    // Set the ibo count depending on the number of quads being rendered
    // If the center quad is not used, just adjust the ibo count because
    // the center quad is just reused verts anyways and is that last 6 in the IBO
    // If the frame bottom is not being use, just subtract.
    // Center quad and no frame bottom can't co-exist.
	m_iboCount = 6 * 8;
    if( m_scaledFrame.m_centerQuad )
	m_iboCount += 6;

    else if( !m_scaledFrame.m_bottomFrame )
	m_iboCount -= 6 * 3;
        
}   // GenerateScaledFrame


/************************************************************************
*    desc:  Generate a scaled frame with a mesh file
************************************************************************/
void CObjectVisualData2D::GenerateScaledFrameMeshFile( 
    const CTexture & texture, const std::string & group, const CSize<int> & size )
{
    // Construct the name used for vbo and ibo
    std::string name = "scaled_frame_mesh_" + m_meshFile;

    // See if it already exists before loading the mesh file
    m_vbo = CVertBufMgr::Instance().IsVBO( group, name );
    if( m_vbo == 0 )
    {
        std::vector<CVertex2D> vertVec;
        std::vector<GLubyte> iboVec = {
            0,1,2,     0,3,1,
            2,4,5,     2,1,4,
            1,6,4,     1,7,6,
            7,8,6,     7,9,8,
            10,9,7,    10,11,9,
            12,11,10,  12,13,11,
            14,10,3,   14,12,10,
            15,3,0,    15,14,3 };

        if( m_scaledFrame.m_centerQuad )
        {
            std::vector<GLubyte> exraVec = { 3,7,1, 3,10,7 };
            iboVec.insert( iboVec.end(), exraVec.begin(), exraVec.end() );
        }

        // Load a mesh from XML file
        LoadMeshFromXML( texture, group, size, 16, vertVec, iboVec );

        // create the vbo
        m_vbo = CVertBufMgr::Instance().CreateScaledFrame(
            group, name, m_scaledFrame, texture.GetSize(), size, vertVec );

        // Create the unique IBO buffer
        m_ibo = CVertBufMgr::Instance().CreateIBO( group, name, iboVec.data(), sizeof(GLubyte)*iboVec.size() );

		m_iboCount = iboVec.size();
    }

}   // GenerateScaledFrameMeshFile


/************************************************************************
*    desc:  Generate a mesh file
************************************************************************/
void CObjectVisualData2D::GenerateFromMeshFile( 
    const CTexture & texture, const std::string & group, const CSize<int> & size )
{
    // Construct the name used for vbo and ibo
    std::string name = "mesh_file_" + m_meshFile;

    // See if it already exists before loading the mesh file
    m_vbo = CVertBufMgr::Instance().IsVBO( group, name );
    if( m_vbo == 0 )
    {
        std::vector<CVertex2D> vertVec;
        std::vector<GLubyte> iboVec;

        // Load a mesh from XML file
        LoadMeshFromXML( texture, group, size, 16, vertVec, iboVec );

        // create the vbo
        m_vbo = CVertBufMgr::Instance().CreateVBO( group, name, vertVec );

        // Create the unique IBO buffer
        m_ibo = CVertBufMgr::Instance().CreateIBO( group, name, iboVec.data(), sizeof(GLubyte)*iboVec.size() );

		m_iboCount = iboVec.size();
    }

}   // GenerateFromMeshFile


/************************************************************************
*    desc:  Load a mesh from XML file
************************************************************************/
void CObjectVisualData2D::LoadMeshFromXML( 
    const CTexture & texture,
    const std::string & group,
    const CSize<int> & size,
    int iboOffset,
    std::vector<CVertex2D> & rVertVec,
    std::vector<GLubyte> & rIboVec )
{
    float additionalOffsetX = 0;
    if( (int)size.GetW() % 2 != 0 )
        additionalOffsetX = 0.5f;

    float additionalOffsetY = 0;
    if( (int)size.GetH() % 2 != 0 )
        additionalOffsetY = 0.5f;

    // This converts the data to a center aligned vertex buffer
    const CSize<float> centerAlignSize(-(size.w / 2), (size.h / 2));

    // Open and parse the XML file:
    const XMLNode mainNode = XMLNode::openFileHelper( m_meshFile.c_str(), "mesh" );
    const XMLNode vboNode = mainNode.getChildNode( "vbo" );
    if( !vboNode.isEmpty() )
    {
        CVertex2D vert;
        
        rVertVec.reserve( vboNode.nChildNode() );

        for( int i = 0; i < vboNode.nChildNode(); ++i )
        {
            // Load the 2D vert
            vert = NParseHelper::LoadVertex2d( vboNode.getChildNode( "vert", i ) );

            // This converts the data to a center aligned vertex buffer
            vert.vert.x = centerAlignSize.w + vert.vert.x + additionalOffsetX;
            vert.vert.y = centerAlignSize.h - vert.vert.y + additionalOffsetY;
            vert.uv.u = vert.uv.u / texture.GetSize().GetW();
            vert.uv.v = vert.uv.v / texture.GetSize().GetH();

            rVertVec.emplace_back( vert );
        }
    }

    const XMLNode iboNode = mainNode.getChildNode( "ibo" );
    if( !iboNode.isEmpty() )
    {            
        for( int i = 0; i < iboNode.nChildNode(); ++i )
        {
            const XMLNode iNode = iboNode.getChildNode( "i", i );

            rIboVec.push_back( iboOffset + std::atoi(iNode.getText()) );
        }
    }

}   // LoadMeshFromXML


/************************************************************************
*    desc:  Get the gne type
************************************************************************/
NDefs::EGenerationType CObjectVisualData2D::GetGenerationType() const 
{
    return m_genType;
}


/************************************************************************
*    desc:  Get the texture ID
************************************************************************/
GLuint CObjectVisualData2D::GetTextureID( uint index ) const 
{
    if( m_textureIDVec.empty() )
        return 0;
    else
        return m_textureIDVec[index];
}


/************************************************************************
*    desc:  Get the name of the shader ID
************************************************************************/
const std::string & CObjectVisualData2D::GetShaderID() const
{
    return m_shaderID;
}


/************************************************************************
*    desc:  Get the color
************************************************************************/
const CColor & CObjectVisualData2D::GetColor() const 
{
    return m_color;
}


/************************************************************************
*    desc:  Get the VBO
************************************************************************/
GLuint CObjectVisualData2D::GetVBO() const 
{
    return m_vbo;
}


/************************************************************************
*    desc:  Get the IBO
************************************************************************/
GLuint CObjectVisualData2D::GetIBO() const 
{
    return m_ibo;
}


/************************************************************************
*    desc:  Get the vertex count
************************************************************************/
int CObjectVisualData2D::GetIBOCount() const 
{
    return m_iboCount;
}


/************************************************************************
*    desc:  Get the frame count
************************************************************************/
size_t CObjectVisualData2D::GetFrameCount() const 
{
    if( m_genType == NDefs::EGT_SPRITE_SHEET )
        return m_spriteSheet.GetCount();
    
    return m_textureIDVec.size();
}


/************************************************************************
*    desc:  Get the vertex scale
************************************************************************/
const CPoint<float> & CObjectVisualData2D::GetVertexScale() const 
{
    return m_vertexScale;
}


/************************************************************************
*    desc:  Whether or not the visual tag was specified
************************************************************************/
bool CObjectVisualData2D::IsEmpty() const 
{
    return (m_genType == NDefs::EGT_NULL);
}


/************************************************************************
*    desc:  Get the sprite sheet
************************************************************************/
const CSpriteSheet & CObjectVisualData2D::GetSpriteSheet() const 
{
    return m_spriteSheet;
}
