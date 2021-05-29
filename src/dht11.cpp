#include "DHT.h"

#include "config.h"
#include "log.h"

#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);

float temp;
float hum;

void dht11_setup()
{
    dht.begin();
    Log.printf("  DHT11 sucessfully initialized.\r\n");
}

void dht11_update()
{
    float tempOld = temp;
    float humOld = hum;
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temp = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(hum) || isnan(temp))
    {
        Log.printf("Failed to read from DHT sensor!\r\n");
        return;
    }
    if ((tempOld != temp) || (humOld != hum)) {
      Log.printf("Temp: %.1f C, Humidity: %.1f%%\r\n", temp, hum);
    }
}
