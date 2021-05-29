#include "config.h"
#include "log.h"

#if defined(useTelnetStream)
#include "TelnetStream.h"
#endif

// https://www.learncpp.com/cpp-tutorial/class-code-and-header-files/

LogStreamClass::LogStreamClass(void) {
}

int LogStreamClass::read() {
  // return Serial.read();
  return -1;
}

int LogStreamClass::available() {
  // return Serial.available();
  return -1;
}

int LogStreamClass::peek() {
  // return Serial.peek();
  return -1;
}

#ifdef ESP32
void LogStreamClass::flush() {
  return;
}
#endif

size_t LogStreamClass::write(uint8_t val) {
  // return Serial.write(val);
  return -1;
}

size_t LogStreamClass::write(const uint8_t *buf, size_t size) {
  // return Serial.write(buf, size);
  return -1;
}

size_t LogStreamClass::printf(const char * format, ...) {
// https://stackoverflow.com/questions/3530771/passing-variable-arguments-to-another-function-that-accepts-a-variable-argument
// https://stackoverflow.com/questions/1056411/how-to-pass-variable-number-of-arguments-to-printf-sprintf

  size_t res;
  va_list args;
  va_start(args, format);

  // maximum number of characters in log message
  char buf[1000];
  vsnprintf(buf, sizeof(buf), format, args);

  // print out #1: Serial
  #if defined(useSerial)
  res = Serial.printf(MY_LOG_FORMAT("%s"), buf);
  #endif

  // print out #2: TelnetStream
  #if defined(useTelnetStream)
  res = TelnetStream.printf(MY_LOG_FORMAT("%s"), buf);
  #endif

  va_end(args);
  return res;
};

LogStreamClass Log;
