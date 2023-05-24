#include <signal.h>
#include <stdio.h>
#include <cstring>

#include "helpers/SleepTimer.h"
#include "helpers/ConsoleInput.h"
#include "controller/RpiLightsController.h"

#define INI_FILE "lights.ini"

#define MSG_SKP_INFO( str ) do { std::cout << "StageKitPied : INFO : " << str << std::endl; } while( false )
#define MSG_SKP_ERROR( str ) do { std::cout << "StageKitPied : ERROR : " << str << std::endl; } while( false )

// *******
// sigterm
// *******
volatile sig_atomic_t done = 0;

void term( int signum )
{
  // Caught signal termination
  done = 1;
}

// ****
// main
// ****

// Stage Kit Pied

int main(int arc, char *argv[]) {
  // Setup killswitch
  struct sigaction action;
  memset(&action, 0, sizeof(action) );
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);

  int pid = getpid();

  MSG_SKP_INFO( "Program started with PID = " << pid );

  RpiLightsController lightsController( INI_FILE );
  if( !lightsController.Start() ) {
    MSG_SKP_ERROR( "Unable to start.");
    //return 0;
  }

  MSG_SKP_INFO( "Started" );

  // Sleepy timer
  long sleepyTime = 0;

  SleepTimer sleeper;
  sleeper.SetSleepTimeMax( sleepyTime );
  sleeper.Start();

  long slept_ms = 10;
  long sleep_time_new = 10;

  ConsoleInput console;

  if( !console.Start() ) {
    MSG_SKP_ERROR( "Unable to get console input." );
  } else {
    MSG_SKP_INFO( "Press any key to exit program." );
  }

  console.LineBuffered( false );
  console.Echo( false );


  // Main loop
  while( !done ) {

    sleep_time_new = lightsController.Update( slept_ms );

    sleeper.SetSleepTimeMax( sleep_time_new );
    slept_ms = sleeper.Sleep();

    if( console.IsKeyPressed( 0 ) ) {
      done = true;
    }
  }

  lightsController.Stop();

  console.Stop();

  MSG_SKP_INFO( "Program ended." );

  return 0;
}
