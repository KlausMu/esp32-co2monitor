#include <Arduino.h>
#include <iomanip>

#include "config.h"
#include "incidenceMap.h"
#include "storage.h"
#include "log.h"
#include "getCovid19incidence.h"
#include "timeHelper.h"
#include "ezTime.h"

/*
https://npgeo-corona-npgeo-de.hub.arcgis.com/datasets/917fc37a709542548cc3be077a786c17_0
In the map, click on your region and look for the value "ObjectID" in the table. Put this value in file "incidenceMap.cpp". You can even provide two or more regions.
*/
IncidenceMap incidenceRegions[numberOfIncidenceRegions] {
  // take care to set numberOfIncidenceRegions accordingly  
  {"KA Stadt", "193", 0x72F4},
  {"KA Land",  "194", 0xBA15}
};

int updateIntervalInMinutes = 60;

void incidenceMaps_init() {
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    incidenceRegions[i].restoreAllIncidenceFromStorage();
  }
}

void incidenceMaps_maintainValues() {
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    incidenceRegions[i].maintainValues();
  }
}

void IncidenceMap::restoreAllIncidenceFromStorage() {
  mapIncidence.clear();
  getNvsEntriesAndSaveToIncidenceMap(*this);
}

void IncidenceMap::restoreIncidenceFromStorage(time_t aDate, float aincidence) {
  if (mapIncidence.find(aDate) == mapIncidence.end()) {
    mapIncidence.insert(std::make_pair(aDate, aincidence));
  }
}

void IncidenceMap::logAllIncidence() {
  std::map<time_t, float>::iterator it;
  Log.printf("List of incidence for %s:\r\n", regionName.c_str());
  char buff[20];
  time_t date;
  for (it = mapIncidence.begin(); it != mapIncidence.end(); it++) {
    date = it->first;
    strftime(buff, 20, "%Y-%m-%d", gmtime(&date));
    Log.printf("preferencesIncidence.putUInt(\"%s_%s\", %d); // date: %s, value: %.1f\r\n", regionID.c_str(), buff, int(it->second*10), buff, it->second);
  }
}

void IncidenceMap::getUpdateFromRKI() {
  if ( ((millis() - lastUpdateFromRKI) < updateIntervalInMinutes*60*1000) && (lastUpdateFromRKI != 0) ) {
    return;
  }
  float newIncidence; String newIncidenceDate;
  if (getCovid19incidenceForRegion(*this, newIncidence, newIncidenceDate)) {

    newIncidence = floor(newIncidence*10 + 0.5) / 10;

    struct std::tm tm = {0,0,0,0,0,0,0,0,0};
    tm.tm_year = atoi(newIncidenceDate.substring(6,10).c_str()) - 1900;
    tm.tm_mon  = atoi(newIncidenceDate.substring(3,5).c_str())  - 1;
    tm.tm_mday = atoi(newIncidenceDate.substring(0,2).c_str());
  
    std::time_t dateOfSevenDayincidence = mktime(&tm);
    // char buff[20];
    // strftime(buff, 20, "%Y-%m-%d", gmtime(&dateOfSevenDayincidence));
    // Log.printf("  RKI data for date %s\r\n", buff);
    addIncidence(dateOfSevenDayincidence, newIncidence);
  }
}

bool IncidenceMap::addIncidence(time_t aDate, float aincidence) {
  if (mapIncidence.find(aDate) == mapIncidence.end()) {
    Log.printf("New RKI incidence for %s, will add it to map and storage\r\n", regionName.c_str());
    mapIncidence.insert(std::make_pair(aDate, aincidence));

    char buff[20];
    strftime(buff, 20, "%Y-%m-%d", gmtime(&aDate));
    String strKey = regionID + "_" + buff;
    // Log.printf("  regionID %s\r\n", regionID.c_str());
    // Log.printf("  key %s\r\n", strKey.c_str());
    preferencesIncidence.putUInt(strKey.c_str(), aincidence*10);
    return true;
  } else {
    Log.printf("Already known RKI incidence for %s, will not add it\r\n", regionName.c_str());
    return false;
  }
}

float IncidenceMap::getLastSevenDayIncidence() {
  if (mapIncidence.empty()) {return -1;};

  return mapIncidence.rbegin()->second;
}

String IncidenceMap::getLastSevenDayIncidenceDate() {
  if (mapIncidence.empty()) {return "";};

  time_t date = time_t(mapIncidence.rbegin()->first);
  char buff[22];
  strftime(buff, 22, "%d.%m.%Y, %H:%M Uhr", gmtime(&date));  // e.g. 16.05.2021, 00:00 Uhr
  return buff;
}

void IncidenceMap::maintainValues() {
  if (!timeHelper_realTimeAvailable()) {return;};
  time_t timeNow;

  std::map<time_t, float>::iterator it;
  // Delete everything older than 1,5 year: UTC.now - 24*60*60*365*1,5
  // Has to be deleted both in map and in storage (at least the entries with correct timestamp)
  timeNow = UTC.now();
  for (it = mapIncidence.begin(); it != mapIncidence.end();) { // no increment }; it++) {
    if (timeNow - it->first > 1.5*365*24*60*60) {
      time_t tempTime = it->first;
      char buff[20];
      strftime(buff, 20, "%Y-%m-%d", gmtime(&it->first));
      if (doLogDuringMaintenance) {Log.printf("Will remove old incidence value: %s, %.1f\r\n", buff, it->second);};
      preferencesIncidence.remove(String(tempTime).c_str());
      mapIncidence.erase(it++);
    } else {
      it++;
    }
  }
}

bool IncidenceMap::deleteIncidence(const char * aDateStr) {
  time_t tDateTime;
  tDateTime = getEpochTime(std::string(aDateStr), "%Y-%m-%d");

  std::map<time_t, float>::iterator it;
  for (it = mapIncidence.begin(); it != mapIncidence.end();) { // no increment }; it++) {
    if (it->first == tDateTime) {
      Log.printf("Incidence to be deleted found!\r\n");
      String strKey = regionID + "_" + aDateStr;
      if (!preferencesIncidence.remove(strKey.c_str())) {
        Log.printf("  Tried to remove value from storage for key %s, but this was not successful\r\n", strKey.c_str());
      }
      mapIncidence.erase(it++);
      return true;
    } else {
      it++;
    }
  }
  return false;
}
