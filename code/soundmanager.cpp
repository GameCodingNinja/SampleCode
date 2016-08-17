
/************************************************************************
*    FILE NAME:       soundmanager.cpp
*
*    DESCRIPTION:     Class template
************************************************************************/

// Physical component dependency
#include <managers/soundmanager.h>

// Game lib dependencies
#include <utilities/xmlParser.h>
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <utilities/settings.h>
#include <common/defs.h>

// SDL lib dependencies
#include <SDL_mixer.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Standard lib dependencies
#include <cstring>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CSoundMgr::CSoundMgr() :
    m_mixChannel(0),
    m_maxMixChannels(MIX_CHANNELS),
    m_dummySnd(CSound::EST_NULL)
{
    // Init for the OGG compressed file format
    Mix_Init(MIX_INIT_OGG);

    // Setup the audio format
    // High frenquency plus low chunk size = low latency audio playback
    Mix_OpenAudio(
        CSettings::Instance().GetFrequency(),     // Usually 22050 or 44100
        MIX_DEFAULT_FORMAT,
        CSettings::Instance().GetSoundChannels(), // mono, stero, quad, suround, etc
        CSettings::Instance().GetChunkSize() );
            
    if( CSettings::Instance().GetMixChannels() != m_maxMixChannels )
        m_maxMixChannels = Mix_AllocateChannels( CSettings::Instance().GetMixChannels() );
    
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CSoundMgr::~CSoundMgr()
{
    // Free all sounds in all groups
    for( auto & mapMapIter : m_soundMapMap )
    {
        for( auto & mapIter : mapMapIter.second )
        {
            mapIter.second.Free();
        }
    }

}	// destructer


/************************************************************************
*    desc:  Load all of the sounds of a specific group
*
*    param: string & group - specified group of scripts to load
************************************************************************/
void CSoundMgr::LoadGroup( const std::string & group )
{
    // Make sure the group we are looking has been defined in the list table file
    auto listTableIter = m_listTableMap.find( group );
    if( listTableIter == m_listTableMap.end() )
        throw NExcept::CCriticalException("Sound List Load Group Data Error!",
            boost::str( boost::format("Sound list group name can't be found (%s).\n\n%s\nLine: %s") 
                % group % __FUNCTION__ % __LINE__ ));

    // Load the group data if it doesn't already exist
    if( m_soundMapMap.find( group ) == m_soundMapMap.end() )
    {
        for( auto & iter : listTableIter->second )
            LoadFromXML( group, iter );
    }
    else
    {
        throw NExcept::CCriticalException("Sound Data List 2D load Error!",
            boost::str( boost::format("Sound data list group has alread been loaded (%s).\n\n%s\nLine: %s")
                % group % __FUNCTION__ % __LINE__ ));
    }

}   // LoadGroup


/************************************************************************
*    desc:  Load all object information from an xml
*
*	 param:	string & filePath - file path of the object data list XML
************************************************************************/
void CSoundMgr::LoadFromXML( const std::string & group, const std::string & filePath )
{
    // Open and parse the XML file:
    const XMLNode mainNode = XMLNode::openFileHelper( filePath.c_str(), "soundList" );

    // Create a new map inside of our map and get an iterator into it
    auto soundMapIter = m_soundMapMap.emplace( group, std::map<const std::string, CSound>() ).first;

    // Get the node to the sound files
    const XMLNode soundFilesNode = mainNode.getChildNode( "soundFiles" );

    const CSound::ESoundType typeEnumArry[] = { CSound::EST_LOADED, CSound::EST_STREAM };
    const char typeCharArry[CSound::EST_MAX][10] = { "load", "stream" };

    for( uint i = 0; i < CSound::EST_MAX; ++i )
    {
        // Get the number of load children
        int count = soundFilesNode.nChildNode( typeCharArry[i] );

        for( int j = 0; j < count; ++j )
        {
            // Get the object data node
            const XMLNode loadNode = soundFilesNode.getChildNode(typeCharArry[i], j);

            // Get the id
            const std::string id = loadNode.getAttribute( "id" );

            // Add the sound data to the map
            auto iter = soundMapIter->second.emplace( id, typeEnumArry[i] );

            // Check for duplicate id names
            if( !iter.second )
            {
                throw NExcept::CCriticalException("Sound Data Load Group Error!",
                    boost::str( boost::format("Duplicate sound ID (%s - %s).\n\n%s\nLine: %s")
                        % id % group % __FUNCTION__ % __LINE__ ));
            }

            // Now try to load the sound
            iter.first->second.Load( loadNode.getAttribute( "file" ) );
        }
    }

    // Get the node to the play lists
    const XMLNode playListSetNode = mainNode.getChildNode( "playlistSet" );

    // Create a new map inside of our map and get an iterator into it
    auto playListMapIter = m_playListMapMap.emplace( group, std::map<const std::string, CPlayList>() ).first;

    for( int i = 0; i < playListSetNode.nChildNode(); ++i )
    {
        // Get the play list node
        const XMLNode playListNode = playListSetNode.getChildNode(i);

        // Get the id
        const std::string id = playListNode.getAttribute( "id" );

        // Get the type
        CPlayList::EPlayListType type;

        if( std::strcmp(playListNode.getAttribute( "playtype" ), "random") == 0 )
            type = CPlayList::EST_RANDOM;

        else if( std::strcmp(playListNode.getAttribute( "playtype" ), "sequential") == 0 )
            type = CPlayList::EST_SEQUENTIAL;

        // Add the playlist data to the map
        auto iter = playListMapIter->second.emplace( id, type );

        // Check for duplicate names
        if( !iter.second )
        {
            throw NExcept::CCriticalException("Playlist Data Group Load Error!",
                boost::str( boost::format("Duplicate playlist name! (%s - %s).\n\n%s\nLine: %s")
                    % id % group % __FUNCTION__ % __LINE__ ));
        }

        for( int j = 0; j < playListNode.nChildNode(); ++j )
        {
            // Get the play list node
            const XMLNode soundIdNode = playListNode.getChildNode(j);

            // Get the id
            const std::string id = soundIdNode.getAttribute( "id" );

            // Set the sound to the playlist
            // NOTE: The playlist does not own this sound
            auto soundIter = soundMapIter->second.find( id );
            if( soundIter != soundMapIter->second.end() )
            {
                iter.first->second.SetSound( soundIter->second );
            }
            else
            {
                throw NExcept::CCriticalException("Playlist Data Group Load Error!",
                    boost::str( boost::format("Playlist sound Id does not exist! (%s - %s).\n\n%s\nLine: %s")
                        % id % group % __FUNCTION__ % __LINE__ ));
            }
        }
    }

}   // LoadFromXML


/************************************************************************
*    desc:  Delete a sound group
************************************************************************/
void CSoundMgr::DeleteSoundGroup( const std::string & group )
{
    // Free the sound group if it exists
    auto mapMapIter = m_soundMapMap.find( group );
    if( mapMapIter != m_soundMapMap.end() )
    {
        // Delete all the textures in this group
        for( auto & mapIter : mapMapIter->second )
        {
            mapIter.second.Free();
        }

        // Erase this group
        m_soundMapMap.erase( mapMapIter );
    }

}   // DeleteSoundGroup


/************************************************************************
*    desc:  Get the sound
************************************************************************/
CSound & CSoundMgr::GetSound( const std::string & group, const std::string & soundID )
{
    // Check if this is a playlist sound ID
    auto playListMapIter = m_playListMapMap.find( group );
    if( playListMapIter != m_playListMapMap.end() )
    {
        auto iter = playListMapIter->second.find( soundID );
        if( iter != playListMapIter->second.end() )
            return iter->second.GetSound();
    }

    auto soundMapIter = m_soundMapMap.find( group );
    if( soundMapIter == m_soundMapMap.end() )
    {
        NGenFunc::PostDebugMsg( boost::str( boost::format("Sound group can't be found (%s).") % group ) );
        return m_dummySnd;
    }

    auto iter = soundMapIter->second.find( soundID );
    if( iter == soundMapIter->second.end() )
    {
        NGenFunc::PostDebugMsg( boost::str( boost::format("Sound ID can't be found (%s - %s).") % group % soundID ) );
        return m_dummySnd;
    }

    return iter->second;

}   // GetSound


/************************************************************************
*    desc:  Play a sound
************************************************************************/
void CSoundMgr::Play( const std::string & group, const std::string & soundID )
{
    GetSound( group, soundID ).Play( 0, m_mixChannel );
    m_mixChannel = (m_mixChannel + 1) % m_maxMixChannels;

}   // Play


/************************************************************************
*    desc:  Pause a sound
************************************************************************/
void CSoundMgr::Pause( const std::string & group, const std::string & soundID )
{
    GetSound( group, soundID ).Pause();

}   // Pause


/************************************************************************
*    desc:  Resume a sound
************************************************************************/
void CSoundMgr::Resume( const std::string & group, const std::string & soundID )
{
    GetSound( group, soundID ).Resume();

}   // Resume


/************************************************************************
*    desc:  Resume a sound
************************************************************************/
void CSoundMgr::Stop( const std::string & group, const std::string & soundID )
{
    GetSound( group, soundID ).Stop();

}   // Resume

