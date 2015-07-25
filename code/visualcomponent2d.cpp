
/************************************************************************
*    FILE NAME:       visualcomponent2d.cpp
*
*    DESCRIPTION:     Class for handling the visual part of the sprite
************************************************************************/

// Glew dependencies
#include <GL/glew.h>

// Physical component dependency
#include <2d/visualcomponent2d.h>

// Standard lib dependencies

// Boost lib dependencies
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>

// Game lib dependencies
#include <objectdata/objectdata2d.h>
#include <objectdata/objectvisualdata2d.h>
#include <managers/shadermanager.h>
#include <managers/texturemanager.h>
#include <managers/vertexbuffermanager.h>
#include <managers/fontmanager.h>
#include <common/quad2d.h>
#include <system/device.h>
#include <utilities/xmlparsehelper.h>
#include <utilities/genfunc.h>
#include <utilities/exceptionhandling.h>
#include <utilities/statcounter.h>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CVisualComponent2d::CVisualComponent2d( const CObjectData2D & objectData )
    : m_visualData( objectData.GetVisualData() ),
      m_vertexLocation(0),
      m_uvLocation(0),
      m_text0Location(0),
      m_matrixLocation(0),
      m_vertexBufSize( sizeof(CVertex2D) ),
      m_uvOffset( sizeof(CPoint<float>) ),
      m_textureID( m_visualData.GetTextureID() ),
      m_programID(0),
      m_color( m_visualData.GetColor() ),
      m_vbo( m_visualData.GetVBO() ),
      m_ibo( m_visualData.GetIBO() ),
      m_vertexCount( m_visualData.GetVertexCount() ),
      m_drawMode( (m_visualData.GetGenerationType() == NDefs::EGT_QUAD) ? GL_TRIANGLE_FAN : GL_TRIANGLES ),
      m_indiceType( (m_visualData.GetGenerationType() == NDefs::EGT_FONT) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE ),
      m_charCount(0),
      m_kerning(0.f),
      m_spaceCharKerning(0.f),
      m_lineWrapWidth(-1.f),
      m_lineWrapHeight(0.f),
      m_hAlign(NDefs::EHA_HORZ_CENTER),
      m_vAlign(NDefs::EVA_VERT_CENTER)
{
    if( IsActive() )
    {
        const CShaderData & shaderData( CShaderMgr::Instance().GetShaderData( objectData.GetVisualData().GetShaderID() ) );

        // Do we have a texture? This could be a solid rect
        if( (m_textureID > 0) || (m_visualData.GetGenerationType() == NDefs::EGT_FONT) )
        {
            m_uvLocation = shaderData.GetAttributeLocation( "in_uv" );
            m_text0Location = shaderData.GetUniformLocation( "text0" );
        }

        m_programID = shaderData.GetProgramID();
        m_vertexLocation = shaderData.GetAttributeLocation( "in_position" );
        m_matrixLocation = shaderData.GetUniformLocation( "cameraViewProjMatrix" );
        m_colorLocation = shaderData.GetUniformLocation( "color" );
    }

}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CVisualComponent2d::~CVisualComponent2d()
{
    // Delete the VBO if this is a font
    if( (m_visualData.GetGenerationType() == NDefs::EGT_FONT) && (m_vbo > 0) )
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
        // Increment our stat counter to keep track of what is going on.
        CStatCounter::Instance().IncDisplayCounter();

        // Bind the shader. This must be done first
        CShaderMgr::Instance().BindShaderProgram( m_programID );

        // Bind the VBO and IBO
        CVertBufMgr::Instance().BindBuffers( m_vbo, m_ibo );

        // Are we rendering with a texture?
        if( m_textureID > 0 )
        {
            // Bind the texture
            CTextureMgr::Instance().BindTexture( m_textureID );
            glUniform1i( m_text0Location, 0); // 0 = TEXTURE0

            // Enable the UV attribute shade data
            glEnableVertexAttribArray( m_uvLocation );
            glVertexAttribPointer( m_uvLocation, 2, GL_FLOAT, GL_FALSE, m_vertexBufSize, (void*)m_uvOffset );
        }

        // Enable the vertex attribute shader data
        glEnableVertexAttribArray( m_vertexLocation );
        glVertexAttribPointer( m_vertexLocation, 3, GL_FLOAT, GL_FALSE, m_vertexBufSize, nullptr );

        // Send the color to the shader
        glUniform4fv( m_colorLocation, 1, (float *)&m_color );

        // If this is a quad, we need to take into account the vertex scale
        if( m_visualData.GetGenerationType() == NDefs::EGT_QUAD )
        {
            // Calculate the final matrix
            CMatrix finalMatrix;
            finalMatrix.Scale( m_visualData.GetVertexScale() );
            finalMatrix *= matrix;

            // Send the final matrix to the shader
            glUniformMatrix4fv( m_matrixLocation, 1, GL_FALSE, finalMatrix() );
        }
        else
        {
            glUniformMatrix4fv( m_matrixLocation, 1, GL_FALSE, matrix() );
        }

        // Render it
        glDrawElements( m_drawMode, m_vertexCount, m_indiceType, nullptr );
    }

}   // Render


/************************************************************************
*    desc:  Is this component active?
************************************************************************/
bool CVisualComponent2d::IsActive()
{
    return (m_visualData.GetGenerationType() != NDefs::EGT_NULL);

}   // IsActive


/************************************************************************
*    desc:  Set the font info
*
*    parm: fontName - font to use
*          _kerning - distance between each character
*          _spaceCharKerning - special kerning just for the space character
*          lineWrapWidth - width of line to force wrap
*          lineWrapHeightPadding - add spacing to the lines
*          hAlign - horzontal alignment
*          vAlign - vertical alignment
*
*    NOTE: Line wrap feature only supported for horizontal left
************************************************************************/
void CVisualComponent2d::SetFontInfo(
    const std::string & fontName,
    float kerning,
    float spaceCharKerning,
    float lineWrapWidth,
    float lineWrapHeight,
    NDefs::EHorzAlignment hAlign,
    NDefs::EVertAlignment vAlign )
{
    m_fontName = fontName;
    m_kerning = kerning;
    m_spaceCharKerning = spaceCharKerning;
    m_lineWrapWidth = lineWrapWidth;
    m_lineWrapHeight = lineWrapHeight;
    m_hAlign = hAlign;
    m_vAlign = vAlign;

}   // SetFontInfo


/************************************************************************
*    desc:  Create the font string
************************************************************************/
void CVisualComponent2d::CreateFontString( const std::string & fontString )
{
    CreateFontString(
        fontString,
        m_fontName,
        m_kerning,
        m_spaceCharKerning,
        m_lineWrapWidth,
        m_lineWrapHeight,
        m_hAlign,
        m_vAlign );

}   // CreateFontString


/************************************************************************
*    desc:  Create the font string
*
*    parm: str - character string to render
*          fontName - font to use
*          _kerning - distance between each character
*          _spaceCharKerning - special kerning just for the space character
*          lineWrapWidth - width of line to force wrap
*          lineWrapHeightPadding - add spacing to the lines
*          hAlign - horzontal alignment
*          vAlign - vertical alignment
*
*    NOTE: Line wrap feature only supported for horizontal left
************************************************************************/
void CVisualComponent2d::CreateFontString(
    const std::string & fontString,
    const std::string & fontName,
    float kerning,
    float spaceCharKerning,
    float lineWrapWidth,
    float lineWrapHeight,
    NDefs::EHorzAlignment hAlign,
    NDefs::EVertAlignment vAlign )
{
    // Qualify if we want to build the font string
    if( !fontString.empty() && !fontName.empty() && (fontString != m_fontString) )
    {
        const CFont & font = CFontMgr::Instance().GetFont( fontName );

        m_textureID = font.GetTextureID();

        m_fontString = fontString;

        // count up the number of space characters
        const int spaceCharCount = NGenFunc::CountStrOccurrence( m_fontString, " " );

        // count up the number of bar | characters
        const int barCharCount = NGenFunc::CountStrOccurrence( m_fontString, "|" );

        // Size of the allocation
        m_charCount = m_fontString.size() - spaceCharCount - barCharCount;
        m_vertexCount = m_charCount * 6;

        // Calculate the highest value range needed to store in index buffer
        const int quadValueMax = m_charCount * 4;

        // Allocate the quad array
        boost::scoped_array<CQuad2D> quadBuf( new CQuad2D[m_charCount] );

        // Create a buffer to hold the indicies
        boost::scoped_array<GLushort> indxBuf( new GLushort[m_vertexCount] );

        float xOffset = 0.f;
        float width = 0.f;
        float lineHeightOffset = 0.f;
        float lineHeightWrap = font.GetLineHeight() + font.GetVertPadding() + lineWrapHeight;
        float initialHeightOffset = font.GetBaselineOffset() + font.GetVertPadding();
        float lineSpace = font.GetLineHeight() - font.GetBaselineOffset();

        uint counter = 0;
        int lineCount = 0;

        // Get the size of the texture
        CSize<float> textureSize = font.GetTextureSize();

        // Handle the horizontal alignment
        std::vector<float> lineWidthOffsetVec = CalcLineWidthOffset( font, m_fontString, kerning, spaceCharKerning, lineWrapWidth, hAlign );

        // Set the initial line offset
        xOffset = lineWidthOffsetVec[lineCount++];

        // Handle the vertical alighnmenrt
        if( vAlign == NDefs::EVA_VERT_TOP )
            lineHeightOffset = -initialHeightOffset;

        if( vAlign == NDefs::EVA_VERT_CENTER )
        {
            lineHeightOffset = -(initialHeightOffset - ((font.GetBaselineOffset()-lineSpace) / 2.f) - font.GetVertPadding());

            if( lineWidthOffsetVec.size() > 1 )
                lineHeightOffset = ((lineHeightWrap * lineWidthOffsetVec.size()) / 2.f) - font.GetBaselineOffset();
        }

        else if( vAlign == NDefs::EVA_VERT_BOTTOM )
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
                    quadBuf[counter].vert[0].vert.x = xOffset + charData.offset.w + additionalOffsetX;
                    quadBuf[counter].vert[0].vert.y = yOffset + additionalOffsetY;
                    quadBuf[counter].vert[0].uv.u = rect.x1 / textureSize.w;
                    quadBuf[counter].vert[0].uv.v = (rect.y1 + rect.y2) / textureSize.h;

                    // Calculate the second vertex of the first face
                    quadBuf[counter].vert[1].vert.x = xOffset + rect.x2 + charData.offset.w + additionalOffsetX;
                    quadBuf[counter].vert[1].vert.y = yOffset + rect.y2 + additionalOffsetY;
                    quadBuf[counter].vert[1].uv.u = (rect.x1 + rect.x2) / textureSize.w;
                    quadBuf[counter].vert[1].uv.v = rect.y1 / textureSize.h;

                    // Calculate the third vertex of the first face
                    quadBuf[counter].vert[2].vert.x = xOffset + charData.offset.w + additionalOffsetX;
                    quadBuf[counter].vert[2].vert.y = yOffset + rect.y2 + additionalOffsetY;
                    quadBuf[counter].vert[2].uv.u = rect.x1 / textureSize.w;
                    quadBuf[counter].vert[2].uv.v = rect.y1 / textureSize.h;

                    // Calculate the second vertex of the second face
                    quadBuf[counter].vert[3].vert.x = xOffset + rect.x2 + charData.offset.w + additionalOffsetX;
                    quadBuf[counter].vert[3].vert.y = yOffset + additionalOffsetY;
                    quadBuf[counter].vert[3].uv.u = (rect.x1 + rect.x2) / textureSize.w;
                    quadBuf[counter].vert[3].uv.v = (rect.y1 + rect.y2) / textureSize.h;

                    // Create the indicies into the VBO
                    int arrayIndex = counter * 6;
                    int vertIndex = counter * 4;

                    indxBuf[arrayIndex] = vertIndex;
                    indxBuf[arrayIndex+1] = vertIndex+1;
                    indxBuf[arrayIndex+2] = vertIndex+2;

                    indxBuf[arrayIndex+3] = vertIndex;
                    indxBuf[arrayIndex+4] = vertIndex+3;
                    indxBuf[arrayIndex+5] = vertIndex+1;

                    ++counter;
                }

                // Inc the font position
                float inc = charData.xAdvance + kerning + font.GetHorzPadding();

                // Add in any additional spacing for the space character
                if( id == ' ' )
                        inc += spaceCharKerning;

                width += inc;
                xOffset += inc;

                // Wrap to another line
                if( (id == ' ') && (lineWrapWidth > 0.f) )
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
                            nextWord += anotherCharData.xAdvance + kerning + font.GetHorzPadding();
                        }
                    }

                    if( width + nextWord >= lineWrapWidth )
                    {
                        xOffset = lineWidthOffsetVec[lineCount++];
                        width = 0.f;

                        lineHeightOffset += -lineHeightWrap;
                    }
                }
            }
        }

        // Save the data
        // If one doesn't exist, create the VBO and IBO for this font
        if( m_vbo == 0 )
            glGenBuffers( 1, &m_vbo );

        glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof(CQuad2D) * m_charCount, quadBuf.get(), GL_DYNAMIC_DRAW );

        // All fonts share the same IBO because it's always the same and the only difference is it's length
        // This updates the current IBO if it exceeds the current max
        m_ibo = CVertBufMgr::Instance().CreateDynamicFontIBO( CFontMgr::Instance().GetGroup(), "dynamic_font_ibo", indxBuf.get(), m_vertexCount );

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
    float kerning,
    float spaceCharKerning,
    float lineWrapWidth,
    NDefs::EHorzAlignment hAlign )
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
            AddLineWithToVec( font, lineWidthOffsetVec, hAlign, width, firstCharOffset, lastCharOffset );

            counter = 0;
            width = 0;
        }
        else
        {
            // Get the next character
            const CCharData & charData = font.GetCharData( id );

            if(counter == 0)
                    firstCharOffset = charData.offset.w;

            spaceWidth = charData.xAdvance + kerning + font.GetHorzPadding();

            // Add in any additional spacing for the space character
            if( id == ' ' )
                    spaceWidth += spaceCharKerning;

            width += spaceWidth;

            if( id != ' ')
                    lastCharOffset = charData.offset.w;

            ++counter;
        }

        // Wrap to another line
        if( (id == ' ') && (lineWrapWidth > 0.f) )
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
                    nextWord += charData.xAdvance + kerning + font.GetHorzPadding();
                }
            }

            if( width + nextWord >= lineWrapWidth )
            {
                // Add the line width to the vector based on horz alignment
                AddLineWithToVec( font, lineWidthOffsetVec, hAlign, width-spaceWidth, firstCharOffset, lastCharOffset );

                counter = 0;
                width = 0;
            }
        }
    }

    // Add the line width to the vector based on horz alignment
    AddLineWithToVec( font, lineWidthOffsetVec, hAlign, width, firstCharOffset, lastCharOffset );

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
*    desc:  Set the texture ID from index
************************************************************************/
void CVisualComponent2d::SetTextureID( uint index )
{
    m_textureID = m_visualData.GetTextureID( index );

}   // SetTextureID

