
#include "RpiLightsController.h"

RpiLightsController::RpiLightsController(const char* ini_file) {

  // A lot of default values can be overridden by ini file.
  m_stagekit_colour_red         = 0x00;  // Bit set for led number indication
  m_stagekit_colour_green       = 0x00;  // Bit set for led number indication
  m_stagekit_colour_blue        = 0x00;  // Bit set for led number indication
  m_stagekit_colour_yellow      = 0x00;  // Bit set for led number indication
  m_stagekit_strobe_speed       = 0x00;  // Current strobe speed
  m_stagekit_strobe_next_on_ms  = 0;     // Next strobe update time
  m_sleeptime_idle              = 500;   // Time to sleep when program is in idle mode
  m_sleeptime_stagekit          = 10;    // Time to sleep when program is in stagekit mode
  m_sleeptime_strobe            = 5;     // Time to sleep when program is currently doing strobe
  m_stagekit_strobe_rate_1_ms   = 120;   // Time in MS between strobes for stagekit rate 1
  m_stagekit_strobe_rate_2_ms   = 100;   // Time in MS between strobes for stagekit rate 2
  m_stagekit_strobe_rate_3_ms   = 80;    // Time in MS between strobes for stagekit rate 3
  m_stagekit_strobe_rate_4_ms   = 60;    // Time in MS between strobes for stagekit rate 4

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


    // Stagekit startup settings
    if( !mINI_Handler.SetSection( "STAGEKIT" ) ) {
      MSG_RPLC_INFO( "INI section 'STAGEKIT' not found - Using default settings." );
    } else {
      // xbox status led
      int enable = mINI_Handler.GetTokenValue( "ENABLE_XBOX_LED_STATUS" );
      if( enable == 1 ) {
        mUSB_360StageKit.EnableStatusLEDs( true );
      } else {
        mUSB_360StageKit.EnableStatusLEDs( false );
      }

      // Pod lights
      enable = mINI_Handler.GetTokenValue( "ENABLE_POD_LIGHTS" );
      if( enable == 1 ) {
        mUSB_360StageKit.EnableLights( true );
      } else {
        mUSB_360StageKit.EnableLights( false );
      }

      // Fogger
      enable = mINI_Handler.GetTokenValue( "ENABLE_FOG" );
      if( enable == 1 ) {
        mUSB_360StageKit.EnableFog( true );
      } else {
        mUSB_360StageKit.EnableFog( false );
      }

      enable = mINI_Handler.GetTokenValue( "ENABLE_STROBE" );
      if( enable == 1 ) {
        mUSB_360StageKit.EnableStrobe( true );
      } else {
        mUSB_360StageKit.EnableStrobe( false );
      }
      m_stagekit_strobe_rate_1_ms = mINI_Handler.GetTokenValue( "STROBE_RATE_1_MS" );
      m_stagekit_strobe_rate_2_ms = mINI_Handler.GetTokenValue( "STROBE_RATE_2_MS" );
      m_stagekit_strobe_rate_3_ms = mINI_Handler.GetTokenValue( "STROBE_RATE_3_MS" );
      m_stagekit_strobe_rate_4_ms = mINI_Handler.GetTokenValue( "STROBE_RATE_4_MS" );
    }

    // LED startup settings
    if( !mINI_Handler.SetSection( "LEDS" ) ) {
      MSG_RPLC_INFO( "INT section 'LEDS' not found - No LED settings loaded." );
      m_leds_ini_amount = 0;
    } else {
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

      // LEDs Config
      bytes_read = readlink( "/proc/self/exe", path_buffer, len );
      path_buffer[ bytes_read ] = '\0';
      file = dirname( path_buffer );
      file += "/";
      file += m_leds_ini[ m_leds_ini_number - 1 ];

      if( !mLEDS.Start( led_device, led_amount ) ) {
        MSG_RPLC_ERROR( "LED Array start-up failed.  Check LED DEVICE in INI." );
        return;
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

      if( flash_amount > 0 ) {
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

  if( mUSB_360StageKit.IsConnected() ) {
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

uint16_t RpiLightsController::Update( uint16_t time_passed_MS ) {

  this->SerialAdapter_Poll();

  m_sleep_time = this->StageKit_Update( time_passed_MS );

  //this->StageKit_PollButtons();

  return m_sleep_time;
};

void RpiLightsController::Stop() {

  mSerialAdapter.Close();

  mUSB_360StageKit.End();

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

  int length = mUSB_360StageKit.Send( &m_control_request, m_control_request.header.wLength );

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

    uint8_t update = mSerialAdapter.Payload()[ 4 ];

    switch( update ) {
      case SKRUMBLEDATA::SK_LED_RED:
        MSG_RPLC_DEBUG( "RED LED" );
        this->Handle_LEDUpdate(  SKRUMBLEDATA::SK_LED_RED, mSerialAdapter.Payload()[ 3 ] );
        break;
     case SKRUMBLEDATA::SK_LED_GREEN:
        MSG_RPLC_DEBUG( "GREEN LED" );
        this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_GREEN, mSerialAdapter.Payload()[ 3 ] );
        break;
      case SKRUMBLEDATA::SK_LED_BLUE:
        MSG_RPLC_DEBUG( "BLUE LED" );
        this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_BLUE, mSerialAdapter.Payload()[ 3 ] );
        break;
      case SKRUMBLEDATA::SK_LED_YELLOW:
        MSG_RPLC_DEBUG( "YELLLOW LED" );
        this->Handle_LEDUpdate( SKRUMBLEDATA::SK_LED_YELLOW, mSerialAdapter.Payload()[ 3 ] );
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
        MSG_RPLC_INFO( "Unhandled stagekit data received from serial adapter : " << mSerialAdapter.Payload()[ 2 ] );
        break;
    }
  } else {
    if( mSerialAdapter.PayloadLength() == 3 ) {
      if( mSerialAdapter.Payload()[ 0 ] == 0x01 && mSerialAdapter.Payload()[ 1 ] == 0x03 ) {
        mUSB_360StageKit.SetStatusLEDs( mSerialAdapter.Payload()[ 2 ] );
        return;
      }
    }
    MSG_RPLC_INFO( "Unknown payload received from serial adapter : Length %i = " << mSerialAdapter.PayloadLength() );
  }

};

void RpiLightsController::Stagekit_ResetVariables() {
  m_stagekit_colour_red      = 0;
  m_stagekit_colour_green    = 0;
  m_stagekit_colour_blue     = 0;
  m_stagekit_colour_yellow   = 0;
  m_stagekit_strobe_speed    = 0;
};

void RpiLightsController::StageKit_PollButtons() {
  // Controller button polling, not used for anything.
  // Could maybe use xbox button to reset.
  if( mUSB_360StageKit.PollButtons() ) {
    uint16_t buttons = mUSB_360StageKit.GetButtons();
    if( ( buttons & SKBUTTON::SK_BUTTON_XBOX ) == SKBUTTON::SK_BUTTON_XBOX ) {
      MSG_RPLC_INFO( "XBOX Button!" );
    }
  }
};

bool RpiLightsController::Handle_StagekitConnect() {
  // If already connected then reset the connection
  if( mUSB_360StageKit.IsConnected() ) {
    mUSB_360StageKit.End();
  }

  if( !mUSB_360StageKit.Init() ) {
    MSG_RPLC_ERROR( "Unable to connect to USB SK360 POD." );

    return false;
  }

  MSG_RPLC_INFO( "Connected to X360 StageKit POD." );

  return true;
};

void RpiLightsController::Handle_StagekitDisconnect() {
  // Ensure strobe is turned off.
  this->Handle_StrobeUpdate( 0 );

  // Ensure fogger is turned off.
  this->Handle_FogUpdate( false );

  // Turn off all LEDs
  this->Handle_LEDUpdate( SKRUMBLEDATA::SK_ALL_OFF, SKRUMBLEDATA::SK_ALL_OFF );

  // Disconnect from the USB Stage Kit
  mUSB_360StageKit.End();

  MSG_RPLC_INFO( "Disconnected from StageKit POD." );

  this->Stagekit_ResetVariables();
};

bool RpiLightsController::Handle_SerialConnect() {
  if( !mUSB_360StageKit.IsConnected() ) {
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

  mUSB_360StageKit.SetLights( leds, colour );
};

void RpiLightsController::Handle_FogUpdate( const bool fogOn ) {
  mUSB_360StageKit.SetFog( fogOn );
};

void RpiLightsController::Handle_StrobeUpdate( const uint8_t strobe_speed ) {

  m_stagekit_strobe_speed = strobe_speed;

  if( strobe_speed == 0  ) {
    m_stagekit_strobe_next_on_ms = 0;
    mLEDS.Strobe( false );
  }

  mUSB_360StageKit.SetStrobe( strobe_speed );
};

uint16_t RpiLightsController::StageKit_Update( uint16_t time_passed_ms ) {

  uint16_t time_to_sleep = m_sleeptime_stagekit;

  // Check strobe status
  if( m_stagekit_strobe_speed > 0 ) {
    MSG_RPLC_DEBUG( "STROBE TIMER : TP = " << time_passed_ms << " : Next on = " << m_stagekit_strobe_next_on_ms );

    if( time_passed_ms < m_stagekit_strobe_next_on_ms ) {
      m_stagekit_strobe_next_on_ms -= time_passed_ms;
    } else {
      mLEDS.Strobe( true );
      switch( m_stagekit_strobe_speed ) {
        case 1:
          m_stagekit_strobe_next_on_ms += m_stagekit_strobe_rate_1_ms;
          break;
        case 2:
          m_stagekit_strobe_next_on_ms += m_stagekit_strobe_rate_2_ms;
          break;
        case 3:
          m_stagekit_strobe_next_on_ms += m_stagekit_strobe_rate_3_ms;
          break;
        case 4:
          m_stagekit_strobe_next_on_ms += m_stagekit_strobe_rate_4_ms;
          break;
      };
      m_stagekit_strobe_next_on_ms -= time_passed_ms;
      mLEDS.Strobe( false );
    }

    // How long till the strobe needs to be checked?
    if( m_stagekit_strobe_next_on_ms < time_to_sleep ) {
      time_to_sleep = m_stagekit_strobe_next_on_ms;
    } else {
      time_to_sleep = m_sleeptime_strobe;
    }
  }

  return time_to_sleep;
};
