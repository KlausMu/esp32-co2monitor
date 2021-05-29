#include <driver/gpio.h>
#include <esp32-hal-gpio.h>

// --- wifi ---------------------------------------------------------------------------------------------------------------------------------
const char* const wifi_ssid              = "YourWifiSSID";
const char* const wifi_password          = "YourWifiPassword";

// --- mqtt ---------------------------------------------------------------------------------------------------------------------------------
const char* const mqtt_server            = "IPAddressOfYourBroker";
const int mqtt_server_port               = 1883;
const char* const mqtt_user              = "myUser or empty";
const char* const mqtt_pass              = "myPassword or empty";
const char* const mqtt_clientName        = "esp32_co2";

// --- DHT11 --------------------------------------------------------------------------------------------------------------------------------
const int DHT_PIN = GPIO_NUM_32;                 // ADC1

// --- Senseair S8 --------------------------------------------------------------------------------------------------------------------------
const int TX_PIN = GPIO_NUM_17;  // blue         // ADC1
const int RX_PIN = GPIO_NUM_16;  // green        // ADC1
const int CO2_THRESHOLD_YELLOW      = 1000; // values above are treated as yellow
const int CO2_THRESHOLD_RED         = 2000; // values above are treated as red
const int CO2_THRESHOLD_MALFUNCTION = 4000; // values above are treated as malfunction

// --- tft ----------------------------------------------------------------------------------------------------------------------------------
const int TFT_CS                = GPIO_NUM_5 ;           //diplay chip select
const int TFT_DC                = GPIO_NUM_4 ;           //display d/c
const int TFT_RST               = GPIO_NUM_22;           //display reset
const int TFT_MOSI              = GPIO_NUM_23;           //diplay MOSI
const int TFT_CLK               = GPIO_NUM_18;           //display clock
const int TFT_LED               = GPIO_NUM_15;   // ADC2 //display background LED
const int TFT_MISO              = GPIO_NUM_19;           //display MISO
const int TFT_rotation          = 1; // use 1 (landscape) or 3 (landscape upside down), nothing else. 0 and 2 (portrait) will not give a nice result.

const int TOUCH_CS              = GPIO_NUM_14;   // ADC2
const int TOUCH_IRQ             = GPIO_NUM_27;   // ADC2 // touch screen interrupt
const int LED_ON                = HIGH;

// --- voltage ------------------------------------------------------------------------------------------------------------------------------
const int VOLTAGE_PIN           = GPIO_NUM_33;   // ADC1

// --- OTA Update ---------------------------------------------------------------------------------------------------------------------------
#define useOTAUpdate
#define useOTA_RTOS     // recommended

#if !defined(ESP32) && defined(useOTA_RTOS)
static_assert(false, "\"#define useOTA_RTOS\" is only possible with ESP32");
#endif
#if defined(useOTA_RTOS) && !defined(useOTAUpdate)
static_assert(false, "You cannot use \"#define useOTA_RTOS\" without \"#define useOTAUpdate\"");
#endif

#define useSerial
#define useTelnetStream

// --- language and timezone ----------------------------------------------------------------------------------------------------------------
#include "lang/en.h"

// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
#define myTimezone                 "Europe/Berlin"

// --- COVID-19 incidences ------------------------------------------------------------------------------------------------------------------
// Only activate if you are in Germany. Set your region in file "incidenceMap.cpp"
#define useIncidenceFromRKI

// ------------------------------------------------------------------------------------------------------------------------------------------

/*
ADC1
32 33 34 35 36 (37) (38) 39

ADC2 (WIFI)
00 02 04 12 13 14 15 25 26 27
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc.html
The ESP32 integrates two 12-bit SAR (Successive Approximation Register) ADCs, supporting a total of 18 measurement channels (analog enabled pins).
The ADC driver API supports ADC1 (8 channels, attached to GPIOs 32 - 39), and ADC2 (10 channels, attached to GPIOs 0, 2, 4, 12 - 15 and 25 - 27). However, the usage of ADC2 has some restrictions for the application:
ADC2 is used by the Wi-Fi driver. Therefore the application can only use ADC2 when the Wi-Fi driver has not started.
Some of the ADC2 pins are used as strapping pins (GPIO 0, 2, 15) thus cannot be used freely. Such is the case in the following official Development Kits:
ESP32 DevKitC: GPIO 0 cannot be used due to external auto program circuits.
ESP-WROVER-KIT: GPIO 0, 2, 4 and 15 cannot be used due to external connections for different purposes.
*/
