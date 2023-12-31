#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define WIFI_SSID ""
#define WIFI_PASSWORD ""


#define WUNDERGROUND_API_KEY "" //https://www.wunderground.com/member/api-keys
#define WUNDERGROUND_DEVICE_ID "KOHCLE" // https://www.wunderground.com/member/devices
#define WUNDERGROUND_DEVICE_PASSWORD "" // https://www.wunderground.com/member/devices
#define WUNDERGROUND_API_BASE_URL "http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php" + String("?ID=") + WUNDERGROUND_DEVICE_ID + String("&PASSWORD=") + WUNDERGROUND_DEVICE_PASSWORD + String("&apiKey=") + WUNDERGROUND_API_KEY + String("&dateutc=") + String("now") + String("&action=") + String("updateraw") + String("&format=") + String("json")

Adafruit_BME280 bme;
boolean hasError;
String errorMessage;

String toLowerCase(String str)
{
  String _str = String(str);
  _str.toLowerCase();

  return _str;
}

void error(String message)
{
  hasError = true;
  errorMessage = "Error: " + message;
}

class Reading
{
private:
  float _temperature_c;
  float _temperature_f;
  float _pressure_hPa;
  float _pressure_mmHg;
  float _humidity;

  Adafruit_BME280 _bme;

public:
  Reading(Adafruit_BME280 bme)
  {
    _bme = bme;
  }

  void perform()
  {
    if (!bme.init() && !bme.begin())
    {
      error("BME280 not connected.");
    }

    _pressure_hPa = bme.readPressure() / 100.0F;
    _pressure_mmHg = (_pressure_hPa * 0.75006157584566F);
    _temperature_c = bme.readTemperature();
    _temperature_f = ((((_temperature_c * 9) + 3) / 5) + 32);
    _humidity = bme.readHumidity();
  }

  float temperature()
  {
    return _temperature_c;
  }

  float temperature(String units)
  {
    String _units = toLowerCase(units);
    if (_units == "c")
    {
      return _temperature_c;
    }
    else if (_units == "f")
    {
      return _temperature_f;
    }

    return NAN;
  }

  float pressure()
  {
    return _pressure_hPa;
  }

  float pressure(String units)
  {
    String _units = toLowerCase(units);

    if (_units == "hpa")
    {
      return _pressure_hPa;
    }
    else if (_units == "mmhg")
    {
      return _pressure_mmHg;
    }

    return NAN;
  }

  float humidity()
  {
    return _humidity;
  }

  String serializeTemperatureC(Reading r)
  {
    return String(r.temperature("c"));
  }

  String serializeTemperatureF(Reading r)
  {
    return String(r.temperature("f"));
  }

  String serializeTemperature(Reading r, String units)
  {
    return serializeTemperature(r, units, units);
  }

  String serializeTemperature(Reading r, String units, String unitString)
  {
    const String _unitsLabel = unitString ? unitString : units;
    String _units = toLowerCase(units);

    if (_units == String("c") || _units == String("f"))
    {
      return String(r.temperature(_units)) + _unitsLabel;
    }

    return String(r.temperature("c")) + _unitsLabel;
  }

  String serializePressure(Reading r, String units)
  {
    return serializePressure(r, units, units);
  }

  String serializePressure(Reading r, String units, String unitString)
  {
    const String _unitsLabel = unitString ? unitString : units;
    String _units = toLowerCase(units);

    if ((_units == "hpa") || (_units = "mmhg"))
    {
      return String(r.pressure(_units)) + _unitsLabel;
    }

    return String(r.pressure());
  }

  String serializeHumidity(Reading r)
  {
    return serializeHumidity(r, "");
  }

  String serializeHumidity(Reading r, String units)
  {
    return serializeHumidity(r, units, units);
  }

  String serializeHumidity(Reading r, String units, String unitString)
  {
    const String _unitsLabel = unitString ? unitString : units;
    String _units = toLowerCase(units);

    if ((_units == "%"))
    {
      return String(r.humidity() + _unitsLabel);
    }

    return String(r.humidity());
  }
};

enum Steps
{
  STEP_START,
  STEP_MEASURE,
  STEP_SERIALIZE,
  STEP_SEND,
  STEP_DONE,
  STEP_ERROR
};
int step = STEP_START;
Reading reading = Reading(bme);

void lightSleep(uint64_t duration)
{
  delay(5000);

  // Deep Sleep is a Low Power Mode that
  // It doesn't save anything.
  // The setup() will run afterwards
  esp_sleep_enable_timer_wakeup(duration);
  int status = esp_light_sleep_start();

  if (status != ESP_OK)
  {
    // This means something is wrong and we don't control
    // it so grab a debugger and your copy of Google.
    error("failed to perform light sleep.");
  }
}

void deepSleep(uint64_t duration)
{
  // Deep Sleep is a Low Power Mode that
  // It doesn't save anything.
  // The setup() will run afterwards
  esp_sleep_enable_timer_wakeup(duration);
  esp_deep_sleep_start();

  // This means something is wrong and we don't control
  // it so grab a debugger and your copy of Google.
  error("failed to perform deep sleep");
}

void start()
{
  Serial.println("Start");
  step = STEP_MEASURE;
}

void measure()
{
  Serial.println("Measure");

  reading.perform();

  step = STEP_SEND;
}

void send()
{
  Serial.println("Send");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  WiFiClient client;
  HTTPClient http;
  String serverPath = WUNDERGROUND_API_BASE_URL + String("&humidity=") + reading.serializeHumidity(reading) + String("&tempf=") + reading.serializeTemperatureF(reading) + String("&baromin=") + String(reading.serializePressure(reading, "mmhg"));

  http.begin(serverPath);
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  
  Serial.println("T: " + reading.serializeTemperature(reading, "f"));
  Serial.println("P: " + reading.serializePressure(reading, "mmhg"));
  Serial.println("H: " + reading.serializeHumidity(reading));

  WiFi.disconnect();
  // Send over WiFi
  step = STEP_DONE;
}

void done()
{
  Serial.println("Done");
  delay(1000);
  step = STEP_START;
  // Serial.println("Humidity: " + String(reading.humidity()));
  // Serial.println("Pressure: " + String(reading.pressure()));
  // Serial.println("Temperature: " + String(reading.temperature()));

  // Sleep for 60s
  deepSleep(60000000LL);
}

void setup()
{
  Serial.begin(115200);
  delay(1500);
  Serial.println("Checking BME280 Connectivity");

  bool status = bme.begin();
  if (!status)
  {
    error("Could not find a valid BME280 sensor, check wiring!");
  }

  Serial.println("BME280 Temp Humidity Pressure");
  Serial.println();

  step = STEP_START;
}

void loop()
{
  if (hasError == true)
  {
    Serial.println("Error" + errorMessage);
    deepSleep(60000000LL);
  }

  switch (step)
  {
  case STEP_START:
    return start();
  case STEP_MEASURE:
    return measure();
  case STEP_SEND:
    return send();
  case STEP_DONE:
    return done();
    break;
  default:
    step = STEP_START;
  }
}