
/************************************************************************
*    FILE NAME:       visualcomponent2d.cpp
*
*    DESCRIPTION:     Class for handling the visual part of the sprite
************************************************************************/

#if !(defined(__IPHONEOS__) || defined(__ANDROID__))
// Glew dependencies (have to be defined first)
#include <GL/glew.h>
#endif

// Physical component dependency
#include <2d/visualcomponent2d.h>

// Game lib dependencies
#include <objectdata/objectvisualdata2d.h>
#include <managers/shadermanager.h>
#include <managers/texturemanager.h>
#include <managers/vertexbuffermanager.h>
#include <managers/fontmanager.h>
#include <common/quad2d.h>
#include <system/device.h>
#include <utilities/xmlParser.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/genfunc.h>
#include <utilities/exceptionhandling.h>
#include <utilities/statcounter.h>

// AngelScript lib dependencies
#include <angelscript.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Standard lib dependencies
#include <memory>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CVisualComponent2d::CVisualComponent2d( const CObjectVisualData2D & visualData ) :
    m_programID(0),
    m_vbo( visualData.GetVBO() ),
    m_ibo( visualData.GetIBO() ),
    m_textureID( visualData.GetTextureID() ),
    m_vertexLocation(0),
    m_uvLocation(0),
    m_text0Location(0),
    m_colorLocation(0),
    m_matrixLocation(0),
    m_glyphLocation(0),
    GENERATION_TYPE( visualData.GetGenerationType() ),
    m_quadVertScale( visualData.GetVertexScale() ),
    m_visualData( visualData ),
    m_color( visualData.GetColor() ),
    m_iboCount( visualData.GetIBOCount() ),
    m_drawMode( (visualData.GetGenerationType() == NDefs::EGT_QUAD || visualData.GetGenerationType() == NDefs::EGT_SPRITE_SHEET) ? GL_TRIANGLE_FAN : GL_TRIANGLES ),
    m_indiceType( (visualData.GetGenerationType() == NDefs::EGT_FONT) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE )
{
    if( IsActive() )
    {
        const CShaderData & shaderData( CShaderMgr::Instance().GetShaderData( visualData.GetShaderID() ) );

        // Common shader members
        m_programID = shaderData.GetProgramID();
        m_vertexLocation = shaderData.GetAttributeLocation( "in_position" );
        m_matrixLocation = shaderData.GetUniformLocation( "cameraViewProjMatrix" );
        m_colorLocation = shaderData.GetUniformLocation( "color" );
        
        // Do we have a texture? This could be a solid rect
        if( (m_textureID > 0) || (GENERATION_TYPE == NDefs::EGT_FONT) )
        {
            m_uvLocation = shaderData.GetAttributeLocation( "in_uv" );
            m_text0Location = shaderData.GetUniformLocation( "text0" );
        }

        // Is this a sprite sheet? Get the glyph rect position
        if( GENERATION_TYPE == NDefs::EGT_SPRITE_SHEET )
        {
            m_glyphLocation = shaderData.GetUniformLocation( "glyphRect" );
            
            m_glyphUV = visualData.GetSpriteSheet().GetGlyph().GetUV();
        }
    }

}   // constructor


/************************************************************************
*    desc:  destructer
************************************************************************/
CVisualComponent2d::~CVisualComponent2d()
{
    // Delete the VBO if this is a font
    if( (GENERATION_TYPE == NDefs::EGT_FONT) && (m_vbo > 0) )
        glDeleteBuffers(1, &m_vbo);

    // The IBO for the font is managed by the vertex buffer manager.
    // Font IBO are all the same with the only difference being
    // length of the character string.

}   // destructer


/************************************************************************
*    desc:  do the render
************************************************************************/
void CVisualComponent2d::Render( const CMatrix & matrix )
{
    if( IsActive() )
    {
        const int VERTEX_BUF_SIZE( sizeof(CVertex2D) );
            
        // Increment our stat counter to keep track of what is going on.
        CStatCounter::Instance().IncDisplayCounter();

        // Bind the shader. This must be done first
        CShaderMgr::Instance().BindShaderProgram( m_programID );

        // Bind the VBO and IBO
        CVertBufMgr::Instance().BindBuffers( m_vbo, m_ibo );

        // Are we rendering with a texture?
        if( m_textureID > 0 )
        {
            const int UV_OFFSET( sizeof(CPoint<float>) );
            
            // Bind the texture
            CTextureMgr::Instance().BindTexture2D( m_textureID );
            glUniform1i( m_text0Location, 0); // 0 = TEXTURE0

            // Enable the UV attribute shade data
            glEnableVertexAttribArray( m_uvLocation );
            glVertexAttribPointer( m_uvLocation, 2, GL_FLOAT, GL_FALSE, VERTEX_BUF_SIZE, (void*)UV_OFFSET );
        }

        // Enable the vertex attribute shader data
        glEnableVertexAttribArray( m_vertexLocation );
        glVertexAttribPointer( m_vertexLocation, 3, GL_FLOAT, GL_FALSE, VERTEX_BUF_SIZE, nullptr );

        // Send the color to the shader
        glUniform4fv( m_colorLocation, 1, (float *)&m_color );

        // If this is a quad, we need to take into account the vertex scale
        if( GENERATION_TYPE == NDefs::EGT_QUAD )
        {
            // Calculate the final matrix
            CMatrix finalMatrix;
            finalMatrix.Scale( m_quadVertScale );
            finalMatrix *= matrix;

            // Send the final matrix to the shader
            glUniformMatrix4fv( m_matrixLocation, 1, GL_FALSE, finalMatrix() );
        }
        // If this is a sprite sheet, we need to take into account the vertex scale and glyph rect
        else if( GENERATION_TYPE == NDefs::EGT_SPRITE_SHEET )
        {
            // Calculate the final matrix
            CMatrix finalMatrix;
            finalMatrix.Scale( m_quadVertScale );
            finalMatrix *= matrix;

            // Send the final matrix to the shader
            glUniformMatrix4fv( m_matrixLocation, 1, GL_FALSE, finalMatrix() );
            
            // Send the glyph rect
            glUniform4fv( m_glyphLocation, 1, (GLfloat *)&m_glyphUV );
        }
        // this is for scaled frame and font rendering
        else
        {
            glUniformMatrix4fv( m_matrixLocation, 1, GL_FALSE, matrix() );
        }

        // Render it
        glDrawElements( m_drawMode, m_iboCount, m_indiceType, nullptr );
    }

}   // Render


/************************************************************************
*    desc:  Load the font properties from XML node
************************************************************************/
void CVisualComponent2d::LoadFontPropFromNode( const XMLNode & node )
{
    // Get the must have font related name
    m_fontProp.m_fontName = node.getAttribute( "fontName" );

    // Get the attributes node
    const XMLNode attrNode = node.getChildNode( "attributes" );
    if( !attrNode.isEmpty() )
    {
        if( attrNode.isAttributeSet( "kerning" ) )
            m_fontProp.m_kerning = std::atof(attrNode.getAttribute( "kerning" ));

        if( attrNode.isAttributeSet( "spaceCharKerning" ) )
            m_fontProp.m_spaceCharKerning = std::atof(attrNode.getAttribute( "spaceCharKerning" ));

        if( attrNode.isAttributeSet( "lineWrapWidth" ) )
            m_fontProp.m_lineWrapWidth = std::atof(attrNode.getAttribute( "lineWrapWidth" ));

        if( attrNode.isAttributeSet( "lineWrapHeight" ) )
            m_fontProp.m_lineWrapHeight = std::atof(attrNode.getAttribute( "lineWrapHeight" ));
    }

    // Get the alignment node
    const XMLNode alignmentNode = node.getChildNode( "alignment" );
    if( !alignmentNode.isEmpty() )
    {
        // Set the default alignment
        m_fontProp.m_hAlign = NParseHelper::LoadHorzAlignment( alignmentNode, NDefs::EHA_HORZ_CENTER );
        m_fontProp.m_vAlign = NParseHelper::LoadVertAlignment( alignmentNode, NDefs::EVA_VERT_CENTER );
    }
    
}   // LoadFontPropFromNode


/************************************************************************
*    desc:  Create the font string
************************************************************************/
void CVisualComponent2d::CreateFontString( const std::string & fontString )
{
    CreateFontString( fontString, m_fontProp );

}   // CreateFontString


/************************************************************************
*    desc:  Create the font string
*
*    NOTE: Line wrap feature only supported for horizontal left
************************************************************************/
void CVisualComponent2d::CreateFontString(
    const std::string & fontString,
    const CFontProperties & fontProp )
{
    // Qualify if we want to build the font string
    if( !fontString.empty() && !fontProp.m_fontName.empty() && (fontString != m_fontString) )
    {
        m_fontStrSize.Reset();
        float lastCharDif(0.f);
        
        const CFont & font = CFontMgr::Instance().GetFont( fontProp.m_fontName );

        m_textureID = font.GetTextureID();

        m_fontString = fontString;

        // count up the number of space characters
        const int spaceCharCount = NGenFunc::CountStrOccurrence( m_fontString, " " );

        // count up the number of bar | characters
        const int barCharCount = NGenFunc::CountStrOccurrence( m_fontString, "|" );

        // Size of the allocation
        int charCount = m_fontString.size() - spaceCharCount - barCharCount;
		m_iboCount = charCount * 6;

        // Allocate the quad array
        std::unique_ptr<CQuad2D[]> upQuadBuf( new CQuad2D[charCount] );

        // Create a buffer to hold the indicies
        std::unique_ptr<GLushort[]> upIndxBuf( new GLushort[m_iboCount] );

        float xOffset = 0.f;
        float width = 0.f;
        float lineHeightOffset = 0.f;
        float lineHeightWrap = font.GetLineHeight() + font.GetVertPadding() + fontProp.m_lineWrapHeight;
        float initialHeightOffset = font.GetBaselineOffset() + font.GetVertPadding();
        float lineSpace = font.GetLineHeight() - font.GetBaselineOffset();

        uint counter = 0;
        int lineCount = 0;

        // Get the size of the texture
        CSize<float> textureSize = font.GetTextureSize();

        // Handle the horizontal alignment
        std::vector<float> lineWidthOffsetVec = CalcLineWidthOffset( font, m_fontString, fontProp );

        // Set the initial line offset
        xOffset = lineWidthOffsetVec[lineCount++];

        // Handle the vertical alighnmenrt
        if( fontProp.m_vAlign == NDefs::EVA_VERT_TOP )
            lineHeightOffset = -initialHeightOffset;

        if( fontProp.m_vAlign == NDefs::EVA_VERT_CENTER )
        {
            lineHeightOffset = -(initialHeightOffset - ((font.GetBaselineOffset()-lineSpace) / 2.f) - font.GetVertPadding());

            if( lineWidthOffsetVec.size() > 1 )
                lineHeightOffset = ((lineHeightWrap * lineWidthOffsetVec.size()) / 2.f) - font.GetBaselineOffset();
        }

        else if( fontProp.m_vAlign == NDefs::EVA_VERT_BOTTOM )
        {
            lineHeightOffset = -(initialHeightOffset - font.GetBaselineOffset() - font.GetVertPadding());

            if( lineWidthOffsetVec.size() > 1 )
                lineHeightOffset += (lineHeightWrap * (lineWidthOffsetVec.size()-1));
        }

        // Remove any fractional component of the line height offset
        lineHeightOffset = (int)lineHeightOffset;

        // Setup each character in the vertex buffer
        for( size_t i = 0; i < m_fontString.size(); ++i )
        {
            char id = m_fontString[i];

            // Line wrap if '|' character was used
            if( id == '|' )
            {
                xOffset = lineWidthOffsetVec[lineCount];
                width = 0.f;

                lineHeightOffset += -lineHeightWrap;
                ++lineCount;
            }
            else
            {
                // See if we can find the character
                const CCharData & charData = font.GetCharData(id);

                // Ignore space characters
                if( id != ' ' )
                {
                    CRect<float> rect = charData.rect;

                    float yOffset = (font.GetLineHeight() - rect.y2 - charData.offset.h) + lineHeightOffset;

                    // Check if the width or height is odd. If so, we offset
                    // by 0.5 for proper orthographic rendering
                    float additionalOffsetX = 0;
                    if( (int)rect.x2 % 2 != 0 )
                        additionalOffsetX = 0.5f;

                    float additionalOffsetY = 0;
                    if( (int)rect.y2 % 2 != 0 )
                        additionalOffsetY = 0.5f;

                    // Calculate the first vertex of the first face
                    upQuadBuf[counter].vert[0].vert.x = xOffset + charData.offset.w + additionalOffsetX;
                    upQuadBuf[counter].vert[0].vert.y = yOffset + additionalOffsetY;
                    upQuadBuf[counter].vert[0].uv.u = rect.x1 / textureSize.w;
                    upQuadBuf[counter].vert[0].uv.v = (rect.y1 + rect.y2) / textureSize.h;

                    // Calculate the second vertex of the first face
                    upQuadBuf[counter].vert[1].vert.x = xOffset + rect.x2 + charData.offset.w + additionalOffsetX;
                    upQuadBuf[counter].vert[1].vert.y = yOffset + rect.y2 + additionalOffsetY;
                    upQuadBuf[counter].vert[1].uv.u = (rect.x1 + rect.x2) / textureSize.w;
                    upQuadBuf[counter].vert[1].uv.v = rect.y1 / textureSize.h;

                    // Calculate the third vertex of the first face
                    upQuadBuf[counter].vert[2].vert.x = xOffset + charData.offset.w + additionalOffsetX;
                    upQuadBuf[counter].vert[2].vert.y = yOffset + rect.y2 + additionalOffsetY;
                    upQuadBuf[counter].vert[2].uv.u = rect.x1 / textureSize.w;
                    upQuadBuf[counter].vert[2].uv.v = rect.y1 / textureSize.h;

                    // Calculate the second vertex of the second face
                    upQuadBuf[counter].vert[3].vert.x = xOffset + rect.x2 + charData.offset.w + additionalOffsetX;
                    upQuadBuf[counter].vert[3].vert.y = yOffset + additionalOffsetY;
                    upQuadBuf[counter].vert[3].uv.u = (rect.x1 + rect.x2) / textureSize.w;
                    upQuadBuf[counter].vert[3].uv.v = (rect.y1 + rect.y2) / textureSize.h;

                    // Create the indicies into the VBO
                    int arrayIndex = counter * 6;
                    int vertIndex = counter * 4;

                    upIndxBuf[arrayIndex] = vertIndex;
                    upIndxBuf[arrayIndex+1] = vertIndex+1;
                    upIndxBuf[arrayIndex+2] = vertIndex+2;

                    upIndxBuf[arrayIndex+3] = vertIndex;
                    upIndxBuf[arrayIndex+4] = vertIndex+3;
                    upIndxBuf[arrayIndex+5] = vertIndex+1;

                    ++counter;
                }

                // Inc the font position
                float inc = charData.xAdvance + fontProp.m_kerning + font.GetHorzPadding();

                // Add in any additional spacing for the space character
                if( id == ' ' )
                    inc += fontProp.m_spaceCharKerning;

                width += inc;
                xOffset += inc;
                
                // Get the longest width of this font string
                if( m_fontStrSize.w < width )
                {
                    m_fontStrSize.w = width;
                    
                    // This is the space between this character and the next.
                    // Save this difference so that it can be subtracted at the end
                    lastCharDif = inc - charData.rect.x2;
                }

                // Wrap to another line
                if( (id == ' ') && (fontProp.m_lineWrapWidth > 0.f) )
                {
                    float nextWord = 0.f;

                    // Get the length of the next word to see if if should wrap
                    for( size_t j = i+1; j < m_fontString.size(); ++j )
                    {
                        id = m_fontString[j];

                        if( id != '|' )
                        {
                            // See if we can find the character
                            const CCharData & anotherCharData = font.GetCharData(id);

                            // Break here when space is found
                            // Don't add the space to the size of the next word
                            if( id == ' ' )
                                break;

                            // Don't count the
                            nextWord += anotherCharData.xAdvance + fontProp.m_kerning + font.GetHorzPadding();
                        }
                    }

                    if( width + nextWord >= fontProp.m_lineWrapWidth )
                    {
                        xOffset = lineWidthOffsetVec[lineCount++];
                        width = 0.f;

                        lineHeightOffset += -lineHeightWrap;
                    }
                }
            }
        }
        
        // Subtract the extra space after the last character
        m_fontStrSize.w -= lastCharDif;
        m_fontStrSize.h = font.GetLineHeight();

        // Save the data
        // If one doesn't exist, create the VBO and IBO for this font
        if( m_vbo == 0 )
            glGenBuffers( 1, &m_vbo );

        glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CQuad2D) * charCount, upQuadBuf.get(), GL_STATIC_DRAW );

        // All fonts share the same IBO because it's always the same and the only difference is it's length
        // This updates the current IBO if it exceeds the current max
        m_ibo = CVertBufMgr::Instance().CreateDynamicFontIBO( CFontMgr::Instance().GetGroup(), "dynamic_font_ibo", upIndxBuf.get(), m_iboCount );

        // unbind the buffers
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }

}   // SetFontString


/************************************************************************
*    desc:  Add up all the character widths
************************************************************************/
std::vector<float> CVisualComponent2d::CalcLineWidthOffset(
    const CFont & font,
    const std::string & str,
    const CFontProperties & fontProp )
{
    float firstCharOffset = 0;
    float lastCharOffset = 0;
    float spaceWidth = 0;
    float width = 0;
    int counter = 0;
    std::vector<float> lineWidthOffsetVec;

    for( size_t i = 0; i < str.size(); ++i )
    {
        char id = str[i];

        // Line wrap if '|' character was used
        if( id == '|' )
        {
            // Add the line width to the vector based on horz alignment
            AddLineWithToVec( font, lineWidthOffsetVec, fontProp.m_hAlign, width, firstCharOffset, lastCharOffset );

            counter = 0;
            width = 0;
        }
        else
        {
            // Get the next character
            const CCharData & charData = font.GetCharData( id );

            if(counter == 0)
                    firstCharOffset = charData.offset.w;

            spaceWidth = charData.xAdvance + fontProp.m_kerning + font.GetHorzPadding();

            // Add in any additional spacing for the space character
            if( id == ' ' )
                    spaceWidth += fontProp.m_spaceCharKerning;

            width += spaceWidth;

            if( id != ' ')
                    lastCharOffset = charData.offset.w;

            ++counter;
        }

        // Wrap to another line
        if( (id == ' ') && (fontProp.m_lineWrapWidth > 0.f) )
        {
            float nextWord = 0.f;

            // Get the length of the next word to see if if should wrap
            for( size_t j = i+1; j < str.size(); ++j )
            {
                id = str[j];

                if( id != '|' )
                {
                    // See if we can find the character
                    const CCharData & charData = font.GetCharData(id);

                    // Break here when space is found
                    // Don't add the space to the size of the next word
                    if( id == ' ' )
                        break;

                    // Don't count the
                    nextWord += charData.xAdvance + fontProp.m_kerning + font.GetHorzPadding();
                }
            }

            if( width + nextWord >= fontProp.m_lineWrapWidth )
            {
                // Add the line width to the vector based on horz alignment
                AddLineWithToVec( font, lineWidthOffsetVec, fontProp.m_hAlign, width-spaceWidth, firstCharOffset, lastCharOffset );

                counter = 0;
                width = 0;
            }
        }
    }

    // Add the line width to the vector based on horz alignment
    AddLineWithToVec( font, lineWidthOffsetVec, fontProp.m_hAlign, width, firstCharOffset, lastCharOffset );

    return lineWidthOffsetVec;

}   // CalcLineWidthOffset


/************************************************************************
*    desc:  Add the line width to the vector based on horz alignment
************************************************************************/
void CVisualComponent2d::AddLineWithToVec(
    const CFont & font,
    std::vector<float> & lineWidthOffsetVec,
    const NDefs::EHorzAlignment hAlign,
    float width,
    float firstCharOffset,
    float lastCharOffset )
{
    if( hAlign == NDefs::EHA_HORZ_LEFT )
        lineWidthOffsetVec.push_back(-(firstCharOffset + font.GetHorzPadding()));

    else if( hAlign == NDefs::EHA_HORZ_CENTER )
        lineWidthOffsetVec.push_back(-((width + (firstCharOffset + lastCharOffset)) / 2.f));

    else if( hAlign == NDefs::EHA_HORZ_RIGHT )
        lineWidthOffsetVec.push_back(-(width - lastCharOffset - font.GetHorzPadding()));

    // Remove any fractional component of the line height offset
    lineWidthOffsetVec.back() = (int)lineWidthOffsetVec.back();

}   // AddLineWithToVec


/************************************************************************
*    desc:  Get the displayed font string
************************************************************************/
const std::string & CVisualComponent2d::GetFontString()
{
    return m_fontString;

}   // GetFontString


/************************************************************************
*    desc:  Set/Get the color
************************************************************************/
void CVisualComponent2d::SetColor( const CColor & color )
{
    m_color = color;

}   // SetColor

void CVisualComponent2d::SetRGBA( float r, float g, float b, float a )
{
    // This function assumes values between 0.0 to 1.0.
    m_color.Set( r, g, b, a );

}   // SetRGBA

const CColor & CVisualComponent2d::GetColor() const
{
    return m_color;

}   // GetColor


/************************************************************************
*    desc:  Set/Get the alpha
************************************************************************/
void CVisualComponent2d::SetAlpha( float alpha )
{
    m_color.a = alpha;

}   // SetAlpha

float CVisualComponent2d::GetAlpha() const
{
    return m_color.a;

}   // GetAlpha


/************************************************************************
*    desc:  Set the frame ID from index
************************************************************************/
void CVisualComponent2d::SetFrameID( uint index )
{
    if( GENERATION_TYPE == NDefs::EGT_SPRITE_SHEET )
    {
        auto rGlyph = m_visualData.GetSpriteSheet().GetGlyph( index );
        m_glyphUV = rGlyph.GetUV();
        auto rSize = rGlyph.GetSize();
        m_quadVertScale.x = rSize.w;
        m_quadVertScale.y = rSize.h;
    }
    else
        m_textureID = m_visualData.GetTextureID( index );

}   // SetFrameID


/************************************************************************
*    desc:  Set the default color
************************************************************************/
void CVisualComponent2d::SetDefaultColor()
{
    m_color = m_visualData.GetColor();

}   // SetDefaultColor


/************************************************************************
*    desc:  Is this component active?
************************************************************************/
bool CVisualComponent2d::IsActive()
{
    return (GENERATION_TYPE != NDefs::EGT_NULL);

}   // IsActive


/************************************************************************
*    desc:  Get the font size
************************************************************************/
const CSize<float> & CVisualComponent2d::GetFontSize() const
{
    return m_fontStrSize;

}   // IsActive

