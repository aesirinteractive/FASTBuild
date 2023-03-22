// WorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildWorker/FBuildWorkerOptions.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Other
//------------------------------------------------------------------------------
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 5 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 5 ) // Current version

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
    : m_Mode( PROPORTIONAL )
    , m_IdleThresholdPercent( 20 )
    , m_NumCPUsToUse( 1 )
    , m_LimitCPUMemoryBased ( true )
    , m_StartMinimized( false )
    , m_SettingsWriteTime( 0 )
    , m_MinimumFreeMemoryMiB( 16384 ) // 16 GiB
{
    // half CPUs available to use by default
    const uint32_t numCPUs = Env::GetNumProcessors();
    const uint32_t halfCPUs = Math::Max<uint32_t>(1, numCPUs / 2);

    Load();
    if (m_LimitCPUMemoryBased) {
        const int32_t cpuAllocationBasedOnMemory = FBuildWorkerOptions::GetCPUAllocationBasedOnMemory();
        if (cpuAllocationBasedOnMemory != -1) {
          m_NumCPUsToUse = (uint32_t)cpuAllocationBasedOnMemory;
        }
    } else {
        m_NumCPUsToUse = halfCPUs;
    }

    // handle CPU downgrade
    m_NumCPUsToUse = Math::Min( Env::GetNumProcessors(), m_NumCPUsToUse );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::~WorkerSettings() = default;

// SetMode
//------------------------------------------------------------------------------
void WorkerSettings::SetMode( Mode m )
{
    m_Mode = m;
}

// SetIdleThresholdPercent
//------------------------------------------------------------------------------
void WorkerSettings::SetIdleThresholdPercent( uint32_t p )
{
    m_IdleThresholdPercent = p;
}

// SetNumCPUsToUse
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUse( uint32_t c )
{
    m_NumCPUsToUse = c;
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettings::SetStartMinimized( bool startMinimized )
{
    m_StartMinimized = startMinimized;
}

// SetMinimumFreeMemoryMiB
//------------------------------------------------------------------------------
void WorkerSettings::SetMinimumFreeMemoryMiB( uint32_t value )
{
    m_MinimumFreeMemoryMiB = value;
}

// SetMinimumFreeMemoryMiB
//------------------------------------------------------------------------------
void WorkerSettings::SetLimitCPUMemoryBased( bool value )
{
    m_LimitCPUMemoryBased = value;
}

// Load
//------------------------------------------------------------------------------
void WorkerSettings::Load()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += ".settings";

    FileStream f;
    if ( f.Open( settingsPath.Get(), FileStream::READ_ONLY ) )
    {
        uint8_t header[ 4 ] = { 0 };
        f.Read( &header, 4 );

        const uint8_t settingsVersion = header[ 3 ];
        if ( ( settingsVersion < FBUILDWORKER_SETTINGS_MIN_VERSION ) ||
             ( settingsVersion > FBUILDWORKER_SETTINGS_CURRENT_VERSION ) )
        {
            return; // version is too old, or newer, and cannot be read
        }

        // settings
        uint32_t mode;
        f.Read( mode );
        m_Mode = (Mode)mode;
        if ( header[ 3 ] >= 4 )
        {
            f.Read( m_IdleThresholdPercent );
        }
        f.Read( m_NumCPUsToUse );
        f.Read( m_StartMinimized );
        f.Read( m_LimitCPUMemoryBased );

        f.Close();

        m_SettingsWriteTime = FileIO::GetFileLastWriteTime( settingsPath );
    }
}

// Save
//------------------------------------------------------------------------------
void WorkerSettings::Save()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += ".settings";

    FileStream f;
    if ( f.Open( settingsPath.Get(), FileStream::WRITE_ONLY ) )
    {
        bool ok = true;

        // header
        ok &= ( f.Write( "FWS", 3 ) == 3 );
        ok &= ( f.Write( uint8_t( FBUILDWORKER_SETTINGS_CURRENT_VERSION ) ) );

        // settings
        ok &= f.Write( (uint32_t)m_Mode );
        ok &= f.Write( m_IdleThresholdPercent );
        ok &= f.Write( m_NumCPUsToUse );
        ok &= f.Write( m_StartMinimized );
        ok &= f.Write( m_LimitCPUMemoryBased );

        f.Close();

        if ( ok )
        {
            m_SettingsWriteTime = FileIO::GetFileLastWriteTime( settingsPath );
            return;
        }
    }

    Env::ShowMsgBox( "FBuildWorker", "Failed to save settings." );
}

//------------------------------------------------------------------------------
