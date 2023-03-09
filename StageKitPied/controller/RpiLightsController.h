#ifndef _RPILIGHTSCONTROLLER_H_
#define _RPILIGHTSCONTROLLER_H_

#ifdef DEBUG
  #define MSG_RPLC_DEBUG( str ) do { std::cout << "RpiLightsController : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_RPLC_DEBUG( str ) do { } while ( false )
#endif

#define MSG_RPLC_ERROR( str ) do { std::cout << "RpiLightsController : ERROR : " << str << std::endl; } while( false )
#define MSG_RPLC_INFO( str ) do { std::cout << "RpiLightsController : INFO : " << str << std::endl; } while( false )

//
#include <iostream>
#include <string>
#include <cstdlib>  // system
#include <cstring>  // memcpy
#include <unistd.h> // readlink
#include <libgen.h> // dirname

//
#include "helpers/INI_Handler.h"
#include "helpers/SleepTimer.h"
#include "serial/SerialAdapter.h"
#include "stagekit/USB_ControlRequest.h"
#include "stagekit/USB_360StageKit.h"
#include "stagekit/StageKitConsts.h"
#include "leds/LEDArray.h"

//
#define USB_DIRECTION_IN 0x80
#define ALIVE_CHECK_ITR 1                // Check clients
#define ALIVE_CLEAR_ITR 20               // Remove clients

class RpiLightsController {
public:
  RpiLightsController( const char* ini_file );

  ~RpiLightsController();

  bool Start();

  uint16_t Update( uint16_t time_passed_MS ); // Returns time to sleep in ms

  void Stop();

private:

  void SerialAdapter_Poll();

  void SerialAdapter_HandleControlData();

  void SerialAdapter_HandleOutReport();

  void Stagekit_ResetVariables();

  void StageKit_PollButtons();

  uint16_t StageKit_Update( uint16_t time_passed_ms );

  bool Handle_StagekitConnect();

  void Handle_StagekitDisconnect();

  bool Handle_SerialConnect();

  void Handle_SerialDisconnect();

  void Handle_LEDUpdate( const uint8_t colour, const uint8_t leds );

  void Handle_FogUpdate( const bool fogOn );

  void Handle_StrobeUpdate( const uint8_t strobe_speed );

  SerialAdapter      mSerialAdapter;
  USB_360StageKit    mUSB_360StageKit;
  LEDArray           mLEDS;
  INI_Handler        mINI_Handler;

  USB_ControlRequest m_control_request;
  unsigned char*     m_ptr_control_request_data;
  bool               m_serial_connected_to_x360;

  uint8_t            m_stagekit_colour_red;
  uint8_t            m_stagekit_colour_green;
  uint8_t            m_stagekit_colour_blue;
  uint8_t            m_stagekit_colour_yellow;
  uint8_t            m_stagekit_strobe_speed;
  uint16_t           m_stagekit_strobe_rate_1_ms;
  uint16_t           m_stagekit_strobe_rate_2_ms;
  uint16_t           m_stagekit_strobe_rate_3_ms;
  uint16_t           m_stagekit_strobe_rate_4_ms;
  long               m_stagekit_strobe_next_on_ms;

  uint8_t            m_colour_red;
  uint8_t            m_colour_green;
  uint8_t            m_colour_blue;
  uint8_t            m_colour_brightness;

  long               m_sleep_time;

  std::string*       m_leds_ini;
  uint16_t           m_leds_ini_amount;
  uint8_t            m_leds_ini_number;

  uint16_t           m_sleeptime_idle;
  uint16_t           m_sleeptime_stagekit;
  uint16_t           m_sleeptime_strobe;
};

#endif

