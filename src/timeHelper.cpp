#include <Arduino.h>
#include <iomanip>
#include <sstream>

#if defined(ESP32)
  #include <WiFi.h>
#endif
#if defined(ESP8266)
  #include <ESP8266WiFi.h> 
#endif

#include "ezTime.h"
#include "config.h"
#include "log.h"
/*
https://github.com/ropg/ezTime
Alternatives:
https://www.mischianti.org/2020/08/08/network-time-protocol-ntp-timezone-and-daylight-saving-time-dst-with-esp8266-esp32-or-arduino/
https://github.com/arduino-libraries/NTPClient
https://github.com/PaulStoffregen/Time
https://github.com/JChristensen/Timezone
*/

// set timezone in "config.h"
Timezone myCountry;

void timeHelper_setup() {
  myCountry.setCache(0);

  // setInterval(10);
  // setDebug(INFO);
}

void timeHelper_afterWiFiConnected() {
  waitForSync(5);

  // Log.printf("UTC: %s\r\n", UTC.dateTime().c_str());
  myCountry.setLocation(myTimezone);
  Log.printf("Local time: %s\r\n", myCountry.dateTime(COOKIE).c_str());
}

void timeHelper_update() {
  events();
}

/*
timeNotSet:    time was never set, time is for sure around 1970
timeSet:       time was once set by NTP, but could be out of sync (in that case timeNeedsSync is set)
timeNeedsSync: A scheduled NTP request has been due for more than an hour.
               This can happen both if time was once set via NTP, or if time was never set and is still around 1970.
               In the case of 1970 this happens after 1,5 h
               NTP_INTERVAL			1801				// default update interval in seconds
               NTP_STALE_AFTER	3602				// If update due for this many seconds, set timeStatus to timeNeedsSync


-> seems there is no other way to know if time is around 1970 than by checking UTC.now()
if (timeStatus() == timeNotSet) will be replaced by realTimeAvailable() */

bool timeHelper_realTimeAvailable() {
  return (UTC.now() >= 315529200); // 1980-01-01
}

void getLocalTimeStr(char* localTimeStr) {
  if (!timeHelper_realTimeAvailable()) {
    sprintf(localTimeStr, " ");
  } else {
    sprintf(localTimeStr, "%s", myCountry.dateTime("H:i:s d.m.Y").c_str());
  }
}

// https://stackoverflow.com/questions/11213326/how-to-convert-a-string-variable-containing-time-to-time-t-type-in-c
// https://stackoverflow.com/questions/4781852/how-to-convert-a-string-to-datetime-in-c
std::time_t getEpochTime(const std::string& strDate, const char* format) {
  struct std::tm tm = {0,0,0,0,0,0,0,0,0};
  std::istringstream ss(strDate);
  ss >> std::get_time(&tm, format); // or just %T in this case
  std::time_t time = mktime(&tm);
  return time;
}
