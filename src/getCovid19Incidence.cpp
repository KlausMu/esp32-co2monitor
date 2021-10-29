#include <WString.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <cstdlib>
#include <string>

#include "ArduinoJson.h"
#include "log.h"
#include "incidenceMap.h"

HTTPClient http;
int httpCode;
DynamicJsonDocument rki_doc(4096);
char buffer[100];

bool getCovid19incidenceForRegion(IncidenceMap& incidenceMap, float& incidence, String& dateOfIncidence) {
  bool res = false;
  if (!WiFi.isConnected()) {
    return res;
  }

  http.begin(
    "https://services7.arcgis.com/mOBPykOjAyBO2ZKk/arcgis/rest/services/RKI_Landkreisdaten/FeatureServer/0/query?where=OBJECTID=" +
    incidenceMap.getRegionID() +
    "&outFields=last_update,GEN,BEZ,cases7_per_100k,death7_bl,cases7_lk,death7_lk,cases7_per_100k_txt,cases7_bl_per_100k,cases7_bl&outSR=4326&f=json&returnGeometry=false");
  http.addHeader("Content-Type", "text/plain");
  httpCode = http.GET();
  if (httpCode <= 0) { //Check for the returning code
    Log.printf("Cannot get covid19 cases. Error on HTTP request\r\n");
  } else {
    String payload = http.getString();
    // Log.printf("payload = %s\r\n", payload.c_str());
    
    auto error = deserializeJson(rki_doc, payload);
    if (error) {
      Log.printf("Cannot get covid19 cases. deserializeJson() failed with code %s\r\n", error.c_str());
    } else if (rki_doc["features"].size() == 0) {
      Log.printf("Cannot get covid19 cases. Features are not included in Json.\r\n");
    } else {
      // strcpy(buffer, doc["features"][0]["attributes"]["BEZ"]);
      // Log.printf("BEZ: %s\r\n", buffer);
      // strcpy(buffer, doc["features"][0]["attributes"]["GEN"]);
      // Log.printf("GEN: %s\r\n", buffer);
      // strcpy(buffer, doc["features"][0]["attributes"]["cases7_per_100k_txt"]);
      // Log.printf("cases7_per_100k_txt: %s\r\n", buffer);
      strcpy(buffer, rki_doc["features"][0]["attributes"]["last_update"]);
      dateOfIncidence = buffer;
      // Log.printf("last_update: %s\r\n", buffer);

      incidence = rki_doc["features"][0]["attributes"]["cases7_per_100k"];
      // Log.printf("cases7_per_100k: %.1f\r\n", incidence);

      incidenceMap.setLastUpdateFromRKI(millis());
      res = true;
    }
  }
  http.end(); //Free the resources
  return res;
}
