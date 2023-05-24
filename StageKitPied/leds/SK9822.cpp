
#include "SK9822.h"

SK9822::SK9822() {
  m_enabled = false;
  m_number_leds = 0;
  m_current_led_offset = 0;  // LEDS have range: 1 to m_number_leds
  m_file_descriptor = -1;
};

SK9822::~SK9822() {

  this->Stop();

  delete [] m_buffer;
};

bool SK9822::SetEnabled( const bool enabled ) {
  if( m_enabled == enabled ) {
    return true;
  }

  if( enabled ) {
    if( this->Start() ) {
      return true;
    }
    this->Stop();
    return false;
  }
  
  this->Stop();
  return true;
};

bool SK9822::IsEnabled() {
  return m_enabled;
};

bool SK9822::Init( const int number_leds, const std::string& device_name ) {
  if( number_leds == 0 ) {
    return false;
  }

  // SK9822 needs a blank end frame buffer.
  int end_buffer_size = 1 + ( ( number_leds / 2 ) / 32 );

  m_device_name = device_name;

  m_number_leds = number_leds;
  m_buffer = new SK9822_struct[ number_leds + 2 + end_buffer_size ];  // Data format = Start frame + LEDS + End frame
  m_buffer_size = ( number_leds + 2 + end_buffer_size ) * sizeof( SK9822_struct );

  // Start frame
  m_buffer[ 0 ].m_brightness = 0x00;
  m_buffer[ 0 ].m_blue       = 0x00;
  m_buffer[ 0 ].m_green      = 0x00;
  m_buffer[ 0 ].m_red        = 0x00;

  for( int i = number_leds + 1; ( i < number_leds + 1 + end_buffer_size ); i++ ) {
    m_buffer[ i ].m_brightness = 0x00;
    m_buffer[ i ].m_blue       = 0x00;
    m_buffer[ i ].m_green      = 0x00;
    m_buffer[ i ].m_red        = 0x00;
  }

  // Ensure all LEDS are off at start
  this->AllOff();

  return true;
};

bool SK9822::Update() {
  if( !m_enabled || m_number_leds == 0 ) {
    return false;
  }

  ssize_t bytes_written = write( m_file_descriptor, m_buffer, m_buffer_size );
  if( bytes_written == -1 ) {
    return false;
  }

  return true;
};

void SK9822::SetColour( const int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness ) {
  if( led_number > 0 && led_number <= m_number_leds ) {
    brightness |= 0xE0;
    this->SetColourNC( led_number, red, green, blue, brightness );
  } else {
    MSG_SK9822_ERROR( "Tried setting LED out of range ( " << led_number << " : 1 - " << m_number_leds << " )" );
  }
};

// NC - No range check & brightness must already have first 3 bits set to 1
void SK9822::SetColourNC( int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness ) {

  // Adjust led_number to range 1 - m_number_leds with offset.
  led_number += m_current_led_offset;
  led_number %= (m_number_leds + 1);

  m_buffer[ led_number ].m_brightness = brightness;
  m_buffer[ led_number ].m_blue       = blue;
  m_buffer[ led_number ].m_green      = green;
  m_buffer[ led_number ].m_red        = red;
};

// Brightness = 0-31
void SK9822::SetColourAll( const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness ) {
  brightness |= 0xE0;
  SK9822_struct* led = &m_buffer[ 1 ];
  for( int led_number = 1; led_number <= m_number_leds; led_number++ ) {
    led->m_brightness = brightness;   // First 3 bits must be 1
    led->m_blue       = blue;
    led->m_green      = green;
    led->m_red        = red;
    led++;
  }
};

void SK9822::SetOff( const int led_number ) {
  this->SetColour( led_number, 0, 0, 0, 0 );
};

void SK9822::AllOff() {
  this->SetColourAll( 0, 0, 0, 0 );
};

// Reduced brightness (0-31) by step amount
void SK9822::ReduceBrightness( const uint8_t step ) {
  SK9822_struct* led = &m_buffer[ 1 ];

  for( int led_number = 1; led_number <= m_number_leds; led_number++ ) {
    if( step > (led->m_brightness & 0x1F) ) {
      led->m_brightness = 0;
    } else {
      led->m_brightness -= step;
    }
    led++;
  }
};

// Takes it that LED are positioned counter-clockwise.
void SK9822::Rotate( const bool left ) {
  SK9822_struct led_tmp;

  if( left ) {
    m_current_led_offset += 1;

    std::memcpy( &led_tmp, &m_buffer[ 1 ], sizeof( SK9822_struct ) );
    for( int i = 1; i < m_number_leds - 1; i++ ) {
      std::memcpy( &m_buffer[ i ], &m_buffer[ i + 1 ], sizeof( SK9822_struct ) );
    }
    std::memcpy( &m_buffer[ m_number_leds ], &led_tmp, sizeof( SK9822_struct ) );
  } else {

    m_current_led_offset += m_number_leds;

    std::memcpy( &led_tmp, &m_buffer[ m_number_leds ], sizeof( SK9822_struct ) );
    for( int i = m_number_leds; i > 2; i++ ) {
      std::memcpy( &m_buffer[ i ], &m_buffer[ i - 1 ], sizeof( SK9822_struct ) );
    }
    std::memcpy( &m_buffer[ 1 ], &led_tmp, sizeof( SK9822_struct ) );
  }

  m_current_led_offset %= (m_number_leds + 1);
};

void SK9822::RotateOff() {
  int end_buffer_size = 1 + ( ( m_number_leds / 2 ) / 32 );
  int record_amount = m_number_leds + 2 + end_buffer_size;
  SK9822_struct* buffer_tmp = new SK9822_struct[ record_amount ];  // Start frame + LEDS + End frame
  std::memcpy( &buffer_tmp, &m_buffer, sizeof( SK9822_struct ) * record_amount );

  record_amount = m_number_leds - m_current_led_offset;
  std::memcpy( &m_buffer[ 1 ], &buffer_tmp[ m_current_led_offset + 1 ], sizeof( SK9822_struct ) * record_amount );
  std::memcpy( &m_buffer[ record_amount + 1 ], &buffer_tmp[ m_current_led_offset ], sizeof( SK9822_struct ) * m_current_led_offset );

  m_current_led_offset = 0;

  delete [] buffer_tmp;
};

int SK9822::GetAmountLEDS() {
  return m_number_leds;
};

void SK9822::DumpBuffer() {
  int i = 0;
  int ii = 0;
  std::cout << i << " : ";
  while( i < m_number_leds + 2 ) {
    std::cout << "(" << m_buffer[ i ].m_red << "," << m_buffer[ i ].m_green << "," << m_buffer[ i ].m_blue << "," << m_buffer[ i ].m_brightness << ") ";
    ii++;
    i++;
    if( ii == 5 ) {
      std::cout << std::endl << i << " : ";
      ii = 0;
    }
  }
  std::cout << std::endl;
};

bool SK9822::Start() {
  if( m_number_leds == 0 ) {
    return false;
  }

  // Open the file descriptor to the device in write only mode
  m_file_descriptor = open( m_device_name.c_str(), O_WRONLY );
  if( m_file_descriptor < 0 ) {
    return false;
  }

  // Setup SPI with write mode, the bits per word & the maximum writing speed.
  const uint8_t spi_mode      = SPI_MODE_0;
  const uint8_t bits_per_word = 8;
  const uint32_t speed_in_hz  = 4000000; // 4Mhz

  if( ioctl( m_file_descriptor, SPI_IOC_WR_MODE, &spi_mode ) == -1 ) {
    return false;
  }
  if( ioctl( m_file_descriptor, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word ) == -1 ) {
    return false;
  }
  if( ioctl( m_file_descriptor, SPI_IOC_WR_MAX_SPEED_HZ, &speed_in_hz ) == -1 ) {
    return false;
  }

  m_enabled = true;

  this->Update();

  return true;
};

void SK9822::Stop() {
  this->AllOff();
  this->Update();

  if( m_file_descriptor != -1 ) {
    close( m_file_descriptor );
    m_file_descriptor = -1;
  }

  m_enabled = false;
};

