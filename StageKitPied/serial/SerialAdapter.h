#ifndef _SERIALADAPTER_H_
#define _SERIALADAPTER_H_

#ifdef DEBUG
  #define MSG_SERIALADAPTER_DEBUG( str ) do { std::cout << "SerialAdapter : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_SERIALADAPTER_DEBUG( str ) do { } while ( false )
#endif

#define MSG_SERIALADAPTER_ERROR( str ) do { std::cout << "SerialAdapter : ERROR : " << str << std::endl; } while( false )
#define MSG_SERIALADAPTER_INFO( str ) do { std::cout << "SerialAdapter : INFO : " << str << std::endl; } while( false )

#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <cstring> // memcpy
#include <errno.h> // errno
#include <fcntl.h> // file
#include <termios.h> // termio
#include <sys/time.h> // timeval
#include <poll.h> // poll

#define SERIALADAPTER_DEBUG 0

#define SERIALADAPTER_DEFAULT_TIMEOUT 1000
#define SERIALADAPTER_DEFAULT_BAUDRATE B500000

#define HEADER_NO_PACKET    0x00
#define HEADER_GET_TYPE     0x11
#define HEADER_STATUS       0x22
#define HEADER_START        0x33
#define HEADER_CONTROL_DATA 0x44
#define HEADER_RESET        0x55
#define HEADER_SET_IDS      0x66
#define HEADER_VERSION      0x77
#define HEADER_BAUDRATE     0x88
#define HEADER_DEBUG        0x99
#define HEADER_OUT_REPORT   0xee
#define HEADER_IN_REPORT    0xff

#define HEADER_STATUS_NSPOOFED 0x00
#define HEADER_STATUS_SPOOFED  0x01

#define ADAPTER_TYPE_X360   0x01
#define ADAPTER_TYPE_XONE   0x06
#define ADAPTER_TYPE_X360SK 0xfe  // Not an official GIMX adapter type designation.

class SerialAdapter
{
public:
  SerialAdapter();

  ~SerialAdapter();

  bool Init( const char* path, bool surpress_warnings );

  void Close();

  bool Poll();

  int PayloadType();

  unsigned char* Payload();

  int PayloadLength();

  void DumpData( bool showpayload );

  void Dump( unsigned char* data, int data_length );

  bool IsRunning();

  bool SendControlReply( unsigned char* ptr_control_reply,
                         unsigned char  control_reply_size );

  bool SendInterruptReply( unsigned char* ptr_interrupt_reply,
                           unsigned char  interrupt_reply_size );

  int GetStatus();

private:
  int Read( unsigned char* buffer, unsigned int count );

  int Write( unsigned char* buffer, unsigned int count );

  bool WaitForReply( int header_value, int payload_length );

  bool GetType();

  bool GetVersion();

  bool GetBaudrate();

  bool Reset();

  bool Start();

  struct timeval m_timeout;
  int m_filedescriptor;
  unsigned char m_header[ 2 ]; // For debug - should be 2!
  unsigned char m_payload[ 255 ];
  unsigned char m_payload_out[ 257 ];

  bool m_surpress_warnings;
  int m_payload_length;
  int m_type;  // Type of GIMX adapter connected.
  int m_version_major;
  int m_version_minor;
  int m_baudrate;
  int m_vid;
  int m_pid;
  int m_status;

  // Poll
  struct pollfd m_poll_fds[ 1 ];
  int m_poll_timeout_msecs;

};

#endif
