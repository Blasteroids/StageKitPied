#ifndef _SLEEPTIMER_H_
#define _SLEEPTIMER_H_

#include <chrono> // high resolution timer
#include <time.h> // nanosleep
#include <cerrno> // errno, EINTR

class SleepTimer {
public:
    //using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
    //                                 std::chrono::high_resolution_clock,
    //                                 std::chrono::steady_clock>;
private:
    std::chrono::steady_clock::time_point m_time_start_last;
    std::chrono::steady_clock::time_point m_time_start;
    std::chrono::steady_clock::time_point m_time_end;
    long m_sleep_max_ms;
    long m_time_difference_ms;

public:

    SleepTimer() {
    }

    ~SleepTimer() {
    }

    void SetSleepTimeMax( const long milliseconds ) {
      m_sleep_max_ms = milliseconds;
    }

    void Start() {
      m_time_start = std::chrono::steady_clock::now();
      m_time_start_last = m_time_start;
    }

    long Sleep() {
      m_time_end = std::chrono::steady_clock::now();
      m_time_difference_ms = std::chrono::duration_cast<std::chrono::milliseconds>( m_time_end - m_time_start ).count();

      if( m_time_difference_ms < m_sleep_max_ms ) {
        this->DoSleep( m_sleep_max_ms - m_time_difference_ms);
      }

      m_time_start_last = m_time_start;
      m_time_start = std::chrono::steady_clock::now();

      return std::chrono::duration_cast<std::chrono::milliseconds>( m_time_start - m_time_start_last ).count();
    }

private:
  void DoSleep( const long milliseconds ) {

    struct timespec sleeptimespec;

    if( milliseconds < 0 ) {
      return;
    }

    sleeptimespec.tv_sec = milliseconds / 1000;
    sleeptimespec.tv_nsec = ( milliseconds % 1000 ) * 1000000;

    int result;
    do {
      result = nanosleep( &sleeptimespec, &sleeptimespec );
    } while( result && errno == EINTR );
  }

};

#endif
