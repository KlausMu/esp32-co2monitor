#include <Arduino.h>
#include <ArduinoOTA.h>
#if defined(ESP32)
  #include <WiFi.h>
#endif
#if defined(ESP8266)
  #include <ESP8266WiFi.h> 
#endif
#include <WiFiClient.h>
#include <PubSubClient.h>

#include "config.h"
#include "log.h"
#include "wifiCommunication.h"
#include "mqtt.h"
#include "senseair_s8.h"
#include "dht11.h"
#include "liIonVoltage.h"
#include "incidenceMap.h"
#include "co2ValuesArray.h"
#include "storage.h"

// https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/
// https://github.com/knolleary/pubsubclient
// https://gist.github.com/igrr/7f7e7973366fc01d6393

unsigned long reconnectInterval = 5000;
// in order to do reconnect immediately ...
unsigned long lastReconnectAttempt = millis() - reconnectInterval - 1;

void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;

PubSubClient mqttClient(mqtt_server, mqtt_server_port, callback, wifiClient);

bool checkMQTTconnection();

void mqtt_loop(){
  if (!mqttClient.connected()) {
    unsigned long currentMillis = millis();
    if ((currentMillis - lastReconnectAttempt) > reconnectInterval) {
      lastReconnectAttempt = currentMillis;
      // Attempt to reconnect
      checkMQTTconnection();
    }
  }  

  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

bool checkMQTTconnection() {
  if (wifiIsDisabled) return false;

  if (WiFi.isConnected()) {
    if (mqttClient.connected()) {
      return true;
    } else {
      // try to connect to mqtt server
      if (mqttClient.connect((char*) mqtt_clientName, (char*) mqtt_user, (char*) mqtt_pass)) {
        Log.printf("  Successfully connected to MQTT broker\r\n");
    
        // subscribes to messages with given topic.
        // Callback function will be called 1. in client.loop() 2. when sending a message
        // mqttClient.subscribe(mqttCmndReset);
        mqttClient.subscribe(mqttCmndLogincidence);
        mqttClient.subscribe(mqttCmndLogCO2values);
        mqttClient.subscribe(mqttCmndOTA);
      } else {
        Log.printf("  MQTT connection failed (but WiFi is available). Will try later ...\r\n");
      }
      return mqttClient.connected();
    }
  } else {
    Log.printf("  No connection to MQTT server, because WiFi ist not connected.\r\n");
    return false;
  }  
}

void publishMQTTMessage( const char *topic, const char *payload){
  if (wifiIsDisabled) return;

  if (checkMQTTconnection()) {
//  Log.printf("Sending mqtt payload to topic \"%s\": %s\r\n", topic, payload);
      
    if (mqttClient.publish(topic, payload)) {
      // Log.printf("Publish ok\r\n");
    }
    else {
      Log.printf("Publish failed\r\n");
    }
  } else {
    Log.printf("  Cannot publish mqtt message, because checkMQTTconnection failed (WiFi or mqtt is not connected)\r\n");
  }
}

void mqtt_publish_stat_logincidence() {
  publishMQTTMessage(mqttStatLogincidence, "OFF");
};

void mqtt_publish_stat_logCO2values() {
  publishMQTTMessage(mqttStatLogCO2values, "OFF");
};

void mqtt_publish_tele() {
  // maximum message length 128 Byte
  String payload = "";
  char buffer [50];

  // co2
  payload = "";
  payload += "{\"co2\":";
  payload += String(co2_value);
  payload += ",\"co2status\":";
  payload += String(co2_status);
  // payload += ",\"co2ABCperiod\":";
  // payload += String(co2_ABCperiod);
  payload += ",\"temp\":";
  sprintf(buffer, "%.1f", temp);
  payload += buffer;
  payload += ",\"hum\":";
  sprintf(buffer, "%.1f", hum);
  payload += buffer;
  payload += ",\"volt\":";
  sprintf(buffer, "%.2f", voltage);
  payload += buffer;
  payload += ",\"battstate\":";
  sprintf(buffer, "%d", batterystate);
  payload += buffer;
  payload += "}";
  publishMQTTMessage(mqttTeleState1, payload.c_str());

  // WiFi
  payload = "";
  payload += "{\"wifiRSSI\":";
  payload += WiFi.RSSI();
  payload += ",\"wifiChan\":";
  payload += WiFi.channel();
  payload += ",\"wifiSSID\":";
  payload += WiFi.SSID();
  payload += ",\"wifiBSSID\":";
  payload += WiFi.BSSIDstr();
  payload += "}";
  publishMQTTMessage(mqttTeleState2, payload.c_str());

  // ESP32 stats
  payload = "";
  payload += "{\"up\":";
  payload += String(millis());
  payload += ",\"heapSize\":";
  payload += String(ESP.getHeapSize());
  payload += ",\"heapFree\":";
  payload += String(ESP.getFreeHeap());
  payload += ",\"heapMin\":";
  payload += String(ESP.getMinFreeHeap());
  payload += ",\"heapMax\":";
  payload += String(ESP.getMaxAllocHeap());

  payload += ",\"nvsUsed\":";
  sprintf(buffer, "%d", nvsStatsUsedEntries);
  payload += buffer;
  payload += ",\"nvsTotal\":";
  sprintf(buffer, "%d", nvsStatsTotalEntries);
  payload += buffer;

  payload += "}";
  publishMQTTMessage(mqttTeleState3, payload.c_str());

}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  std::string strPayload(reinterpret_cast<const char *>(payload), length);

  Log.printf("MQTT message arrived [%s] %s\r\n", topic, strPayload.c_str());

  String topicReceived(topic);

  String topicCmndLogincidence(mqttCmndLogincidence);
  String topicCmndLogCO2values(mqttCmndLogCO2values);
  String topicCmndOTA(mqttCmndOTA);
  if (topicReceived == topicCmndLogincidence) {
    if (strPayload == "ON") {

      for (int i=0; i<numberOfIncidenceRegions; i++) {
        incidenceRegions[i].logAllIncidence();
      }

      mqtt_publish_stat_logincidence();
    } else {
      Log.printf("Payload %s not supported\r\n", strPayload.c_str());
    }
  } else if (topicReceived == topicCmndLogCO2values) {
    if (strPayload == "ON") {

      co2values.logAllCO2values();

      mqtt_publish_stat_logCO2values();
    } else {
      Log.printf("Payload %s not supported\r\n", strPayload.c_str());
    }
  } else if (topicReceived == topicCmndOTA) {
    if (strPayload == "ON") {
      Log.printf("MQTT command TURN ON OTA received\r\n");
      ArduinoOTA.begin();
    } else if (strPayload == "OFF") {
      Log.printf("MQTT command TURN OFF OTA received\r\n");
      ArduinoOTA.end();
    } else {
      Log.printf("Payload %s not supported\r\n", strPayload.c_str());
    }
  }
}
