#ifndef co2ValuesArray_h
#define co2ValuesArray_h

#include "Arduino.h"

const int numberOfCO2values = 1440;

// Each map entry takes 40 bytes, although the data itself takes only 8 bytes.
// Each struct instance, stored in an array, takes only 8 bytes.
// So static array is better than map here. With a map with 1440 entries the ESP32 got unstable, probably because of insufficient heap space needed by WiFi.
struct co2value {
  time_t dateTime;
  uint32_t co2value;
};

class Co2valuesArray {
  co2value co2valuesPending[numberOfCO2values];
  co2value co2values[numberOfCO2values];
  // index always shows on next free entry of array
  int co2valuesPendingIndex = 0;
  int co2valuesIndex = 0;
  // Logging takes about 1 sec! for 1 hour of pending values, and 0,5 sec for 1 hour of old values
  bool doLogDuringMaintenance = false;
  // Logging during restore is faster
  bool doLogDuringRestore = false;
  uint32_t getPreviousIndex(uint32_t aIndex);
  uint32_t getNextIndex(uint32_t aIndex);
  void checkPendingCO2values();
  void deleteOldCO2values();
  void sortValuesPendingAndSetIndex();
  void sortValuesAndSetIndex();
public:
  co2value* getCO2valuesPending() {return co2valuesPending;};
  co2value* getCO2values() {return co2values;};
  bool getDoLogDuringRestore() {return doLogDuringRestore;};
  void restoreAllCO2valuesFromStorage();
  void restoreCO2valueFromStorage(time_t aDateTime, uint32_t aCO2value);
  void logAllCO2values();
  void addCO2value(uint32_t aCO2value);
  bool deleteCO2value(const char * aDateTimeStr, bool pending);
  void maintainValues();
};

extern Co2valuesArray co2values;

void co2valuesArray_init();

#endif /* co2ValuesArray_h */
