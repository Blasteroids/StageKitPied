
#include "RpiLightsController.h"

RpiLightsController::RpiLightsController( const char* ini_file ) {

  // A lot of default values will be overridden by ini file.
  m_rb3e_listener_enabled       = false; // By default RB3Enhanced mode is off
  m_rb3e_sender_enabled         = false; // By default RB3Enhanced packet sending is off.

  m_stagekit_colour_red         = 0x00;  // Bit set for led number indication
  m_stagekit_colour_green       = 0x00;  // Bit set for led number indication
  m_stagekit_colour_blue        = 0x00;  // Bit set for led number indication
  m_stagekit_colour_yellow      = 0x00;  // Bit set for led number indication

  m_sleeptime_idle              = 500;   // Time to sleep when program is in idle mode
  m_sleeptime_stagekit          = 10;    // Time to sleep when program is in stagekit mode
  m_sleeptime_strobe            = 5;     // Time to sleep when program is currently doing strobe

  m_leds_enabled                = false; // Default no leds
  m_leds_strobe_enabled         = false; // Default don't strobe
  m_leds_strobe_rate[ 0 ]       = 120;   // Time in MS between strobes for stagekit rate 1
  m_leds_strobe_rate[ 1 ]       = 100;   // Time in MS between strobes for stagekit rate 2
  m_leds_strobe_rate[ 2 ]       = 80;    // Time in MS between strobes for stagekit rate 3
  m_leds_strobe_rate[ 3 ]       = 60;    // Time in MS between strobes for stagekit rate 4
  m_leds_strobe_speed_current   = 0;
  m_leds_strobe_next_on_ms      = 0;

  m_nodata_ms                   = 10 * 1000;
  m_nodata_ms_count             = 0;
  m_nodata_red                  = 0;
  m_nodata_green                = 0;
  m_nodata_blue                 = 0;
  
  m_button_check_delay          = 2000;   // Time between next button repeat
  
  m_ptr_control_request_data = (unsigned char*) &m_control_request.header;
  m_serial_connected_to_x360 = false;

  // Path
  char path_buffer[ 256 ];
  size_t len = sizeof( path_buffer );
  int bytes_read = readlink( "/proc/self/exe", path_buffer, len );
  path_buffer[ bytes_read ] = '\0';

  // Config
  std::string file = dirname( path_buffer );
  file += "/";
  file += ini_file;

  if( !mINI_Handler.Load( file ) ) {
    MSG_RPLC_ERROR( "Error loading config.ini" );
    MSG_RPLC_ERROR( " - Err= " << mINI_Handler.GetErrorMessage() );
  } else {
    // Sleep times from ini if found
    if( mINI_Handler.SetSection( "SLEEP_TIMES" ) ) {
      // Pod lights
      m_sleeptime_idle      = mINI_Handler.GetTokenValue( "IDLE" );
      m_sleeptime_stagekit  = mINI_Handler.GetTokenValue( "STAGEKIT" );
      m_sleeptime_strobe    = mINI_Handler.GetTokenValue( "STROBE" );
    }

    // RB3Enhanced mode?
    if( !mINI_Handler.SetSection( "RB3E" ) ) {
      MSG_RPLC_INFO( "INI section 'RB3E' not found - Defaulting to serial mode." );
    } else {
      int enabled = mINI_Handler.GetTokenValue( "ENABLED" );
      if( enabled == 1 ) {
        m_rb3e_listener_enabled = true;
      } else {
        m_rb3e_listener_enabled = false;
      }
      m_rb3e_source_ip = mINI_Handler.GetTokenString( "SOURCE_IP" );
      m_rb3e_listening_port = mINI_Handler.GetTokenValue( "LISTENING_PORT" );
    }

    if( !mINI_Handler.SetSection( "NETWORK" ) ) {
      MSG_RPLC_INFO( "INI section 'NETWORK' not found." );
    } else {
      int enabled = mINI_Handler.GetTokenValue( "ENABLED" );
      if( enabled == 1 ) {
        m_rb3e_listener_enabled = false;
        m_rb3e_sender_enabled = true;
      } else {
        m_rb3e_sender_enabled = false;
      }
      m_rb3e_target_ip = mINI_Handler.GetTokenString( "TARGET_IP" );
      m_rb3e_target_port = mINI_Handler.GetTokenValue( "TARGET_PORT" );
    }
    
    std::string section_name;
    for( uint8_t config_id = 1; config_id < 5; config_id++ ) {
      // Stagekit configs.  Config=0 is internal for all off.
      section_name = "STAGEKIT_CONFIG_";
      section_name += std::to_string( config_id );
      
      if( !mINI_Handler.SetSection( section_name ) ) {
        MSG_RPLC_INFO( "INI section 'STAGEKIT_CONFIG_" << config_id << "' not found - Using default settings." );
      } else {
        // Pod lights
        int enable = mINI_Handler.GetTokenValue( "ENABLE_POD_LIGHTS" );
        if( enable == 1 ) {
          mStageKitManager.ConfigEnableLights( config_id, true );
        } else {
          mStageKitManager.ConfigEnableLights( config_id, false );
        }

        // Strobe
        enable = mINI_Handler.GetTokenValue( "ENABLE_STROBE" );
        if( enable == 1 ) {
          mStageKitManager.ConfigEnableStrobe( config_id, true );
        } else {
          mStageKitManager.ConfigEnableStrobe( config_id, false );
        }
        
        // Fogger
        enable = mINI_Handler.GetTokenValue( "ENABLE_FOG" );
        if( enable == 1 ) {
          mStageKitManager.ConfigEnableFog( config_id, true );
        } else {
          mStageKitManager.ConfigEnableFog( config_id, false );
        }
        long fog_instance_time_ms = mINI_Handler.GetTokenValue( "FOG_MAX_INSTANCE_TIME_SECONDS" ) * 1000;
        long fog_total_time_ms    = mINI_Handler.GetTokenValue( "FOG_MAX_TOTAL_TIME_SECONDS" ) * 1000;
        mStageKitManager.ConfigSetFogTimes( config_id, fog_instance_time_ms, fog_total_time_ms );      
      }
    }

    // LED startup settings
    if( !mINI_Handler.SetSection( "LEDS" ) ) {
      MSG_RPLC_INFO( "INT section 'LEDS' not found - No LED settings loaded." );
      m_leds_ini_amount = 0;
    } else {
      // LED lights
      int enable = mINI_Handler.GetTokenValue( "ENABLED" );
      if( enable == 1 ) {
        m_leds_enabled = true;
      } else {
        m_leds_enabled = false;
      }

      std::string led_device = mINI_Handler.GetTokenString( "DEVICE" );
      int led_amount = mINI_Handler.GetTokenValue( "LED_AMOUNT" );
      m_leds_ini_amount = mINI_Handler.GetTokenValue( "INI_AMOUNT" );
      m_leds_ini_number = mINI_Handler.GetTokenValue( "INI_DEFAULT" );

      m_leds_ini = new std::string[ m_leds_ini_amount ];
      std::string token;

      for( int i = 0; i < m_leds_ini_amount; i++ ) {
        token = "INI";
        token += std::to_string( i + 1 );
        m_leds_ini[ i ] = mINI_Handler.GetTokenString( token );
      }

      // Strobe
      enable = mINI_Handler.GetTokenValue( "STROBE_ENABLED" );
      if( enable == 1 ) {
        m_leds_strobe_enabled = true;
      } else {
        m_leds_strobe_enabled = false;
      }
      m_leds_strobe_rate[ 0 ] = mINI_Handler.GetTokenValue( "STROBE_RATE_1_MS" );
      m_leds_strobe_rate[ 1 ] = mINI_Handler.GetTokenValue( "STROBE_RATE_2_MS" );
      m_leds_strobe_rate[ 2 ] = mINI_Handler.GetTokenValue( "STROBE_RATE_3_MS" );
      m_leds_strobe_rate[ 3 ] = mINI_Handler.GetTokenValue( "STROBE_RATE_4_MS" );

      // LEDs Config
      bytes_read = readlink( "/proc/self/exe", path_buffer, len );
      path_buffer[ bytes_read ] = '\0';
      file = dirname( path_buffer );
      file += "/";
      file += m_leds_ini[ m_leds_ini_number - 1 ];

      if( !mLEDS.Init( led_device, led_amount ) ) {
        MSG_RPLC_ERROR( "LED Array init failed." );
        return;
      }

      if( !mLEDS.SetEnabled( m_leds_enabled ) ) {
        MSG_RPLC_ERROR( "LED Array start-up failed.  Check LED DEVICE in INI." );
        return;
      }

      if( mINI_Handler.SetSection( "NO_DATA" ) ) {
        m_nodata_ms = mINI_Handler.GetTokenValue( "NO_DATA_SECONDS" );
        m_nodata_ms *= 1000;
        m_nodata_ms_count = 0;

        std::string tmp;
        std::stringstream rgb;
        rgb.str( mINI_Handler.GetTokenString( "NO_DATA_RGB" ) );

        std::getline( rgb, tmp, ',' );
        m_nodata_red = std::stoi( tmp );

        std::getline( rgb, tmp, ',' );
        m_nodata_green = std::stoi( tmp );

        std::getline( rgb, tmp, ',' );
        m_nodata_blue = std::stoi( tmp );

        m_nodata_brightness = mINI_Handler.GetTokenValue( "NO_DATA_BRIGHTNESS" );
      }

      int flash_red        = 0;
      int flash_green      = 255;
      int flash_blue       = 0;
      int flash_brightness = 15;
      int flash_amount     = 3;
      int flash_delay_ms   = 200;

      if( mINI_Handler.SetSection( "STARTUP" ) ) {
        std::string tmp;
        std::stringstream rgb;
        rgb.str( mINI_Handler.GetTokenString( "FLASH_RGB" ) );

        std::getline( rgb, tmp, ',' );
        flash_red = std::stoi( tmp );

        std::getline( rgb, tmp, ',' );
        flash_green = std::stoi( tmp );

        std::getline( rgb, tmp, ',' );
        flash_blue = std::stoi( tmp );

        flash_amount = mINI_Handler.GetTokenValue( "FLASH_AMOUNT" );
        flash_delay_ms = mINI_Handler.GetTokenValue( "FLASH_DELAY_MS" );
        flash_brightness = mINI_Handler.GetTokenValue( "FLASH_BRIGHTNESS" );
        if( flash_brightness > 15 ) {
          flash_brightness = 15;
        }
      }

      if( m_leds_enabled && flash_amount > 0 ) {
        SleepTimer timer;
        timer.SetSleepTimeMax( flash_delay_ms );
        timer.Start();

        while( flash_amount > 0 ) {
          mLEDS.SetAllLED( flash_red, flash_green, flash_blue, flash_brightness );
          timer.Sleep();
          mLEDS.TurnOff();
          timer.Sleep();
          flash_amount--;
        }
      }

      if( mLEDS.LoadSettingsSK( file ) ) {
        MSG_RPLC_INFO( "LED Settings loaded." );
      } else {
        MSG_RPLC_ERROR( "Failed to load LED settings." );
      }
    }
  }

  // Idle sleep time.
  m_sleep_time = m_sleeptime_idle;

};

RpiLightsController::~RpiLightsController() {
  this->Stop();

  // Clean up led ini filenames
  if( m_leds_ini_amount > 0 ) {
    delete [] m_leds_ini;
  }
};

bool RpiLightsController::Start() {
  // RB3E MODE
  if( m_rb3e_listener_enabled ) {
    if( !mRB3E_Network.StartReceiver( m_rb3e_source_ip, m_rb3e_listening_port ) ) {
      MSG_RPLC_ERROR( "Failed to start networking for RB3E mode." );
      return false;
    }

    if( mStageKitManager.ConfigHasAnythingEnabled() ) {
      if( mStageKitManager.IsConnected() ) {
        // If already connected, try a reset.
        this->Handle_StagekitDisconnect();
      }
      if( !this->Handle_StagekitConnect() ) {
        MSG_RPLC_ERROR( "Config has StageKit support enabled but unable to find a StageKit. Running without." );
      }
    }    
    return true;
  }

  // SERIAL MODE
  
  // Use RB3e network data structure to send data out?
  if( m_rb3e_sender_enabled ) {
    if( !mRB3E_Network.StartSender( m_rb3e_target_ip, m_rb3e_target_port ) ) {
      MSG_RPLC_ERROR( "Failed to start networking for NETWORK mode. Running without." );
    }
  }

  if( mStageKitManager.IsConnected() ) {
    // If already connected, try a reset.
    this->Handle_SerialDisconnect();
    this->Handle_StagekitDisconnect();
  }
    
  if( !this->Handle_StagekitConnect() ) {
    return false;
  }

  if( !this->Handle_SerialConnect() ) {
    return false;
  }

  return true;
};

long RpiLightsController::Update( long time_passed_ms ) {
  
  if( m_rb3e_listener_enabled ) {
    this->RB3ENetwork_Poll();
  } else {
    this->SerialAdapter_Poll();
  }

  m_sleep_time = this->Handle_TimeUpdate( time_passed_ms );

  // Yeah this isn't right, since we probably had data but that data will reset counter to 0
  // so it'll be close enough :D
  if( m_nodata_ms > 0 ) {
    m_nodata_ms_count += m_sleep_time;
    if( m_nodata_ms_count > m_nodata_ms ) {
      mLEDS.SetAllLED( m_nodata_red, m_nodata_green, m_nodata_blue, m_nodata_brightness );
      m_nodata_ms_count = 0;
    }
  }

  this->StageKit_PollButtons( time_passed_ms );

  return m_sleep_time;
};

void RpiLightsController::Stop() {
  if( m_rb3e_listener_enabled || m_rb3e_sender_enabled ) {
    mRB3E_Network.Stop();
    return;
  }
  
  mSerialAdapter.Close();

  mStageKitManager.End();
};


void RpiLightsController::SerialAdapter_Poll() {

  if( mSerialAdapter.Poll() ) {
    switch( mSerialAdapter.PayloadType() ) {
      case HEADER_CONTROL_DATA:
        MSG_RPLC_DEBUG( "Received control data payload from serial adapter." );
        this->SerialAdapter_HandleControlData();
        break;
      case HEADER_OUT_REPORT:
        if( !m_serial_connected_to_x360 ) {
          if( mSerialAdapter.GetStatus() == HEADER_STATUS_SPOOFED ) {
            m_serial_connected_to_x360 = true;
            MSG_RPLC_INFO( "Connected to X360 with serial adapter." );
          }
        }
        MSG_RPLC_DEBUG( "Received data report from serial adapter." );
        this->SerialAdapter_HandleOutReport();
        break;
      default:
        MSG_RPLC_INFO( "Skipping unhandled data returned by serial adapter." );
        break;
    }
  }
};

void RpiLightsController::SerialAdapter_HandleControlData() {
  memcpy( m_ptr_control_request_data, mSerialAdapter.Payload(), mSerialAdapter.PayloadLength() );

  bool direction_in = m_control_request.header.bRequestType & USB_DIRECTION_IN;

  int length = mStageKitManager.Send( &m_control_request, m_control_request.header.wLength );

  if( length < 0 ) {
    MSG_RPLC_ERROR( "libusb_control_transfer failed with error: " << length );
  } else {
    if( direction_in ) {
      if( !mSerialAdapter.SendControlReply( m_control_request.data, length ) ) {
        MSG_RPLC_ERROR( "Error sending control reply with serial adapter." );
      }
    }
  }
};

void RpiLightsController::SerialAdapter_HandleOutReport() {
  /*
  // NOTE: RB3 doesn't send light data when in practice mode!
  */

  if( mSerialAdapter.PayloadLength() == 8 ) {
    if( mSerialAdapter.Payload()[ 0 ] != 0x00 || mSerialAdapter.Payload()[ 1 ] != 0x08 || mSerialAdapter.Payload()[ 2 ] != 0x00 ) {
      // Ignore other data
      return;
    }

    this->Handle_RumbleData( mSerialAdapter.Payload()[ 3 ], mSerialAdapter.Payload()[ 4 ] );
    
  } else {
    if( mSerialAdapter.PayloadLength() == 3 ) {
      if( mSerialAdapter.Payload()[ 0 ] == 0x01 && mSerialAdapter.Payload()[ 1 ] == 0x03 ) {
        // No longer used.
        // mSerialAdapter.Payload()[ 2 ] is the LED Status ID indicating the controller ID number.
        return;
      }
    }
    MSG_RPLC_INFO( "Unknown payload received from serial adapter : Length %i = " << mSerialAdapter.PayloadLength() );
  }

};

void RpiLightsController::RB3ENetwork_Poll() {
  if( mRB3E_Network.Poll() ) {
    MSG_RPLC_DEBUG( "Received RB3E network data." );
    if( mRB3E_Network.EventWasStagekit() ) {
      MSG_RPLC_DEBUG( "RB3E data is stage kit." );
      this->Handle_RumbleData( mRB3E_Network.GetWeightLeft(), mRB3E_Network.GetWeightRight() );
    }
  }
};

void RpiLightsController::Stagekit_ResetVariables() {
  m_stagekit_colour_red       = 0;
  m_stagekit_colour_green     = 0;
  m_stagekit_colour_blue      = 0;
  m_stagekit_colour_yellow    = 0;
  m_leds_strobe_speed_current = 0;
  m_leds_strobe_next_on_ms    = 0;
};

void RpiLightsController::StageKit_PollButtons( long time_passed_in_ms ) {
  if( time_passed_in_ms < m_button_check_delay ) {
    m_button_check_delay -= time_passed_in_ms;
  } else {
    m_button_check_delay = 100;
    
    for( uint8_t stagekit_id = 0; stagekit_id < mStageKitManager.AmountOfStageKits(); stagekit_id++ ) {  
      MSG_RPLC_DEBUG( "Testing for Xbox Button on stagekit [ " << +stagekit_id << " ]" );
      if( mStageKitManager.PollButtons( stagekit_id ) ) {
        uint16_t buttons = mStageKitManager.GetButtons( stagekit_id );
        if( ( buttons & SKBUTTON::SK_BUTTON_XBOX ) == SKBUTTON::SK_BUTTON_XBOX ) {
          MSG_RPLC_DEBUG( "Xbox Button pressed on Stage Kit [ " << +stagekit_id << " ]" );
          uint8_t config_id = mStageKitManager.GetConfigIDForStageKit( stagekit_id );
          if( ++config_id > 4 ) {
            config_id = 0;  // 0 = off.
          }
          mStageKitManager.SetConfigIDForStageKit( stagekit_id, config_id );
          MSG_RPLC_DEBUG( "Setting Stage Kit [ " << +stagekit_id << " ] to config [ " << +config_id << " ]" );
        }
      }
    }
  }
};

bool RpiLightsController::Handle_StagekitConnect() {
  // If already connected then reset the connection
  if( mStageKitManager.IsConnected() ) {
    mStageKitManager.End();
  }

  if( !mStageKitManager.Init() ) {
    MSG_RPLC_ERROR( "Unable to connect to a USB SK360 POD." );

    return false;
  }

  MSG_RPLC_INFO( "Connected to a X360 StageKit POD." );

  // If there's only 1 stagekit then use config 1 by default
  if( mStageKitManager.AmountOfStageKits() == 1 ) {
    mStageKitManager.SetConfigIDForStageKit( 0, 1 );
  }

  return true;
};

void RpiLightsController::Handle_StagekitDisconnect() {
  // Ensure strobe is turned off.
  this->Handle_StrobeUpdate( 0 );

  // Ensure fogger is turned off.
  this->Handle_FogUpdate( false );

  // Turn off all LEDs
  this->Handle_LEDUpdate( SKRUMBLEDATA::SK_ALL_OFF, SKRUMBLEDATA::SK_ALL_OFF );

  // Disconnect from any actual USB Stage Kits
  mStageKitManager.End();

  MSG_RPLC_INFO( "Disconnection from any StageKit POD completed." );

  this->Stagekit_ResetVariables();
};

bool RpiLightsController::Handle_SerialConnect() {
  if( !mStageKitManager.IsConnected() ) {
    MSG_RPLC_ERROR( "A USB SK360 POD needs to be connected before starting the Serial Adapter." );
    return false;
  }

  if( !mSerialAdapter.IsRunning() ) {
    if( !mINI_Handler.SetSection( "SERIAL_INTERFACE" ) ) {
      MSG_RPLC_ERROR( "No SERIAL_INTERFACE section found in ini file." );
      return false;
    }

    bool surpress_warnings = mINI_Handler.GetTokenValue( "SURPRESS_WARNINGS" ) > 0 ? true : false;

    std::string serial_port = mINI_Handler.GetTokenString( "SERIAL_PORT_1" );
    MSG_RPLC_INFO( "Attempting Serial Adapter connection on '" << serial_port << "'" );

    if( !mSerialAdapter.Init( serial_port.c_str(), surpress_warnings ) ) {
      serial_port = mINI_Handler.GetTokenString( "SERIAL_PORT_2" );
      MSG_RPLC_INFO( "Attempting Serial Adapter connection on '" << serial_port << "'" );
      if( !mSerialAdapter.Init( serial_port.c_str(), surpress_warnings ) ) {
        MSG_RPLC_ERROR( "Unable to find a connected Serial Adapter." );
        return false;
      }
    }
    MSG_RPLC_INFO( "Connected to Serial Adapter." );

    this->Stagekit_ResetVariables();
  }

  return true;
};

void RpiLightsController::Handle_SerialDisconnect() {
  // Turn off the serial adapter
  mSerialAdapter.Close();
  MSG_RPLC_INFO( "Disconnected from Serial Adapter." );
};

void RpiLightsController::Handle_RumbleData( uint8_t left_weight, uint8_t right_weight ) {
  switch( right_weight ) {
    case SKRUMBLEDATA::SK_LED_RED:
      MSG_RPLC_DEBUG( "RED LED" );
      this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_RED, left_weight );
      break;
   case SKRUMBLEDATA::SK_LED_GREEN:
      MSG_RPLC_DEBUG( "GREEN LED" );
      this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_GREEN, left_weight );
      break;
    case SKRUMBLEDATA::SK_LED_BLUE:
      MSG_RPLC_DEBUG( "BLUE LED" );
      this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_BLUE, left_weight );
      break;
    case SKRUMBLEDATA::SK_LED_YELLOW:
      MSG_RPLC_DEBUG( "YELLLOW LED" );
      this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_YELLOW, left_weight );
      break;
    case SKRUMBLEDATA::SK_FOG_ON:
      MSG_RPLC_DEBUG( "FOG ON" );
      this->Handle_FogUpdate( true );
      break;
    case SKRUMBLEDATA::SK_FOG_OFF:
      MSG_RPLC_DEBUG( "FOG OFF" );
      this->Handle_FogUpdate( false );
      break;
    case SKRUMBLEDATA::SK_STROBE_OFF:
      MSG_RPLC_DEBUG( "Strobe OFF" );
      this->Handle_StrobeUpdate( 0 );
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_1:
      MSG_RPLC_DEBUG( "Strobe - Speed 1" );
      this->Handle_StrobeUpdate( 1 );
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_2:
      MSG_RPLC_DEBUG( "Strobe - Speed 2" );
      this->Handle_StrobeUpdate( 2 );
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_3:
      MSG_RPLC_DEBUG( "Strobe - Speed 3" );
      this->Handle_StrobeUpdate( 3 );
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_4:
      MSG_RPLC_DEBUG( "Strobe - Speed 4" );
      this->Handle_StrobeUpdate( 4 );
      break;
    case SKRUMBLEDATA::SK_ALL_OFF:
      // I suspect all off includes fog & strobe.
      MSG_RPLC_DEBUG( "ALL OFF - LEDS & STROBE - " );
      this->Handle_LEDUpdate( SKRUMBLEDATA::SK_ALL_OFF, SKRUMBLEDATA::SK_ALL_OFF );
      this->Handle_StrobeUpdate( 0 );
      this->Handle_FogUpdate( false );
      break;
    default:
      MSG_RPLC_INFO( "Unhandled stagekit data received : " << right_weight );
      break;
  }

  // Anything other than fog off counts as new data since fog off is constantly sent.
  if( right_weight != SKRUMBLEDATA::SK_FOG_OFF ) {
    m_nodata_ms_count = 0;
  }
  
  if( m_rb3e_sender_enabled ) {
    mRB3E_Network.SendLightEvent( left_weight, right_weight );
  }
};

// Colour = SK colour - From serial
void RpiLightsController::Handle_LEDUpdate( const uint8_t colour, const uint8_t leds ) {

  switch( colour ) {
    case SKRUMBLEDATA::SK_ALL_OFF:
      this->Stagekit_ResetVariables();
      break;
    case SKRUMBLEDATA::SK_LED_RED:
      m_stagekit_colour_red = leds;
      break;
    case SKRUMBLEDATA::SK_LED_GREEN:
      m_stagekit_colour_green = leds;
      break;
    case SKRUMBLEDATA::SK_LED_BLUE:
      m_stagekit_colour_blue = leds;
      break;
    case SKRUMBLEDATA::SK_LED_YELLOW:
      m_stagekit_colour_yellow = leds;
      break;
  }

  mLEDS.SetLights( colour, leds );

  mStageKitManager.SetLights( leds, colour );
};

void RpiLightsController::Handle_FogUpdate( bool fog_on_state ) {
  mStageKitManager.SetFog( fog_on_state );
};

void RpiLightsController::Handle_StrobeUpdate( const uint8_t strobe_speed ) {

  m_leds_strobe_speed_current = strobe_speed;

  if( strobe_speed == 0  ) {
    m_leds_strobe_next_on_ms = 0;
    mLEDS.Strobe( false );
  }

  mStageKitManager.SetStrobe( strobe_speed );
};

long RpiLightsController::Handle_TimeUpdate( long time_passed_ms ) {

  uint16_t time_to_sleep = m_sleeptime_stagekit;

  // Check strobe status
  if( m_leds_strobe_speed_current > 0 ) {
    MSG_RPLC_DEBUG( "STROBE TIMER : TP = " << time_passed_ms << " : Next on = " << m_leds_strobe_next_on_ms );

    if( time_passed_ms < m_leds_strobe_next_on_ms ) {
      m_leds_strobe_next_on_ms -= time_passed_ms;
    } else {
      mLEDS.Strobe( true );
      m_leds_strobe_next_on_ms += m_leds_strobe_rate[ m_leds_strobe_speed_current - 1 ];
      m_leds_strobe_next_on_ms -= time_passed_ms;
      mLEDS.Strobe( false );
    }

    // How long till the strobe needs to be checked?
    if( m_leds_strobe_next_on_ms < time_to_sleep ) {
      time_to_sleep = m_leds_strobe_next_on_ms;
    } else {
      time_to_sleep = m_sleeptime_strobe;
    }
  }
  
  // Stagekit manager will deal with fog
  mStageKitManager.Handle_TimeUpdate( time_passed_ms );
  
  // Attempt to detect a song change by no data for 3 seconds.  Is this long enough?
  if( m_nodata_ms_count >= 3000 ) {
    mStageKitManager.Handle_SongChange();
  }
  return time_to_sleep;
};
