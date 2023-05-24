#ifndef _RB3E_NETWORK_H_
#define _RB3E_NETWORK_H_

#ifdef DEBUG
  #define MSG_RB3E_NETWORK_DEBUG( str ) do { std::cout << "RB3E_Network : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_RB3E_NETWORK_DEBUG( str ) do { } while ( false )
#endif

#define MSG_RB3E_NETWORK_ERROR( str ) do { std::cout << "RB3E_Network : ERROR : " << str << std::endl; } while( false )
#define MSG_RB3E_NETWORK_INFO( str ) do { std::cout << "RB3E_Network : INFO : " << str << std::endl; } while( false )

#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <iomanip>
#include <bitset>

#include "network/RB3E_NetworkHelpers.h"

class RB3E_Network
{
public:
  RB3E_Network();

  ~RB3E_Network();

  bool StartReceiver( std::string& source_ip, uint16_t listening_port );
  
  bool StartSender( std::string& target_ip, uint16_t target_port );

  void Stop();

  bool Poll();  // Returns true if data.

  bool SendLightEvent( const uint8_t left_weight, const uint8_t right_weight );
 
  bool EventWasSongName();

  bool EventWasArtist();

  bool EventWasScore();

  bool EventWasStagekit();

  bool EventWasBandInfo();

  uint8_t GetWeightLeft();
  
  uint8_t GetWeightRight();

  uint32_t GetBandScore();

  uint8_t GetBandStars();

  bool PlayerExists( const uint8_t player_id );

  uint32_t GetPlayerScore( const uint8_t player_id );

  uint8_t GetPlayerDifficulty( const uint8_t player_id );

  uint8_t GetPlayerTrackType( const uint8_t player_id );

private:
  void DumpData();
  
  bool               m_is_sender;

  int                m_network_socket;
  uint32_t           m_expected_source_ip;
  
  struct sockaddr_in m_target_address;
  uint32_t           m_target_ip;

  uint8_t            m_data_buffer[ 512 ];
  ssize_t            m_data_buffer_last_size;

  uint8_t            m_event_type_last;
  uint8_t            m_game_state; // 0 - In menu   1 - In game

  std::string        m_song_name;
  std::string        m_song_name_short;
  std::string        m_song_artist;

  uint8_t            m_weight_left;
  uint8_t            m_weight_right;

  uint32_t           m_band_score;
  uint8_t            m_band_stars;
  uint8_t            m_player_exists[ 4 ];
  uint32_t           m_player_score[ 4 ];
  uint8_t            m_player_difficulty[ 4 ];
  uint8_t            m_player_track_type[ 4 ];  
};

#endif
