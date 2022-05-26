#include <Arduino.h>
#include <iomanip>

#include "co2ValuesArray.h"
#include "storage.h"
#include "log.h"
#include "ezTime.h"
#include "timeHelper.h"

Co2valuesArray co2values;

void co2valuesArray_init() {
  co2values.restoreAllCO2valuesFromStorage();
}

void Co2valuesArray::restoreAllCO2valuesFromStorage() {
  for (int i=0; i<numberOfCO2values; i++) {
    co2valuesPending[i].dateTime = -1;
    co2valuesPending[i].co2value = 0;
    co2values[i].dateTime = -1;
    co2values[i].co2value = 0;
  }
  co2valuesPendingIndex = 0;
  co2valuesIndex = 0;
  
  getNvsEntriesAndSaveToCO2Values(*this);

  sortValuesAndSetIndex();
}

void Co2valuesArray::restoreCO2valueFromStorage(time_t aDateTime, uint32_t aCO2value) {
  // Log.printf("  Will restore value from storage: %li, %d in index %d\r\n", aDateTime, aCO2value, co2valuesIndex);

  co2values[co2valuesIndex].dateTime = aDateTime;
  co2values[co2valuesIndex].co2value = aCO2value;
  co2valuesIndex = getNextIndex(co2valuesIndex);
}

void Co2valuesArray::logAllCO2values() {
  Log.printf("List of co2values\r\n");
  char buff[20];
  time_t dateTime;
  uint32_t co2value;

  for (int i=0; i<numberOfCO2values; i++) {
    dateTime = co2values[i].dateTime;
    co2value = co2values[i].co2value;

    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&dateTime));
    Log.printf("preferencesCO2sensorValues.putUInt(\"%s\", %d); // index: %d, date: %s, value: %d\r\n",  String(dateTime).c_str(), co2value, i, buff, co2value);
  }
}

uint32_t Co2valuesArray::getPreviousIndex(uint32_t aIndex) {
  if (aIndex == 0) {
    return numberOfCO2values-1;
  } else {
    return aIndex-1;
  }
}

uint32_t Co2valuesArray::getNextIndex(uint32_t aIndex) {
  if (aIndex == 1439) {
    return 0;
  } else {
    return aIndex+1;
  }
}

void Co2valuesArray::addCO2value(uint32_t aCO2value) {
  if (aCO2value == 0) {return;}

  // If time is not yet set, add value to pending array (with time starting at 01.01.1970), but not to storage.
  // As soon as time will be set, convert values from 1970 to values with time and save them to storage.
  // Values with time from 1970 are called "pending values" and will be transferred to values with correct timestamp
  //   in method co2valuesArray::checkPendingCO2values()

  char buff[20];
  bool timePending;
  bool valueAdded = false;

  time_t timeNow;
  if (!timeHelper_realTimeAvailable()) {
    timeNow = millis() / 1000;
    timePending = true;
  } else {
    timeNow = UTC.now();
    timePending = false;
  }
  struct tm * ptm;
  ptm = gmtime(&timeNow);

  int denom = 60;
  ptm->tm_sec = (ptm->tm_sec / denom) * denom;
  time_t discreteTime = mktime(ptm);

  strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&discreteTime));
  if (timePending) {
    if (co2valuesPending[getPreviousIndex(co2valuesPendingIndex)].dateTime != discreteTime) {
      Log.printf("New CO2value %d for interval %s, will add value to array index %d, but not to storage\r\n", aCO2value, buff, co2valuesPendingIndex);;
      co2valuesPending[co2valuesPendingIndex].dateTime = discreteTime;
      co2valuesPending[co2valuesPendingIndex].co2value = aCO2value;
      co2valuesPendingIndex = getNextIndex(co2valuesPendingIndex);
      valueAdded = true;
    } else {
      // Log.printf("CO2value %d for already known interval %s, will not add it\r\n", aCO2value, buff);
    }
  } else {
    if (co2values[getPreviousIndex(co2valuesIndex)].dateTime != discreteTime) {
      Log.printf("New CO2value %d for interval %s, will add value to array index %d and storage\r\n", aCO2value, buff, co2valuesIndex);
      // if already existing value in array gets overwritten, remove old value from storage
      if (co2values[co2valuesIndex].dateTime != -1) {
        preferencesCO2sensorValues.remove(String(co2values[co2valuesIndex].dateTime).c_str());
      }
      co2values[co2valuesIndex].dateTime = discreteTime;
      co2values[co2valuesIndex].co2value = aCO2value;
      co2valuesIndex = getNextIndex(co2valuesIndex);
      preferencesCO2sensorValues.putUInt(String(discreteTime).c_str(), aCO2value);
      valueAdded = true;
    } else {
      // Log.printf("CO2value %d for already known interval %s, will not add it\r\n", aCO2value, buff);
    }
  }
  if (valueAdded) {
    // Actually only needed if old value was replaced. But sorting is fast, so do it in any case.
    if (timePending) {
      sortValuesPendingAndSetIndex();
    } else {
      sortValuesAndSetIndex();
    }
  }
}

void Co2valuesArray::sortValuesPendingAndSetIndex() {
  std::sort(std::begin(co2valuesPending), std::end(co2valuesPending), [](co2value a, co2value b) {
        if ((a.dateTime == -1) && (b.dateTime == -1)) {
          // https://stackoverflow.com/questions/45929474/why-must-stdsort-compare-function-return-false-when-arguments-are-equal
          return false;
        } else if ((a.dateTime == -1) && (b.dateTime != -1)) {
          return false;
        } else if ((a.dateTime != -1) && (b.dateTime == -1)) {
          return true;
        } else {
          return a.dateTime < b.dateTime;
        }
  });

  // set index one higher than entry with highest date 
  time_t lastDate;
  lastDate = -1.0; // co2valuesPending[0].dateTime;
  int lastDateIndex = -1;
  for (int i=0; i<numberOfCO2values; i++) {
    if (co2valuesPending[i].dateTime > lastDate) {
      lastDate = co2valuesPending[i].dateTime;
      lastDateIndex = i;
    }
  }
  if (lastDateIndex == -1) {
    // no element found
    co2valuesPendingIndex = 0;  
  } else {
    co2valuesPendingIndex = (lastDateIndex +1) % numberOfCO2values;
  }
  Log.printf("Pending CO2 values sorting finished, next used index is %d\r\n", co2valuesPendingIndex);
}

void Co2valuesArray::sortValuesAndSetIndex() {
  std::sort(std::begin(co2values), std::end(co2values), [](co2value a, co2value b) {
        if ((a.dateTime == -1) && (b.dateTime == -1)) {
          // https://stackoverflow.com/questions/45929474/why-must-stdsort-compare-function-return-false-when-arguments-are-equal
          return false;
        } else if ((a.dateTime == -1) && (b.dateTime != -1)) {
          return false;
        } else if ((a.dateTime != -1) && (b.dateTime == -1)) {
          return true;
        } else {
          return a.dateTime < b.dateTime;
        }
  });

  // set index one higher than entry with highest date 
  time_t lastDate;
  lastDate = -1.0; // co2values[0].dateTime;
  int lastDateIndex = -1;
  for (int i=0; i<numberOfCO2values; i++) {
    if (co2values[i].dateTime > lastDate) {
      lastDate = co2values[i].dateTime;
      lastDateIndex = i;
    }
  }
  if (lastDateIndex == -1) {
    // no element found
    co2valuesIndex = 0;  
  } else {
    co2valuesIndex = (lastDateIndex +1) % numberOfCO2values;
  }
  Log.printf("CO2 values sorting finished, next used index is %d\r\n", co2valuesIndex);
}

void Co2valuesArray::checkPendingCO2values() {
  // If time is available, check if there are old CO2values without timestamp
  if (!timeHelper_realTimeAvailable()) {return;};

  bool pendingValuesFound = false;

  time_t timeNow = UTC.now();

  unsigned long currentMillis = millis();
  int currentSeconds = currentMillis / 1000;

  for (int i=0; i<numberOfCO2values; i++) {

    if (co2valuesPending[i].dateTime != -1) {
      // here we found a value
      if ((co2valuesPending[i].dateTime > currentSeconds)) {
        // simply delete it
        co2valuesPending[i].dateTime = -1;
        co2valuesPending[i].co2value = 0;
        continue;
      }

      // get seconds before timeNow
      int secondsDelta = currentSeconds - co2valuesPending[i].dateTime;

      struct tm * ptm;
      ptm = gmtime(&timeNow);
      // Subtract secondsDelta from timeNow. This is the time where the value was measured.
      ptm->tm_sec = ptm->tm_sec - secondsDelta;
      // Now get discreteTime
      int denom = 60;
      ptm->tm_sec = (ptm->tm_sec / denom) * denom;
      time_t discreteTime = mktime(ptm);

      char buff1[20];
      char buff2[20];
      strftime(buff1, 20, "%Y-%m-%d %H:%M:%S", gmtime(&co2valuesPending[i].dateTime));
      strftime(buff2, 20, "%Y-%m-%d %H:%M:%S", gmtime(&discreteTime));
      if (doLogDuringMaintenance) {Log.printf("Found pending time. Old time: %s, new time: %s\r\n", buff1, buff2);};

      // remove array entry
      uint32_t aCO2value = co2valuesPending[i].co2value;
      co2valuesPending[i].dateTime = -1;
      co2valuesPending[i].co2value = 0;
      // add array entry and save to storage
      if (co2values[getPreviousIndex(co2valuesIndex)].dateTime != discreteTime) {
        if (doLogDuringMaintenance) {Log.printf("New CO2value %d for interval %s, will add value to array index %d and storage\r\n", aCO2value, buff2, co2valuesIndex);};
        // if already existing value in array gets overwritten, remove old value from storage
        if (co2values[co2valuesIndex].dateTime != -1) {
          preferencesCO2sensorValues.remove(String(co2values[co2valuesIndex].dateTime).c_str());
        }
        co2values[co2valuesIndex].dateTime = discreteTime;
        co2values[co2valuesIndex].co2value = aCO2value;
        co2valuesIndex = getNextIndex(co2valuesIndex);
        preferencesCO2sensorValues.putUInt(String(discreteTime).c_str(), aCO2value);
        pendingValuesFound = true;
      }
    }
  }
  if (pendingValuesFound) {
    sortValuesPendingAndSetIndex();
    sortValuesAndSetIndex();
  }
}

void Co2valuesArray::deleteOldCO2values() {
  time_t timeNow;
  bool oldValuesFound = false;

  if (!timeHelper_realTimeAvailable()) {
    // Delete anything older than millis/1000 - 24*60*60.
    // Has to be deleted only in array
    timeNow = millis() / 1000;
    for (int i=co2valuesPendingIndex; i < co2valuesPendingIndex+numberOfCO2values; i++) {
      if (co2valuesPending[i % numberOfCO2values].dateTime == -1) {continue;};
      if (timeNow - co2valuesPending[i % numberOfCO2values].dateTime > 24*60*60) {

        char buff[20];
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&co2valuesPending[i % numberOfCO2values].dateTime));
        if (doLogDuringMaintenance) {Log.printf("Will remove old pending CO2 value %d for interval %s, array index %d\r\n", co2valuesPending[i % numberOfCO2values].co2value, buff, i % numberOfCO2values);};
        co2valuesPending[i % numberOfCO2values].dateTime = -1;
        co2valuesPending[i % numberOfCO2values].co2value = 0;
        oldValuesFound = true;
      }
    }
  } else {
    // Delete anything older than UTC.now - 24*60*60
    // Take care. Also values without time (starting at 01.01.1970) will be deleted. So better call checkPendingCO2values() before.
    checkPendingCO2values();
    // Has to be deleted both in array and in storage (at least the entries with correct timestamp)
    timeNow = UTC.now();
    for (int i=co2valuesIndex; i < co2valuesIndex+numberOfCO2values; i++) {
      if (co2values[i % numberOfCO2values].dateTime == -1) {continue;};
      if (timeNow - co2values[i % numberOfCO2values].dateTime > 24*60*60) {
        time_t tempTime = co2values[i % numberOfCO2values].dateTime;
        char buff[20];
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&tempTime));
        if (doLogDuringMaintenance) {Log.printf("Will remove old CO2 value %d for interval %s, array index %d\r\n", co2values[i % numberOfCO2values].co2value, buff, i % numberOfCO2values);};
        if (!preferencesCO2sensorValues.remove(String(co2values[i % numberOfCO2values].dateTime).c_str())) {
          Log.printf("  also tried to remove value from storage for key %s, but this was not successful\r\n", String(co2values[i % numberOfCO2values].dateTime).c_str());
        }
        co2values[i % numberOfCO2values].dateTime = -1;
        co2values[i % numberOfCO2values].co2value = 0;
        oldValuesFound = true;
      }
    }
  }
  if (oldValuesFound) {
    if (!timeHelper_realTimeAvailable()) {
      sortValuesPendingAndSetIndex();
    } else {
      sortValuesAndSetIndex();
    }
  }
}

void Co2valuesArray::maintainValues() {
  checkPendingCO2values();
  deleteOldCO2values();
}

bool Co2valuesArray::deleteCO2value(const char * aDateTimeStr, bool pending) {
  time_t tDateTime;
  tDateTime = getEpochTime(std::string(aDateTimeStr), "%Y-%m-%d %H:%M:%S");

  co2value* co2valueArray;
  if (pending) {
    co2valueArray = co2valuesPending;
  } else {
    co2valueArray = co2values;
  }

  for (int i=0; i<numberOfCO2values; i++) {
    if (co2valueArray[i].dateTime == tDateTime) {
      Log.printf("CO2 value to be deleted found! Index is %d\r\n", i);
      preferencesCO2sensorValues.remove(String(tDateTime).c_str());
      co2valueArray[i].dateTime = -1;
      co2valueArray[i].co2value = 0;
      if (pending) {
        sortValuesPendingAndSetIndex();
      } else {
        sortValuesAndSetIndex();
      }
      return true;
    }
  }
  return false;
}