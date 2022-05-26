#include "config.h"
#include "drawGraphCO2values.h"
#include "co2ValuesArray.h"
#include "incidenceMap.h"
#include "tft.h"
#include "log.h"
#include "ezTime.h"
#include "timeHelper.h"

const int CO2_GRAPH_MINVALUE_Y = 400;
const int CO2_GRAPH_MAXVALUE_Y = 2500;

int tftGraphCO2Button[4]       = { 80,10,40,25};
int tftGraphIncidenceButton[4] = {200,10,40,25};

int graph = GRAPH_CO2;

void drawButtons() {
  char buffer[20];

  tft.fillRoundRect(tftGraphCO2Button[0],       tftGraphCO2Button[1],       tftGraphCO2Button[2],       tftGraphCO2Button[3],       4, graph == GRAPH_CO2       ? TFT_LIGHTORANGE_CO2 : TFT_DARKORANGE_CO2);
  sprintf(buffer, GraphCO2);               // CO2
  printText(tftGraphCO2Button[0],       tftGraphCO2Button[1]       + 7,  tftGraphCO2Button[2],       buffer, 1, NULL, TFT_BLACK, graph == GRAPH_CO2       ? TFT_LIGHTORANGE_CO2 : TFT_DARKORANGE_CO2, true);

  tft.fillRoundRect(tftGraphIncidenceButton[0], tftGraphIncidenceButton[1], tftGraphIncidenceButton[2], tftGraphIncidenceButton[3], 4, graph == GRAPH_INCIDENCE ? TFT_LIGHTGREEN_WIFI : TFT_DARKGREEN_WIFI);
  sprintf(buffer, GraphIncidence);         // Inzid.
  printText(tftGraphIncidenceButton[0], tftGraphIncidenceButton[1] + 7,  tftGraphIncidenceButton[2], buffer, 1, NULL, TFT_BLACK, graph == GRAPH_INCIDENCE ? TFT_LIGHTGREEN_WIFI : TFT_DARKGREEN_WIFI, true);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// -- CO2 helper -------

int16_t getXForTime(time_t aDateTime, time_t minDateTime, time_t maxDateTime) {
  // 0..319
  if (aDateTime < minDateTime) {
    return 0;    
  } else if (aDateTime > maxDateTime) {
    return 319;    
  } else {
    return (float(aDateTime - minDateTime) / (maxDateTime-minDateTime) * 319);
  }
}

int16_t getYForCO2value(uint32_t aValue) {
  // 0..179: 400:179, 2500:0
  if (aValue < CO2_GRAPH_MINVALUE_Y) {
    return 179;    
  } else if (aValue > CO2_GRAPH_MAXVALUE_Y) {
    return 0;    
  } else {
    return 179 - (float(aValue - CO2_GRAPH_MINVALUE_Y) / (CO2_GRAPH_MAXVALUE_Y-CO2_GRAPH_MINVALUE_Y) * 179);
  }
}

void drawCO2values() {
  bool timeAvailable = (timeHelper_realTimeAvailable());

  int16_t xlast = -1; int16_t ylast = -1;
  int16_t x; int16_t y;
  time_t dateTime;
  time_t minDateTime = 0;
  time_t maxDateTime = 0;

  co2value* co2valueArray;
  if (timeAvailable) {
    co2valueArray = co2values.getCO2values();
  } else {
    co2valueArray = co2values.getCO2valuesPending();
  }

  // --- get min and max values -----------------------------------------------------------
  if (timeAvailable) {
    maxDateTime = UTC.now();
    minDateTime = maxDateTime;
  } else {
    maxDateTime = millis() / 1000;
    minDateTime = maxDateTime;
  }
  for (int i=0; i<numberOfCO2values; i++) {
    if (co2valueArray[i].dateTime > maxDateTime) {
      // ignore everything after maxDateTime. Should never happen
    } else {
      minDateTime = min(minDateTime, co2valueArray[i].dateTime);
    }
  }

  // minDateTime should always be at most 24h before maxDateTime
  if (maxDateTime - minDateTime > 24*60*60) {
    minDateTime = maxDateTime - 24*60*60;
  }
  // show at least one hour
  if (maxDateTime - minDateTime < 60*60) {
    minDateTime = maxDateTime - 60*60;
  }
  
  // --- horizontal line/areas for thresholds ----------------------------
  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);
  bool drawColouredBackground = false;
  if (drawColouredBackground) {
    tft.fillRect(0, 0,                                     320, getYForCO2value(CO2_THRESHOLD_RED),    TFT_RED_XAXIS);
    tft.fillRect(0, getYForCO2value(CO2_THRESHOLD_RED),    320, getYForCO2value(CO2_THRESHOLD_YELLOW)-getYForCO2value(CO2_THRESHOLD_RED), TFT_YELLOW_XAXIS);
    tft.fillRect(0, getYForCO2value(CO2_THRESHOLD_YELLOW), 320, getYForCO2value(CO2_GRAPH_MINVALUE_Y)-getYForCO2value(CO2_THRESHOLD_YELLOW), TFT_GREEN_XAXIS);
  } else {
    tft.fillRect(0, 0, 320, 180, TFT_BLACK);
    tft.drawLine(0, getYForCO2value(CO2_THRESHOLD_RED),    319, getYForCO2value(CO2_THRESHOLD_RED),    TFT_RED_XAXIS);
    tft.drawLine(0, getYForCO2value(CO2_THRESHOLD_YELLOW), 319, getYForCO2value(CO2_THRESHOLD_YELLOW), TFT_YELLOW_XAXIS);
  }
  tft.setCursor(3,  42-10); tft.printf("%d", CO2_THRESHOLD_RED);
  tft.setCursor(3, 127-10); tft.printf("%d", CO2_THRESHOLD_YELLOW);
  tft.setCursor(3, 179-10); tft.printf("%d", CO2_GRAPH_MINVALUE_Y);

  
  // --- vertical line for every hour --------------------------------------------
  tft.setTextColor(TFT_LIGHTGREY);
  float numberOfHours = float(maxDateTime - minDateTime) / (60*60);
  // Log.printf("  number of hours %.1f\r\n", numberOfHours);
  
  int labelEveryXHours;
  /* if (numberOfHours >= 24) {
    labelEveryXHours = 6;
  } else */ if (numberOfHours >= 18) {
    labelEveryXHours = 4;
  } else if (numberOfHours >= 12) {
    labelEveryXHours = 3;
  } else if (numberOfHours >= 6) {
    labelEveryXHours = 2;
  } else if (numberOfHours >= 3) {
    labelEveryXHours = 1;
  } else if (numberOfHours > 1) {
    labelEveryXHours = 1;
  } else { // if (numberOfHours <= 1) {
    // will not be used
    labelEveryXHours = 1;
  }

  if (numberOfHours > 1) {
    // More than one hour will be shown. Draw hour labels for x axis
    for (int i=0; i <= numberOfHours; i++) {
      // Log.printf("  Axis for hour %d\r\n", i);
      tft.drawLine(getXForTime(maxDateTime - 60*60*i, minDateTime, maxDateTime), 174, getXForTime(maxDateTime - 60*60*i, minDateTime, maxDateTime), 179, TFT_LIGHTGREY);
      if ((i % labelEveryXHours == 0) && (i != 0) && (i != numberOfHours)) {
        tft.setCursor(319-(320/numberOfHours*i) - 10, 164);
        tft.printf("-%dh", i);
      }
    }
  } else {
    // Only one hour or even less will be displayed. Draw minute labels for x axis (every 10 minutes)
    float numberOfMinutes = float(maxDateTime - minDateTime) / (60);
    // Log.printf("  numberOfMinutes %.1f\r\n", numberOfMinutes);
    for (int i=0; i <= (numberOfMinutes/10); i++) {
      // Log.printf("  Axis for minute %d\r\n", i*10);
      tft.drawLine(getXForTime(maxDateTime - 10*60*i, minDateTime, maxDateTime), 174, getXForTime(maxDateTime - 10*60*i, minDateTime, maxDateTime), 179, TFT_LIGHTGREY);
      if ((i != 0) && (i != (numberOfMinutes/10))) {
        tft.setCursor(319-(320/(numberOfMinutes/10)*i) - 10, 164);
        tft.printf("-%dm", i*10);
      }
    }
  }

  // --- Draw buttons ----------------------------------------------------------------
#if defined(useIncidenceFromRKI)
  drawButtons();
#endif

  // --- Draw values ----------------------------------------------------------------
  for (int i=0; i<numberOfCO2values; i++) {
    if (co2valueArray[i].dateTime == -1) {continue;};
    dateTime = co2valueArray[i].dateTime;
    // char buff[20];
    // Log.printf("  raw time %ld\r\n", dateTime);
    // strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&dateTime));
    // Log.printf("  draw %s\r\n", buff);
    
    if ((dateTime < minDateTime) || (dateTime > maxDateTime)) {
      continue;
    }
    
    x = getXForTime(dateTime, minDateTime, maxDateTime);
    y = getYForCO2value(co2valueArray[i].co2value);

    // Log.printf("  will draw pixel %d,%d\r\n", x, y);
    if ((xlast != -1) && (x - xlast <= 13)) { // 60/1440*320 ~ 13, so only less than one hour of missing data will be filled with line
    //   tft.drawPixel(x,y,TFT_WHITE);
      tft.drawLine(xlast, ylast, x, y, TFT_LIGHTORANGE_CO2);
    } else {
      tft.drawPixel(x, y, TFT_LIGHTORANGE_CO2);
    }
    xlast = x; ylast = y;
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// -- Incidence helper -------

int16_t getXForDay(time_t aDay, time_t minDay, time_t maxDay) {
  // 0..319
  if (aDay < minDay) {
    return 0;    
  } else if (aDay > maxDay) {
    return 319;    
  } else {
    return (float(aDay - minDay) / (maxDay-minDay) * 319);
  }
}

int16_t getYForIncidence(uint32_t aValue, uint32_t minValue, uint32_t maxValue) {
  if (aValue < minValue) {
    return 179;    
  } else if (aValue > maxValue) {
    return 0;    
  } else {
    return 179 - (float(aValue - minValue) / (maxValue-minValue) * 179);
  }
}

void drawIncidence() {
  if (numberOfIncidenceRegions == 0) {return;};

  bool timeAvailable = (timeHelper_realTimeAvailable());

  tft.fillRect(0, 0, 320, 180, TFT_BLACK);

  int16_t xlast = -1; int16_t ylast = -1;
  int16_t x; int16_t y; int16_t xnext;
  time_t date;
  time_t minDate = 0;
  time_t maxDate = 0;
  
  std::map<time_t, float>::iterator it;
  // --- get min and max values -----------------------------------------------------------
  if (timeAvailable) {
    maxDate = UTC.now();
  }
  float minIncidence = 0; float maxIncidence = 1000;

  // init values
  it = incidenceRegions[0].getMapIncidence().begin();
  if (it != incidenceRegions[0].getMapIncidence().end()) {
    minDate = it->first;
    if (!timeAvailable) {maxDate = it->first;};
    minIncidence = it->second;
    maxIncidence = it->second;
  }
  // loop over values
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    for (it = incidenceRegions[i].getMapIncidence().begin(); it != incidenceRegions[i].getMapIncidence().end(); it++) {
      minDate = min(minDate, it->first);
      if (!timeAvailable) {maxDate = max(maxDate, it->first);};
      minIncidence = min(minIncidence, it->second);
      maxIncidence = max(maxIncidence, it->second);
    }
  }
  // Add one day to maxDate, so that graph shows line even at last day
  time_t realMaxDate = maxDate;
  struct tm * ptm;
  ptm = gmtime(&maxDate);
  ptm->tm_hour = 0;
  ptm->tm_min = 0;
  ptm->tm_sec = 0;
  ptm->tm_mday = ptm->tm_mday + 1;
  maxDate = mktime(ptm);

  minIncidence = 0;
  maxIncidence = maxIncidence * 1.1;
  if (maxIncidence < 19) {
    maxIncidence = 19;
  }
  // Log.printf("minDate %ld, maxDate %ld, minInc %.1f, maxInc %.1f\r\n", minDate, maxDate, minIncidence, maxIncidence);

  // --- horizontal lines --------------------------------------------------------------
  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);
  if (maxIncidence >= 100) {
    for (int i=1; i*50 <= maxIncidence; i++) {
      tft.drawLine(0, getYForIncidence( i*50, minIncidence, maxIncidence), 319, getYForIncidence( i*50, minIncidence, maxIncidence), TFT_INCIDENCE_XAXIS);
      tft.setCursor(0, getYForIncidence( i*50, minIncidence, maxIncidence) - 10);
      tft.printf("%d", i*50);
    }
  } else {
    for (int i=1; i*10 <= maxIncidence; i++) {
      tft.drawLine(0, getYForIncidence( i*10, minIncidence, maxIncidence), 319, getYForIncidence( i*10, minIncidence, maxIncidence), TFT_INCIDENCE_XAXIS);
      tft.setCursor(0, getYForIncidence( i*10, minIncidence, maxIncidence) - 10);
      tft.printf("%d", i*10);
    }
  }

  // --- Draw buttons ----------------------------------------------------------------
  drawButtons();

  // --- Draw values ----------------------------------------------------------------
  for (int i=0; i<numberOfIncidenceRegions; i++) {
    xlast = -1; ylast = -1;
    for (it = incidenceRegions[i].getMapIncidence().begin(); it != incidenceRegions[i].getMapIncidence().end(); it++) {
      date = it->first;
      x = getXForDay(date, minDate, maxDate);
      xnext = getXForDay(date + 24*60*60, minDate, maxDate);
      y = getYForIncidence(it->second, minIncidence, maxIncidence);
      if (xlast != -1) {
        tft.drawLine(x, ylast, x, y, incidenceRegions[i].getColor());
      }
      tft.drawLine(x, y, xnext, y, incidenceRegions[i].getColor());
      xlast = x; ylast = y;
    }
    tft.setFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(incidenceRegions[i].getColor());
    tft.setCursor(265, 10 + i*10);
    tft.printf("%s", incidenceRegions[i].getRegionName().c_str());
  }

  tft.setFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);
  char buff[20];
  strftime(buff, 20, "%d.%m.%Y", gmtime(&minDate));
  tft.setCursor(0,170);
  tft.printf("%s", buff);
  strftime(buff, 20, "%d.%m.%Y", gmtime(&realMaxDate));
  tft.setCursor(250,170);
  tft.printf("%s", buff);
}

void drawGraph() {
  if (graph == GRAPH_CO2) {
    drawCO2values();
#if defined(useIncidenceFromRKI)
  } else if (graph == GRAPH_INCIDENCE) {
    drawIncidence();
#endif    
  }
}