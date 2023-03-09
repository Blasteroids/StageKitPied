
#include "ConsoleInput.h"

ConsoleInput::ConsoleInput() {
  m_is_initialized = false;
}

ConsoleInput::~ConsoleInput() {
  if( m_is_initialized ) {
    this->Stop();
  }
}

bool ConsoleInput::Start() {
  if( !m_is_initialized ) {
    m_is_initialized = (bool)isatty( STDIN_FILENO );
    if( m_is_initialized ) {
      m_is_initialized = (0 == tcgetattr( STDIN_FILENO, &m_initial_settings ));
    }
    if( m_is_initialized ) {
      std::cin.sync_with_stdio();
    }
  }

  return m_is_initialized;
}

void ConsoleInput::Stop() {
  if( m_is_initialized ) {
    tcsetattr( STDIN_FILENO, TCSANOW, &m_initial_settings );
    m_is_initialized = false;
  }
}

bool ConsoleInput::LineBuffered( bool on = true ) {
  struct termios settings;

  if( !m_is_initialized ) {
    return false;
  }

  if( tcgetattr( STDIN_FILENO, &settings ) ) {
    return false;
  }

  if( on ) {
    settings.c_lflag |= ICANON;
  } else {
    settings.c_lflag &= ~(ICANON);
  }

  if( tcsetattr( STDIN_FILENO, TCSANOW, &settings ) ) {
    return false;
  }

  if( on ) {
    setlinebuf( stdin );
  } else {
    setbuf( stdin, NULL );
  }

  return true;
}

bool ConsoleInput::Echo( bool on = true ) {
  struct termios settings;

  if( !m_is_initialized ) {
    return false;
  }

  if( tcgetattr( STDIN_FILENO, &settings ) ) {
    return false;
  }

  if( on ) {
    settings.c_lflag |= ECHO;
  } else {
    settings.c_lflag &= ~(ECHO);
  }

  return 0 == tcsetattr( STDIN_FILENO, TCSANOW, &settings );
}

//#define INFINITE (-1)

bool ConsoleInput::IsKeyPressed( unsigned timeout_ms = 0 ) {
  if( !m_is_initialized ) {
    return false;
  }

  struct pollfd pls[ 1 ];
  pls[ 0 ].fd     = STDIN_FILENO;
  pls[ 0 ].events = POLLIN | POLLPRI;

  return poll( pls, 1, timeout_ms ) > 0;
}

