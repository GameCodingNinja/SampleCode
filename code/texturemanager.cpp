
/************************************************************************
*    FILE NAME:       texturemanager.h
*
*    DESCRIPTION:     texture class singleton
************************************************************************/

#if defined(__IPHONEOS__) || defined(__ANDROID__)
#include "SDL_opengles2.h"
#else
#include <GL/glew.h>     // Glew dependencies (have to be defined first)
#include <SDL_opengl.h>  // SDL/OpenGL lib dependencies
#endif

// Physical component dependency
#include <managers/texturemanager.h>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <utilities/settings.h>

// SOIL lib dependency
#include <soil/SOIL.h>
#include <soil/stb_image_aug.h>

// Boost lib dependencies
#include <boost/format.hpp>

// SDL lib dependencies
#include <SDL.h>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CTextureMgr::CTextureMgr() :
    m_currentTextureID(0),
    m_anisotropicLevel(0)
{
    InitAnisotropic();

}   // constructor


/************************************************************************
*    desc:  destructer
************************************************************************/
CTextureMgr::~CTextureMgr()
{
    // Free all textures in all groups
    for( auto & mapMapIter : m_textureFor2DMapMap )
    {
        for( auto & mapIter : mapMapIter.second )
        {
            glDeleteTextures(1, &mapIter.second.m_id);
        }
    }

    for( auto & mapMapIter : m_textureFor3DMapMap )
    {
        for( auto & mapIter : mapMapIter.second )
        {
            glDeleteTextures(1, &mapIter.second.m_id);
        }
    }

}   // destructer


/************************************************************************
*    desc:  Load the texture from file path
************************************************************************/
const CTexture & CTextureMgr::LoadFor2D( const std::string & group, const std::string & filePath, bool compressed )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_textureFor2DMapMap.find( group );
    if( mapMapIter == m_textureFor2DMapMap.end() )
            mapMapIter = m_textureFor2DMapMap.emplace( group, std::map<const std::string, CTexture>() ).first;

    // See if this texture has already been loaded
    auto mapIter = mapMapIter->second.find( filePath );

    // If it's not found, load the texture and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        CTexture texture;

        // Load the texture from file path
        LoadTexture( texture, filePath, compressed );
        
        // Init with common features until I need to configure differently
        glBindTexture(GL_TEXTURE_2D, texture.GetID());
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        
        glBindTexture(GL_TEXTURE_2D, 0);

        // Insert the new texture info
        mapIter = mapMapIter->second.emplace( filePath, texture ).first;
    }

    return mapIter->second;

}   // LoadFor2D


/************************************************************************
*    desc:  Load the texture from file path
************************************************************************/
const CTexture & CTextureMgr::LoadFor3D( const std::string & group, const std::string & filePath, bool compressed )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_textureFor3DMapMap.find( group );
    if( mapMapIter == m_textureFor3DMapMap.end() )
        mapMapIter = m_textureFor3DMapMap.emplace( group, std::map<const std::string, CTexture>() ).first;

    // See if this texture has already been loaded
    auto mapIter = mapMapIter->second.find( filePath );

    // If it's not found, load the texture and add it to the list
    if( mapIter == mapMapIter->second.end() )
    {
        CTexture texture;

        // Load the texture from file path
        LoadTexture( texture, filePath, compressed );
        
        // Init with common features until I need to configure differently
        glBindTexture(GL_TEXTURE_2D, texture.GetID());
        
        // Set the anisotropic value
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_anisotropicLevel );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        
        glBindTexture(GL_TEXTURE_2D, 0);

        // Insert the new texture info
        mapIter = mapMapIter->second.emplace( filePath, texture ).first;
    }

    return mapIter->second;

}   // LoadFor3D


/************************************************************************
*    desc:  Load the texture from file path
************************************************************************/
void CTextureMgr::LoadTexture( CTexture & texture, const std::string & filePath, bool compressed )
{
    texture.m_id = SOIL_load_OGL_texture(
        filePath.c_str(),
        &texture.m_size.w,
        &texture.m_size.h,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        (compressed == true) ? SOIL_FLAG_COMPRESS_TO_DXT : SOIL_FLAG_ORIGINAL_TEXTURE_FORMAT );

    if( texture.GetID() == 0 )
    {
        throw NExcept::CCriticalException("Load Texture Error!",
            boost::str( boost::format("Error loading texture (%s)(%s).\n\n%s\nLine: %s")
                % stbi_failure_reason() % filePath % __FUNCTION__ % __LINE__ ));
    }

}   // LoadTexture


/************************************************************************
*    desc:  Delete a texture in a group
************************************************************************/
void CTextureMgr::DeleteTextureGroupFor2D( const std::string & group )
{
    // Free the texture group if it exists
    auto mapMapIter = m_textureFor2DMapMap.find( group );
    if( mapMapIter != m_textureFor2DMapMap.end() )
    {
        // Delete all the textures in this group
        for( auto & mapIter : mapMapIter->second )
            glDeleteTextures(1, &mapIter.second.m_id);

        // Erase this group
        m_textureFor2DMapMap.erase( mapMapIter );
    }

}   // DeleteGroupTexturesFor2D


/************************************************************************
*    desc:  Delete a texture in a group
************************************************************************/
void CTextureMgr::DeleteTextureGroupFor3D( const std::string & group )
{
    // Free the texture group if it exists
    auto mapMapIter = m_textureFor3DMapMap.find( group );
    if( mapMapIter != m_textureFor3DMapMap.end() )
    {
        // Delete all the textures in this group
        for( auto & mapIter : mapMapIter->second )
            glDeleteTextures(1, &mapIter.second.m_id);

        // Erase this group
        m_textureFor3DMapMap.erase( mapMapIter );
    }

}   // DeleteGroupTexturesFor2D


/************************************************************************
*    desc:  Function call used to manage what texture is currently bound.
*           This insures that we don't keep rebinding the same texture
************************************************************************/
void CTextureMgr::BindTexture2D( GLuint textureID )
{
    if( m_currentTextureID != textureID )
    {
        // save the current binding
        m_currentTextureID = textureID;

        // Have OpenGL bind this texture now
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

}   // BindTexture

void CTextureMgr::BindTexture3D( GLuint textureID )
{
    if( m_currentTextureID != textureID )
    {
        // save the current binding
        m_currentTextureID = textureID;

        // Have OpenGL bind this texture now
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

}   // BindTexture


/************************************************************************
*    desc:  Unbind the texture and reset the flag
************************************************************************/
void CTextureMgr::UnbindTexture()
{
    m_currentTextureID = 0;
    glBindTexture(GL_TEXTURE_2D, 0);

}   // UnbindTexture


/************************************************************************
*    desc:  Init the Anisotropic filtering
************************************************************************/
void CTextureMgr::InitAnisotropic()
{
    GLint maxAnisotropicLevel(0);
    glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropicLevel );

    if( maxAnisotropicLevel < 0 )
        maxAnisotropicLevel = 0;

    if( CSettings::Instance().GetAnisotropicLevel() <= maxAnisotropicLevel )
        m_anisotropicLevel = CSettings::Instance().GetAnisotropicLevel();
    else
        m_anisotropicLevel = maxAnisotropicLevel;

}   // InitAnisotropic
