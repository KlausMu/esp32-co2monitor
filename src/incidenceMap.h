#ifndef incidenceMap_h
#define incidenceMap_h

#include <Arduino.h>
#include <map>

class IncidenceMap {
  std::map<time_t, float> mapIncidence;
  bool doLogDuringRestore = false;
  bool doLogDuringMaintenance = true;
  String regionName;
  String regionID;
  uint16_t color;
  unsigned long lastUpdateFromRKI = 0;
public:
  IncidenceMap() {};
  IncidenceMap(String aRegionName, String aRegionID, uint16_t aColor) {
    regionName = aRegionName;
    regionID = aRegionID;
    color = aColor;
  }
  std::map<time_t, float> & getMapIncidence() {return mapIncidence;};
  bool getDoLogDuringRestore() {return doLogDuringRestore;};

  String getRegionName() {return regionName;};
  String getRegionID() {return regionID;};
  uint16_t getColor() {return color;};
  
  float getLastSevenDayIncidence();
  String getLastSevenDayIncidenceDate();
  
  unsigned long getLastUpdateFromRKI() {return lastUpdateFromRKI;};
  void setLastUpdateFromRKI(unsigned long aValue) {lastUpdateFromRKI = aValue;};
  
  void restoreAllIncidenceFromStorage();
  void restoreIncidenceFromStorage(time_t aDate, float aincidence);
  void logAllIncidence();
  void getUpdateFromRKI();
  bool addIncidence(time_t aDate, float aincidence);
  bool deleteIncidence(const char * aDateStr);
  void maintainValues();
};

const byte numberOfIncidenceRegions = 2;
extern IncidenceMap incidenceRegions[numberOfIncidenceRegions];

void incidenceMaps_init();
void incidenceMaps_maintainValues();

#endif /* incidenceMap_h */
