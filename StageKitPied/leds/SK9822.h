#ifndef _SK9822_H_
#define _SK9822_H_

#ifdef DEBUG
  #define MSG_SK9822_DEBUG( str ) do { std::cout << "SK9822 : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_SK9822_DEBUG( str ) do { } while ( false )
#endif

#define MSG_SK9822_ERROR( str ) do { std::cout << "SK9822 : ERROR : " << str << std::endl; } while( false )
#define MSG_SK9822_INFO( str ) do { std::cout << "SK9822 : INFO : " << str << std::endl; } while( false )


#include <cstring> // memcpy
#include <string>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

typedef struct {
  uint8_t m_brightness;  // Range? 0-31 (0x1F)
  uint8_t m_blue;
  uint8_t m_green;
  uint8_t m_red;
} SK9822_struct;

class SK9822 {
public:
  SK9822();
  ~SK9822();

  bool SetEnabled( const bool enabled );

  bool IsEnabled();

  bool Init( const int number_leds, const std::string& device_name );
  
  // Push colour/brightness changes to the actual LEDs
  bool Update();

  // led_number has range 1 to m_number_leds.
  // These functions change the internal class data, call Update() afterwards to push the data to the actual LEDs.
  void SetColour( const int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness );

  // Same as SetColout but without range check.
  void SetColourNC( int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness );

  void SetColourAll( const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness );

  void SetOff( const int led_number );

  void AllOff();

  void ReduceBrightness( const uint8_t step = 1 );

  void Rotate( const bool left );

  void RotateOff();

  int GetAmountLEDS();

  void DumpBuffer();

private:
  bool Start();

  void Stop();

  bool           m_enabled;
  std::string    m_device_name;
  int            m_file_descriptor;
  int            m_number_leds;
  int            m_current_led_offset; // Used for rotating led colours left or right.
  SK9822_struct* m_buffer;
  size_t         m_buffer_size;
};

#endif
