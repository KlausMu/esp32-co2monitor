#include <Arduino.h>
#include <Ticker.h>

#include "config.h"
#include "log.h"
#include "tft.h"

//Senseair Sensor UART
#define BAUDRATE 9600

// example from other community sources
// byte CO2req[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25}; // Take care. Only 7 bytes

// from official documentation TDE2067.pdf
byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0xC5};
byte Statusreq[] = {0xFE, 0X04, 0X00, 0X00, 0X00, 0X01, 0X25, 0xC5};
byte CO2andStatusreq[] = {0xFE, 0X04, 0X00, 0X00, 0X00, 0X04, 0XE5, 0xC6};
byte ABCperiodreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0xC3};

byte CalibrationReq1ClearAck[] = {0xFE, 0X06, 0X00, 0X00, 0X00, 0X00, 0X9D, 0xC5};
byte CalibrationReq2StartCalibration[] = {0xFE, 0X06, 0X00, 0X01, 0X7C, 0X06, 0X6C, 0xC7};
byte CalibrationReq3ReadAck[] = {0xFE, 0X03, 0X00, 0X00, 0X00, 0X01, 0X90, 0x05};

byte response[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // maximum responseSize 15

// https://www.lammertbies.nl/comm/info/crc-calculation

HardwareSerial CO2Serial(1);

unsigned long co2_value = 0;
unsigned long co2_status = 0;
unsigned long co2_ABCperiod = 0;
byte calibrationStatus = 0;
Ticker calibrationWatchdog;

void setCO2value(long aValue) {
  if (co2_value < CO2_THRESHOLD_YELLOW && aValue >= CO2_THRESHOLD_YELLOW) {
    turnTFTOn();
  }
  if (co2_value < CO2_THRESHOLD_RED && aValue >= CO2_THRESHOLD_RED) {
    turnTFTOn();
  }
  co2_value = aValue;
}

void co2_setup()
{
  CO2Serial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  Log.printf("  Senseair S8 sucessfully initialized.\r\n");
}

void clearResponse() {
  for (int i = 0; i < 15; i++)
  {
    response[i] = 0;
  }
}

bool doRequest(byte request[], int requestSize, int responseSize)
{
  unsigned long startMillis = millis();
  while (!CO2Serial.available())
  {
    // Log.printf("request: not available\r\n");
    CO2Serial.write(request, requestSize);
    delay(50);

    if (millis()-startMillis > 1000) {
      // timeout
      return false;
    }
  }

  int timeout = 0;
  while (CO2Serial.available() < responseSize)
  {
    timeout++;
    if (timeout > 10)
    {
      while (CO2Serial.available())
        CO2Serial.read();
      break;
    }
    delay(50);

    if (millis()-startMillis > 1000) {
      // timeout
      return false;
    }
  }

  // Log.printf("request: final copy to CO2out\r\n");
  for (int i = 0; i < responseSize; i++)
  {
    response[i] = CO2Serial.read();
  }

  return true;
}

void co2_requestValue()
{
  clearResponse();
  if (!doRequest(CO2req, 8, 7)) {return;};
  setCO2value(response[3] * 256 + response[4]);
  if (co2_value > CO2_THRESHOLD_MALFUNCTION)
  {
    setCO2value(0);
  }
}

void co2_requestStatus()
{
  clearResponse();
  if (!doRequest(Statusreq, 8, 7)) {return;};
  co2_status = response[3] * 256 + response[4];
}

void co2_requestValueAndStatus()
{
  unsigned long co2_valueOld = co2_value;
  unsigned long co2_statusOld = co2_status;

  clearResponse();
  if (!doRequest(CO2andStatusreq, 8, 13)) {return;};
  co2_status = response[3] * 256 + response[4];
  setCO2value(response[9] * 256 + response[10]);
  if (co2_value > CO2_THRESHOLD_MALFUNCTION)
  {
    setCO2value(0);
  }

  if ((co2_valueOld != co2_value) || (co2_statusOld != co2_status)) {
    Log.printf("CO2: %lu ppm, status: %lu\r\n", co2_value, co2_status);
  }
}

void co2_requestABCperiod()
{
  unsigned long co2_ABCperiodOld = co2_ABCperiod;

  clearResponse();
  if (!doRequest(ABCperiodreq, 8, 7)) {return;};
  co2_ABCperiod = response[3] * 256 + response[4];

  if (co2_ABCperiodOld != co2_ABCperiod) {
    Log.printf("CO2 ABC-period: %lu h\r\n", co2_ABCperiod);
  }
}

bool co2_clearBackgroundCalibrationAck()
{
  bool checkResult;
  int i;

  // Step 1: clear acknowledgement register
  clearResponse();
  if (!doRequest(CalibrationReq1ClearAck, 8, 8)) {return false;};
  // Slave reply should be the same as the request (see documentation)
  checkResult = true;
  i = 0;
  while (checkResult && (i < 8)) {
    checkResult = CalibrationReq1ClearAck[i] == response[i]; 
    ++i;
  }
  checkResult = checkResult && (i==8);
  if (checkResult) {
    Log.printf("co2_clearBackgroundCalibrationAck successfully started\r\n");
  } else {
    Log.printf("WARNING: co2_clearBackgroundCalibrationAck could not be started\r\n");
    return false;
  }

  return true;
}

bool co2_startBackgroundCalibration()
{
  bool checkResult;
  int i;

  // automatically set calibrationStatus back to 0 after 60 seconds
  calibrationWatchdog.detach();
  calibrationWatchdog.attach(20, [](){calibrationStatus = 0;});
  calibrationStatus = 1;

  // Step 1: clear acknowledgement register
  checkResult = co2_clearBackgroundCalibrationAck();
  if (!checkResult) {
    return false;
  }

  // Step 2: start background calibration
  clearResponse();
  if (!doRequest(CalibrationReq2StartCalibration, 8, 8)) {return false;};
  // Slave reply should be the same as the request (see documentation)
  checkResult = true;
  i = 0;
  while (checkResult && (i < 8)) {
    checkResult = CalibrationReq2StartCalibration[i] == response[i]; 
    ++i;
  }
  checkResult = checkResult && (i==8);
  if (checkResult) {
    Log.printf("co2_startBackgroundCalibration successfully started\r\n");
  } else {
    Log.printf("WARNING: co2_startBackgroundCalibration could not be started\r\n");
    return false;
  }

  calibrationStatus = 2;
  return true;
}

bool co2_checkBackgroundCalibrationAck()
{
  bool checkResult;

  // Step 3: read acknowledgement register
  clearResponse();
  if (!doRequest(CalibrationReq3ReadAck, 8, 7)) {return false;};
  // check that bit 5 (CI6) is 1.
  checkResult = true;
  checkResult = checkResult && (response[2] = 0x02);
  checkResult = checkResult && ((response[4] & (1 << 5)) != 0);
  if (checkResult) {
    Log.printf("co2_checkBackgroundCalibrationAck successfully received\r\n");
  } else {
    Log.printf("co2_checkBackgroundCalibrationAck not yet received\r\n");
    return false;
  }

  calibrationStatus = 3;
  // automatically set calibrationStatus back to 0 after 20 seconds
  calibrationWatchdog.detach();
  calibrationWatchdog.attach(20, [](){calibrationStatus = 0;});

  return true;
}
