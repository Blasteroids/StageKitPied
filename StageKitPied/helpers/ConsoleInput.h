#ifndef _CONSOLE_INPUT_H_
#define _CONSOLE_INPUT_H_

// From: https://cplusplus.com/forum/general/5304/#msg23940

#include <cstdio>
#include <iostream>
#include <limits>
#include <string>

#include <unistd.h>
#include <termios.h>
#include <poll.h>

class ConsoleInput
{
public:
  ConsoleInput();

  ~ConsoleInput();

  bool Start();

  void Stop();

  bool LineBuffered( bool on );

  bool Echo( bool on );

  bool IsKeyPressed( unsigned timeout_ms );

private:
  bool m_is_initialized;

  struct termios m_initial_settings;
};


#endif
