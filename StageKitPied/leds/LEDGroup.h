#ifndef _LEDGROUP_H_
#define _LEDGROUP_H_

#ifdef DEBUG
  #define MSG_LEDGROUP_DEBUG( str ) do { std::cout << "LEDGroup : DEBUG : " << str << std::endl; } while( false )
#else
  #define MSG_LEDGROUP_DEBUG( str ) do { } while ( false )
#endif

#define MSG_LEDGROUP_ERROR( str ) do { std::cout << "LEDGroup : ERROR : " << str << std::endl; } while( false )
#define MSG_LEDGROUP_INFO( str ) do { std::cout << "LEDGroup : INFO : " << str << std::endl; } while( false )

#include <cstdint>
#include <iostream>

class LEDGroup {
public:
  LEDGroup();

  ~LEDGroup();

  // Resets number of loaded LEDs to zero.
  void SetNumberOfLEDS( const int number_of_leds );

  void LoadLED( const int led_number );

  void SetRGB( const uint8_t red, const uint8_t green, const uint8_t blue );

  void SetBrightness( const uint8_t brightness );

  int GetNumberOfLEDS();

  // Gets the LED at the group position number
  int GetLED( const int led_position_number );

  // Checks to see if the led_numbers is in this group
  bool HasLED( const int led_number );

  int* GetLEDs();

  uint8_t GetRed();

  uint8_t GetGreen();

  uint8_t GetBlue();

  uint8_t GetBrightness();

  void Dump();

private:
  void Free();

  int     m_number_of_leds_max;
  int     m_number_of_leds_loaded;
  int*    m_leds;
  uint8_t m_red;
  uint8_t m_green;
  uint8_t m_blue;
  uint8_t m_brightness;
};

#endif

