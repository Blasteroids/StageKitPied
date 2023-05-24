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
#include "stagekit/StageKitManager.h"
#include "stagekit/StageKitConsts.h"
#include "leds/LEDArray.h"
#include "network/RB3E_Network.h"

//
#define USB_DIRECTION_IN 0x80
#define ALIVE_CHECK_ITR 1                // Check clients
#define ALIVE_CLEAR_ITR 20               // Remove clients

class RpiLightsController {
public:
  RpiLightsController( const char* ini_file );

  ~RpiLightsController();

  bool Start();

  long Update( const long time_passed_ms ); // Returns time to sleep in ms

  void Stop();

private:

  void SerialAdapter_Poll();

  void SerialAdapter_HandleControlData();

  void SerialAdapter_HandleOutReport();
  
  void RB3ENetwork_Poll();

  void Stagekit_ResetVariables();

  void StageKit_PollButtons( const long time_passed_ms );

  long Handle_TimeUpdate( const long time_passed_ms );

  bool Handle_StagekitConnect();

  void Handle_StagekitDisconnect();

  bool Handle_SerialConnect();

  void Handle_SerialDisconnect();

  void Handle_RumbleData( uint8_t left_weight, uint8_t right_weight );

  void Handle_LEDUpdate( const uint8_t colour, const uint8_t leds );

  void Handle_FogUpdate( bool fog_on_state );

  void Handle_StrobeUpdate( const uint8_t strobe_speed );

  SerialAdapter      mSerialAdapter;
  StageKitManager    mStageKitManager;
  LEDArray           mLEDS;
  INI_Handler        mINI_Handler;
  RB3E_Network       mRB3E_Network;
  
  bool               m_rb3e_listener_enabled;
  bool               m_rb3e_sender_enabled;
  std::string        m_rb3e_source_ip;
  uint16_t           m_rb3e_listening_port;  
  std::string        m_rb3e_target_ip;
  uint16_t           m_rb3e_target_port;  

  USB_ControlRequest m_control_request;
  unsigned char*     m_ptr_control_request_data;
  bool               m_serial_connected_to_x360;

  uint8_t            m_stagekit_colour_red;
  uint8_t            m_stagekit_colour_green;
  uint8_t            m_stagekit_colour_blue;
  uint8_t            m_stagekit_colour_yellow;
  uint8_t            m_stagekit_strobe_speed;
  long               m_stagekit_strobe_next_on_ms;

  uint8_t            m_colour_red;
  uint8_t            m_colour_green;
  uint8_t            m_colour_blue;
  uint8_t            m_colour_brightness;

  long               m_sleep_time;

  // LED array
  bool               m_leds_enabled;
  std::string*       m_leds_ini;
  uint16_t           m_leds_ini_amount;
  uint8_t            m_leds_ini_number;
  
  bool               m_leds_strobe_enabled;
  uint16_t           m_leds_strobe_rate[ 4 ];
  uint8_t            m_leds_strobe_speed_current;
  long               m_leds_strobe_next_on_ms;

  uint16_t           m_sleeptime_idle;
  uint16_t           m_sleeptime_stagekit;
  uint16_t           m_sleeptime_strobe;

  // NO DATA
  long               m_nodata_ms;
  long               m_nodata_ms_count;
  uint8_t            m_nodata_red;
  uint8_t            m_nodata_green;
  uint8_t            m_nodata_blue;
  uint8_t            m_nodata_brightness;
  
  long               m_button_check_delay;
};

#endif

