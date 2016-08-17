
/************************************************************************
*    FILE NAME:       scriptmanager.cpp
*
*    DESCRIPTION:     Class to manage AngelScripts
************************************************************************/

// Physical component dependency
#include <script/scriptmanager.h>

// Game lib dependencies
#include <utilities/xmlParser.h>
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <utilities/statcounter.h>

// AngelScript lib dependencies
#include <angelscript.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Standard lib dependencies
#include <cstring>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CScriptManager::CScriptManager()
{
    // Create the script engine
    scpEngine.reset( asCreateScriptEngine(ANGELSCRIPT_VERSION) );
    if( scpEngine.isNull() )
    {
        throw NExcept::CCriticalException("Error Creating AngelScript Engine!",
            boost::str( boost::format("AngelScript engine could not be created.\n\n%s\nLine: %s")
                % __FUNCTION__ % __LINE__ ));
    }

    // We don't use call backs for every line execution so set this property to not build with line cues
    scpEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);

    // Set the message callback to print the messages that the engine gives in case of errors
    if( scpEngine->SetMessageCallback(asMETHOD(CScriptManager, MessageCallback), this, asCALL_THISCALL) < 0 )
    throw NExcept::CCriticalException("Error Creating AngelScript Engine!",
        boost::str( boost::format("AngelScript message callback could not be created.\n\n%s\nLine: %s")
            % __FUNCTION__ % __LINE__ ));

}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CScriptManager::~CScriptManager()
{
    // Release the context pool
    for( auto iter : m_contextPoolVec )
        iter->Release();

}   // destructer


/************************************************************************
*    desc:  Load all of the scripts of a specific group
*
*    param: string & group - specified group of scripts to load
************************************************************************/
void CScriptManager::LoadGroup( const std::string & group )
{
    // Make sure the group we are looking has been defined in the list table file
    auto listTableIter = m_listTableMap.find( group );
    if( listTableIter == m_listTableMap.end() )
        throw NExcept::CCriticalException("Script List Load Group Data Error!",
            boost::str( boost::format("Script list group name can't be found (%s).\n\n%s\nLine: %s") 
                % group % __FUNCTION__ % __LINE__ ));

    // Create the module if it doesn't already exist
    asIScriptModule * pScriptModule = scpEngine->GetModule(group.c_str(), asGM_CREATE_IF_NOT_EXISTS);
    if( pScriptModule == nullptr )
    {
        throw NExcept::CCriticalException("Script List load Error!",
            boost::str( boost::format("Error creating script group module (%s).\n\n%s\nLine: %s")
                % group % __FUNCTION__ % __LINE__ ));
    }

    // Add the scripts to the module
    for( auto & iter : listTableIter->second )
        AddScript( pScriptModule, iter );

    // Build all the scripts added to the module
    BuildScript( pScriptModule, group );

}   // LoadGroup


/************************************************************************
*    desc:  Add the script to the module
************************************************************************/
void CScriptManager::AddScript( asIScriptModule * pScriptModule, const std::string & filePath )
{
    // Load the script file into a charater array
    std::shared_ptr<char> spChar = NGenFunc::FileToBuf( filePath );

    // Load script into module section - the file path is it's ID
    if( pScriptModule->AddScriptSection(filePath.c_str(), spChar.get() ) < 0 ) // std::strlen( spChar.get() )
    {
        throw NExcept::CCriticalException("Script List load Error!",
            boost::str( boost::format("Error loading script (%s).\n\n%s\nLine: %s")
                % filePath % __FUNCTION__ % __LINE__ ));
    }

}   // AddScript


/************************************************************************
*    desc:  Build all the scripts added to the module
************************************************************************/
void CScriptManager::BuildScript( asIScriptModule * pScriptModule, const std::string & group )
{
    // Build the script
    int error = pScriptModule->Build();
    if( error < 0 )
    {
        throw NExcept::CCriticalException("Script List build Error!",
            boost::str( boost::format("Error building script group with error code %d. (%s).\n\n%s\nLine: %s")
                % error % group % __FUNCTION__ % __LINE__ ));
    }

}   // BuildScript


/************************************************************************
*    desc:  Get the script context from a managed pool
*
*    NOTE: The receiver of this pointer is the owner if it's still 
*          holding on to it when the game terminates
************************************************************************/
asIScriptContext * CScriptManager::GetContext()
{
    if( !m_contextPoolVec.empty() )
    {
        asIScriptContext * pContex = m_contextPoolVec.back();
        m_contextPoolVec.pop_back();

        return pContex;
    }
    
    // Maintain a total count of contexts
    CStatCounter::Instance().IncScriptContexCounter();

    return scpEngine->CreateContext();

}   // GetContext


/************************************************************************
*    desc:  Add the script context back to the managed pool
************************************************************************/
void CScriptManager::RecycleContext( asIScriptContext * pContext )
{
    m_contextPoolVec.push_back( pContext );

}   // RecycleContext


/************************************************************************
*    desc:  Get pointer to function name
************************************************************************/
asIScriptFunction * CScriptManager::GetPtrToFunc( const std::string & group, const std::string & name )
{
    // Create the map group if it doesn't already exist
    auto mapMapIter = m_scriptFunctMapMap.find( group );
    if( mapMapIter == m_scriptFunctMapMap.end() )
        mapMapIter = m_scriptFunctMapMap.emplace( group, std::map<const std::string, asIScriptFunction *>() ).first;

    // See if this function pointer has already been saved
    auto mapIter = mapMapIter->second.find( name );

    // If it's not found, find the function and add it to the map
    // It's faster to keep the function pointers in a map then to use AngelScript's GetFunction call.
    if( mapIter == mapMapIter->second.end() )
    {
        asIScriptModule * pScriptModule = scpEngine->GetModule(group.c_str(), asGM_ONLY_IF_EXISTS);
        if( pScriptModule == nullptr )
        {
            throw NExcept::CCriticalException("Error Getting Pointer to Function!",
                boost::str( boost::format("AngelScript module does not exist (%s, %s).\n\n%s\nLine: %s")
                    % group % name % __FUNCTION__ % __LINE__ ));
        }

        // Try to get the pointer to the function
        asIScriptFunction *pScriptFunc = pScriptModule->GetFunctionByName(name.c_str());
        if( pScriptFunc == nullptr )
        {
            throw NExcept::CCriticalException("Error Getting Pointer to Function!",
                boost::str( boost::format("AngelScript function does not exist (%s, %s).\n\n%s\nLine: %s")
                    % group % name % __FUNCTION__ % __LINE__ ));
        }

        // Insert the function pointer into the map
        mapIter = mapMapIter->second.emplace( name, pScriptFunc ).first;
    }

    return mapIter->second;

}   // GetPtrToFunc


/************************************************************************
*    desc:  Call back to display AngelScript messages                                                             
************************************************************************/
void CScriptManager::MessageCallback(const asSMessageInfo & msg)
{
    std::string type = "ERROR";
    if( msg.type == asMSGTYPE_WARNING ) 
        type = "WARN";

    else if( msg.type == asMSGTYPE_INFORMATION ) 
        type = "INFO";

    std::string msgStr = boost::str( boost::format( "%s (%d, %d) : %s : %s" ) % msg.section % msg.row % msg.col % type % msg.message );

    NGenFunc::PostDebugMsg( msgStr );

}   // MessageCallback


/************************************************************************
*    desc:  Get the pointer to the script engine
************************************************************************/
asIScriptEngine * CScriptManager::GetEnginePtr()
{
    return scpEngine.get();

}   // GetEnginePtr


/************************************************************************
*    desc:  Free all of the scripts of a specific data group
************************************************************************/
void CScriptManager::FreeGroup( const std::string & group )
{
    // Make sure the group we are looking for exists
    auto listTableIter = m_listTableMap.find( group );
    if( listTableIter == m_listTableMap.end() )
        throw NExcept::CCriticalException("Script List Group Error!",
            boost::str( boost::format("Script group can't be found (%s).\n\n%s\nLine: %s")
                % group % __FUNCTION__ % __LINE__ ));

    // Discard the module and free its memory.
    scpEngine->DiscardModule( group.c_str() );

    // Erase the group from the map
    auto mapMapIter = m_scriptFunctMapMap.find( group );
    if( mapMapIter != m_scriptFunctMapMap.end() )
            m_scriptFunctMapMap.erase( mapMapIter );

}   // FreeGroup
