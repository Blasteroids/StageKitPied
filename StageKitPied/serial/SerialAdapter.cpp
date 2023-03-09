
#include "SerialAdapter.h"

SerialAdapter::SerialAdapter() {
  m_filedescriptor = -1;
  m_status = -1;

  time_t sec = SERIALADAPTER_DEFAULT_TIMEOUT / 1000;
  __suseconds_t usec = (SERIALADAPTER_DEFAULT_TIMEOUT - sec * 1000) * 1000;
  m_timeout = {.tv_sec = sec, .tv_usec = usec};

  m_poll_timeout_msecs = 10;

};

SerialAdapter::~SerialAdapter() {
  this->Close();
};

void SerialAdapter::Close() {
  if( m_filedescriptor != -1 ) {
    // Try to turn the adapter off
    this->Reset();

    close( m_filedescriptor );
    m_filedescriptor = -1;
    m_status = -1;
  }
};

int SerialAdapter::PayloadType() {
  return m_header[ 0 ];
};

unsigned char* SerialAdapter::Payload() {
  return m_payload;
};

int SerialAdapter::PayloadLength() {
  return m_payload_length;
};

bool SerialAdapter::Init( const char* path, bool surpress_warnings ) {
  if( m_filedescriptor != -1 ) {
    return false;
  }

  m_filedescriptor = open( path, O_RDWR | O_NOCTTY | O_NONBLOCK );
  if( m_filedescriptor < 0 ) {
    return false;
  }

  struct termios options;

  if( tcgetattr( m_filedescriptor, &options ) < 0 ) {
    return false;
  }

  speed_t baudrate = SERIALADAPTER_DEFAULT_BAUDRATE;

  cfsetispeed( &options, baudrate );
  cfsetospeed( &options, baudrate );
  cfmakeraw( &options );

  if( tcsetattr( m_filedescriptor, TCSANOW, &options ) < 0) {
    return false;
  }

  tcflush( m_filedescriptor, TCIFLUSH );

  if( !this->GetType() ) {
    MSG_SERIALADAPTER_ERROR( "Failed to get adapter type." );
    return false;
  }

  if( m_type == ADAPTER_TYPE_X360SK ) {
    MSG_SERIALADAPTER_INFO( "Correct adapter type found = X360SK" );
  } else {
    if( m_type == ADAPTER_TYPE_X360 ) {
      MSG_SERIALADAPTER_ERROR( "Unsupported adapter type found = X360" );
    } else if( m_type == ADAPTER_TYPE_XONE ) {
      MSG_SERIALADAPTER_ERROR( "Unsupported adapter type found = XONE" );
    } else {
      MSG_SERIALADAPTER_ERROR( "Unknown adapter type found." );
    }

    this->DumpData( true );

    return false;
  }

  // Get version
  if( !this->GetVersion() ) {
    MSG_SERIALADAPTER_ERROR( "Failed to get adapter version." );

    this->DumpData( true );

    return false;
  }
  MSG_SERIALADAPTER_INFO( "Adapter version = " << m_version_major << "." << m_version_minor );

  if( !this->Start() ) {
    MSG_SERIALADAPTER_ERROR( "Serial Adapter : ERROR : Failed to start." );
    return false;
  }

  // Setup polling options
  m_poll_fds[ 0 ].fd = m_filedescriptor;
  m_poll_fds[ 0 ].events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;  // input/output/error/disconnect/fd invalid.  

  // Save warnings surpress
  m_surpress_warnings = surpress_warnings;

  return true;
};

int SerialAdapter::Write( unsigned char* buffer, unsigned int count )
{
#ifdef DEBUG
  MSG_SERIALADAPTER_DEBUG( "Write.." );

  this->Dump( buffer, count );
#endif

  unsigned int bytes_written = 0;
  int write_amount;

  fd_set writefds;

  while( bytes_written != count ) {
    FD_ZERO( &writefds );
    FD_SET( m_filedescriptor, &writefds );
    int status = select( m_filedescriptor + 1, NULL, &writefds, NULL, &m_timeout );
    if( status > 0 ) {
      if( FD_ISSET( m_filedescriptor, &writefds ) ) {
        write_amount = write( m_filedescriptor, buffer + bytes_written, count - bytes_written );
        if( write_amount > 0 ) {
          bytes_written += write_amount;
        }
      }
    } else if( errno == EINTR ) {
      continue;
    } else {
      if( !m_surpress_warnings ) {
        MSG_SERIALADAPTER_INFO( "*WARNING* Select error failed." );
      }
      break;
    }
  }

  return bytes_written;
};

int SerialAdapter::Read( unsigned char* buffer, unsigned int count ) {

  unsigned int bytes_read = 0;
  int read_amount;

  fd_set readfds;

  while( bytes_read != count ) {
    FD_ZERO( &readfds );
    FD_SET( m_filedescriptor, &readfds );
    int status = select( m_filedescriptor + 1, &readfds, NULL, NULL, &m_timeout );
    if( status > 0 ) {
      if( FD_ISSET( m_filedescriptor, &readfds ) ) {
        read_amount = read( m_filedescriptor, buffer + bytes_read, count - bytes_read );
        if( read_amount > 0 ) {
          bytes_read += read_amount;
        }
      }
    } else if( status < 0 ) {
      if( errno == EINTR ) {
        continue;
      }
      if( !m_surpress_warnings ) {
        MSG_SERIALADAPTER_INFO( "*WARNING* Select error failed." );
      }
    } else {
      if( !m_surpress_warnings ) {
        MSG_SERIALADAPTER_INFO( "*WARNING* Select timeout." );
      }
      break; // timeout
    }
  }

#ifdef DEBUG
  MSG_SERIALADAPTER_DEBUG( "Read.." );

  this->Dump( buffer, bytes_read );
#endif

  return bytes_read;
};

bool SerialAdapter::WaitForReply( int header_value, int payload_length ) {
  int read_amount;
  int timeout = 10000;

  while( timeout-- > 0 ) {
    // Find header
    read_amount = this->Read( m_header, 2 );
    if( read_amount != 2 ) {
      if( !m_surpress_warnings ) {
        MSG_SERIALADAPTER_INFO( "*WARNING* Header size miss-match.  Received " << read_amount );
      }
      return false;
    }

    if( m_header[ 1 ] > 0 ) {
      read_amount = this->Read( m_payload, m_header[ 1 ] );
      if( read_amount != payload_length ) {
        if( !m_surpress_warnings ) {
          MSG_SERIALADAPTER_INFO( "*WARNING* Malformed payload length : Expected " << payload_length << " : Received " << read_amount );
        }
        return false;
      }
    }

    // Check this packet is the command response.
    if( m_header[ 0 ] == header_value ) {
      return true;
    }
  }

  if( !m_surpress_warnings ) {
    MSG_SERIALADAPTER_INFO( "*WARNING* WaitForReply Timeout." );
  }

  return false;
};

// Gets the type of adapter.
bool SerialAdapter::GetType() {
  m_header[ 0 ] = HEADER_GET_TYPE;
  m_header[ 1 ] = 0x00;

  if( this->Write( m_header, 2 ) != 2 ) {
    return false;
  }

  // Wait for the reply : Reply contains packet size 1
  if( !this->WaitForReply( HEADER_GET_TYPE, 1 ) ) {
    return false;
  }

  m_type = m_payload[ 0 ];

  return true;
};

bool SerialAdapter::GetVersion() {
  m_header[ 0 ] = HEADER_VERSION;
  m_header[ 1 ] = 0x00;

  if( this->Write( m_header, 2 ) != 2 ) {
    return false;
  }

  // Wait for the reply : Reply contains packet size 2
  if( !this->WaitForReply( HEADER_VERSION, 2 ) ) {
    return false;
  }

  m_version_major = m_payload[ 0 ];
  m_version_minor = m_payload[ 1 ];

  return true;
};

bool SerialAdapter::GetBaudrate() {
  m_header[ 0 ] = HEADER_BAUDRATE;
  m_header[ 1 ] = 0x00;

  if( this->Write( m_header, 2 ) != 2 ) {
    return false;
  }

    // Wait for the reply : Reply contains packet size 1
  if( !this->WaitForReply( HEADER_BAUDRATE, 1 ) ) {
    return false;
  }

  m_baudrate = m_payload[ 0 ] * 1000;

  return true;
};

int SerialAdapter::GetStatus() {
  m_header[ 0 ] = HEADER_STATUS;
  m_header[ 1 ] = 0x00;

  if( this->Write( m_header, 2 ) != 2 ) {
    return -1;
  }

  // Wait for the reply : Reply contains packet size 1
  if( !this->WaitForReply( HEADER_STATUS, 1 ) ) {
    return -1;
  }

  return m_payload[ 0 ];
};

bool SerialAdapter::Reset() {
  m_header[ 0 ] = HEADER_RESET;
  m_header[ 1 ] = 0x00;

  return ( this->Write( m_header, 2 ) == 2 );
};

bool SerialAdapter::Start() {
  m_header[ 0 ] = HEADER_START;
  m_header[ 1 ] = 0x00;


  if( this->Write( m_header, 2 ) != 2 ) {
    m_status = -1;
    return false;
  }

  m_status = 1;
  return true;
};

bool SerialAdapter::IsRunning() {
  return ( m_status == 1 );
};

void SerialAdapter::DumpData( bool showpayload ) {

  MSG_SERIALADAPTER_INFO( "Header..." );
  std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0');
  std::cout << m_header[ 0 ] << " : " << m_header[ 1 ] << std::endl;

  if( showpayload ) {
    MSG_SERIALADAPTER_INFO( "Payload..." );

    int i, j, k;

    for( i = 0; i < m_header[ 1 ]; i += 16 ) {
      std::cout << std::endl << "0x" << std::hex << std::setw(8) << std::setfill('0') << i;

      for( j = 0, k = 0; k < 16; j++, k++ ) {
        if( i+j < m_header[ 1 ] ) {
          std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << m_payload[ i+j ];
        } else {
          std::cout << "  ";
        }
        std::cout << " ";
      }
      std::cout << " ";
      for( j = 0, k = 0; k < 16; j++, k++ ) {
        if( i+j < m_header[ 1 ] ) {
          if( ( m_payload[ i+j ] < 32 ) || ( m_payload[ i+j ] > 126 ) ) {
            std::cout << ".";
          } else {
            std::cout << std::dec << m_payload[ i+j ] << std::hex;
          }
        }
      }
    }
    std::cout << std::endl;
  }

  std::cout << std::nouppercase << std::dec;
};

bool SerialAdapter::SendControlReply( unsigned char* ptr_control_reply, unsigned char control_reply_size ) {
  m_payload_out[ 0 ] = HEADER_CONTROL_DATA;
  m_payload_out[ 1 ] = control_reply_size;

  std::memcpy( &m_payload_out[ 2 ], ptr_control_reply, control_reply_size );

  if( this->Write( m_payload_out, control_reply_size + 2 ) == control_reply_size + 2 ) {
    return true;
  }

  return false;
};

bool SerialAdapter::SendInterruptReply( unsigned char* ptr_interrupt_reply, unsigned char interrupt_reply_size ) {
  m_payload_out[ 0 ] = HEADER_IN_REPORT;
  m_payload_out[ 1 ] = interrupt_reply_size;

  std::memcpy( &m_payload_out[ 2 ], ptr_interrupt_reply, interrupt_reply_size );

  if( this->Write( m_payload_out, interrupt_reply_size + 2 ) == interrupt_reply_size + 2 ) {
    return true;
  }

  return false;
};

bool SerialAdapter::Poll() {
  if( m_filedescriptor == -1 ) {
    return false;
  }

  int poll_result = poll( m_poll_fds, 1, m_poll_timeout_msecs );
  int header_size;

  if( poll_result > 0 )  {
    if( m_poll_fds[ 0 ].revents & POLLIN ) {
      header_size = this->Read( m_header, 2 );
      if( header_size == 2 ) {
        if( m_header[ 1 ] > 0 ) {
          m_payload_length = m_header[ 1 ];
          this->Read( m_payload, m_payload_length );
          if( m_header[ 0 ] == HEADER_START ) {
            MSG_SERIALADAPTER_INFO( "Adapter replied as started." );
          } else if( m_header[ 0 ] == HEADER_CONTROL_DATA || m_header[ 0 ] == HEADER_OUT_REPORT ) {
            return true;
          }
        } else {
          if( !m_surpress_warnings ) {
            MSG_SERIALADAPTER_INFO( "*WARNING* Header received without payload..." );
            std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0');
            std::cout << m_header[ 0 ] << " : " << m_header[ 1 ] << std::endl;
            std::cout << std::dec;
          }
        }
      } else {
        if( !m_surpress_warnings ) {
          MSG_SERIALADAPTER_INFO( "*WARNING* Header size error! - No payload? : Size = " << header_size );
        }
      }
    }

    if( m_poll_fds[ 0 ].revents & (POLLERR | POLLHUP | POLLNVAL) ) {
      // Error noted on file descriptor
      MSG_SERIALADAPTER_ERROR( "Polling issue.  Check adapter." );

      this->Close();

      return false;
    }
  }

  return false;
};
