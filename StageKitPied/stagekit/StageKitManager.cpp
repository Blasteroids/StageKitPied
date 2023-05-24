
#include "StageKitManager.h"

StageKitManager::StageKitManager() {
  m_usb_context             = NULL;
  m_amount_of_stagekits     = 0;
  m_fog_current_state_is_on = false;
  m_fog_just_changed_to_off = false;
  for( uint8_t config_id = 0; config_id < 5; config_id++ ) {
    m_stagekit_config[ config_id ].m_light_pod_enabled        = false;
    m_stagekit_config[ config_id ].m_strobe_enabled           = false;
    m_stagekit_config[ config_id ].m_fog_enabled              = false;
    m_stagekit_config[ config_id ].m_fog_instance_time_max_ms = 0;
    m_stagekit_config[ config_id ].m_fog_total_time_max_ms    = 0;
  }
  
  for( uint8_t stagekit_id = 0; stagekit_id < MAX_STAGEKITS_IN_EXISTENCE; stagekit_id++ ) {
    m_stagekit_config_number[ stagekit_id ] = 0;
  }  
};

StageKitManager::~StageKitManager() {
  this->End();
};

uint8_t StageKitManager::Init() {
  if( m_usb_context ) {
    MSG_STAGEKITMANAGER_ERROR( "Already Init." );
    return 0;
  }

  if( libusb_init( &m_usb_context ) ) {
    MSG_STAGEKITMANAGER_ERROR( "libusb_init" );
    return 0;
  }

  libusb_device** usb_devicelist;
  ssize_t devicelist_count = libusb_get_device_list( m_usb_context, &usb_devicelist );
  MSG_STAGEKITMANAGER_DEBUG( "Found [ " << +devicelist_count << " ] device(s) on USB line." );
  if( devicelist_count < 1 ) {
    MSG_STAGEKITMANAGER_ERROR( "No USB devices detected." );
    this->End();
    return 0;
  }

  bool done = false;
  ssize_t usb_device_number = 0;
  m_amount_of_stagekits = 0;
  while( !done ) {
    
    libusb_device* device = usb_devicelist[ usb_device_number ];
    libusb_device_descriptor device_descriptor = {0};
    
    if( libusb_get_device_descriptor( device, &device_descriptor ) == 0 ) {
      // Found device...
      MSG_STAGEKITMANAGER_DEBUG( "Identifying USB Device [ " << usb_device_number
                                                             << " ] : 0x" << std::setfill( '0' ) << std::setw( 4 ) << std::hex 
                                                             << device_descriptor.idVendor
                                                             << " : 0x" << std::setfill( '0' ) << std::setw( 4 ) << std::hex
                                                             << device_descriptor.idProduct );
      if( device_descriptor.idVendor == STAGEKIT_VID && device_descriptor.idProduct == STAGEKIT_PID ) {
        // Is Stage Kit
        libusb_device_handle* ptr_usb_device_handle;
        if( libusb_open( device, &ptr_usb_device_handle ) == 0 ) {
          MSG_STAGEKITMANAGER_DEBUG( "USB Device = Stage Kit.  With handle = " << ptr_usb_device_handle );
          if( m_stagekit[ m_amount_of_stagekits ].Init( ptr_usb_device_handle ) ) {
            m_amount_of_stagekits++;
          }
        }
      }
    }

    if( m_amount_of_stagekits == MAX_STAGEKITS_IN_EXISTENCE || ++usb_device_number >= devicelist_count ) {
      done = true;
    }
  }
  
  libusb_free_device_list( usb_devicelist, 1 );
  
  if( m_amount_of_stagekits == 0 ) {
    MSG_STAGEKITMANAGER_ERROR( "Device not connected" );
    MSG_STAGEKITMANAGER_ERROR( " --  or  -- " );
    MSG_STAGEKITMANAGER_ERROR( "program not running with USB access." );
    this->End();
  } else {
    MSG_STAGEKITMANAGER_INFO( "Found [ " << +m_amount_of_stagekits << " ] Stage Kit(s) connected." );
  }
  
  return m_amount_of_stagekits;
};

int StageKitManager::Send( USB_ControlRequest* ptr_control_request, unsigned short length ) {
  // Uses first connected kit.
  if( !this->IsConnected( 0 ) ) {
    return -1;
  }
  return m_stagekit[ 0 ].Send( ptr_control_request, length );
};

void StageKitManager::End() {
  for( uint8_t stagekit_id = 0; stagekit_id < m_amount_of_stagekits; stagekit_id++ ) {
    // Force Set Lights to send SK_ALL_OFF.
    m_stagekit[ stagekit_id ].End();
  }
  
  if( m_usb_context != NULL ) {
    libusb_exit( m_usb_context );
    m_usb_context = NULL;
  }
  
  m_amount_of_stagekits = 0;
};

uint8_t StageKitManager::AmountOfStageKits() {
  return m_amount_of_stagekits;
};

bool StageKitManager::IsConnected( const uint8_t stagekit_id ) {
  if( stagekit_id <  m_amount_of_stagekits ) {
    return m_stagekit[ stagekit_id ].IsConnected();
  }
  return false;
};

bool StageKitManager::PollButtons( const uint8_t stagekit_id ) {
  if( this->IsConnected( stagekit_id ) ) {
    return m_stagekit[ stagekit_id ].PollButtons();
  }
  return false;
};

uint16_t StageKitManager::GetButtons( const uint8_t stagekit_id ) {
  if( this->IsConnected( stagekit_id ) ) {
    return m_stagekit[ stagekit_id ].GetButtons();
  }
  return 0;
};

void StageKitManager::SetConfigIDForStageKit( const uint8_t stagekit_id, const uint8_t config_id ) {
  if( stagekit_id < 4 && config_id < 5 ) {
    m_stagekit_config_number[ stagekit_id ] = config_id;
    m_stagekit[ stagekit_id ].SetConfig( &m_stagekit_config[ config_id ] );
    if( config_id == 0 ) {
      m_stagekit[ stagekit_id ].UpdateStatusLEDs( SKSTATUSLEDS::SK_STATUS_OFF );
    } else {
      m_stagekit[ stagekit_id ].UpdateStatusLEDs( SKSTATUSLEDS::SK_STATUS_ON_1 + config_id - 1 );
    }
  }
};

int8_t StageKitManager::GetConfigIDForStageKit( const uint8_t stagekit_id ) {
  if( stagekit_id < 4 ) {
    return m_stagekit_config_number[ stagekit_id ];
  }
  
  return -1;
};

void StageKitManager::ConfigEnableLights( const uint8_t config_id, const bool on ) {
  if( config_id > 0 && config_id < 5 ) {
    m_stagekit_config[ config_id ].m_light_pod_enabled = on;
  }
};

void StageKitManager::ConfigEnableStrobe( const uint8_t config_id, const bool on ) {
  if( config_id > 0 && config_id < 5 ) {
    m_stagekit_config[ config_id ].m_strobe_enabled = on;
  }
};

void StageKitManager::ConfigEnableFog( const uint8_t config_id, const bool on ) {
  if( config_id > 0 && config_id < 5 ) {
    m_stagekit_config[ config_id ].m_fog_enabled = on;
  }
};

void StageKitManager::ConfigSetFogTimes( const uint8_t config_id, const long instance_time_max_ms, const long total_max_time_per_song_ms ) {
  if( config_id > 0 && config_id < 5 ) {
    m_stagekit_config[ config_id ].m_fog_instance_time_max_ms = instance_time_max_ms;
    m_stagekit_config[ config_id ].m_fog_total_time_max_ms    = total_max_time_per_song_ms;
  }
};

bool StageKitManager::ConfigHasLightsEnabled( const uint8_t config_id ) {
  if( config_id > 0 && config_id < 5 ) {
    return m_stagekit_config[ config_id ].m_light_pod_enabled;
  }
  return false;
};

bool StageKitManager::ConfigHasStrobeEnabled( const uint8_t config_id ) {
  if( config_id > 0 && config_id < 5 ) {
    return m_stagekit_config[ config_id ].m_strobe_enabled;
  }
  return false;
};

bool StageKitManager::ConfigHasFogEnabled( const uint8_t config_id ) {
  if( config_id > 0 && config_id < 5 ) {
    return m_stagekit_config[ config_id ].m_fog_enabled;
  }
  return false;
};

bool StageKitManager::ConfigHasAnythingEnabled() {
  for( uint8_t config_id = 1; config_id < 5; config_id++ ) {
    if( m_stagekit_config[ config_id ].m_light_pod_enabled || m_stagekit_config[ config_id ].m_strobe_enabled || m_stagekit_config[ config_id ].m_fog_enabled ) {
      return true;
    }
  }
  return false;
};

void StageKitManager::SetLights( const uint8_t left_weight, const uint8_t right_weight ) {
  for( uint8_t stagekit_id = 0; stagekit_id < m_amount_of_stagekits; stagekit_id++ ) {
    m_stagekit[ stagekit_id ].UpdateLights( left_weight, right_weight );
  }
};

void StageKitManager::SetStrobe( const uint8_t speed ) {
  for( uint8_t stagekit_id = 0; stagekit_id < m_amount_of_stagekits; stagekit_id++ ) {
    m_stagekit[ stagekit_id ].UpdateStrobe( speed );
  }
};

void StageKitManager::SetFog( const bool on ) {
  for( uint8_t stagekit_id = 0; stagekit_id < m_amount_of_stagekits; stagekit_id++ ) {
    m_stagekit[ stagekit_id ].UpdateFog( on );
    if( m_fog_current_state_is_on != on && !on ) {
      // Changed state to off
      m_fog_just_changed_to_off = true;
      m_fog_instance_time_current = 0;
    }
    m_fog_current_state_is_on = on;
  }
};

bool StageKitManager::SetStatusLEDs( const uint8_t stagekit_id, const uint8_t status_value ) {
  if( this->IsConnected( stagekit_id ) ) {
    return m_stagekit[ stagekit_id ].UpdateStatusLEDs( status_value );
  }
  return false;
};

void StageKitManager::Handle_TimeUpdate( const long time_passed_ms ) {
  // Check instance time
  if( !m_fog_just_changed_to_off ) {
    m_fog_instance_time_current += time_passed_ms;
    for( uint8_t config_id = 1; config_id < 5; config_id++ ) {
      if( m_stagekit_config[ config_id ].m_fog_instance_time_max_ms != 0 && m_fog_instance_time_current >= m_stagekit_config[ config_id ].m_fog_instance_time_max_ms ) {
        for( uint8_t stagekit_id = 0; stagekit_id < MAX_STAGEKITS_IN_EXISTENCE; stagekit_id++ ) {
          if( m_stagekit_config_number[ stagekit_id ] == config_id ) {
            m_stagekit[ stagekit_id ].UpdateFog( false );
          }
        }
      }
    }
  }

  // Recently changed to off, then reset current instance time.
  if( m_fog_just_changed_to_off || m_fog_current_state_is_on ) {

    m_fog_total_time_current += time_passed_ms;
    if( !m_fog_just_changed_to_off ) {
      for( uint8_t config_id = 1; config_id < 5; config_id++ ) {
        if( m_stagekit_config[ config_id ].m_fog_total_time_max_ms != 0 && m_fog_total_time_current >= m_stagekit_config[ config_id ].m_fog_total_time_max_ms ) {
          for( uint8_t stagekit_id = 0; stagekit_id < MAX_STAGEKITS_IN_EXISTENCE; stagekit_id++ ) {
            if( m_stagekit_config_number[ stagekit_id ] == config_id ) {
              m_stagekit[ stagekit_id ].UpdateFog( false );
            }
          }
        }
      }
    }
  }
  
  if( m_fog_just_changed_to_off ) {
    m_fog_just_changed_to_off = false;
  }
};

void StageKitManager::Handle_SongChange() {
  m_fog_instance_time_current = 0;
  m_fog_total_time_current    = 0;
};
