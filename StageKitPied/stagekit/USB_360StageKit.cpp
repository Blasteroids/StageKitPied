
#include "USB_360StageKit.h"

USB_360StageKit::USB_360StageKit() {
  m_usb_device_handle   = NULL;
  m_usb_context         = NULL;
  m_status_leds         = SKSTATUSLEDS::SK_STATUS_OFF;
  m_status_leds_enabled = true;
  m_lights_enabled      = true;
  m_fog_enabled         = true;
  m_strobe_enabled      = true;
};

USB_360StageKit::~USB_360StageKit() {
  this->End();
};

bool USB_360StageKit::Init() {
  if( m_usb_device_handle ) {
    MSG_USB360SK_ERROR( "Already connected" );
    return false;
  }

  if( libusb_init( &m_usb_context ) ) {
    MSG_USB360SK_ERROR( "libusb_init" );
    return false;
  }

  //libusb_set_option( m_usb_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG );

  m_usb_device_handle = libusb_open_device_with_vid_pid( m_usb_context, STAGEKIT_VID, STAGEKIT_PID );

  if( !m_usb_device_handle ) {
    MSG_USB360SK_ERROR( "libusb_open_device_with_vid_pid" );
    MSG_USB360SK_ERROR( "Device not connected" );
    MSG_USB360SK_ERROR( " --  or  -- " );
    MSG_USB360SK_ERROR( "program not running with USB access." );
    return false;
  }

  if( libusb_set_auto_detach_kernel_driver( m_usb_device_handle, 1 ) ) {
    MSG_USB360SK_ERROR( "libusb_set_auto_detach_kernal_driver" );
  }

  if( libusb_reset_device( m_usb_device_handle ) ) {
    MSG_USB360SK_ERROR( "libusb_reset_device" );
    return false;
  }

  int config;
  if( libusb_get_configuration( m_usb_device_handle, &config ) ) {
    MSG_USB360SK_ERROR( "libusb_get_configuration" );
    return false;
  }

  if( config == 0 ) {
    config = 1;
    libusb_set_configuration( m_usb_device_handle, 1 );
  }

  if( !this->ClaimInterfaces() ) {
    return false;
  }

  // When interfaces are claimed the POD likes to blink the status LED.
  // Doing a reset of the status LEDs will turn them off if required.
  this->EnableStatusLEDs( m_status_leds_enabled );

  return true;
};

bool USB_360StageKit::IsConnected() {
  return ( m_usb_device_handle != NULL );
};

void USB_360StageKit::EnableStatusLEDs( bool on ) {
  if( on == false ) {
    m_status_leds_enabled = true;
    this->SetStatusLEDs( SKSTATUSLEDS::SK_STATUS_OFF );
  } else {
    this->SetStatusLEDs( m_status_leds );
  }

  m_status_leds_enabled = on;
}

void USB_360StageKit::EnableLights( bool on ) {
  if( on == false ) {
    // Setting this true so off command is processed.
    m_lights_enabled = true;

    // Turn off anything which maybe on.
    this->SetLights( SK_ALL_OFF, SK_ALL_OFF );
  }

  m_lights_enabled = on;
};

void USB_360StageKit::EnableFog( bool on ) {
  if( on == false ) {
    // Setting this true so off command is processed.
    m_fog_enabled = true;

    // Turn off anything which maybe on.
    this->SetFog( false );
  }

  m_fog_enabled = on;
};

void USB_360StageKit::EnableStrobe( bool on ) {
  if( on == false ) {
    // Setting this true so off command is processed.
    m_strobe_enabled = true;

    // Turn off anything which maybe on.
    this->SetStrobe( SK_STROBE_OFF );
  }

  m_strobe_enabled = on;
};

bool USB_360StageKit::LightsEnabled() {
  return m_lights_enabled;
};

bool USB_360StageKit::FogEnabled() {
  return m_fog_enabled;
};

bool USB_360StageKit::StrobeEnabled() {
  return m_strobe_enabled;
};

bool USB_360StageKit::ClaimInterfaces() {
  if( m_usb_device_handle == NULL ) {
    return false;
  }

  libusb_device*                   usb_device = libusb_get_device( m_usb_device_handle );
  struct libusb_device_descriptor  usb_device_descriptor;

  if( libusb_get_device_descriptor( usb_device, &usb_device_descriptor ) ) {
    MSG_USB360SK_ERROR( "ClaimInterfaces : 'libusb_get_device_descriptor'" );
    return false;
  }

  if( usb_device_descriptor.bNumConfigurations ) {

    struct libusb_config_descriptor* configuration;
    if( libusb_get_config_descriptor( usb_device, 0, &configuration ) ) {
      MSG_USB360SK_ERROR( "ClaimInterfaces : 'libusb_get_config_descriptor'" );
      return false;
    }

    int err;

    for( int interfaceIndex = 0; interfaceIndex < configuration->bNumInterfaces; ++interfaceIndex ) {
      const struct libusb_interface* interface = configuration->interface + interfaceIndex;
      err = libusb_claim_interface( m_usb_device_handle,  interface->altsetting->bInterfaceNumber);
      if( err != LIBUSB_SUCCESS ) {
        MSG_USB360SK_ERROR( "ClaimInterfaces : 'libusb_claim_interface' failed for " << interfaceIndex << " with " << err );
        libusb_free_config_descriptor( configuration );
        return false;
      }
      MSG_USB360SK_DEBUG( "ClaimInterfaces : 'libusb_claim_interface' succeeded for " << interfaceIndex );
    }
    libusb_free_config_descriptor( configuration );
  }

  return true;
};

bool USB_360StageKit::ReleaseInterfaces() {
  if( m_usb_device_handle == NULL ) {
    return false;
  }

  libusb_device*                   usb_device = libusb_get_device( m_usb_device_handle );
  struct libusb_device_descriptor  usb_device_descriptor;

  if( libusb_get_device_descriptor( usb_device, &usb_device_descriptor ) ) {
    MSG_USB360SK_ERROR( "ReleaseInterfaces : 'libusb_get_device_descriptor'" );
    return false;
  }

  if( usb_device_descriptor.bNumConfigurations ) {

    struct libusb_config_descriptor* configuration;
    if( libusb_get_config_descriptor( usb_device, 0, &configuration ) ) {
      MSG_USB360SK_ERROR( "ReleaseInterfaces : Error : 'libusb_get_config_descriptor'" );
      return false;
    }

    int err;

    for( int interfaceIndex = 0; interfaceIndex < configuration->bNumInterfaces; ++interfaceIndex ) {
      const struct libusb_interface* interface = configuration->interface + interfaceIndex;
      err = libusb_release_interface( m_usb_device_handle,  interface->altsetting->bInterfaceNumber );
      if( err != LIBUSB_SUCCESS ) {
        MSG_USB360SK_ERROR( "ReleaseInterfaces : 'libusb_release_interface' failed for " << interfaceIndex << " with " << err );
        libusb_free_config_descriptor( configuration );
        return false;
      }
      MSG_USB360SK_DEBUG( "ReleaseInterfaces : 'libusb_release_interface' succeeded for " << interfaceIndex );
    }
    libusb_free_config_descriptor( configuration );
  }

  return true;
};

int USB_360StageKit::Send( USB_ControlRequest* ptr_control_request, unsigned short length ) {
  int ret;
  int retries = 0;

  do {
    ret = libusb_control_transfer( m_usb_device_handle,                         // Device/Handle
                                   ptr_control_request->header.bRequestType,    // LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE
                                   ptr_control_request->header.bRequest,        // HID_GET_REPORT
                                   ptr_control_request->header.wValue,          // ( HID_REPORT_TYPE_INPUT << 8 ) | 0x00
                                   ptr_control_request->header.wIndex,          // 0
                                   ptr_control_request->data,                   // report
                                   length,                                      // report_length
                                   USB_REQUEST_TIMEOUT);                        // 1000

    if( ret == LIBUSB_ERROR_PIPE ) {
      libusb_clear_halt( m_usb_device_handle, 0 );
    }
    retries++;
  } while ( ( ret == LIBUSB_ERROR_PIPE ) && ( retries < USB_WRITE_RETRIES ) );

  if( ret == LIBUSB_ERROR_PIPE ) {
    MSG_USB360SK_ERROR( "During Send : 'LIBUSB_ERROR_PIPE' failed to send data with length = " << length );
  }
  return ret;
};

void USB_360StageKit::End() {

  // Force Set Lights to send SK_ALL_OFF.
  this->EnableLights( false );
  this->EnableFog( false );
  this->EnableStrobe( false );

  // Release USB interfaces
  this->ReleaseInterfaces();

  // Close USB device
  if( m_usb_device_handle != NULL ) {
    libusb_close( m_usb_device_handle );
    m_usb_device_handle = NULL;
  }

  if( m_usb_context != NULL ) {
    libusb_exit( m_usb_context );
    m_usb_context = NULL;
  }
};

bool USB_360StageKit::SetStatusLEDs( uint8_t status_value ) {

  m_status_leds = status_value;

  int retVal = 0;

  if( ( m_usb_device_handle != NULL ) && m_status_leds_enabled ) {
    MSG_USB360SK_DEBUG( "Setting status LEDs to '" << static_cast<int16_t>( status_value ) << "'" );
    m_report_out[ 0 ] = 0x01;
    m_report_out[ 1 ] = 0x03;
    m_report_out[ 2 ] = status_value;
    m_report_out[ 3 ] = 0x00;
    m_report_out[ 4 ] = 0x00;
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_usb_device_handle, 
                                      LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE, // request_type
                                      HID_SET_REPORT,                                                          // request
                                      ( HID_REPORT_TYPE_OUTPUT << 8 ) | 0x00,                                  // value
                                      0,                                                                       // index
                                      m_report_out,                                                            // pointer to data buffer
                                      3,                                                                       // data buffer size
                                      USB_REQUEST_TIMEOUT );

  };

  return ( retVal < 0 ) ? false : true;
};

// Turns LEDs on & off based on the given values.
// lValue & rValue are the haptic weight controller values.
// lvalue = led_numbers
// rvalue = colour / item
bool USB_360StageKit::SetLights( uint8_t lValue, uint8_t rValue ) {

  int retVal = 0;
  if( ( m_usb_device_handle != NULL ) && ( m_lights_enabled ) ) {
    MSG_USB360SK_DEBUG( "Set lights - building USB out report." );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = lValue; // big weight
    m_report_out[ 4 ] = rValue; // small weight
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_usb_device_handle, 
                                      LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE, // request_type
                                      HID_SET_REPORT,                                                          // request
                                      ( HID_REPORT_TYPE_OUTPUT << 8 ) | 0x00,                                  // value
                                      0,                                                                       // index
                                      m_report_out,                                                            // pointer to data buffer
                                      8,                                                                       // data buffer size
                                      USB_REQUEST_TIMEOUT );

  };

  return ( retVal < 0 ) ? false : true;
};

bool USB_360StageKit::SetStrobe( uint8_t speed ) {
  int retVal = 0;
  if( ( m_usb_device_handle != NULL ) && ( m_strobe_enabled ) ) {
    MSG_USB360SK_DEBUG( "SetStrobe - building USB out report." );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = 0x00; // large weight
    switch( speed ) {
      case 1:
        m_report_out[ 4 ] = SK_STROBE_SPEED_1;
        break;
      case 2:
        m_report_out[ 4 ] = SK_STROBE_SPEED_2;
        break;
      case 3:
        m_report_out[ 4 ] = SK_STROBE_SPEED_3;
        break;
      case 4:
        m_report_out[ 4 ] = SK_STROBE_SPEED_4;
        break;
      default:
        m_report_out[ 4 ] = SK_STROBE_OFF;
        break;
    }
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_usb_device_handle, 
                                      LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE, // request_type
                                      HID_SET_REPORT,                                                          // request
                                      ( HID_REPORT_TYPE_OUTPUT << 8 ) | 0x00,                                  // value
                                      0,                                                                       // index
                                      m_report_out,                                                            // pointer to data buffer
                                      8,                                                                       // data buffer size
                                      USB_REQUEST_TIMEOUT );

  };

  return ( retVal < 0 ) ? false : true;
};

bool USB_360StageKit::SetFog( bool on ) {
  int retVal = 0;
  if( ( m_usb_device_handle != NULL ) && ( m_fog_enabled ) ) {
    MSG_USB360SK_DEBUG( "SetFog - building USB out report." );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = 0x00; // large weight
    if( on ) {
      m_report_out[ 4 ] = SK_FOG_ON;
    } else {
      m_report_out[ 4 ] = SK_FOG_OFF;
    }
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_usb_device_handle, 
                                      LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE, // request_type
                                      HID_SET_REPORT,                                                           // request
                                      ( HID_REPORT_TYPE_OUTPUT << 8 ) | 0x00,                                   // value
                                      0,                                                                        // index
                                      m_report_out,                                                             // pointer to data buffer
                                      8,                                                                        // data buffer size
                                      USB_REQUEST_TIMEOUT );

  };

  return ( retVal < 0 ) ? false : true;
};

// For testing only. Not required.
int USB_360StageKit::TestInterrupt( int position ) {
  unsigned char char_data[ 2 ];
  int ret;
  int transferred_amount;
  if( position == 1 ) {
    char_data[ 0 ] = 0x03;
    char_data[ 1 ] = 0x01;
    ret = libusb_interrupt_transfer( m_usb_device_handle,    // struct libusb_device_handle *  	dev_handle,
                                     0x00,                   // unsigned char  	endpoint,
                                     char_data,              // unsigned char *  	data,
                                     2,                      // int  	length,
                                     &transferred_amount,    // int *  	transferred,
                                     USB_REQUEST_TIMEOUT );  // unsigned int  	timeout
  } else {
    if( position == 2 ) {
      char_data[ 0 ] = 0x03;
      char_data[ 1 ] = 0x02;
      ret = libusb_interrupt_transfer( m_usb_device_handle,    // struct libusb_device_handle *  	dev_handle,
                                       0x00,                   // unsigned char  	endpoint,
                                       char_data,              // unsigned char *  	data,
                                       2,                      // int  	length,
                                       &transferred_amount,    // int *  	transferred,
                                       USB_REQUEST_TIMEOUT );  // unsigned int  	timeout
    }
  }

  return ret;
};

// Returns true if an update report is collected
bool USB_360StageKit::PollButtons() {
  if( m_usb_device_handle == NULL )
  {
    return false;
  }

  int ret = libusb_control_transfer( m_usb_device_handle, 
                                     LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE, // request type
                                     HID_GET_REPORT,                                                          // request
                                     ( HID_REPORT_TYPE_INPUT << 8 ) | 0x00,                                   // value
                                     0,                                                                       // index
                                     m_report_in,                                                             // pointer to data buffer
                                     STAGEKIT_MAX_INPUT_BUFFER,                                               // data buffer max size
                                     USB_REQUEST_TIMEOUT );

  if( ret < 0 )
  {
    return false;
  }

  // Check if it's the correct report - the controller also sends different status reports
  // report[ 0 ] : Report Type, should be 0x00
  // report[ 1 ] : Report Length, should be 0x14 (20 bytes long)
  if( m_report_in[ 0 ] == 0x00 && m_report_in[ 1 ] == 0x14 ) {
    // button state
    m_buttonstate = (uint32_t)(                 m_report_in[ 5 ]
                                | ( (uint16_t)  m_report_in[ 4 ] << 8 )
                                | ( (uint32_t)  m_report_in[ 3 ] << 16 )
                                | ( (uint32_t)  m_report_in[ 2 ] << 24 ) );

    if( m_buttonstate != m_buttonstate_old )
    {
      m_buttonstate_clicked = (m_buttonstate >> 16) & ((~m_buttonstate_old) >> 16);
      m_buttonstate_old = m_buttonstate;
    }
    return true;
  }

  return false;
};

uint16_t USB_360StageKit::GetButtons() {
  return m_buttonstate_clicked;
};


