
#include "USB_360StageKit.h"

USB_360StageKit::USB_360StageKit() {
  m_ptr_usb_device_handle = NULL;
  m_ptr_stagekit_config   = NULL;
};

USB_360StageKit::~USB_360StageKit() {
  this->End();
};

bool USB_360StageKit::Init( libusb_device_handle* ptr_usb_device_handle ) {
  if( m_ptr_usb_device_handle ) {
    MSG_USB360SK_ERROR( "Already connected to a Stage Kit" );
    return false;
  }
  
  m_ptr_usb_device_handle = ptr_usb_device_handle;
  
  if( libusb_set_auto_detach_kernel_driver( m_ptr_usb_device_handle, 1 ) ) {
    MSG_USB360SK_ERROR( "libusb_set_auto_detach_kernal_driver" );
  }

  if( libusb_reset_device( m_ptr_usb_device_handle ) ) {
    MSG_USB360SK_ERROR( "libusb_reset_device" );
    return false;
  }

  int config;
  if( libusb_get_configuration( m_ptr_usb_device_handle, &config ) ) {
    MSG_USB360SK_ERROR( "libusb_get_configuration" );
    return false;
  }

  if( config == 0 ) {
    config = 1;
    libusb_set_configuration( m_ptr_usb_device_handle, 1 );
  }

  if( !this->ClaimInterfaces() ) {
    return false;
  }
  MSG_USB360SK_DEBUG( "USB Device claimed." );

  // When interfaces are claimed the POD likes to blink the status LED.
  // Make them rotate to show pod is active.
  if( !this->SetStatusLEDs( SKSTATUSLEDS::SK_STATUS_BLINK_ALL ) ) {
    MSG_USB360SK_ERROR( "Unable to set status LEDs." );
  }

  return true;
};

bool USB_360StageKit::IsConnected() {
  return ( m_ptr_usb_device_handle != NULL );
};

int USB_360StageKit::Send( USB_ControlRequest* ptr_control_request, unsigned short length ) {
  int ret;
  int retries = 0;

  do {
    ret = libusb_control_transfer( m_ptr_usb_device_handle,                     // Device/Handle
                                   ptr_control_request->header.bRequestType,    // LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE
                                   ptr_control_request->header.bRequest,        // HID_GET_REPORT
                                   ptr_control_request->header.wValue,          // ( HID_REPORT_TYPE_INPUT << 8 ) | 0x00
                                   ptr_control_request->header.wIndex,          // 0
                                   ptr_control_request->data,                   // report
                                   length,                                      // report_length
                                   USB_REQUEST_TIMEOUT );                       // 1000

    if( ret == LIBUSB_ERROR_PIPE ) {
      libusb_clear_halt( m_ptr_usb_device_handle, 0 );
    }
    retries++;
  } while ( ( ret == LIBUSB_ERROR_PIPE ) && ( retries < USB_WRITE_RETRIES ) );

  if( ret == LIBUSB_ERROR_PIPE ) {
    MSG_USB360SK_ERROR( "During Send : 'LIBUSB_ERROR_PIPE' failed to send data with length = " << length );
  }
  return ret;
};

void USB_360StageKit::End() {
  // Ensure nothing is left on
  this->SetFog( false );
  this->SetStrobe( 0 );
  this->SetLights( SKRUMBLEDATA::SK_NONE, SKRUMBLEDATA::SK_ALL_OFF );
  
  // Release USB interfaces
  this->ReleaseInterfaces();

  // Close USB device
  if( m_ptr_usb_device_handle != NULL ) {
    libusb_close( m_ptr_usb_device_handle );
    m_ptr_usb_device_handle = NULL;
  }
};

// Returns true if an update report is collected
bool USB_360StageKit::PollButtons() {
  if( m_ptr_usb_device_handle == NULL )
  {
    MSG_USB360SK_DEBUG( "PollButtons : No Handle." );
    return false;
  }

  int ret = libusb_control_transfer( m_ptr_usb_device_handle, 
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
      m_buttonstate_clicked = ( m_buttonstate >> 16 ) & ( ( ~m_buttonstate_old ) >> 16 );
      m_buttonstate_old     = m_buttonstate;
      MSG_USB360SK_DEBUG( "PollButtons : Button state change." );
      return true;
    }
  }
  return false;
};

uint16_t USB_360StageKit::GetButtons() {
  return m_buttonstate_clicked;
};

void USB_360StageKit::SetConfig( StageKitConfig* ptr_config ) {
  if( !ptr_config ) {
    return;
  }

  m_ptr_stagekit_config = ptr_config;

  if( !m_ptr_stagekit_config->m_light_pod_enabled ) {
    this->SetLights( SKRUMBLEDATA::SK_NONE, SKRUMBLEDATA::SK_ALL_OFF );
  }

  if( !m_ptr_stagekit_config->m_strobe_enabled ) {
    this->SetStrobe( 0 );
  }

  if( !m_ptr_stagekit_config->m_fog_enabled ) {
    this->SetFog( false );
  }
};

bool USB_360StageKit::UpdateStatusLEDs( const uint8_t status_value ) {
  return this->SetStatusLEDs( status_value );
};

bool USB_360StageKit::UpdateLights( const uint8_t left_weight, const uint8_t right_weight ) {
  if( !m_ptr_stagekit_config ) {
    MSG_USB360SK_DEBUG( "Update lights called without a valid config." );
    return false;
  }

  if( !m_ptr_stagekit_config->m_light_pod_enabled ) {
    return false;
  }

  return this->SetLights( left_weight, right_weight );
};

bool USB_360StageKit::UpdateStrobe( const uint8_t speed ) {
  if( !m_ptr_stagekit_config ) {
    MSG_USB360SK_DEBUG( "Update strobe called without a valid config." );
    return false;
  }

  if( !m_ptr_stagekit_config->m_strobe_enabled && speed != 0 ) {
    return false;
  }

  return this->SetStrobe( speed );

};

bool USB_360StageKit::UpdateFog( const bool on ) {
  if( !m_ptr_stagekit_config ) {
    MSG_USB360SK_DEBUG( "Update fog called without a valid config." );
    return false;
  }

  if( !m_ptr_stagekit_config->m_fog_enabled && !on ) {
    return false;
  }

  return this->SetFog( on );

};

bool USB_360StageKit::ClaimInterfaces() {
  if( m_ptr_usb_device_handle == NULL ) {
    return false;
  }

  libusb_device*                   ptr_usb_device = libusb_get_device( m_ptr_usb_device_handle );
  struct libusb_device_descriptor  usb_device_descriptor;

  if( libusb_get_device_descriptor( ptr_usb_device, &usb_device_descriptor ) ) {
    MSG_USB360SK_ERROR( "ClaimInterfaces : 'libusb_get_device_descriptor'" );
    return false;
  }

  if( usb_device_descriptor.bNumConfigurations ) {

    struct libusb_config_descriptor* configuration;
    if( libusb_get_config_descriptor( ptr_usb_device, 0, &configuration ) ) {
      MSG_USB360SK_ERROR( "ClaimInterfaces : 'libusb_get_config_descriptor'" );
      return false;
    }

    int err;

    for( int interfaceIndex = 0; interfaceIndex < configuration->bNumInterfaces; ++interfaceIndex ) {
      const struct libusb_interface* interface = configuration->interface + interfaceIndex;
      err = libusb_claim_interface( m_ptr_usb_device_handle,  interface->altsetting->bInterfaceNumber);
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
  if( m_ptr_usb_device_handle == NULL ) {
    return false;
  }

  libusb_device*                   ptr_usb_device = libusb_get_device( m_ptr_usb_device_handle );
  struct libusb_device_descriptor  usb_device_descriptor;

  if( libusb_get_device_descriptor( ptr_usb_device, &usb_device_descriptor ) ) {
    MSG_USB360SK_ERROR( "ReleaseInterfaces : 'libusb_get_device_descriptor'" );
    return false;
  }

  if( usb_device_descriptor.bNumConfigurations ) {

    struct libusb_config_descriptor* configuration;
    if( libusb_get_config_descriptor( ptr_usb_device, 0, &configuration ) ) {
      MSG_USB360SK_ERROR( "ReleaseInterfaces : Error : 'libusb_get_config_descriptor'" );
      return false;
    }

    int err;

    for( int interfaceIndex = 0; interfaceIndex < configuration->bNumInterfaces; ++interfaceIndex ) {
      const struct libusb_interface* interface = configuration->interface + interfaceIndex;
      err = libusb_release_interface( m_ptr_usb_device_handle,  interface->altsetting->bInterfaceNumber );
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

bool USB_360StageKit::SetStatusLEDs( const uint8_t status_value ) {
  int retVal = 0;
  if( m_ptr_usb_device_handle != NULL ) {
    MSG_USB360SK_DEBUG( "USB building out report: Setting status LEDs to '" << +status_value << "'" );
    m_report_out[ 0 ] = 0x01;
    m_report_out[ 1 ] = 0x03;
    m_report_out[ 2 ] = status_value;
    m_report_out[ 3 ] = 0x00;
    m_report_out[ 4 ] = 0x00;
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_ptr_usb_device_handle, 
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
// left_weight & right_weight are the haptic weight controller values.
// left_weight = led_numbers
// right_weight = colour or item
bool USB_360StageKit::SetLights( const uint8_t left_weight, const uint8_t right_weight ) {
  int retVal = 0;
  if( m_ptr_usb_device_handle != NULL ) {
    MSG_USB360SK_DEBUG( "USB building out report: Set lights." );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = left_weight; // big weight
    m_report_out[ 4 ] = right_weight; // small weight
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_ptr_usb_device_handle, 
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

bool USB_360StageKit::SetStrobe( const uint8_t speed ) {
  int retVal = 0;
  if( m_ptr_usb_device_handle != NULL ) {
    MSG_USB360SK_DEBUG( "USB building out report: SetStrobe ( " << +speed << " )" );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = 0x00; // large weight
    switch( speed ) {
      case 1:
        m_report_out[ 4 ] = SKRUMBLEDATA::SK_STROBE_SPEED_1;
        break;
      case 2:
        m_report_out[ 4 ] = SKRUMBLEDATA::SK_STROBE_SPEED_2;
        break;
      case 3:
        m_report_out[ 4 ] = SKRUMBLEDATA::SK_STROBE_SPEED_3;
        break;
      case 4:
        m_report_out[ 4 ] = SKRUMBLEDATA::SK_STROBE_SPEED_4;
        break;
      default:
        m_report_out[ 4 ] = SKRUMBLEDATA::SK_STROBE_OFF;
        break;
    }
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_ptr_usb_device_handle, 
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

bool USB_360StageKit::SetFog( const bool on ) {
  int retVal = 0;
  if( m_ptr_usb_device_handle != NULL ) {
    MSG_USB360SK_DEBUG( "USB building out report: SetFog " << (on? "on" : "off") );
    m_report_out[ 0 ] = 0x00;
    m_report_out[ 1 ] = 0x08;
    m_report_out[ 2 ] = 0x00;
    m_report_out[ 3 ] = 0x00; // large weight
    if( on ) {
      m_report_out[ 4 ] = SKRUMBLEDATA::SK_FOG_ON;
    } else {
      m_report_out[ 4 ] = SKRUMBLEDATA::SK_FOG_OFF;
    }
    m_report_out[ 5 ] = 0x00;
    m_report_out[ 6 ] = 0x00;
    m_report_out[ 7 ] = 0x00;

    retVal = libusb_control_transfer( m_ptr_usb_device_handle, 
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
