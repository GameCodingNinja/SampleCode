
/************************************************************************
*    FILE NAME:       xact.cpp
*
*    DESCRIPTION:     DirectX sound class wrapper
************************************************************************/

// Physical component dependency
#include <xact/xact.h>

// DirectX lib dependencies
#include <d3dx9.h>

// Boost lib dependencies
#include <boost/format.hpp>

// Game lib dependencies
#include <utilities/exceptionhandling.h>
#include <utilities/genfunc.h>
#include <utilities/smartpointers.h>
#include <common/defs.h>

// Required namespace(s)
using namespace std;

/************************************************************************
*    desc:  Constructor                                                             
************************************************************************/
CXAct::CXAct()
     : pEngine(NULL)
{
}   // Constructor


/************************************************************************
*    desc:  Destructor                                                             
************************************************************************/
CXAct::~CXAct()
{
    // Shutdown XACT
    if( pEngine != NULL )
    {
        pEngine->ShutDown();
        pEngine->Release();
        pEngine = NULL;
    }

    // Close memory map and streaming file handles
    for( wavBankIter = spWaveBankMap.begin();
         wavBankIter != spWaveBankMap.end();
         ++wavBankIter )
    {
        if( wavBankIter->second->pMemMapBuffer != NULL )
        {
            UnmapViewOfFile( wavBankIter->second->pMemMapBuffer );
        }
        else if( wavBankIter->second->streamFileHandle != NULL )
        {
            CloseHandle( wavBankIter->second->streamFileHandle );
        }
    }

    CoUninitialize();

}   // Destructor


/************************************************************************
*    desc:  Init the xact audio system with the global settings file
*
*    param: std::string & filePath - path to the global settings file
*
*	 ret: bool - false on fail
************************************************************************/  
void CXAct::Init( const std::string & filePath )
{
    HRESULT hr;

    // COINIT_APARTMENTTHREADED will work too
    if( SUCCEEDED( hr = CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) )
        hr = XACT3CreateEngine( 0, &pEngine );

    if( FAILED( hr ) || pEngine == NULL )
        DisplayError( hr );

    bool bSuccess = false;

    // Initialize & create the XACT runtime
    XACT_RUNTIME_PARAMETERS xrParams = {0};
    xrParams.globalSettingsFlags = XACT_FLAG_GLOBAL_SETTINGS_MANAGEDATA;
    xrParams.fnNotificationCallback = XACTNotificationCallback;
    xrParams.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;

    NSmart::scoped_win_handle_ptr<WIN_HANDLE> hFile( CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) );
    if( hFile.isValid() )
    {
        xrParams.globalSettingsBufferSize = GetFileSize( hFile.get(), NULL );
        if( xrParams.globalSettingsBufferSize != INVALID_FILE_SIZE )
        {
            // Using CoTaskMemAlloc so that XACT can clean up this data when its done
            xrParams.pGlobalSettingsBuffer = CoTaskMemAlloc( xrParams.globalSettingsBufferSize );
            if( xrParams.pGlobalSettingsBuffer != NULL )
            {
                DWORD dwBytesRead;
                if( 0 != ReadFile( hFile.get(), xrParams.pGlobalSettingsBuffer, xrParams.globalSettingsBufferSize, &dwBytesRead, NULL ) )
                {
                    bSuccess = true;
                }
            }
        }
    }

    if( !bSuccess )
    {
        if( xrParams.pGlobalSettingsBuffer != NULL )
            CoTaskMemFree( xrParams.pGlobalSettingsBuffer );

        throw NExcept::CCriticalException("XACT Sound Error!",
            boost::str( boost::format("Error reading in global settings file (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));
    }

    // Initialize & create the XACT runtime
    if( FAILED( hr = pEngine->Initialize( &xrParams ) ) )
        DisplayError( hr );

    // Initialize 3D settings
    if( FAILED( hr = XACT3DInitialize( pEngine, x3DInstance ) ) )
        DisplayError( hr );

    // Init for 3D sound
    Init3DSound();

}	// Init


/************************************************************************
*    desc:  Init the class member variables for 3D sound
************************************************************************/  
void CXAct::Init3DSound()
{
    HRESULT hr;

    // Setup 3D audio structs
    ZeroMemory( &listener, sizeof( listener ) );
    listener.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    listener.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    listener.Position = D3DXVECTOR3( 0, 0, 0 );
    listener.Velocity = D3DXVECTOR3( 0, 0, 0 );

    ZeroMemory( &emitter, sizeof( emitter ) );
    emitter.pCone = NULL;
    emitter.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    emitter.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    emitter.Position = D3DXVECTOR3( 0, 0, 0 );
    emitter.Velocity = D3DXVECTOR3( 0, 0, 0 );
    emitter.ChannelCount = 2;
    emitter.ChannelRadius = 1.0f;
    emitter.pChannelAzimuths = NULL;
    emitter.pVolumeCurve = NULL;
    emitter.pLFECurve = NULL;
    emitter.pLPFDirectCurve = NULL;
    emitter.pLPFReverbCurve = NULL;
    emitter.pReverbCurve = NULL;
    emitter.CurveDistanceScaler = 1.0f;
    emitter.DopplerScaler = NULL;

    // query number of channels on the final mix
    WAVEFORMATEXTENSIBLE wfxFinalMixFormat;
    if( FAILED( hr = pEngine->GetFinalMixFormat( &wfxFinalMixFormat ) ) )
    {
        // Just show an error message. Don't kill the game over this
        DisplayError( hr );
    }

    // Init MatrixCoefficients. XACT will fill in the values
    ZeroMemory( &matrixCoefficients, sizeof( matrixCoefficients ) );

    ZeroMemory( &dspSettings, sizeof( dspSettings ) );
    ZeroMemory( &delayTimes, sizeof( delayTimes ) );
    dspSettings.pMatrixCoefficients = matrixCoefficients;
    dspSettings.pDelayTimes = delayTimes;
    dspSettings.SrcChannelCount = 2;
    dspSettings.DstChannelCount = wfxFinalMixFormat.Format.nChannels;

}	// Init3DSound


/************************************************************************
*    desc:  Load a wav bank
*
*    param: std::string & filePath - path to the wav bank file
*
*	 ret: bool - false of fail
************************************************************************/  
void CXAct::LoadWaveBank( const std::string & filePath )
{
    CWaveBankFileHeader header;
    unsigned long bytesRead;
    DWORD dwFileSize;
    HRESULT hr;
    CWaveBank * pWaveBank;

    // See if this wave bank has already been loaded
    wavBankIter = spWaveBankMap.find( filePath );

    // If it's not found, allocate the wave bank class and add it to the list
    if( wavBankIter == spWaveBankMap.end() )
    {
        pWaveBank = new CWaveBank;
        spWaveBankMap.insert( filePath, pWaveBank );
    }
    else
        throw NExcept::CCriticalException("XACT Sound Error!",
            boost::str( boost::format("Wave bank already loaded (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

    // NOTE: This is a bit of a hack because parts of the xwb file header is unknown.
    // As far as I can tell, the part that I isolated is the flag that indicates if
    // the wav bank is loaded into memory or that it is ment to be streamed.

    // Open the file on the hard drive and read in the header
    NSmart::scoped_win_handle_ptr<WIN_HANDLE> hFile( CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL ) );

    // Check to see that we have a valid file handle
    if( !hFile.isValid() )
        throw NExcept::CCriticalException("XACT Sound Error!",
            boost::str( boost::format("Can't open wave bank (%s) to read header information.\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

    // Read contents of file into memory
    BOOL readResult = ReadFile( hFile.get(), &header, sizeof(header), &bytesRead, NULL );

    if( !readResult || bytesRead != sizeof(header) )
    {
        throw NExcept::CCriticalException("XACT Sound Error!",
            boost::str( boost::format("Can't read wave bank file (%s) for header information.\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));
    }

    if( header.streaming )
    {
        HANDLE managedFileHandle = CreateFile( filePath.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                            NULL );

        if( managedFileHandle != INVALID_HANDLE_VALUE )
        {
            XACT_WAVEBANK_STREAMING_PARAMETERS wsParams;
            ZeroMemory( &wsParams, sizeof( XACT_WAVEBANK_STREAMING_PARAMETERS ) );
            wsParams.file = managedFileHandle;
            wsParams.offset = 0;

            // 64 means to allocate a 64 * 2k buffer for streaming.  
            // This is a good size for DVD streaming and takes good advantage of the read ahead cache
            wsParams.packetSize = 64;

            if( FAILED( hr = pEngine->CreateStreamingWaveBank( &wsParams, &pWaveBank->pWaveBank ) ) )
            {
                CloseHandle( managedFileHandle );
                DisplayError( hr );
            }
        }
        else
            throw NExcept::CCriticalException("XACT Sound Error!",
                boost::str( boost::format("Can't open wave bank (%s) to create streaming wav bank.\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

        // Record the file handle
        pWaveBank->streamFileHandle = managedFileHandle;
    }
    else
    {
        // assume failure
        hr = E_FAIL; 

        // Read and register the wave bank file with XACT using memory mapped file IO
        // Memory mapped files tend to be the fastest for most situations assuming you
        // have enough virtual address space for a full map of the file
        hFile.reset( CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) );
        if( hFile.isValid() )
        {
            dwFileSize = GetFileSize( hFile.get(), NULL );
            if( dwFileSize != -1 )
            {
                NSmart::scoped_win_handle_ptr<WIN_HANDLE> hMapFile( CreateFileMapping( hFile.get(), NULL, PAGE_READONLY, 0, dwFileSize, NULL ) );
                if( !hMapFile.isNull() )
                {
                    pWaveBank->pMemMapBuffer = MapViewOfFile( hMapFile.get(), FILE_MAP_READ, 0, 0, 0 );
                    if( pWaveBank->pMemMapBuffer != NULL )
                        hr = pEngine->CreateInMemoryWaveBank( pWaveBank->pMemMapBuffer, dwFileSize, 0, 0, &pWaveBank->pWaveBank );
                }
            }
        }

        if( FAILED( hr ) )
            DisplayError( hr );
    }

    // Register for XACT notification if this wave bank is to be destroyed
    XACT_NOTIFICATION_DESCRIPTION desc = {0};
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    desc.pWaveBank = pWaveBank->pWaveBank;
    desc.pvContext = this; // never forget the this pointer!

    if( FAILED( hr = pEngine->RegisterNotification( &desc ) ) )
        DisplayError( hr );

}	// LoadWavBank


/************************************************************************
*    desc:  Load a sound bank
*
*    param: std::string & filePath - path to the sound bank file
*
*	 ret: bool - false of fail
************************************************************************/  
void CXAct::LoadSoundBank( const std::string & filePath )
{
    unsigned long bytesRead;
    DWORD dwFileSize;
    HRESULT hr;
    CSoundBank * pSoundBank;

    // See if this wave bank has already been loaded
    sndBankIter = spSoundBankMap.find( filePath );

    // If it's not found, allocate the wave bank class and add it to the list
    if( sndBankIter == spSoundBankMap.end() )
    {
        pSoundBank = new CSoundBank;
        spSoundBankMap.insert( filePath, pSoundBank );
    }
    else
        throw NExcept::CCriticalException("XACT Sound Error!",
            boost::str( boost::format("Sound bank already loaded (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

    hr = E_FAIL; // assume failure
    NSmart::scoped_win_handle_ptr<WIN_HANDLE> hFile( CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ) );
    if( hFile.isValid() )
    {
        dwFileSize = GetFileSize( hFile.get(), NULL );
        if( dwFileSize != -1 )
        {
            // Allocate the data here and free the data when recieving the sound bank destroyed notification
            pSoundBank->spSoundBankBuffer.reset( new unsigned char[dwFileSize] );
            if( pSoundBank->spSoundBankBuffer.get() == NULL )
                throw NExcept::CCriticalException("XACT Sound Error!",
                    boost::str( boost::format("Sound bank buffer allocate error (%s).\n\n%s\nLine: %s") % filePath.c_str() % __FUNCTION__ % __LINE__ ));

            BOOL readResult = ReadFile( hFile.get(), pSoundBank->spSoundBankBuffer.get(), dwFileSize, &bytesRead, NULL );
            if( readResult || bytesRead == dwFileSize )
            {
                hr = pEngine->CreateSoundBank( pSoundBank->spSoundBankBuffer.get(), dwFileSize, 0, 0, &pSoundBank->pSoundBank );
            }
        }
    }

    if( FAILED( hr ) )
        DisplayError( hr );

    // Register for XACT notification if this wave bank is to be destroyed
    XACT_NOTIFICATION_DESCRIPTION desc = {0};
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED;
    desc.pSoundBank = pSoundBank->pSoundBank;
    desc.pvContext = this; // never forget the this pointer!

    if( FAILED( hr = pEngine->RegisterNotification( &desc ) ) )
        DisplayError( hr );

}	// LoadSoundBank


/************************************************************************
*    desc:  Build the sound Que map
************************************************************************/  
void CXAct::BuildSoundCueMap()
{
    soundCueMap.clear();

    for( sndBankIter = spSoundBankMap.begin();
         sndBankIter != spSoundBankMap.end();
         ++sndBankIter )
    {
        XACTINDEX cueNo;
        sndBankIter->second->pSoundBank->GetNumCues( &cueNo );

        // Add the sound Cues to the map
        for( XACTINDEX i = 0; i < cueNo; ++i )
        {
            XACT_CUE_PROPERTIES properties;
            sndBankIter->second->pSoundBank->GetCueProperties(i, &properties);

            CSoundCue soundCue;

            soundCue.cueIndex = i;
            soundCue.pSoundBank = sndBankIter->second->pSoundBank;

            soundCueMap.insert( make_pair(properties.friendlyName, soundCue) );
        }
    }

}	// BuildSoundCueMap


/************************************************************************
*    desc:  Destroy a wave bank
*
*    param: std::string & filePath - path to the wave bank file
************************************************************************/  
void CXAct::DestroyWaveBank( const std::string & filePath )
{
    // See if this wave bank has been loaded
    wavBankIter = spWaveBankMap.find( filePath );

    // Only destroy it if it has been found
    if( wavBankIter != spWaveBankMap.end() )
    {
        // an XACTNotificationCallback will be sent and there 
        // is where the clean-up will be handled
        wavBankIter->second->pWaveBank->Destroy();

        // Erase it from the map
        spWaveBankMap.erase( wavBankIter );
    }

}	// DestroyWaveBank


/************************************************************************
*    desc:  Destroy a sound bank
*
*    param: std::string & filePath - path to the wave bank file
************************************************************************/  
void CXAct::DestroySoundBank( const std::string & filePath )
{
    // See if this wave bank has been loaded
    sndBankIter = spSoundBankMap.find( filePath );

    // Only destroy it if it has been found
    if( sndBankIter != spSoundBankMap.end() )
    {
        // an XACTNotificationCallback will be sent and there 
        // is where the clean-up will be handled
        sndBankIter->second->pSoundBank->Destroy();

        // Erase it from the map
        spSoundBankMap.erase( sndBankIter );
    }

}	// DestroyWaveBank


/************************************************************************
*    desc:  Position the sound cue based on the point
*
*    param: std::string & sndCueStr - sound cue string
*
*    ret: CSoundCue - filled in sound cue class
************************************************************************/ 
void CXAct::PositionCue( const CPoint & point, IXACT3Cue * pCue )
{
    listener.Position = D3DXVECTOR3( point.x, point.y, point.z );

    XACT3DCalculate( x3DInstance, &listener, &emitter, &dspSettings );
    XACT3DApply( &dspSettings, pCue );

}	// PositionCue


/************************************************************************
*    desc:  This is the callback for handling XACT notifications
*
*    param: const XACT_NOTIFICATION* pNotification - notification pointer
*
*	 ret: WINAPI
************************************************************************/ 
void WINAPI CXAct::XACTNotificationCallback( const XACT_NOTIFICATION * pNotification )
{
    if( (pNotification != NULL) && (pNotification->pvContext != NULL) )
    {
        // Cast our pointer
        // Be sure to set pNotification->pvContext to the "this" pointer
        CXAct * pXAct = NGenFunc::ReintCast<CXAct *>(pNotification->pvContext);

        pXAct->HandleNotification( pNotification );
    }

}	// XACTNotificationCallback


/************************************************************************
*    desc:  Function for handling XACT notifications
*
*    param: const XACT_NOTIFICATION* pNotification - notification pointer
************************************************************************/ 
void CXAct::HandleNotification( const XACT_NOTIFICATION * pNotification )
{
    if( pNotification->type == XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED )
    {
        // Close memory map and streaming file handles
        for( wavBankIter = spWaveBankMap.begin();
             wavBankIter != spWaveBankMap.end();
             ++wavBankIter )
        {
            // Find the wave bank we want to delete
            if( wavBankIter->second->pWaveBank == pNotification->waveBank.pWaveBank )
            {
                if( wavBankIter->second->pMemMapBuffer != NULL )
                {
                    UnmapViewOfFile( wavBankIter->second->pMemMapBuffer );
                }
                else if( wavBankIter->second->streamFileHandle != NULL )
                {
                    CloseHandle( wavBankIter->second->streamFileHandle );
                }

                NGenFunc::PostDebugMsg( "Wave Bank Deleted: %s", wavBankIter->first.c_str() );
                spWaveBankMap.erase( wavBankIter );
                break;
            }
        }
    }
    else if( pNotification->type == XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED )
    {
        for( sndBankIter = spSoundBankMap.begin();
             sndBankIter != spSoundBankMap.end();
             ++sndBankIter )
        {
            // Find the sound bank we want to delete
            if( sndBankIter->second->pSoundBank == pNotification->soundBank.pSoundBank )
            {
                NGenFunc::PostDebugMsg( "Sound Bank Deleted: %s", sndBankIter->first.c_str() );
                spSoundBankMap.erase( sndBankIter );
                break;
            }
        }
    }
}	// HandleNotification


/************************************************************************
*    desc:  Allows XACT to do required periodic work. call within game loop
************************************************************************/ 
void CXAct::Update()
{
    if( pEngine != NULL )
        pEngine->DoWork();

}	// Update


/************************************************************************
*    desc:  Get a sound cue
*
*    param: std::string & sndCueStr - sound cue string
*
*    ret: CSoundCue - filled in sound cue class
************************************************************************/ 
CSoundCue * CXAct::GetSoundCue( const std::string & sndCueStr )
{
    // Find the sound que in the map
    soundCueIter = soundCueMap.find( sndCueStr );

    // Only destroy it if it has been found
    if( soundCueIter != soundCueMap.end() )
        return &soundCueIter->second;

    return NULL;

}	// GetSoundCue


/************************************************************************
*    desc:  Play a sound - Good for "Fire & Forget" but use GetSoundCue
*           for more options.
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
void CXAct::Play( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            pTmpCue->Play();
    }

}	// Play


/************************************************************************
*    desc:  Stop a sound - Good for "Fire & Forget" but use GetSoundCue
*           for more options.
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
void CXAct::Stop( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            pTmpCue->Stop();
    }

}	// Stop


/************************************************************************
*    desc:  Prepare a sound - Good for "Fire & Forget" but use GetSoundCue
*           for more options.
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
void CXAct::Prepare( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            pTmpCue->Prepare();
    }

}	// Stop


/************************************************************************
*    desc:  Check if a cue is busy
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
bool CXAct::IsBusy( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            return pTmpCue->IsBusy();
    }

    return false;

}	// IsBusy


/************************************************************************
*    desc:  Check if a cue is stopped
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
bool CXAct::IsStopped( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            return pTmpCue->IsStopped();
    }

    return false;

}	// IsBusy


/************************************************************************
*    desc:  Check if a cue is paused
*
*    param: std::string & - string ID of sound cue. Name entered in XACT
*                           tool.
************************************************************************/ 
bool CXAct::IsPaused( const std::string & sndCueStr )
{
    if( pEngine != NULL )
    {
        CSoundCue * pTmpCue = GetSoundCue( sndCueStr );

        if( pTmpCue != NULL )
            return pTmpCue->IsPaused();
    }

    return false;

}	// IsBusy


/************************************************************************
*    desc:  Pause the play of a catagory
************************************************************************/ 
void CXAct::Pause( const std::string & catagoryStr, bool paused )
{
    if( pEngine != NULL )
    {
        XACTCATEGORY category = pEngine->GetCategory( catagoryStr.c_str() );

        pEngine->Pause( category, paused );
    }

}	// Pause


/************************************************************************
*    desc:  Set the volume level of a catagory
************************************************************************/ 
void CXAct::SetVolumeLevel( const std::string & catagoryStr, float level )
{
    if( pEngine != NULL )
    {
        XACTCATEGORY category = pEngine->GetCategory( catagoryStr.c_str() );

        pEngine->SetVolume( category, level );
    }

}	// SetVolumeLevel


/************************************************************************
*    desc:  Display error information
*
*    param: HRESULT hr - return result from function call
************************************************************************/
void CXAct::DisplayError( HRESULT hr )
{
    switch( hr )
    {
        case XACTENGINE_E_OUTOFMEMORY:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "XACT engine out of memory.");
            break;
        }

        case XACTENGINE_E_INVALIDARG:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid arguments.");
            break;
        }

        case XACTENGINE_E_NOTIMPL:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Feature not implemented.");
            break;
        }

        case XACTENGINE_E_ALREADYINITIALIZED:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "XACT engine is already initialized.");
            break;
        }

        case XACTENGINE_E_NOTINITIALIZED:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "XACT engine has not been initialized.");
            break;
        }		

        case XACTENGINE_E_EXPIRED:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "XACT engine has expired (demo or pre-release version).");
            break;
        }

        case XACTENGINE_E_NONOTIFICATIONCALLBACK:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No notification callback.");
            break;
        }

        case XACTENGINE_E_NOTIFICATIONREGISTERED:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No notification callback.");
            break;
        }

        case XACTENGINE_E_INVALIDUSAGE:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No notification callback.");
            break;
        }

        case XACTENGINE_E_INVALIDDATA:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No notification callback.");
            break;
        }

        case XACTENGINE_E_INSTANCELIMITFAILTOPLAY:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Fail to play due to instance limit.");
            break;
        }

        case XACTENGINE_E_NOGLOBALSETTINGS:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Global Settings not loaded.");
            break;
        }

        case XACTENGINE_E_INVALIDVARIABLEINDEX:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid variable index.");
            break;
        }

        case XACTENGINE_E_INVALIDCATEGORY:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid category.");
            break;
        }

        case XACTENGINE_E_INVALIDCUEINDEX:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid cue index.");
            break;
        }

        case XACTENGINE_E_INVALIDWAVEINDEX:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid wave index.");
            break;
        }

        case XACTENGINE_E_INVALIDTRACKINDEX:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid track index.");
            break;
        }

        case XACTENGINE_E_INVALIDSOUNDOFFSETORINDEX:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid sound offset or index.");
            break;
        }

        case XACTENGINE_E_READFILE:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Error reading a file.");
            break;
        }

        case XACTENGINE_E_UNKNOWNEVENT:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Unknown event type.");
            break;
        }

        case XACTENGINE_E_INCALLBACK:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid call of method of function from callback.");
            break;
        }

        case XACTENGINE_E_NOWAVEBANK:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No wavebank exists for desired operation.");
            break;
        }

        case XACTENGINE_E_SELECTVARIATION:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Unable to select a variation.");
            break;
        }

        case XACTENGINE_E_MULTIPLEAUDITIONENGINES:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "There can be only one audition engine.");
            break;
        }

        case XACTENGINE_E_WAVEBANKNOTPREPARED:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "The wavebank is not prepared.");
            break;
        }

        case XACTENGINE_E_NORENDERER:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "No audio device found on.");
            break;
        }

        case XACTENGINE_E_INVALIDENTRYCOUNT:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Invalid entry count for channel maps.");
            break;
        }

        case XACTENGINE_E_SEEKTIMEBEYONDCUEEND:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Time offset for seeking is beyond the cue end.");
            break;
        }

        case XACTENGINE_E_SEEKTIMEBEYONDWAVEEND:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Time offset for seeking is beyond the wave end.");
            break;
        }

        case XACTENGINE_E_NOFRIENDLYNAMES:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Friendly names are not included in the bank.");
            break;
        }

        default:
        {
            throw NExcept::CCriticalException("XACT Sound Error!", "Unknown error.");
            break;
        }
    }
}	// DisplayError
