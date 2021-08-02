const char* const mqttCmndLogincidence   = "esp32_co2/cmnd/LOGincidence";
const char* const mqttStatLogincidence   = "esp32_co2/stat/LOGincidence";
const char* const mqttCmndLogCO2values   = "esp32_co2/cmnd/LOGCO2values";
const char* const mqttStatLogCO2values   = "esp32_co2/stat/LOGCO2values";

const char* const mqttCmndOTA            = "esp32_co2/cmnd/OTA";

const char* const mqttTeleState1         = "esp32_co2/tele/STATE1";
const char* const mqttTeleState2         = "esp32_co2/tele/STATE2";
const char* const mqttTeleState3         = "esp32_co2/tele/STATE3";

void mqtt_loop(void);
void mqtt_publish_tele(void);
