#ifndef storage_h
#define storage_h

#include <ctime>
#include <Preferences.h>
#include "incidenceMap.h"
#include "co2ValuesArray.h"

extern Preferences preferencesIncidence;
extern Preferences preferencesCO2sensorValues;
extern Preferences preferencesSettings;

extern size_t nvsStatsUsedEntries;
extern size_t nvsStatsTotalEntries;

void storage_init(void);
void getNvsEntriesAndSaveToIncidenceMap(IncidenceMap& incidence);
void getNvsEntriesAndSaveToCO2Values(Co2valuesArray& co2values);

void getNVSStatistics();

bool getDisplayAutoTurnOnTFTSetting();
void setDisplayAutoTurnOnTFTSetting(bool aDisplayAutoTurnOnTFTSetting);
bool getWifiIsDisabledSetting();
void setWifiIsDisabledSetting(bool aWiFiIsDisabledSetting);

#endif /* storage_h */
