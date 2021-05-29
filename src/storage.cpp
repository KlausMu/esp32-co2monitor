#include <nvs.h>
#include <nvs_flash.h>
#include <esp_partition.h>
#include <Preferences.h>

#include "log.h"
#include "storage.h"
#include "timeHelper.h"


struct nvs_entry
{
  uint8_t  Ns ;         // Namespace ID
  uint8_t  Type ;       // Type of value
  uint8_t  Span ;       // Number of entries used for this item
  uint8_t  Rvs ;        // Reserved, should be 0xFF
  uint32_t CRC ;        // CRC
  char     Key[16] ;    // Key in Ascii
  uint64_t Data ;       // Data in entry 
} ;

struct nvs_page                                     // For nvs entries
{                                                   // 1 page is 4096 bytes
  uint32_t  State ;
  uint32_t  Seqnr ;
  
  uint32_t  Unused[5] ;
  uint32_t  CRC ;
  uint8_t   Bitmap[32] ;
  nvs_entry Entry[126] ;
} ;

const char* partitionName =            "nvsco2";
const esp_partition_t*    nvsco2 ;                          // Pointer to partition struct
esp_partition_iterator_t  pi ;                              // Iterator for find

const char* namespaceincidence       = "incidence";
int namespaceincidenceID;
const char* namespaceCO2sensorValues = "CO2values";
int namespaceCO2sensorValuesID;
const char* namespaceSettings        = "settings";

Preferences preferencesIncidence;
Preferences preferencesCO2sensorValues;
Preferences preferencesSettings;

size_t nvsStatsUsedEntries;
size_t nvsStatsTotalEntries;

bool isPartitionAvailable() {
  bool res = false;

  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,          // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,        // this partition
                            partitionName ) ;
  if (pi) {
    nvsco2 = esp_partition_get ( pi ) ;                       // Get partition struct
    esp_partition_iterator_release ( pi ) ;                   // Release the iterator
    Log.printf("  Partition %s found, %d bytes\r\n", partitionName, nvsco2->size );
    res = true;
  } else {
    Log.printf("  WARNING: Partition %s not found!\r\n", partitionName );
  }

  return res;
}

bool isPartitionInitialised() {
  bool res = false;

  esp_err_t  result = ESP_OK ;
  result = nvs_flash_init_partition(partitionName);
  if ( result != ESP_OK ) {
    Log.printf("  WARNING: Partition %s cannot be initialised!\r\n", partitionName );
  } else {
    Log.printf("  Partition %s is or already was successfully initialised!\r\n", partitionName );
    res = true;
  }

  return res;
}

void printNvsStats() {
  esp_err_t  result = ESP_OK ;
  nvs_stats_t nvs_stats;
  result = nvs_get_stats(partitionName, &nvs_stats);
  if ( result != ESP_OK ) {
    Log.printf("  WARNING: nvs_get_stats for partition %s failed!\r\n", partitionName );
  } else {
    Log.printf("  Statistics for nvs partition %s: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d), NameSpaceCount = (%d)\n",
      partitionName, nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries, nvs_stats.namespace_count);
  }
}

bool isNamespaceAvailable(const char* namespaceName) {
  bool res = false;

  esp_err_t  result = ESP_OK ;
  nvs_handle handle;
  result = nvs_open_from_partition(partitionName, namespaceName, NVS_READONLY, &handle);
  if ( result != ESP_OK ) {
    Log.printf("  WARNING: Namespace %s not found!\r\n", namespaceName );
  } else {
    size_t used_entries;
    size_t total_entries_namespace;
    if(nvs_get_used_entry_count(handle, &used_entries) == ESP_OK) {
        // the total number of entries occupied by the namespace
        total_entries_namespace = used_entries + 1;
        Log.printf("  Namespace %s found. Number of entries: %d\r\n", namespaceName, total_entries_namespace);
    }
    nvs_close(handle);
    res = true;
  }

  return res;
}

bool openPreferences(const char* namespaceName, Preferences& preferences) {
  bool res = false;
  res = preferences.begin(namespaceName, false, partitionName); // readonly doesn't create the namespace ...
  if (!res) {
    Log.printf("  WARNING: Couldn't initialise preferences for namespace %s\r\n", namespaceName);
  } else {
    // Log.printf("  Preferences successfully initialised for namespace %s, free entries: %d (number probably wrong)\r\n", namespaceName, preferences.freeEntries());
    Log.printf("  Preferences successfully initialised for namespace %s\r\n", namespaceName);
    // preferences.end(); // keep it open
  }
  return res;
}

void clearNamespace(const char* namespaceName, Preferences& preferences) {
  bool res;
  res = preferences.clear();
  if (!res) {
    Log.printf("  WARNING: Couldn't clear preferences for namespace %s\r\n", namespaceName);
  } else {
    Log.printf("  Namespace %s cleared\r\n", namespaceName);
  }
}

void putHistoricalIncidence() {
  // List of incidence for KA Stadt:  
  preferencesIncidence.putUInt("193_2021-05-01", 1490); // date: 2021-05-01, value: 149.0

  // List of incidence for KA Land:
  preferencesIncidence.putUInt("194_2021-05-01", 1530); // date: 2021-05-01, value: 153.0
}

void putHistoricalCO2() {
  preferencesCO2sensorValues.putUInt("1621157640", 439); // date: 2021-05-16 09:34:00, value: 439
  preferencesCO2sensorValues.putUInt("1621157700", 444); // date: 2021-05-16 09:35:00, value: 444
}

void getPartitionAndNamespaceIDs() {
  // --- partition ---------------
  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,          // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,        // this partition
                            partitionName ) ;
  if (pi) {
    nvsco2 = esp_partition_get ( pi ) ;                       // Get partition struct
    esp_partition_iterator_release ( pi ) ;                   // Release the iterator
    Log.printf("  Partition %s found, %d bytes\r\n", partitionName, nvsco2->size );
  } else {
    Log.printf("  WARNING: Partition %s not found!\r\n", partitionName );
  }

  // --- namespaceIDs ---------------
  nvs_page                  buf ;
  esp_err_t                 result = ESP_OK ;
  uint8_t                   pagenr = 0 ;                      // Page number in NVS
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint32_t                  offset = 0 ;                      // Offset in nvs partition

  while ( offset < nvsco2->size ) {
    result = esp_partition_read ( nvsco2, offset,                // Read 1 page in nvs partition
                                  &buf,
                                  sizeof(nvs_page) ) ;
    if ( result != ESP_OK ) {
      Log.printf("Error reading NVS!\r\n");
      return ;
    }
    
    i = 0 ;
    while ( i < 126 ) {
      bm = ( buf.Bitmap[i/4] >> ( ( i % 4 ) * 2 ) ) & 0x03 ;  // Get bitmap for this entry
      if ( bm == 2 ) {
        if ( buf.Entry[i].Ns == 0 ) {
          if ( strcmp ( namespaceincidence, buf.Entry[i].Key ) == 0 )  {
            namespaceincidenceID = buf.Entry[i].Data & 0xFF;
            Log.printf("  namespace %s found, ID=%d\r\n", namespaceincidence, namespaceincidenceID);
          }
          if ( strcmp ( namespaceCO2sensorValues, buf.Entry[i].Key ) == 0 )  {
            namespaceCO2sensorValuesID = buf.Entry[i].Data & 0xFF;
            Log.printf("  namespace %s found, ID=%d\r\n", namespaceCO2sensorValues, namespaceCO2sensorValuesID);
          }
        }
        i += buf.Entry[i].Span ;                              // Next entry
      } else {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
    pagenr++ ;
  }
}

void storage_init() {
  // WARNING: only if everything should be deleted, e.g. after partition resize!
  // nvs_flash_erase_partition(partitionName);
  // nvs_flash_init_partition(partitionName);

  // not necessary, checked in preferences.begin
  // if (!isPartitionAvailable()) return;
  // if (!isPartitionInitialised()) return;

  if (!openPreferences(namespaceincidence,       preferencesIncidence)) return;
  if (!openPreferences(namespaceCO2sensorValues, preferencesCO2sensorValues)) return;
  if (!openPreferences(namespaceSettings,        preferencesSettings)) return;

  getPartitionAndNamespaceIDs();

  printNvsStats();

  // not necessary, checked in preferences.begin
  // if (!isNamespaceAvailable(namespaceincidence)) return;
  // if (!isNamespaceAvailable(namespaceCO2sensorValues)) return;
  // if (!isNamespaceAvailable(namespaceSettings)) return;

  // Put some historical data, if needed
  // clearNamespace(namespaceincidence, preferencesIncidence);
  // putHistoricalIncidence();
  // clearNamespace(namespaceCO2sensorValues, preferencesCO2sensorValues);
  // putHistoricalCO2();
  // clearNamespace(namespaceSettings, preferencesSettings);
}

void getNvsEntriesAndSaveToIncidenceMap(IncidenceMap& incidence) {

  int namespaceID = namespaceincidenceID;
  Log.printf("  Will restore incidence map for region %s\r\n", incidence.getRegionName().c_str());

  String strKey;
  String tempRegionID;
  String strDate;
  time_t tDate;
  uint32_t uiincidenceValue;
  float_t flincidenceValue;
  
  // --- namespaceIDs ---------------
  nvs_page                  buf ;
  esp_err_t                 result = ESP_OK ;
  uint8_t                   pagenr = 0 ;                      // Page number in NVS
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint32_t                  offset = 0 ;                      // Offset in nvs partition

  while ( offset < nvsco2->size ) {
    result = esp_partition_read ( nvsco2, offset,                // Read 1 page in nvs partition
                                  &buf,
                                  sizeof(nvs_page) ) ;
    if ( result != ESP_OK ) {
      Log.printf("Error reading NVS!\r\n");
      return ;
    }
    
    i = 0 ;
    while ( i < 126 ) {
      bm = ( buf.Bitmap[i/4] >> ( ( i % 4 ) * 2 ) ) & 0x03 ;  // Get bitmap for this entry
      if ( bm == 2 ) {
        if ( buf.Entry[i].Ns == namespaceID ) {
          // Log.printf("-- key found: %s\r\n", buf.Entry[i].Key);
          strKey = String(buf.Entry[i].Key);
          // tempRegionID = atoi(strKey.substring(0, 3).c_str());
          tempRegionID = strKey.substring(0, 3).c_str();
          strDate = strKey.substring(4, 14).c_str();
          uiincidenceValue = uint32_t(buf.Entry[i].Data & 0xFFFFFFFF);
          flincidenceValue = float(uiincidenceValue) / 10;
          if (strcmp(tempRegionID.c_str(), incidence.getRegionID().c_str()) == 0) {
            if (incidence.getDoLogDuringRestore()) {Log.printf("    Storage entry: RegionID: %s, Date: %s, incidence: %.1f\r\n", tempRegionID.c_str(), strDate.c_str(), flincidenceValue);};
            tDate = getEpochTime(std::string(strDate.c_str()), "%Y-%m-%d");
            incidence.restoreIncidenceFromStorage(tDate, flincidenceValue);
          }
        }

        i += buf.Entry[i].Span ;                              // Next entry
      } else {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
    pagenr++ ;
  }
}

void getNvsEntriesAndSaveToCO2Values(Co2valuesArray& co2values) {

  int counter = 0;

  int namespaceID = namespaceCO2sensorValuesID;
  Log.printf("  Will restore co2values map\r\n");

  String strKey;
  time_t dateTime;
  char buff[20];
  uint32_t uiCO2value;
  
  // --- namespaceIDs ---------------
  nvs_page                  buf ;
  esp_err_t                 result = ESP_OK ;
  uint8_t                   pagenr = 0 ;                      // Page number in NVS
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint32_t                  offset = 0 ;                      // Offset in nvs partition

  while ( offset < nvsco2->size ) {
    result = esp_partition_read ( nvsco2, offset,                // Read 1 page in nvs partition
                                  &buf,
                                  sizeof(nvs_page) ) ;
    if ( result != ESP_OK ) {
      Log.printf("Error reading NVS!\r\n");
      return ;
    }
    
    i = 0 ;
    while ( i < 126 ) {
      bm = ( buf.Bitmap[i/4] >> ( ( i % 4 ) * 2 ) ) & 0x03 ;  // Get bitmap for this entry
      if ( bm == 2 ) {
        if ( buf.Entry[i].Ns == namespaceID ) {
          // Log.printf("-- key found: %s\r\n", buf.Entry[i].Key);
          strKey = String(buf.Entry[i].Key);
          dateTime = atoi(strKey.c_str());
          strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&dateTime));
          uiCO2value = uint32_t(buf.Entry[i].Data & 0xFFFFFFFF);
          if (co2values.getDoLogDuringRestore()) {Log.printf("    Storage entry found: date: %s, co2value: %d\r\n", buff, uiCO2value);};

          if (counter < numberOfCO2values) {
            co2values.restoreCO2valueFromStorage(dateTime, uiCO2value);
          } else {
            // Restore only 1440 values. Everything above will be removed.
            Log.printf("      WILL IGNORE AND REMOVE THIS ADDITIONAL STORAGE ENTRY\r\n");
            if (!preferencesCO2sensorValues.remove(buf.Entry[i].Key)) {
              Log.printf("        REMOVE WAS NOT SUCCESSFUL\r\n");
            }
          }
          counter++;
        }

        i += buf.Entry[i].Span ;                              // Next entry
      } else {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
    pagenr++ ;
  }
}

void getNVSStatistics() {
  esp_err_t  result = ESP_OK ;
  nvs_stats_t nvs_stats;
  result = nvs_get_stats(partitionName, &nvs_stats);
  if ( result != ESP_OK ) {
    nvsStatsUsedEntries = 0; nvsStatsTotalEntries = 0;
    // strcpy(strStatistics, "unknown");
  } else {
    nvsStatsUsedEntries = nvs_stats.used_entries;
    nvsStatsTotalEntries = nvs_stats.total_entries;
    Log.printf("NVS: %d/%d used\r\n", nvsStatsUsedEntries, nvsStatsTotalEntries);
  }
}

bool getDisplayAutoTurnOnTFTSetting() {
  return preferencesSettings.getBool("autoTurnOnTFT", true);
}
void setDisplayAutoTurnOnTFTSetting(bool aDisplayAutoTurnOnTFTSetting) {
  preferencesSettings.putBool("autoTurnOnTFT", aDisplayAutoTurnOnTFTSetting);
}
bool getWifiIsDisabledSetting() {
  return preferencesSettings.getBool("wifiIsDisabled", false);
}
void setWifiIsDisabledSetting(bool aWiFiIsDisabledSetting) {
  preferencesSettings.putBool("wifiIsDisabled", aWiFiIsDisabledSetting);
}
