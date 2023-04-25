
#include "RB3E_Network.h"

RB3E_Network::RB3E_Network() {
  m_network_socket = -1;
  m_receive_buffer_last_size = 0;

  m_event_type_last = 0;
  m_game_state      = 0;
  m_weight_left     = 0;
  m_weight_right    = 0;
  m_band_score      = 0;
  m_band_stars      = 0;
  for( int i = 0; i < 4; i++ ) {
    m_player_exists[ i ] = 0;
    m_player_score[ i ] = 0;
    m_player_difficulty[ i ] = 0;
    m_player_track_type[ i ] = 0;
  }  
};

RB3E_Network::~RB3E_Network() {
  this->Stop();
};

// source_ip = The IP of the machine running RB3E
// listening_port = The local port to listen on for the RB3E data
bool RB3E_Network::Start( std::string& source_ip, uint16_t listening_port ) {
  if( m_network_socket != -1 ) {
    this->Stop();
  }

  MSG_RB3E_NETWORK_DEBUG( "Listening on port = " << listening_port << " : Expected source IP = " << source_ip );

  m_network_socket =  socket( AF_INET, SOCK_DGRAM, 0 );
  if( m_network_socket < 0 ) {
    MSG_RB3E_NETWORK_ERROR( "Failed to create socket." );
    this->Stop();
    return false;
  }

  int flags = fcntl( m_network_socket, F_GETFL );
  if( flags == -1 ) {
    MSG_RB3E_NETWORK_ERROR( "Failed to get socket flags." );
    this->Stop();
    return false;
  }

  if( fcntl( m_network_socket, F_SETFL, flags | O_NONBLOCK ) == -1 ) {
    MSG_RB3E_NETWORK_ERROR( "Failed to set socket as non-block." );
    this->Stop();
    return false;
  }

  struct sockaddr_in my_address;
  memset( &my_address, 0, sizeof( my_address ) );
  my_address.sin_family = AF_INET;
  my_address.sin_addr.s_addr = htonl( INADDR_ANY );
  my_address.sin_port = htons( listening_port );

  if( bind( m_network_socket, (struct sockaddr *)&my_address, sizeof( my_address ) ) < 0 ) {
    MSG_RB3E_NETWORK_ERROR( "Failed to bind socket." );
    this->Stop();
    return false;
  }

  MSG_RB3E_NETWORK_INFO( "Network socket created.  Listening port = " << listening_port );

  // Save expected source ip
  m_expected_source_ip = inet_addr( source_ip.c_str() );

  return true;
};

void RB3E_Network::Stop() {
  if( m_network_socket != -1 ) {
    close( m_network_socket );
    m_network_socket = -1;
  }
};

bool RB3E_Network::Poll() {
  if( m_network_socket == -1 ) {
    return false;
  }
  
  struct sockaddr_in senders_address;
  socklen_t senders_address_length = sizeof( senders_address );

  m_receive_buffer_last_size = recvfrom( m_network_socket,
                                         m_receive_buffer,
                                         sizeof( m_receive_buffer ),
                                         MSG_DONTWAIT,
                                         (struct sockaddr*)& senders_address,
                                         &senders_address_length );
 
  if( m_receive_buffer_last_size < 1 ) {
    return false;
  }

#ifdef DEBUG
  char ip[ INET_ADDRSTRLEN ];
  inet_ntop( AF_INET, &( senders_address.sin_addr ), ip, INET_ADDRSTRLEN );
  MSG_RB3E_NETWORK_DEBUG( "Received data from IP: " << ip << "    Port: " << ntohs( senders_address.sin_port ) << " Data Size = " << m_receive_buffer_last_size );
#endif

  if( m_expected_source_ip != 0 ) {
    if( m_expected_source_ip != ntohl( senders_address.sin_addr.s_addr ) ) {
      char source_ip[ INET_ADDRSTRLEN ];
      inet_ntop( AF_INET, &( senders_address.sin_addr ), source_ip, INET_ADDRSTRLEN );
      MSG_RB3E_NETWORK_INFO( "Ignoring packet from unexpected source : " << source_ip );
      return false;
    }
  }

  RB3E_EventPacket* packet = (RB3E_EventPacket*)&m_receive_buffer;
  if( ntohl( packet->Header.ProtocolMagic ) != RB3E_NETWORK_MAGICKEY ) {
    MSG_RB3E_NETWORK_INFO( "Incorrect RB3E magic key in packet." );
    m_receive_buffer_last_size = 0;
    return false;
  }

  // Process received data
  m_event_type_last = packet->Header.PacketType;

  switch( packet->Header.PacketType ) {
    case RB3E_EVENT_ALIVE:
      break;
    case RB3E_EVENT_STATE: {
      // content is a char - 00=menus, 01=ingame
      m_game_state = packet->Data[ 0 ];
      break;
    }
    case RB3E_EVENT_SONG_NAME: {
      // content is a string of the current song name
      m_song_name.assign( (char *)packet->Data, packet->Header.PacketSize );
      break;
    }
    case RB3E_EVENT_SONG_ARTIST: {
      // content is a string of the current song artist
      m_song_artist.assign( (char *)packet->Data, packet->Header.PacketSize );
      break;
    }
    case RB3E_EVENT_SONG_SHORTNAME: {
      // content is a string of the current shortname
      m_song_name_short.assign( (char *)packet->Data, packet->Header.PacketSize );
      break;
    }
    case RB3E_EVENT_SCORE: {
      // content is a RB3E_EventScore struct with score info
      RB3E_EventScore* score_event_data = (RB3E_EventScore *)packet->Data;
      m_player_score[ 0 ] = score_event_data->MemberScores[ 0 ];
      m_player_score[ 1 ] = score_event_data->MemberScores[ 1 ];
      m_player_score[ 2 ] = score_event_data->MemberScores[ 2 ];
      m_player_score[ 3 ] = score_event_data->MemberScores[ 3 ];
      m_band_stars = score_event_data->Stars;
      break;
    }
    case RB3E_EVENT_STAGEKIT: {
      // content is a RB3E_EventStagekit struct with stagekit info
      RB3E_EventStagekit* stagekit_event_data = (RB3E_EventStagekit *)packet->Data;
      m_weight_left  = stagekit_event_data->LeftChannel;
      m_weight_right = stagekit_event_data->RightChannel;
      break;
    }
    case RB3E_EVENT_BAND_INFO: {
      // content is a RB3E_EventBandInfo struct with band info
      RB3E_EventBandInfo* bandinfo_event_data = (RB3E_EventBandInfo *)packet->Data;
      for( int i = 0; i < 4; i++ ) {
        m_player_exists[ i ]     = bandinfo_event_data->MemberExists[ i ];
        m_player_difficulty[ i ] = bandinfo_event_data->Difficulty[ i ];
        m_player_track_type[ i ] = bandinfo_event_data->TrackType[ i ];
      }
      break;
    }
    default: {
      MSG_RB3E_NETWORK_ERROR( "RB3E_EVENT_UNKNOWN: " << std::hex << std::setw( 2 ) << std::setfill( '0' ) << packet->Header.PacketType );
      break;
    }
  }

#ifdef DEBUG
  this->DumpData();
#endif

  return true;
};

bool RB3E_Network::EventWasSongName() {
  return m_event_type_last == RB3E_EVENT_SONG_NAME;
};

bool RB3E_Network::EventWasArtist() {
  return m_event_type_last == RB3E_EVENT_SONG_ARTIST;
};

bool RB3E_Network::EventWasScore() {
  return m_event_type_last == RB3E_EVENT_SCORE;
};

bool RB3E_Network::EventWasStagekit() {
  return m_event_type_last == RB3E_EVENT_STAGEKIT;
};

bool RB3E_Network::EventWasBandInfo() {
  return m_event_type_last == RB3E_EVENT_BAND_INFO;
};

uint8_t RB3E_Network::GetWeightLeft() {
  return m_weight_left;
};

uint8_t RB3E_Network::GetWeightRight() {
  return m_weight_right;
};

uint32_t RB3E_Network::GetBandScore() {
  return m_band_score;
};

uint8_t RB3E_Network::GetBandStars() {
  return m_band_stars;
};

bool RB3E_Network::PlayerExists( uint8_t player_id ) {
  if( player_id < 4 ) {
    return m_player_exists[ player_id ] == 0 ? false:true;
  }

  return false;
};

uint32_t RB3E_Network::GetPlayerScore( uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_score[ player_id ];
};

uint8_t RB3E_Network::GetPlayerDifficulty( uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_difficulty[ player_id ];
};

uint8_t RB3E_Network::GetPlayerTrackType( uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_track_type[ player_id ];
};


void RB3E_Network::DumpData() {
  bool dump_raw_data = false;
  
  RB3E_EventPacket* packet = (RB3E_EventPacket*)&m_receive_buffer;

  switch( packet->Header.PacketType ) {
    case RB3E_EVENT_ALIVE:
      break;
    case RB3E_EVENT_STATE: {
      // content is a char - 00=menus, 01=ingame
      std::cout << "RB3E_EVENT_STATE: ";
      switch( packet->Data[ 0 ] ) {
        case 0:
          std::cout << "In menu.";
          break;
        case 1:
          std::cout << "In game.";
          break;
        default:
          std::cout << "Unknown.";
          break;
      }
      std::cout << std::endl;
      break;
    }
    case RB3E_EVENT_SONG_NAME: {
      // content is a string of the current song name
      std::cout << "RB3E_EVENT_SONG_NAME: ";
      for( int i = 0; i < packet->Header.PacketSize; i++ ) {
         std::cout << packet->Data[ i ];
      }
      std::cout << std::endl;
      break;
    }
    case RB3E_EVENT_SONG_ARTIST: {
      // content is a string of the current song artist
      std::cout << "RB3E_EVENT_SONG_ARTIST: ";
      for( int i = 0; i < packet->Header.PacketSize; i++ ) {
         std::cout << packet->Data[ i ];
      }
      std::cout << std::endl;
      break;
    }
    case RB3E_EVENT_SONG_SHORTNAME: {
      // content is a string of the current shortname
      std::cout << "RB3E_EVENT_SONG_SHORTNAME: ";
      for( int i = 0; i < packet->Header.PacketSize; i++ ) {
         std::cout << packet->Data[ i ];
      }
      std::cout << std::endl;
      break;
    }
    case RB3E_EVENT_SCORE: {
      // content is a RB3E_EventScore struct with score info
      RB3E_EventScore* score_event_data = (RB3E_EventScore *)packet->Data;
      std::cout << "RB3E_EVENT_SCORE:" << std::endl;
      std::cout << " BAND = " << static_cast<int>( score_event_data->TotalScore );
      std::cout << " P1 = " << static_cast<int>( score_event_data->MemberScores[ 0 ] );
      std::cout << " P2 = " << static_cast<int>( score_event_data->MemberScores[ 1 ] );
      std::cout << " P3 = " << static_cast<int>( score_event_data->MemberScores[ 2 ] );
      std::cout << " P4 = " << static_cast<int>( score_event_data->MemberScores[ 3 ] );
      std::cout << " STARS = " << score_event_data->Stars << std::endl;  // Not sure how stars are represent.
      break;
    }
    case RB3E_EVENT_STAGEKIT: {
      // content is a RB3E_EventStagekit struct with stagekit info
      RB3E_EventStagekit* stagekit_event_data = (RB3E_EventStagekit *)packet->Data;
      std::cout << "RB3E_EVENT_STAGEKIT:";
      std::cout << "  LEFT WEIGHT: " << std::bitset<8>( stagekit_event_data->LeftChannel );
      std::cout << "  RIGHT WEIGHT: " << std::bitset<8>( stagekit_event_data->RightChannel ) << std::endl;
      break;
    }
    case RB3E_EVENT_BAND_INFO: {
      // content is a RB3E_EventBandInfo struct with band info
      RB3E_EventBandInfo* bandinfo_event_data = (RB3E_EventBandInfo *)packet->Data;
      std::cout << "RB3E_EVENT_BAND_INFO:";
      for( int i = 0; i < 4; i++ ) {
        std::cout << " P" << (i+1) << " = " << static_cast<int>( bandinfo_event_data->MemberExists[ i ] );
        std::cout << " D" << static_cast<int>( bandinfo_event_data->Difficulty[ i ] );
        std::cout << " T" << static_cast<int>( bandinfo_event_data->TrackType[ i ] );
      }
      std::cout << std::endl;
      break;
    }
    default: {
      std::cout << "RB3E_EVENT_UNKNOWN: ";
      std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << packet->Header.PacketType << std::endl;
      break;
    }
  }

  if( dump_raw_data ) {
    for( int i = 0; i < m_receive_buffer_last_size; i++ ) {
      std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<int>( m_receive_buffer[ i ] ) << " ";
    }  
    std::cout << std::endl;
  }

};
