#include "LEDGroup.h"

LEDGroup::LEDGroup() {
  m_number_of_leds_max    = 0;
  m_number_of_leds_loaded = 0;
  m_red   = 0;
  m_green = 0;
  m_blue  = 0;
  m_brightness = 0;
};

LEDGroup::~LEDGroup() {
  this->Free();
};

void LEDGroup::SetNumberOfLEDS( const int number_of_leds ) {
  this->Free();

  m_number_of_leds_max    = number_of_leds;
  m_number_of_leds_loaded = 0;

  m_leds = new int[ number_of_leds ];
};

void LEDGroup::LoadLED( const int led_number ) {
  if( m_number_of_leds_loaded < m_number_of_leds_max ) {
    m_leds[ m_number_of_leds_loaded ] = led_number;
    m_number_of_leds_loaded++;
  }
};

void LEDGroup::SetRGB( const uint8_t red, const uint8_t green, const uint8_t blue ) {
  m_red = red;
  m_green = green;
  m_blue = blue;
};

void LEDGroup::SetBrightness( const uint8_t brightness ) {
  m_brightness = brightness;
};

int LEDGroup::GetNumberOfLEDS() {
  return m_number_of_leds_loaded;
};

int LEDGroup::GetLED( const int led_position_number ) {
  if( led_position_number >= m_number_of_leds_loaded ) {
    return -1;
  }

  return m_leds[ led_position_number ];
};

bool LEDGroup::HasLED( const int led_number ) {
  for( int i = 0; i < m_number_of_leds_loaded; i++ ) {
    if( m_leds[ i ] == led_number ) {
      return true;
    }
  }
  return false;
};

int* LEDGroup::GetLEDs() {
  return m_leds;
};

uint8_t LEDGroup::GetRed() {
  return m_red;
};

uint8_t LEDGroup::GetGreen() {
  return m_green;
};

uint8_t LEDGroup::GetBlue() {
  return m_blue;
};

uint8_t LEDGroup::GetBrightness() {
  return m_brightness;
};

void LEDGroup::Free() {
  if( m_number_of_leds_max > 0 ) {
    delete [] m_leds;
    m_number_of_leds_max = 0;
  }
};

void LEDGroup::Dump() {
  std::cout << m_red << "," << m_green << "," << m_blue << " @ " << m_brightness << " : ";

  for( int i = 0; i < m_number_of_leds_loaded; i++ ) {
    std::cout << m_leds[ i ] << ",";
  }
  std::cout << std::endl;
}
