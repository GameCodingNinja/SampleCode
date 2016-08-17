
/************************************************************************
*    FILE NAME:       scriptcomponent.cpp
*
*    DESCRIPTION:     Class for handling the scripting
************************************************************************/

// Physical component dependency
#include <script/scriptcomponent.h>

// Game lib dependencies
#include <script/scriptmanager.h>
#include <utilities/exceptionhandling.h>
#include <utilities/statcounter.h>

// AngelScript lib dependencies
#include <angelscript.h>

// Boost lib dependencies
#include <boost/format.hpp>

/************************************************************************
*    desc:  Constructer
************************************************************************/
CScriptComponent::CScriptComponent( const std::string & group ) :
    m_group(group)
{
}   // constructor


/************************************************************************
*    desc:  destructer                                                             
************************************************************************/
CScriptComponent::~CScriptComponent()
{
    // Release the contextes we are still holding on to
    for( auto iter : m_pContextVec )
        iter->Release();

}   // destructer


/************************************************************************
*    desc:  Prepare the script function to run
************************************************************************/
void CScriptComponent::Prepare(
    const std::string & name,
    const std::vector<CScriptParam> & paramVec )
{
    // Get a context from the script manager pool
    m_pContextVec.push_back( CScriptManager::Instance().GetContext() );

    // Get the function pointer
    asIScriptFunction * pScriptFunc = 
        CScriptManager::Instance().GetPtrToFunc(m_group, name);

    // Prepare the function to run
    if( m_pContextVec.back()->Prepare(pScriptFunc) < 0 )
    {
        throw NExcept::CCriticalException("Error Preparing Script!",
            boost::str( boost::format("There was an error preparing the script (%s).\n\n%s\nLine: %s")
                % name % __FUNCTION__ % __LINE__ ));
    }
    
    // Pass the parameters to the script function
    for( size_t i = 0; i < paramVec.size(); ++i )
    {
        int returnVal(0);
        
        if( paramVec[i].GetType() == CScriptParam::EPT_BOOL )
        {
            returnVal = GetContext()->SetArgByte(i, paramVec[i].Get<bool>());
        }
        else if( paramVec[i].GetType() == CScriptParam::EPT_INT )
        {
            returnVal = GetContext()->SetArgDWord(i, paramVec[i].Get<int>());
        }
        else if( paramVec[i].GetType() == CScriptParam::EPT_UINT )
        {
            returnVal = GetContext()->SetArgDWord(i, paramVec[i].Get<uint>());
        }
        else if( paramVec[i].GetType() == CScriptParam::EPT_FLOAT )
        {
            returnVal = GetContext()->SetArgFloat(i, paramVec[i].Get<float>());
        }
        else if( paramVec[i].GetType() == CScriptParam::EPT_REG_OBJ )
        {
            returnVal = GetContext()->SetArgObject(i, paramVec[i].Get<void *>());
        }
        
        if( returnVal < 0 )
        {
            throw NExcept::CCriticalException("Error Setting Script Param!",
                boost::str( boost::format("There was an error setting the script parameter (%s).\n\n%s\nLine: %s")
                    % name % __FUNCTION__ % __LINE__ ));
        }
    }

}   // Prepare


/************************************************************************
*    desc:  Update the script
************************************************************************/
void CScriptComponent::Update()
{
    if( IsActive() )
    {
        auto iter = m_pContextVec.begin();
        while( iter != m_pContextVec.end() )
        {
            // See if this context is still being used
            if( ((*iter)->GetState() == asEXECUTION_SUSPENDED) || 
                ((*iter)->GetState() == asEXECUTION_PREPARED) )
            {
                // Increment the active script contex counter
                CStatCounter::Instance().IncActiveScriptContexCounter();

                // Execute the script and check for errors
                // Since the script can be suspended, this also is used to continue execution
                if( (*iter)->Execute() < 0 )
                {
                    throw NExcept::CCriticalException("Error Calling Script!",
                        boost::str( boost::format("There was an error executing the script.\n\n%s\nLine: %s")
                            % __FUNCTION__ % __LINE__ ));
                }

                // Return the context to the pool if it has not been suspended
                if( (*iter)->GetState() != asEXECUTION_SUSPENDED )
                {
                    CScriptManager::Instance().RecycleContext( (*iter) );
                    iter = m_pContextVec.erase( iter );
                }
                else
                {
                    ++iter;
                }
            }
        }
    }

}   // Update


/************************************************************************
*    desc:  Get the context
************************************************************************/
asIScriptContext * CScriptComponent::GetContext()
{
    return m_pContextVec.back();

}   // GetContext


/************************************************************************
*    desc:  Is this component active?
************************************************************************/
bool CScriptComponent::IsActive()
{
    return !m_pContextVec.empty();

}   // IsActive


/************************************************************************
*    desc:  Reset the contexts and recycle
************************************************************************/
void CScriptComponent::ResetAndRecycle()
{
    if( IsActive() )
    {
        for( auto iter : m_pContextVec )
        {
            if( iter->GetState() == asEXECUTION_SUSPENDED )
                iter->Abort();

            CScriptManager::Instance().RecycleContext( iter );
        }

        m_pContextVec.clear();
    }

}   // ResetAndRecycle
