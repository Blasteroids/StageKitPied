
#include "LEDArray.h"

LEDArray::LEDArray() {
  m_started = false;
  m_SK_LED_Number[ 0 ] = SK_LED_1;
  m_SK_LED_Number[ 1 ] = SK_LED_2;
  m_SK_LED_Number[ 2 ] = SK_LED_3;
  m_SK_LED_Number[ 3 ] = SK_LED_4;
  m_SK_LED_Number[ 4 ] = SK_LED_5;
  m_SK_LED_Number[ 5 ] = SK_LED_6;
  m_SK_LED_Number[ 6 ] = SK_LED_7;
  m_SK_LED_Number[ 7 ] = SK_LED_8;
};

LEDArray::~LEDArray() {
  this->TurnOff();
};

void LEDArray::TurnOff() {
  if( m_started ) {
    mSK9822.AllOff();
    mSK9822.Update();
  }
};

bool LEDArray::Start( const std::string& device_name, const int led_amount ) {
  MSG_LEDARRAY_INFO( "Device = " << device_name );
  MSG_LEDARRAY_INFO( "LEDS   = " << led_amount );

  this->TurnOff();

  m_started = mSK9822.Start( led_amount, device_name.c_str() );
  if( !m_started ) {
    this->TurnOff();
  }

  return m_started;
};

bool LEDArray::LoadSettingsSK( const std::string& ini_file ) {

  INI_Handler ini_handler;

  if( !ini_handler.Load( ini_file ) ) {
    MSG_LEDARRAY_ERROR( "SK INI file not loaded." << ini_file );
    MSG_LEDARRAY_ERROR( "  - " << ini_handler.GetErrorMessage() );
    return false;
  }

  if( !ini_handler.SetSection( "SK_COLOURS" ) ) {
    return false;
  }
  // **** COLOURS ****

  std::stringstream rgb;   // Line from ini
  std::string red;         // Red value string
  std::string green;       // Green value string
  std::string blue;        // Blue value string

  // Load RGB values for red
  rgb.str( ini_handler.GetTokenString( "RGB_RED" ) );

  std::getline( rgb, red, ',' );
  std::getline( rgb, green, ',' );
  std::getline( rgb, blue, ',' );

  int red_r = std::stoi( red );
  int red_g = std::stoi( green );
  int red_b = std::stoi( blue );


  MSG_LEDARRAY_DEBUG("RGB_RED = " << red << "," << green << "," << blue );

  // Load RGB values for green
  rgb.clear();
  rgb.str( ini_handler.GetTokenString( "RGB_GREEN" ) );

  std::getline( rgb, red, ',' );
  std::getline( rgb, green, ',' );
  std::getline( rgb, blue, ',' );

  int green_r = std::stoi( red );
  int green_g = std::stoi( green );
  int green_b = std::stoi( blue );

  MSG_LEDARRAY_DEBUG("RGB_GREEN = " << red << "," << green << "," << blue );

  // Load RGB values for blue
  rgb.clear();
  rgb.str( ini_handler.GetTokenString( "RGB_BLUE" ) );

  std::getline( rgb, red, ',' );
  std::getline( rgb, green, ',' );
  std::getline( rgb, blue, ',' );

  int blue_r = std::stoi( red );
  int blue_g = std::stoi( green );
  int blue_b = std::stoi( blue );

  MSG_LEDARRAY_DEBUG("RGB_BLUE = " << red << "," << green << "," << blue );

  // Load RGB values for yellow
  rgb.clear();
  rgb.str( ini_handler.GetTokenString( "RGB_YELLOW" ) );

  std::getline( rgb, red, ',' );
  std::getline( rgb, green, ',' );
  std::getline( rgb, blue, ',' );

  int yellow_r = std::stoi( red );
  int yellow_g = std::stoi( green );
  int yellow_b = std::stoi( blue );

  MSG_LEDARRAY_DEBUG("RGB_YELLOW = " << red << "," << green << "," << blue );

  // Load RGB values for strobe
  rgb.clear();
  rgb.str( ini_handler.GetTokenString( "RGB_STROBE" ) );

  std::getline( rgb, red, ',' );
  std::getline( rgb, green, ',' );
  std::getline( rgb, blue, ',' );

  int strobe_r = std::stoi( red );
  int strobe_g = std::stoi( green );
  int strobe_b = std::stoi( blue );

   MSG_LEDARRAY_DEBUG("RGB_STROBE = " << red << "," << green << "," << blue );

  // Load all 8 sections for each colour
  std::string section_name;
  for( int i = 0; i < 8; i++ ) {
    section_name = "RED_GROUP_";
    section_name += std::to_string(i + 1);
    MSG_LEDARRAY_DEBUG("Loading RED Group = " << section_name);
    this->LoadLEDData( &ini_handler, section_name, &m_LEDGroups_Red[ i ], red_r, red_g, red_b  );
    section_name = "GREEN_GROUP_";
    section_name += std::to_string(i + 1);
    MSG_LEDARRAY_DEBUG("Loading GREEN Group = " << section_name);
    this->LoadLEDData( &ini_handler, section_name, &m_LEDGroups_Green[ i ], green_r, green_g, green_b );
    section_name = "BLUE_GROUP_";
    section_name += std::to_string(i + 1);
    MSG_LEDARRAY_DEBUG("Loading BLUE Group = " << section_name);
    this->LoadLEDData( &ini_handler, section_name, &m_LEDGroups_Blue[ i ], blue_r, blue_g, blue_b );
    section_name = "YELLOW_GROUP_";
    section_name += std::to_string(i + 1);
    MSG_LEDARRAY_DEBUG("Loading YELLOW Group = " << section_name);
    this->LoadLEDData( &ini_handler, section_name, &m_LEDGroups_Yellow[ i ], yellow_r, yellow_g, yellow_b );
  }

  // **** STROBE ****
  section_name = "STROBE";

  if( ini_handler.SetSection( section_name ) ) {
    int strobe_all = ini_handler.GetTokenValue( "LEDS_ALL" );
    if( strobe_all == 1 ) {
      // Build the strobe group
      m_LEDGroup_Strobe.SetNumberOfLEDS( mSK9822.GetAmountLEDS() );
      m_LEDGroup_Strobe.SetRGB( strobe_r, strobe_g, strobe_b );
      m_LEDGroup_Strobe.SetBrightness( (uint8_t) ini_handler.GetTokenValue( "BRIGHTNESS" ) );

      for( int led_number = 1; led_number < mSK9822.GetAmountLEDS() + 1; led_number++ ) {
        m_LEDGroup_Strobe.LoadLED( led_number );
      }

    } else {
      int strobe_auto = ini_handler.GetTokenValue( "LEDS_AUTO" );
      if( strobe_auto != 1 ) {
        // Load strobe LED numbers from ini
        this->LoadLEDData( &ini_handler, section_name, &m_LEDGroup_Strobe, strobe_r, strobe_g, strobe_b );
      } else {
        // Build strobe LED numbers from unassigned LEDs
        int* strobe_leds = new int[ mSK9822.GetAmountLEDS() ];
        int strobe_leds_amount = 0;
        bool found;

        for( int led_number = 1; led_number < mSK9822.GetAmountLEDS() + 1; led_number++ ) {
          found = false;
          for( int section_number = 0; section_number < 8 && !found; section_number++ ) {
            if( !found && m_LEDGroups_Red[ section_number ].HasLED( led_number ) ) {
              found = true;
            } else if ( !found && m_LEDGroups_Green[ section_number ].HasLED( led_number ) ) {
              found = true;
            } else if ( !found && m_LEDGroups_Blue[ section_number ].HasLED( led_number ) ) {
              found = true;
            } else if ( !found && m_LEDGroups_Yellow[ section_number ].HasLED( led_number ) ) {
              found = true;
            }
          }

          if( !found ) {
            strobe_leds[ strobe_leds_amount ] = led_number;
            strobe_leds_amount++;
          }
        }

        // Build the strobe group
        m_LEDGroup_Strobe.SetNumberOfLEDS( strobe_leds_amount );
        m_LEDGroup_Strobe.SetRGB( strobe_r, strobe_g, strobe_b );
        m_LEDGroup_Strobe.SetBrightness( (uint8_t) ini_handler.GetTokenValue( "BRIGHTNESS" ) );

        for( int i = 0; i < strobe_leds_amount; i++ ) {
          m_LEDGroup_Strobe.LoadLED( strobe_leds[ i ] );
        }

        delete [] strobe_leds;
      }
    }
  }

  MSG_LEDARRAY_INFO( "Settings loaded from INI." );

  return true;
};

void LEDArray::LoadLEDData( INI_Handler* ptrINI_Handler, const std::string& section_name, LEDGroup* ptr_led_group, const uint8_t red, const uint8_t green, const uint8_t blue ) {
  if( !ptrINI_Handler->SetSection( section_name ) ) {
    return;
  }
  uint8_t amount_of_leds = (uint8_t) ptrINI_Handler->GetTokenValue( "AMOUNT" );
  std::stringstream leds;
  leds.str( ptrINI_Handler->GetTokenString( "LEDS" ) );

  ptr_led_group->SetNumberOfLEDS( amount_of_leds );
  ptr_led_group->SetRGB( red, green, blue );

  std::string tmp;
  for( int i = 0; i < amount_of_leds; i++ ) {
    std::getline( leds, tmp, ',' );
    ptr_led_group->LoadLED( std::stoi( tmp ) );
  }

  ptr_led_group->SetBrightness( (uint8_t) ptrINI_Handler->GetTokenValue( "BRIGHTNESS" ) );
}

void LEDArray::SetLights( const uint8_t colour, const uint8_t leds ) {
  switch( colour ) {
    case SK_ALL_OFF:
      mSK9822.AllOff();
      break;
    case SK_LED_RED:
      this->SetLEDS( leds, m_LEDGroups_Red );
      break;
    case SK_LED_GREEN:
      this->SetLEDS( leds, m_LEDGroups_Green );
      break;
    case SK_LED_BLUE:
      this->SetLEDS( leds, m_LEDGroups_Blue );
      break;
    case SK_LED_YELLOW:
      this->SetLEDS( leds, m_LEDGroups_Yellow );
      break;
    default:
      return;
  }
  mSK9822.Update();
};

void LEDArray::SetLEDS( const uint8_t leds, LEDGroup the_led_groups[] ) {
  int     number_leds;
  int*    led_numbers;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t brightness;

  for( uint8_t sk_led_number = 0; sk_led_number < 8; sk_led_number++ ) {
    if( ( leds & m_SK_LED_Number[ sk_led_number ] ) == 0 ) {
      brightness = 0;
    } else {
      brightness = the_led_groups[ sk_led_number ].GetBrightness();
    }
    number_leds = the_led_groups[ sk_led_number ].GetNumberOfLEDS();
    led_numbers = the_led_groups[ sk_led_number ].GetLEDs();
    red         = the_led_groups[ sk_led_number ].GetRed();
    green       = the_led_groups[ sk_led_number ].GetGreen();
    blue        = the_led_groups[ sk_led_number ].GetBlue();

    for( int i = 0; i < number_leds; i++ ) {
      mSK9822.SetColour( *led_numbers, red, green, blue, brightness );
      led_numbers++;
    }
  }
};

void LEDArray::Strobe( const bool on ) {
  int     number_leds;
  int*    led_numbers;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t brightness;

  if( on ) {
    brightness = m_LEDGroup_Strobe.GetBrightness();
  } else {
    brightness = 0;
  }

  number_leds = m_LEDGroup_Strobe.GetNumberOfLEDS();
  led_numbers = m_LEDGroup_Strobe.GetLEDs();
  red         = m_LEDGroup_Strobe.GetRed();
  green       = m_LEDGroup_Strobe.GetGreen();
  blue        = m_LEDGroup_Strobe.GetBlue();

  for( int i = 0; i < number_leds; i++ ) {
    mSK9822.SetColour( *led_numbers, red, green, blue, brightness );
    led_numbers++;
  }

  mSK9822.Update();

};

void LEDArray::SetLED( const int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness ) {
  mSK9822.SetColour( led_number, red, green, blue, brightness );
  mSK9822.Update();
};

void LEDArray::SetAllLED( const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness ) {
  mSK9822.SetColourAll( red, green, blue, brightness );
  mSK9822.Update();
};

