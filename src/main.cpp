#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "log.h"
#include "wifiCommunication.h"
#include "mqtt.h"
#include "dht11.h"
#include "liIonVoltage.h"
#include "senseair_s8.h"
#include "storage.h"
#include "incidenceMap.h"
#include "co2ValuesArray.h"
#include "tft.h"
#include "tftTouch.h"
#include "timeHelper.h"
#include "restAPI.h"

#if defined(useOTAUpdate)
  // https://github.com/SensorsIot/ESP32-OTA
  #include "OTA.h"
  #if !defined(useOTA_RTOS)
    #include <ArduinoOTA.h>
  #endif
#endif
#if defined(useTelnetStream)
#include "TelnetStream.h"
#endif

unsigned long previousMillisProbe = 0;
unsigned long intervalProbe = 4000;      // this is the internal update interval of the CO2 sensor
unsigned long previousMillisPublish = 0;
unsigned long intervalPublish = 10000;

void setup()
{
  Serial.begin(115200);
  Serial.println();

  timeHelper_setup();

  storage_init();
#if defined(useIncidenceFromRKI)
  incidenceMaps_init();
  // maintenance for incidence only needed once at startup
  incidenceMaps_maintainValues();
#endif
  co2valuesArray_init();

  wifi_setup();
  if (!getWifiIsDisabledSetting()) {
    wifi_enable();
  }
  restAPI_setup();

#if defined(useOTAUpdate)
  // Do not start OTA. Save heap space and start it via MQTT only when needed.
  OTA_setup("CO2monitor");
#endif
#if defined(useTelnetStream)
  TelnetStream.begin();
#endif

  tft_init();
  autoTurnOnTFT = getDisplayAutoTurnOnTFTSetting();
  touch_init();

  co2_setup();
  dht11_setup();
  liIonVoltage_setup();

  drawScreen();
}

void loop()
{

#if defined(useOTAUpdate) && !defined(useOTA_RTOS)
// If you do not use FreeRTOS, you have to regulary call the handle method
  ArduinoOTA.handle();
#endif

  touch_processUserInput();
  mqtt_loop();
  restAPI_handle();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisProbe >= intervalProbe)
  {
    previousMillisProbe = currentMillis;

    co2_requestValueAndStatus();
    co2values.addCO2value(co2_value);

    if (calibrationStatus == 2) {
      co2_checkBackgroundCalibrationAck();
    }

    dht11_update();

    liIonVoltage_update();
  
    drawScreen();
  }

  if (currentMillis - previousMillisPublish >= intervalPublish)
  {
    previousMillisPublish = currentMillis;

    timeHelper_update();

#if defined(useIncidenceFromRKI)
    for (int i=0; i<numberOfIncidenceRegions; i++) {
      incidenceRegions[i].getUpdateFromRKI();
    }
#endif

    co2values.maintainValues();

    getNVSStatistics();

    mqtt_publish_tele();
  }
}
