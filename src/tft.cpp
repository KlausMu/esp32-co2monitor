#if defined(ESP32)
  #include <WiFi.h>
#endif
#if defined(ESP8266)
  #include <ESP8266WiFi.h> 
#endif

#include "config.h"
#include "log.h"
#include "wifiCommunication.h"
#include "tft.h"
#include "senseair_s8.h"
#include "dht11.h"
#include "liIonVoltage.h"
#include "storage.h"
#include "incidenceMap.h"
#include "co2ValuesArray.h"
#include "timeHelper.h"
#include "drawGraphCO2values.h"

#include <Adafruit_ILI9341.h>
// #include <Fonts/FreeSans9pt7b.h>
// #include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
// #include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <SD.h>

//prepare driver for display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

const GFXfont *myFont9;
// const GFXfont *myFont9Bold;
const GFXfont *myFont12;
const GFXfont *myFont18;
const GFXfont *myFont24;
int baseTextSize = 0;

// number of screen to display
int screen = SCREEN_MAIN;
bool autoTurnOnTFT = true;

uint16_t trafficLightBackgroundColor;
uint16_t trafficLightFontColor;

void setDimensionsOfElements(void);
void drawScreen(void);
void drawMenu(void);

void tft_init(void) {
  // switch display on
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LED_ON);
  tft.begin();
 
  // myFont = &FreeSans9pt7b;
  // myFont = &FreeMono9pt7b;
  myFont9     = &FreeSerif9pt7b;
  // myFont9Bold = &FreeSerifBold9pt7b;
  myFont12    = &FreeSerif12pt7b;
  myFont18    = &FreeSerif18pt7b;
  myFont24    = &FreeSerif24pt7b;
  // myFont = NULL;
  tft.setFont(myFont9);

  tft.setRotation(TFT_rotation); 

  setDimensionsOfElements();

  // clear screen
  tft.fillScreen(TFT_BLACK);
  drawMenu();

  // log the displays resolution
  Log.printf("  TFT sucessfully initialized.\r\n");
  Log.printf("  tftx = %d, tfty = %d\r\n", tft.width(), tft.height());
}

void turnTFTOn(void) {
  if (autoTurnOnTFT) {
    digitalWrite(TFT_LED, HIGH);
  }
}

// rect: x, y, width, heigth
int tftMainRect[4];
int tftGraphRect[4];
int tftSettingsRect[4];
int tftOnOffRect[4];
int tftWiFiOnOffRect[4];
int tftCalibrateCO2[4];
int tftDisplayAutoOn[4];
int tftConfirm1Yes[4];
int tftConfirm1No[4];
int tftConfirm2Yes[4];
int tftConfirm2No[4];
int tftConfirm3Yes[4];
int tftConfirm3No[4];

void setDimensionsOfElements(void) {
  // // upper left corner is 0,0
  // // width and heigth are only valid for landscape (rotation=1) or landscape upside down (rotation=3)
  // //                       ILI9341
  // //                       AZ-Touch
  // // tft.width   0 <= x <  320
  // // tft.height  0 <= y <  240

  tftMainRect[0] = 0;
  tftMainRect[1] = 180;
  tftMainRect[2] = 80;
  tftMainRect[3] = 60;

  tftGraphRect[0] = 80;
  tftGraphRect[1] = 180;
  tftGraphRect[2] = 80;
  tftGraphRect[3] = 60;

  tftSettingsRect[0] = 160;
  tftSettingsRect[1] = 180;
  tftSettingsRect[2] = 80;
  tftSettingsRect[3] = 60;

  tftOnOffRect[0] = 240;
  tftOnOffRect[1] = 180;
  tftOnOffRect[2] = 80;
  tftOnOffRect[3] = 60;

  tftWiFiOnOffRect[0] = 220;
  tftWiFiOnOffRect[1] = 15;
  tftWiFiOnOffRect[2] = 80;
  tftWiFiOnOffRect[3] = 40;
  
  tftCalibrateCO2[0] = 220;
  tftCalibrateCO2[1] = 70;
  tftCalibrateCO2[2] = 80;
  tftCalibrateCO2[3] = 40;

  tftDisplayAutoOn[0] = 220;
  tftDisplayAutoOn[1] = 125;
  tftDisplayAutoOn[2] = 80;
  tftDisplayAutoOn[3] = 40;

  tftConfirm1Yes[0] = 40;
  tftConfirm1Yes[1] = 85;
  tftConfirm1Yes[2] = 100;
  tftConfirm1Yes[3] = 60;

  tftConfirm1No[0] = 180;
  tftConfirm1No[1] = 85;
  tftConfirm1No[2] = 100;
  tftConfirm1No[3] = 60;

  tftConfirm2Yes[0] = 180;
  tftConfirm2Yes[1] = 85;
  tftConfirm2Yes[2] = 100;
  tftConfirm2Yes[3] = 60;

  tftConfirm2No[0] = 40;
  tftConfirm2No[1] = 85;
  tftConfirm2No[2] = 100;
  tftConfirm2No[3] = 60;

  tftConfirm3Yes[0] = 40;
  tftConfirm3Yes[1] = 85;
  tftConfirm3Yes[2] = 100;
  tftConfirm3Yes[3] = 60;

  tftConfirm3No[0] = 180;
  tftConfirm3No[1] = 85;
  tftConfirm3No[2] = 100;
  tftConfirm3No[3] = 60;
}

int16_t tft_getWidth(void) {
  return tft.width();
}

int16_t tft_getHeight(void) {
  return tft.height();
}

void tft_fillScreen(void) {
  tft.fillScreen(TFT_BLACK);
};

String percentEscaped = "%%";

void getUptime(char* uptimeStr) {
  unsigned long uptime = millis();
	unsigned long seconds = uptime / 1000;
	unsigned long minutes = seconds / 60;
	unsigned long hours = minutes / 60;
	unsigned long days = hours / 24;
  
  sprintf(uptimeStr, "%lud %02lu:%02lu:%02luh", days, hours % 24, minutes % 60, seconds %60);
  return;
}

/*
https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
                        AZ-Touch
              ST7735    ILI9341
  TextSize    Standard  FreeSans9pt7b FreeSerif9pt7b  FreeMono9pt7b  FreeSerifBoldItalic24pt7b
              y1/h
  1 pwm hum   0/8       -12/17        -11/16          -9/13          -30/40
  2 temp      0/16      -24/34        -22/32          -18/26         -60/80
  3           0/24      -36/51        -33/48          -27/39         -90/120
  4 countdown 0/32      -48/68        -44/64          -36/52         -120/160
  8           0/64      -96/136       -88/128         -72/104        -240/320
  15          0/120     -180/255      -165240         -135/195       -450/600
*/

void printText(int areaX, int areaY, int areaWidth, const char *str, uint8_t textSize, const GFXfont *f, uint16_t fontColor, uint16_t backgroundColor, bool xCentered = false) {
  // get text bounds
  GFXcanvas1 testCanvas(tft_getWidth(), tft_getHeight());
  int16_t x1; int16_t y1; uint16_t w; uint16_t h;
  testCanvas.setFont(f);
  testCanvas.setTextSize(textSize);
  testCanvas.setTextWrap(false);
  testCanvas.getTextBounds("0WIYgy,", 0, 0, &x1, &y1, &w, &h);
  // Log.printf("  x1 = %d, y1 = %d, w=%d, h=%d\r\n", x1, y1, w, h);
  int textHeight = h;
  int textAreaHeight = textHeight +2; // additional 2 px as vertical spacing between lines
  // y1=0 only for standardfont, with every other font this value gets negative!
  // This means that when using standarfont at (0,0), it really gets printed at 0,0.
  // With every other font, printing at (0,0) means that text starts at (0, y1) with y1 being negative!
  int textAreaOffset = -y1;
  // Don't know why, but a little additional correction has to be done for every font other than standard font. Doesn't work perfectly, sometimes position is wrong by 1 pixel
  //if (textAreaOffset != 0) {
    textAreaOffset = textAreaOffset + textSize +1;
  //}

  int16_t xOffsetForCentered = 0;
  if (xCentered) {
    testCanvas.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    xOffsetForCentered = float(areaWidth - w) / 2;
  }

  tft.setFont(f);
  tft.setTextSize(textSize);
  tft.setTextWrap(false);
  tft.setTextColor(fontColor);
  tft.fillRect (areaX, areaY, areaWidth, textAreaHeight, backgroundColor);
  tft.setCursor(areaX + xOffsetForCentered, areaY + textAreaOffset);
  tft.printf(str);
}

void drawMenu(void) {
  char buffer[100];

  tft.fillRect(tftMainRect[0],     tftMainRect[1],     tftMainRect[2],     tftMainRect[3],     screen == SCREEN_MAIN ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU);
  sprintf(buffer, Menu1Values);    // Values
  printText( tftMainRect[0], tftMainRect[1] + 20, tftMainRect[2], buffer, 1, myFont9, TFT_BLACK, screen == SCREEN_MAIN ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU, true);
  tft.drawRect(tftMainRect[0],     tftMainRect[1],     tftMainRect[2],     tftMainRect[3],     TFT_DARKGREY);

  tft.fillRect(tftGraphRect[0],    tftGraphRect[1],    tftGraphRect[2],    tftGraphRect[3],    screen == SCREEN_GRAPH ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU);
  sprintf(buffer, Menu2Graph);     // Graph
  printText( tftGraphRect[0], tftGraphRect[1] + 20, tftGraphRect[2], buffer, 1, myFont9, TFT_BLACK, screen == SCREEN_GRAPH ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU, true);
  tft.drawRect(tftGraphRect[0],    tftGraphRect[1],    tftGraphRect[2],    tftGraphRect[3],    TFT_DARKGREY);

  tft.fillRect(tftSettingsRect[0], tftSettingsRect[1], tftSettingsRect[2], tftSettingsRect[3], screen == SCREEN_SETTINGS ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU);
  sprintf(buffer, Menu3Settings);  // Settings
  printText( tftSettingsRect[0], tftSettingsRect[1] + 20, tftSettingsRect[2], buffer, 1, myFont9, TFT_BLACK, screen == SCREEN_SETTINGS ? TFT_LIGTHGREY_MENU : TFT_DARKGREY_MENU, true);
  tft.drawRect(tftSettingsRect[0], tftSettingsRect[1], tftSettingsRect[2], tftSettingsRect[3], TFT_DARKGREY);

  tft.fillRect(tftOnOffRect[0],    tftOnOffRect[1],    tftOnOffRect[2],    tftOnOffRect[3],    TFT_DARKGREY_MENU);
  sprintf(buffer, Menu4Onff);      // On/Off
  printText( tftOnOffRect[0], tftOnOffRect[1] + 20, tftOnOffRect[2], buffer, 1, myFont9, TFT_BLACK, TFT_DARKGREY_MENU, true);
  tft.drawRect(tftOnOffRect[0],    tftOnOffRect[1],    tftOnOffRect[2],    tftOnOffRect[3],    TFT_DARKGREY);
}

void drawMain(void) {
  char buffer[100];
  String lastUpdate;

  tft.fillRect(0, 0, 320, 180, trafficLightBackgroundColor);

  sprintf(buffer, ValuesBattery, batterystate, percentEscaped.c_str()); // Battery
  printText(258, 10, 62, buffer, 1, NULL, trafficLightFontColor, batterystate <=30 ? trafficLightBackgroundColor != TFT_RED ? TFT_RED : TFT_YELLOW : trafficLightBackgroundColor);

  byte offsetCO2 = 0;
#if !defined(useIncidenceFromRKI)
  offsetCO2 = 32;
#endif
  sprintf(buffer, "CO");
  printText( 20,  25 + offsetCO2,  66, buffer, 1, myFont24, trafficLightFontColor, trafficLightBackgroundColor);
  sprintf(buffer, "2");
  printText( 86,  51 + offsetCO2,  18, buffer, 1, myFont12, trafficLightFontColor, trafficLightBackgroundColor);
  sprintf(buffer, "%lu", co2_value);
  printText(140,  25 + offsetCO2, 130, buffer, 1, myFont24, trafficLightFontColor, trafficLightBackgroundColor);

#if defined(useIncidenceFromRKI)
  if (numberOfIncidenceRegions > 0) {
    // Works well with no, one or two regions. Adjust to your needs, if you have more than two regions.
    for (int i=0; i<numberOfIncidenceRegions; i++) {
      sprintf(buffer, incidenceRegions[i].getRegionName().c_str());
      printText( 20,  90 + i*20,  90, buffer, 1, myFont9, trafficLightFontColor, trafficLightBackgroundColor);
      if (incidenceRegions[i].getLastSevenDayIncidence() != -1) {
        sprintf(buffer, "%.1f", incidenceRegions[i].getLastSevenDayIncidence());
      } else {
        sprintf(buffer, ValuesIncidenceUnknown);         // unknown
      }
      printText(140,  90 + i*20, 100, buffer, 1, myFont9, trafficLightFontColor, trafficLightBackgroundColor);
    }  
    if (incidenceRegions[0].getLastSevenDayIncidence() != -1) {
      sprintf(buffer, ValuesIncidenceDate, incidenceRegions[0].getLastSevenDayIncidenceDate().substring(0,10).c_str());      // value is from date ...
      printText(20, 90+((numberOfIncidenceRegions-1)*20)+22, 120, buffer, 1, NULL, trafficLightFontColor, trafficLightBackgroundColor);
    }
  }
#endif
  
  getLocalTimeStr(buffer);
  printText( 20, 153,  120, buffer, 1, NULL, trafficLightFontColor, trafficLightBackgroundColor);
}

void drawGraphTft(void) {
  drawGraph();
}

void drawSettings(void) {
  char buffer[100];
  String tempStr;
  byte col1Left = 10; byte col1Width = 40;
  byte col2Left = 50; byte col2Width = 170;
  sprintf(buffer, "WiFi");
  printText(col1Left, 10,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  //                                              disconnecting                       connected                                        disconnected                       connecting
  WiFi.isConnected() ? wifiIsDisabled ? tempStr = WiFiStatusDisconnecting : tempStr = WiFiStatusConnected : wifiIsDisabled ? tempStr = WiFiStatusDisconnected : tempStr = WiFiStatusConnecting;
  sprintf(buffer, tempStr.c_str());
  printText(col2Left, 10,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "IP");
  printText(col1Left, 20,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "");
  printText(col2Left, 20,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "SSID");
  printText(col1Left, 30,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? String(WiFi.SSID()).c_str() : "");
  printText(col2Left, 30,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "BSSID");
  printText(col1Left, 40,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? String(WiFi.BSSIDstr()).c_str() : "");
  printText(col2Left, 40,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "AP");
  printText(col1Left, 50,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? accessPointName.c_str() : "");
  printText(col2Left, 50,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "Chan.");
  printText(col1Left, 60,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? String(WiFi.channel()).c_str() : "");
  printText(col2Left, 60,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "RSSI");
  printText(col1Left, 70,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  WiFi.isConnected() ? sprintf(buffer, "%sdBm", String(WiFi.RSSI()).c_str()) : sprintf(buffer, " ");
  printText(col2Left, 70,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);

  sprintf(buffer, "TX");
  printText(col1Left, 80,  col1Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);
  sprintf(buffer, WiFi.isConnected() ? String(WiFi.getTxPower()).c_str() : "");
  printText(col2Left, 80,  col2Width, buffer, 1, NULL, TFT_LIGHTGREEN_WIFI, TFT_BLACK);


  sprintf(buffer, "Batt"); // Battery
  printText(col1Left, 95,  col1Width, buffer, 1, NULL, TFT_LIGTHYELLOW_BATT, TFT_BLACK);
  sprintf(buffer, "%.2f%s, %d%s", voltage, "V", batterystate, percentEscaped.c_str());
  printText(col2Left, 95,  col2Width, buffer, 1, NULL, TFT_LIGTHYELLOW_BATT, TFT_BLACK);


  sprintf(buffer, "Uptime");
  printText(col1Left, 110,  col1Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
  getUptime(buffer);
  printText(col2Left, 110,  col2Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, "Heap");
  printText(col1Left, 120,  col1Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, "%d/%d/%d/%d", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  printText(col2Left, 120,  col2Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, "NVS");
  printText(col1Left, 130,  col1Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, "%d/%d used", nvsStatsUsedEntries, nvsStatsTotalEntries);
  printText(col2Left, 130,  col2Width, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);

  sprintf(buffer, SettingTemperature); // Temperature
  printText(col1Left, 145,  col1Width, buffer, 1, NULL, TFT_OCEANBLUE_TEMP, TFT_BLACK);
  sprintf(buffer, "%.1fC", temp);
  printText(col2Left, 145,  col2Width, buffer, 1, NULL, TFT_OCEANBLUE_TEMP, TFT_BLACK);
  sprintf(buffer, SettingHumidity); // Humidity
  printText(col1Left, 155,  col1Width, buffer, 1, NULL, TFT_OCEANBLUE_TEMP, TFT_BLACK);
  sprintf(buffer, "%.0f%s", hum, percentEscaped.c_str());
  printText(col2Left, 155,  col2Width, buffer, 1, NULL, TFT_OCEANBLUE_TEMP, TFT_BLACK);

  tft.fillRoundRect(tftWiFiOnOffRect[0],     tftWiFiOnOffRect[1],     tftWiFiOnOffRect[2],     tftWiFiOnOffRect[3],  4, TFT_LIGHTGREEN_WIFI);
  sprintf(buffer, SettingWiFi);               // WiFi
  printText(tftWiFiOnOffRect[0], tftWiFiOnOffRect[1] + 7,    tftWiFiOnOffRect[2], buffer, 1, NULL, TFT_BLACK, TFT_LIGHTGREEN_WIFI, true);
  if (wifiIsDisabled) {
    sprintf(buffer, SettingWiFiIsOff);        // at the moment, WiFi is off
    printText(tftWiFiOnOffRect[0], tftWiFiOnOffRect[1] + 7+15,    tftWiFiOnOffRect[2], buffer, 1, NULL, TFT_BLACK, TFT_LIGHTGREEN_WIFI, true);
  } else {
    sprintf(buffer, SettingWiFiIsOn);         // at the moment, WiFi is on
    printText(tftWiFiOnOffRect[0], tftWiFiOnOffRect[1] + 7+15,    tftWiFiOnOffRect[2], buffer, 1, NULL, TFT_BLACK, TFT_LIGHTGREEN_WIFI, true);
  }

  tft.fillRoundRect(tftCalibrateCO2[0],     tftCalibrateCO2[1],     tftCalibrateCO2[2],     tftCalibrateCO2[3],  4, TFT_LIGHTORANGE_CO2);
  sprintf(buffer, SettingCO2Sensor);          // CO2 sensor
  printText(tftCalibrateCO2[0], tftCalibrateCO2[1] + 7,    tftCalibrateCO2[2], buffer, 1, NULL, TFT_BLACK, TFT_LIGHTORANGE_CO2, true);
  sprintf(buffer, SettingCO2SensorCalibrate); // calibrate
  printText(tftCalibrateCO2[0], tftCalibrateCO2[1] + 7+15, tftCalibrateCO2[2], buffer, 1, NULL, TFT_BLACK, TFT_LIGHTORANGE_CO2, true);

  tft.fillRoundRect(tftDisplayAutoOn[0],     tftDisplayAutoOn[1],     tftDisplayAutoOn[2],     tftDisplayAutoOn[3],  4, TFT_RED_AUTODISPLAY);
  sprintf(buffer, SettingDisplay);            // Display?
  printText(tftDisplayAutoOn[0], tftDisplayAutoOn[1] + 7,    tftDisplayAutoOn[2], buffer, 1, NULL, TFT_BLACK, TFT_RED_AUTODISPLAY, true);
  if (!autoTurnOnTFT) {
    sprintf(buffer, SettingDisplayStaysOff);  // at the moment, display will NOT automatically turn on when reaching thresholds
    printText(tftDisplayAutoOn[0], tftDisplayAutoOn[1] + 7+15, tftDisplayAutoOn[2], buffer, 1, NULL, TFT_BLACK, TFT_RED_AUTODISPLAY, true);
  } else {
    sprintf(buffer, SettingDisplayAutomatic); // at the moment, display will automatically turn on when reaching thresholds
    printText(tftDisplayAutoOn[0], tftDisplayAutoOn[1] + 7+15, tftDisplayAutoOn[2], buffer, 1, NULL, TFT_BLACK, TFT_RED_AUTODISPLAY, true);
  }
}

void drawConfirm1(void) {
  char buffer[100];
  sprintf(buffer, CO2CalibrationConfirm1a); // Are you already for at least one minute
  printText( 20,  25,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, CO2CalibrationConfirm1b); // outdoor?
  printText( 20,  50,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);

  tft.fillRoundRect(tftConfirm1Yes[0],     tftConfirm1Yes[1],     tftConfirm1Yes[2],     tftConfirm1Yes[3],  4, TFT_RED_ABORT);
  sprintf(buffer, Confirm_yes);
  printText( tftConfirm1Yes[0], tftConfirm1Yes[1] + 20, tftConfirm1Yes[2], buffer, 1, myFont9, TFT_BLACK, TFT_RED_ABORT, true);

  tft.fillRoundRect(tftConfirm1No[0],     tftConfirm1No[1],     tftConfirm1No[2],     tftConfirm1No[3],  4, TFT_LIGHTORANGE_CO2);
  sprintf(buffer, Confirm_no);
  printText( tftConfirm1No[0],  tftConfirm1No[1] + 20,  tftConfirm1No[2],  buffer, 1, myFont9, TFT_BLACK, TFT_LIGHTORANGE_CO2, true);
}

void drawConfirm2(void) {
  char buffer[100];
  sprintf(buffer, CO2CalibrationConfirm2a); // Did the CO2 value already stabilized
  printText( 20,  25,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, CO2CalibrationConfirm2b); // outdoor?
  printText( 20,  50,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);

  tft.fillRoundRect(tftConfirm2No[0],  tftConfirm2No[1],  tftConfirm2No[2],  tftConfirm2No[3], 4, TFT_LIGHTORANGE_CO2);
  sprintf(buffer, Confirm_no);
  printText( tftConfirm2No[0],  tftConfirm2No[1] + 20,  tftConfirm2No[2],  buffer, 1, myFont9, TFT_BLACK, TFT_LIGHTORANGE_CO2, true);

  tft.fillRoundRect(tftConfirm2Yes[0], tftConfirm2Yes[1], tftConfirm2Yes[2], tftConfirm2Yes[3], 4, TFT_RED_ABORT);
  sprintf(buffer, Confirm_yes);
  printText( tftConfirm2Yes[0], tftConfirm2Yes[1] + 20, tftConfirm2Yes[2], buffer, 1, myFont9, TFT_BLACK, TFT_RED_ABORT, true);
}

void drawConfirm3(void) {
  char buffer[100];
  sprintf(buffer, CO2CalibrationConfirm3a);    // Last warning. Sensor value will be
  printText( 20,  25,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);
  sprintf(buffer, CO2CalibrationConfirm3b);    // set to 400. Proceed?
  printText( 20,  50,  300, buffer, 1, myFont9, TFT_WHITE, TFT_BLACK);

  tft.fillRoundRect(tftConfirm3Yes[0],     tftConfirm3Yes[1],     tftConfirm3Yes[2],     tftConfirm3Yes[3],  4, TFT_RED_ABORT);
  sprintf(buffer, Confirm_yes);
  printText( tftConfirm3Yes[0], tftConfirm3Yes[1] + 20, tftConfirm3Yes[2], buffer, 1, myFont9, TFT_BLACK, TFT_RED_ABORT, true);

  tft.fillRoundRect(tftConfirm3No[0],     tftConfirm3No[1],     tftConfirm3No[2],     tftConfirm3No[3],  4, TFT_LIGHTORANGE_CO2);
  sprintf(buffer, Confirm_no);
  printText( tftConfirm3No[0], tftConfirm3No[1] + 20, tftConfirm3No[2], buffer, 1, myFont9, TFT_BLACK, TFT_LIGHTORANGE_CO2, true);

  if (calibrationStatus == 0) {
    sprintf(buffer, CO2CalibrationStatus1);    // not yet started
  } else if (calibrationStatus == 1) {
    sprintf(buffer, CO2CalibrationStatus2);    // couldn't start calibration
  } else if (calibrationStatus == 2) {
    sprintf(buffer, CO2CalibrationStatus3);    // calibration started, awaiting calibration acknowledgement
  } else if (calibrationStatus == 3) {
    sprintf(buffer, CO2CalibrationStatus4);    // acknowledgement received
  };
  printText( 20,  160,  300, buffer, 1, NULL, TFT_WHITE, TFT_BLACK);
}

void setBackgroundTrafficLightColor() {
  if (co2_value == 0) {
    trafficLightBackgroundColor = TFT_BLACK;
    trafficLightFontColor = TFT_WHITE;
  } else if (co2_value < CO2_THRESHOLD_YELLOW) {
    trafficLightBackgroundColor = TFT_GREEN;
    trafficLightFontColor = TFT_BLACK;
  } else if (co2_value < CO2_THRESHOLD_RED) {
    trafficLightBackgroundColor = TFT_YELLOW;
    trafficLightFontColor = TFT_BLACK;
  } else {
    trafficLightBackgroundColor = TFT_RED;
    trafficLightFontColor = TFT_BLACK;
  } 
}

void drawScreen(void) {
  if (screen == SCREEN_MAIN) {
    setBackgroundTrafficLightColor();
    drawMain();
  } else if (screen == SCREEN_GRAPH) {
    drawGraphTft();
  } else if (screen == SCREEN_SETTINGS) {
    drawSettings();
  } else if (screen == SCREEN_SETTINGS_CONFIRM1) {
    drawConfirm1();
  } else if (screen == SCREEN_SETTINGS_CONFIRM2) {
    drawConfirm2();
  } else if (screen == SCREEN_SETTINGS_CONFIRM3) {
    drawConfirm3();
  }
}

uint16_t readPixA(int x, int y) { // get pixel color code in rgb565 format
    tft.startWrite();    //needed for low-level methods.  CS active
    tft.setAddrWindow(x, y, 1, 1);
    tft.writeCommand(0x2E); // memory read command.  sets DC

    uint8_t r, g, b;
    r = tft.spiRead(); // discard dummy read
    r = tft.spiRead();
    g = tft.spiRead();
    b = tft.spiRead();
    tft.endWrite();    //needed for low-level methods.  CS idle

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}
