#include <XPT2046_Touchscreen.h>
#include <TouchEvent.h>
#if defined(ESP32)
  #include <WiFi.h>
#endif
#if defined(ESP8266)
  #include <ESP8266WiFi.h> 
#endif

#include "config.h"
#include "wifiCommunication.h"
#include "log.h"
#include "tft.h"
#include "drawGraphCO2values.h"
#include "senseair_s8.h"
#include "storage.h"

//prepare driver for touch screen
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
// init TouchEvent with pointer to the touch screen driver
TouchEvent tevent(touch);

// point on touchscreen that got hit
TS_Point p;
// current position
int tsx, tsy, tsxraw, tsyraw;

// checks if point x/y is inside rect[]
bool pointInRect(const int rect[], int x, int y, int allowedOffset = 0) {
  if (TFT_rotation == 1) {
    return
      (x >= rect[0]           - allowedOffset) &&
      (x <= rect[0] + rect[2] + allowedOffset) &&
      (y >= rect[1]           - allowedOffset) &&
      (y <= rect[1] + rect[3] + allowedOffset);
  } else if (TFT_rotation == 3) {
    return
      (tft_getWidth()  - x >= rect[0]           - allowedOffset) &&
      (tft_getWidth()  - x <= rect[0] + rect[2] + allowedOffset) &&
      (tft_getHeight() - y >= rect[1]           - allowedOffset) &&
      (tft_getHeight() - y <= rect[1] + rect[3] + allowedOffset);
  }
}

void onClick(TS_Point p) {
  // store x and y as raw data
  tsxraw = p.x;
  tsyraw = p.y;

  tsx = 320 - tsxraw;
  tsy = 240 - tsyraw;
  Log.printf("click %d %d\r\n", tsx, tsy);

  // Menu, always visible
  if (pointInRect(tftMainRect, tsx, tsy)) {
    if (screen == SCREEN_MAIN) {return;};
    screen = SCREEN_MAIN;
    tft_fillScreen();
    drawMenu();
    drawScreen();
  } else if (pointInRect(tftGraphRect, tsx, tsy)) {
    if (screen == SCREEN_GRAPH) {return;};
    screen = SCREEN_GRAPH;
    tft_fillScreen();
    drawMenu();
    drawScreen();
  } else if (pointInRect(tftSettingsRect, tsx, tsy)) {
    if (screen == SCREEN_SETTINGS) {return;};
    screen = SCREEN_SETTINGS;
    tft_fillScreen();
    drawMenu();
    drawScreen();
  } else if (pointInRect(tftOnOffRect, tsx, tsy)) {
    Log.printf("tft: on/off button hit\r\n");
    if (digitalRead(TFT_LED) == HIGH) {
      digitalWrite(TFT_LED, LOW);
    } else {
      digitalWrite(TFT_LED, HIGH);
    }

  } else if (screen == SCREEN_GRAPH) {
    if (pointInRect(tftGraphCO2Button, tsx, tsy, 30)) {
      Log.printf("tft: Graph CO2 button hit\r\n");
      graph = GRAPH_CO2;
      drawGraph();
#if defined(useIncidenceFromRKI)
    } else if (pointInRect(tftGraphIncidenceButton, tsx, tsy, 30)) {
      Log.printf("tft: Graph incidence button hit\r\n");
      graph = GRAPH_INCIDENCE;
      drawGraph();
#endif
    }

  } else if (screen == SCREEN_SETTINGS) {
    if (pointInRect(tftWiFiOnOffRect, tsx, tsy)) {
      if (wifiIsDisabled) {
        Log.printf("tft: WiFi off button hit -> will turn WiFi on\r\n");
        wifi_enable();
      } else {
        Log.printf("tft: WiFi on button hit -> will turn WiFi off\r\n");
        wifi_disable(true);
      }
      drawScreen();
    } else if (pointInRect(tftCalibrateCO2, tsx, tsy)) {
      Log.printf("tft: calibrate CO2 button hit\r\n");
      screen = SCREEN_SETTINGS_CONFIRM1;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    } else if (pointInRect(tftDisplayAutoOn, tsx, tsy)) {
      if (!autoTurnOnTFT) {
        Log.printf("tft: display always off button hit -> display will now automatically turn on\r\n");
        autoTurnOnTFT = true;
        setDisplayAutoTurnOnTFTSetting(true);
      } else {
        Log.printf("tft: display automatic button hit -> display will now stay off\r\n");
        autoTurnOnTFT = false;
        setDisplayAutoTurnOnTFTSetting(false);
      }
      drawScreen();
    }
  
  } else if (screen == SCREEN_SETTINGS_CONFIRM1) {
    if (pointInRect(tftConfirm1Yes, tsx, tsy)) {
      Log.printf("tft: first confirmation YES hit\r\n");
      screen = SCREEN_SETTINGS_CONFIRM2;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    } else if (pointInRect(tftConfirm1No, tsx, tsy)) {
      Log.printf("tft: first confirmation NO hit\r\n");
      screen = SCREEN_SETTINGS;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    }
  
  } else if (screen == SCREEN_SETTINGS_CONFIRM2) {
    if (pointInRect(tftConfirm2Yes, tsx, tsy)) {
      Log.printf("tft: second confirmation YES hit\r\n");
      screen = SCREEN_SETTINGS_CONFIRM3;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    } else if (pointInRect(tftConfirm2No, tsx, tsy)) {
      Log.printf("tft: second confirmation NO hit\r\n");
      screen = SCREEN_SETTINGS;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    }
  
  } else if (screen == SCREEN_SETTINGS_CONFIRM3) {
    if (pointInRect(tftConfirm3Yes, tsx, tsy)) {
      Log.printf("tft: third confirmation YES hit\r\n");

      co2_startBackgroundCalibration();
      
      // screen = SCREEN_SETTINGS; // stay here to see calibration status
      tft_fillScreen();
      drawMenu();
      drawScreen();
    } else if (pointInRect(tftConfirm3No, tsx, tsy)) {
      Log.printf("tft: third confirmation NO hit\r\n");
      screen = SCREEN_SETTINGS;
      tft_fillScreen();
      drawMenu();
      drawScreen();
    }
  }
}

void touch_init(void) {
  //start driver
  touch.begin();

  //init TouchEvent instance
  tevent.setResolution(tft_getWidth(),tft_getHeight());
  tevent.setDblClick(010);
//  tevent.registerOnTouchSwipe(onSwipe);
  tevent.registerOnTouchClick(onClick);
//  tevent.registerOnTouchDblClick(onDblClick);
//  tevent.registerOnTouchLong(onLongClick);
//  tevent.registerOnTouchDraw(onDraw);
//  tevent.registerOnTouchDown(onTouch);
//  tevent.registerOnTouchUp(onUntouch);

  Log.printf("  TFTtouch sucessfully initialized.\r\n");
}

void touch_processUserInput(void) {
  tevent.pollTouchScreen();
}