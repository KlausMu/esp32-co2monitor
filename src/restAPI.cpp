#include <WebServer.h>
#include <string>

#include "ArduinoJson.h"
#include "config.h"
#include "log.h"
#include "co2ValuesArray.h"
#include "incidenceMap.h"
#include "timeHelper.h"

#include "tft.h"

WebServer server(80);
StaticJsonDocument<250> jsonDocument;

/* ------------- get CO2 values ------------------------------------------------------------------------------ */
void handleGetCO2Values() {
  // check if pageno is available
  if (server.hasArg("pageno") == false) {
    server.send(400, "application/json", "missing query parameter pageno");
    return;
  }
  // check if pageno is in range
  int pageno = atoi(server.arg("pageno").c_str());
  byte maxPageNo = numberOfCO2values / 100;
  if ((pageno < 0) or (pageno > maxPageNo)) {
    server.send(400, "application/json", "pageno out of range (0 <= pageno <= " + String(maxPageNo) + ")");
    return;
  }

  DynamicJsonDocument jsondoc(6144);
  time_t dateTime;
  uint32_t co2value;
  char buff[20];
  String res;

  jsondoc.clear();
  JsonObject root = jsondoc.to<JsonObject>();

  // loop over values
  for (int i=pageno*100; i< min(pageno*100 + 100, numberOfCO2values); i++) {
    dateTime = co2values.getCO2values()[i].dateTime;
    co2value = co2values.getCO2values()[i].co2value;
    // Log.printf(" stat: %d, %d\r\n", root.memoryUsage(), root.size());
   
    // Variant 1:
    // {
    //   "2021-05-22 06:13:00": 525,
    //   "2021-05-22 06:14:00": 536,
    //   ...
    // // several keys with (dateTime == -1) wouldn't be possible anyway in JSON ...
    // if (dateTime == -1) {continue;}
    // strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&dateTime));
    // root[buff] = co2value;
    
    // Variant 2:
    // {
    //   "0": {
    //     "2021-05-21 18:01:00": 844
    //   },
    //   "1": {
    //     "2021-05-21 18:02:00": 845
    //   },
    //   ...
    JsonObject index = root.createNestedObject(String(i));
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&dateTime));
    index[buff] = co2value;
  }

  // return json
  if (strcmp(server.arg("pretty").c_str(), "1") == 0) {
    serializeJsonPretty(jsondoc, res);
  } else {
    serializeJson(jsondoc, res);
  }
  server.send(200, "application/json", res);
}

/* ------------- delete CO2 values ------------------------------------------------------------------------------ */
void handleDeleteCO2Values() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "no POST parameters found");
    return;
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  if (!jsonDocument.containsKey("dateTime")) {
    server.send(400, "application/json", "no dateTime parameter found");
    return;
  }

  String dateTime = jsonDocument["dateTime"];
  if (dateTime.length() != 19) {
    server.send(400, "application/json", "dateTime parameter has length " + String(dateTime.length()) + " and value " + dateTime + ", expected is length 19");
    return;
  }

  if (co2values.deleteCO2value(dateTime.c_str(), false)) {
    server.send(200, "application/json", "ok");
  } else {
    server.send(400, "application/json", "dateTime not found");
  }
}

/* ------------- get incidence values ------------------------------------------------------------------------------ */
void handleGetIncidence() {
  // check if pageno is available
  if (server.hasArg("pageno") == false) {
    server.send(400, "application/json", "missing query parameter pageno");
    return;
  }
  // check if regionID is available
  if (server.hasArg("regionID") == false) {
    server.send(400, "application/json", "missing query parameter regionID");
    return;
  }
  String regionID = server.arg("regionID");
  // find region
  IncidenceMap* aIncidenceMap = NULL;
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    if (strcmp(incidenceRegions[i].getRegionID().c_str(), regionID.c_str()) == 0) {
      aIncidenceMap = &incidenceRegions[i];
      break;
    }
  }
  if (aIncidenceMap == NULL) {
    server.send(400, "application/json", "no incidence available for regionID " + regionID);
    return;
  }
  
  // check if pageno is in range
  int pageno = atoi(server.arg("pageno").c_str());
  byte maxPageNo = aIncidenceMap->getMapIncidence().size() / 100;
  if ((pageno < 0) or (pageno > maxPageNo)) {
    server.send(400, "application/json", "pageno out of range (0 <= pageno <= " + String(maxPageNo) + ")");
    return;
  }

  DynamicJsonDocument jsondoc(6144);
  time_t date;
  float incidence;
  char buff[20];
  String res;
  int counter = 0;

  jsondoc.clear();
  JsonObject root = jsondoc.to<JsonObject>();

  // loop over values
  std::map<time_t, float>::iterator it;
  for (it = aIncidenceMap->getMapIncidence().begin(); it != aIncidenceMap->getMapIncidence().end(); it++) {
    if ((pageno*100 <= counter) && (counter < (pageno*100 + 100))) {
      date = it->first;
      incidence = it->second;
      strftime(buff, 20, "%Y-%m-%d", gmtime(&date));
      root[buff] = incidence;
    }
    counter++;
  }

  // return json
  if (strcmp(server.arg("pretty").c_str(), "1") == 0) {
    serializeJsonPretty(jsondoc, res);
  } else {
    serializeJson(jsondoc, res);
  }
  server.send(200, "application/json", res);
}

/* ------------- delete incidence value ------------------------------------------------------------------------------ */
void handleDeleteIncidence() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "no POST parameters found");
    return;
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  // check if regionID is available
  if (!jsonDocument.containsKey("regionID")) {
    server.send(400, "application/json", "missing query parameter regionID");
    return;
  }
  String regionID = jsonDocument["regionID"];
  // find region
  IncidenceMap* aIncidenceMap = NULL;
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    if (strcmp(incidenceRegions[i].getRegionID().c_str(), regionID.c_str()) == 0) {
      aIncidenceMap = &incidenceRegions[i];
      break;
    }
  }
  if (aIncidenceMap == NULL) {
    server.send(400, "application/json", "no incidence available for regionID " + regionID);
    return;
  }

  if (!jsonDocument.containsKey("date")) {
    server.send(400, "application/json", "no dateTime parameter found");
    return;
  }

  String date = jsonDocument["date"];
  if (date.length() != 10) {
    server.send(400, "application/json", "date parameter has length " + String(date.length()) + " and value " + date + ", expected is length 10");
    return;
  }

  if (aIncidenceMap->deleteIncidence(date.c_str())) {
    server.send(200, "application/json", "ok");
  } else {
    server.send(400, "application/json", "dateTime not found");
  }
}

/* ------------- put incidence value ------------------------------------------------------------------------------ */
void handlePutIncidence() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "no POST parameters found");
    return;
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

  // check if regionID is available
  if (!jsonDocument.containsKey("regionID")) {
    server.send(400, "application/json", "missing query parameter regionID");
    return;
  }
  String regionID = jsonDocument["regionID"];
  // find region
  IncidenceMap* aIncidenceMap = NULL;
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    if (strcmp(incidenceRegions[i].getRegionID().c_str(), regionID.c_str()) == 0) {
      aIncidenceMap = &incidenceRegions[i];
      break;
    }
  }
  if (aIncidenceMap == NULL) {
    server.send(400, "application/json", "no incidence available for regionID " + regionID);
    return;
  }

  // check parameter date
  if (!jsonDocument.containsKey("date")) {
    server.send(400, "application/json", "no date parameter found");
    return;
  }
  String date = jsonDocument["date"];
  if (date.length() != 10) {
    server.send(400, "application/json", "date parameter has length " + String(date.length()) + " and value " + date + ", expected is length 10");
    return;
  }
  time_t tDate;
  tDate = getEpochTime(std::string(date.c_str()), "%Y-%m-%d");

  // check parameter value
  if (!jsonDocument.containsKey("value")) {
    server.send(400, "application/json", "no value parameter found");
    return;
  }
  float value = jsonDocument["value"];

  if ((aIncidenceMap->addIncidence(tDate, value))) {
    server.send(200, "application/json", "ok");
  } else {
    server.send(400, "application/json", "dateTime not found");
  }
}

/* ------------- take screenshot ------------------------------------------------------------------------------ */
void handleScreenshot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  // here begin chunked transfer
  server.send(200, "image/bmp", "");

  const int width = tft_getWidth();      // image width in pixels
  const int height = tft_getHeight();     // height
  byte VH, VL;
  int i, j = 0;
  unsigned char bmFlHdr[14] = {
    'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0
  };
  // 54 = std total "old" Windows BMP file header size = 14 + 40
  unsigned char bmInHdr[40] = {
    40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 16, 0
  };   
  // 40 = info header size
  //  1 = num of color planes
  // 16 = bits per pixel
  // all other header info = 0, including RI_RGB (no compr), DPI resolution

  unsigned long fileSize = 2ul * height * width + 54; // pix data + 54 byte hdr
  
  bmFlHdr[ 2] = (unsigned char)(fileSize      ); // all ints stored little-endian
  bmFlHdr[ 3] = (unsigned char)(fileSize >>  8); // i.e., LSB first
  bmFlHdr[ 4] = (unsigned char)(fileSize >> 16);
  bmFlHdr[ 5] = (unsigned char)(fileSize >> 24);

  bmInHdr[ 4] = (unsigned char)(       width       );
  bmInHdr[ 5] = (unsigned char)(       width  >>  8);
  bmInHdr[ 6] = (unsigned char)(       width  >> 16);
  bmInHdr[ 7] = (unsigned char)(       width  >> 24);
  bmInHdr[ 8] = (unsigned char)(       height      );
  bmInHdr[ 9] = (unsigned char)(       height >>  8);
  bmInHdr[10] = (unsigned char)(       height >> 16);
  bmInHdr[11] = (unsigned char)(       height >> 24);

  server.sendContent(reinterpret_cast<const char*>(&bmFlHdr[0]), 14);
  server.sendContent(reinterpret_cast<const char*>(&bmInHdr[0]), 40);

  for (i = height; i > 0; i--) {
    unsigned char line[width*2];

    for (j = 0; j < width; j++) {

      uint16_t rgb = readPixA(j,i); // get pix color in rgb565 format
      
      VH = (rgb & 0xFF00) >> 8; // High Byte
      VL = rgb & 0x00FF;        // Low Byte
      
      //RGB565 to RGB555 conversion... 555 is default for uncompressed BMP
      //this conversion is from ...topic=177361.0 and has not been verified
      VL = (VH << 7) | ((VL & 0xC0) >> 1) | (VL & 0x1f);
      VH = VH >> 1;
      
      line[j*2]     = VL;
      line[j*2 + 1] = VH;
    }
    server.sendContent(reinterpret_cast<const char*>(&line[0]), width*2);
  }

  server.sendContent(F("")); // this tells web client that transfer is done
  server.client().stop();
}


void restAPI_setup() {
  server.on("/co2values", HTTP_GET,    handleGetCO2Values);    // http://<IPaddress>/co2values?pageno=00&pretty=1
  server.on("/co2values", HTTP_DELETE, handleDeleteCO2Values); // curl "http://<IPaddress>/co2values" -X DELETE -d "{ \"dateTime\": \"2021-05-22 19:10:00\"}" -H "Content-Type: application/json"

  server.on("/incidence", HTTP_GET,    handleGetIncidence);    // http://<IPaddress>/incidence?pageno=00&regionID=193&pretty=0
  server.on("/incidence", HTTP_DELETE, handleDeleteIncidence); // curl "http://<IPaddress>/incidence" -X DELETE -d "{\"regionID\": 193, \"date\": \"2021-05-23\"}" -H "Content-Type: application/json"
  server.on("/incidence", HTTP_PUT,    handlePutIncidence);    // curl "http://<IPaddress>/incidence" -X PUT -d "{\"regionID\": 194,\n\"date\": \"2021-05-23\",\n\"value\":68.1}"
  
  server.on("/screenshot", HTTP_GET,   handleScreenshot);      // http://<IPaddress>/screenshot


  // start server
  server.begin();

  Log.printf("  HTTP REST API sucessfully initialized.\r\n");
}

void restAPI_handle() {
  server.handleClient();
}
