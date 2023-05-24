#ifndef _USB_360STAGEKIT_H_
#define _USB_360STAGEKIT_H_

#ifdef DEBUG
  #define MSG_USB360SK_DEBUG( str ) do { std::cout << "USB_360StageKit : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_USB360SK_DEBUG( str ) do { } while ( false )
#endif

#define MSG_USB360SK_ERROR( str ) do { std::cout << "USB_360StageKit : ERROR : " << str << std::endl; } while( false )
#define MSG_USB360SK_INFO( str ) do { std::cout << "USB_360StageKit : INFO : " << str << std::endl; } while( false )


#include <iostream>
#include "libusb.h"

#include "stagekit/USB_ControlRequest.h"
#include "stagekit/StageKitConfig.h"
#include "stagekit/StageKitConsts.h"

// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03

//#define STAGEKIT_VID 0x0E6F
//#define STAGEKIT_PID 0x0103

// Was 255
#define STAGEKIT_MAX_INPUT_BUFFER 20
#define USB_REQUEST_TIMEOUT 1000
#define USB_WRITE_RETRIES 3

class USB_360StageKit {
public:
  USB_360StageKit();

  ~USB_360StageKit();

  bool Init( libusb_device_handle* ptr_usb_device_handle );

  bool IsConnected();

  int Send( USB_ControlRequest* ptr_control_request, unsigned short length );

  void End();

  bool PollButtons(); // Returns true if buttons have changed

  uint16_t GetButtons();

  void SetConfig( StageKitConfig* ptr_config );

  bool UpdateStatusLEDs( const uint8_t status_value );

  bool UpdateLights( const uint8_t left_weight, const uint8_t right_weight );

  bool UpdateStrobe( const uint8_t speed );

  bool UpdateFog( const bool on );

private:
  bool ClaimInterfaces();

  bool ReleaseInterfaces();

  bool SetStatusLEDs( const uint8_t status_value );
  
  bool SetLights( const uint8_t left_weight, const uint8_t right_weight ); // Rumble/SK-LED data
  
  bool SetStrobe( const uint8_t speed ); // Speed = 0 - 4.  0 - Off.

  bool SetFog( const bool on );
  
  libusb_device_handle* m_ptr_usb_device_handle;

  uint8_t m_report_in[ STAGEKIT_MAX_INPUT_BUFFER ]; // For polling buttons
  uint8_t m_report_out[ 8 ];  // For sending rumble/SK-LED data to the device

  StageKitConfig* m_ptr_stagekit_config;
  
  // Button states
  uint32_t m_buttonstate;
  uint32_t m_buttonstate_old;
  uint16_t m_buttonstate_clicked;

};

#endif
