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

#define STAGEKIT_VID 0x0E6F
#define STAGEKIT_PID 0x0103

#define STAGEKIT_MAX_INPUT_BUFFER 255
#define USB_REQUEST_TIMEOUT 1000
#define USB_WRITE_RETRIES 3

class USB_360StageKit {
public:
  USB_360StageKit();

  ~USB_360StageKit();

  bool Init();

  bool IsConnected();

  void EnableStatusLEDs( bool on );

  void EnableLights( bool on );

  void EnableFog( bool on );

  void EnableStrobe( bool on );

  bool LightsEnabled();

  bool FogEnabled();

  bool StrobeEnabled();

  int Send( USB_ControlRequest* ptr_control_request, unsigned short length );

  void End();

  // Buttons data

  bool PollButtons(); // Returns true if buttons have changed

  uint16_t GetButtons();

  // Status LED.  Use SK_STATUS_x
  bool SetStatusLEDs( uint8_t status_value );

  // Rumble/SK-LED data
  // lValue & rValue are the haptic weight controller values.
  // lvalue = led_numbers
  // rvalue = colour / item
  bool SetLights( uint8_t lValue, uint8_t rValue );

  // Speed = 0 - 4.  0 - Off.
  bool SetStrobe( uint8_t speed );

  bool SetFog(bool on);

  int TestInterrupt( int position );

private:
  bool ClaimInterfaces();

  bool ReleaseInterfaces();

  libusb_device_handle* m_usb_device_handle;
  libusb_context*       m_usb_context;

  uint8_t      m_report_in[ STAGEKIT_MAX_INPUT_BUFFER ]; // For polling buttons
  uint8_t      m_report_out[ 8 ];  // For sending rumble/SK-LED data to the device

  bool m_status_leds_enabled; // If false will not show the current xbox LED status.
  bool m_lights_enabled;      // If false will suppress sending the light data to the Stage Kit.
  bool m_fog_enabled;         // If false will suppress sending the fog data to the Stage Kit.
  bool m_strobe_enabled;      // If false will suppress sending the strobe data to the Stage Kit.

  // Current state of the status LEDs
  uint8_t m_status_leds;

  // Button states
  uint32_t m_buttonstate;
  uint32_t m_buttonstate_old;
  uint16_t m_buttonstate_clicked;

};

#endif
