#ifndef _STAGEKITMANAGER_H_
#define _STAGEKITMANAGER_H_

#ifdef DEBUG
  #define MSG_STAGEKITMANAGER_DEBUG( str ) do { std::cout << "StageKitManager : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_STAGEKITMANAGER_DEBUG( str ) do { } while ( false )
#endif

#define MSG_STAGEKITMANAGER_ERROR( str ) do { std::cout << "StageKitManager : ERROR : " << str << std::endl; } while( false )
#define MSG_STAGEKITMANAGER_INFO( str ) do { std::cout << "StageKitManager : INFO : " << str << std::endl; } while( false )

#define STAGEKIT_VID 0x0E6F
#define STAGEKIT_PID 0x0103

#include <iostream>
#include <iomanip>
#include "libusb.h"

#include "stagekit/USB_360StageKit.h"
#include "stagekit/StageKitConfig.h"

// Set as 4, because apparently there's only 4 in existence ;D
#define MAX_STAGEKITS_IN_EXISTENCE 4

// StageKitConfig - 4 Configurations, one for each light segment.
// Each stagekit can be assigned to any of the 4 configs.

class StageKitManager {
public:
  StageKitManager();

  ~StageKitManager();

  uint8_t Init(); // Returns amount of found stage kits

  // USB passthrough to StageKit[ 0 ]
  int Send( USB_ControlRequest* ptr_control_request, unsigned short length );

  void End();
  
  uint8_t AmountOfStageKits();
  
  bool IsConnected( const uint8_t stagekit_id = 0 );

  bool PollButtons( const uint8_t stagekit_id ); // Returns true if buttons have changed

  uint16_t GetButtons( const uint8_t stagekit_id );

  void SetConfigIDForStageKit( const uint8_t stagekit_id, const uint8_t config_id );
  
  int8_t GetConfigIDForStageKit( const uint8_t stagekit_id );
  
  // Configs
  void ConfigEnableLights( const uint8_t config_id, const bool on );

  void ConfigEnableStrobe( const uint8_t config_id, const bool on );

  void ConfigEnableFog( const uint8_t config_id, const bool on );

  void ConfigSetFogTimes( const uint8_t config_id, const long instance_time_max_ms, const long total_max_time_per_song_ms );

  bool ConfigHasLightsEnabled( const uint8_t config_id );

  bool ConfigHasFogEnabled( const uint8_t config_id );

  bool ConfigHasStrobeEnabled( const uint8_t config_id );

  bool ConfigHasAnythingEnabled();
  
  // Apply changes
  void SetLights( const uint8_t left_weight, const uint8_t right_weight );

  void SetStrobe( const uint8_t speed ); // Speed = 0 - 4.  0 = Off.

  void SetFog( const bool on );
  
  void Handle_TimeUpdate( const long time_passed_ms );
  
  void Handle_SongChange();
  
private:
  bool SetStatusLEDs( uint8_t stagekit_id, uint8_t status_value );
  
  uint8_t         m_amount_of_stagekits;
  USB_360StageKit m_stagekit[ MAX_STAGEKITS_IN_EXISTENCE ];
  uint8_t         m_stagekit_config_number[ MAX_STAGEKITS_IN_EXISTENCE ];
  StageKitConfig  m_stagekit_config[ 5 ];  // 0 = off, then 1 for each light segment
  libusb_context* m_usb_context;
  bool            m_fog_current_state_is_on;
  bool            m_fog_just_changed_to_off;
  long            m_fog_instance_time_current;
  long            m_fog_total_time_current;

};

#endif
