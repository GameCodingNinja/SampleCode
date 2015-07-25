
/************************************************************************
*    FILE NAME:       scriptglobals.cpp
*
*    DESCRIPTION:     AngelScript global function registration
************************************************************************/

// Physical component dependency
#include <script/scriptglobals.h>

// Boost lib dependencies
#include <boost/format.hpp>

// AngelScript lib dependencies
#include <angelscript.h>

// Game lib dependencies
#include <utilities/highresolutiontimer.h>
#include <utilities/genfunc.h>
#include <utilities/exceptionhandling.h>
#include <managers/soundmanager.h>

namespace NScriptGlobals
{
    /************************************************************************
    *    desc:  Throw an exception for values less then 0
    ************************************************************************/
    void Throw( int value )
    {
        if( value < 0 )
            throw NExcept::CCriticalException("Error Registering Globals!",
                boost::str( boost::format("Globals could not be created.\n\n%s\nLine: %s") % __FUNCTION__ % __LINE__ ));

    }   // Throw

    /************************************************************************
    *    desc:  Suspend the script to the game loop                                                             
    ************************************************************************/
    void Suspend()
    {
        asIScriptContext *ctx = asGetActiveContext();

        // Suspend the context so the game loop can resumed
        if( ctx )
            ctx->Suspend();

    }   // Suspend

    /************************************************************************
    *    desc:  Register the global functions
    ************************************************************************/
    void Register( asIScriptEngine * pEngine )
    {
        Throw( pEngine->RegisterGlobalFunction("float GetElapsedTime()", asMETHOD(CHighResTimer, GetElapsedTime), asCALL_THISCALL_ASGLOBAL, &CHighResTimer::Instance()) );
        Throw( pEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(NGenFunc::PostDebugMsg), asCALL_CDECL) );
        Throw( pEngine->RegisterGlobalFunction("void Suspend()", asFUNCTION(Suspend), asCALL_CDECL) );
        // The DispatchEvent function has 4 parameters and because they are not defined here, they only return garbage
        // AngelScript is not allowing the other two voided pointers
        Throw( pEngine->RegisterGlobalFunction("void DispatchEvent(int type, int code = 0)", asFUNCTION(NGenFunc::DispatchEvent), asCALL_CDECL) );

        // Global sound calls
        Throw( pEngine->RegisterGlobalFunction("void PlaySound( string &in, string &in )", asMETHOD(CSoundMgr, Play), asCALL_THISCALL_ASGLOBAL, &CSoundMgr::Instance()) );
        Throw( pEngine->RegisterGlobalFunction("void PauseSound( string &in, string &in )", asMETHOD(CSoundMgr, Pause), asCALL_THISCALL_ASGLOBAL, &CSoundMgr::Instance()) );
        Throw( pEngine->RegisterGlobalFunction("void ResumeSound( string &in, string &in )", asMETHOD(CSoundMgr, Resume), asCALL_THISCALL_ASGLOBAL, &CSoundMgr::Instance()) );
        Throw( pEngine->RegisterGlobalFunction("void StopSound( string &in, string &in )", asMETHOD(CSoundMgr, Stop), asCALL_THISCALL_ASGLOBAL, &CSoundMgr::Instance()) );
    }

}   // NScriptGlobals
