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
#define WUNDERGROUND_DEVICE_ID "KOHCLEVE" // https://www.wunderground.com/member/devices
#define WUNDERGROUND_DEVICE_PASSWORD "" // https://www.wunderground.com/member/devices
#define WUNDERGROUND_API_BASE_URL "http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php"

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

struct BME280_Reading {
  float dewpoint_c;
  float dewpoint_f;
  float temperature_c;
  float temperature_f;
  float pressure_hPa;
  float pressure_mmHg;
  float pressure_inHg;
  float humidity;
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
BME280_Reading reading;

void lightSleep(uint64_t duration)
{
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
  delay(1000);

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
  step = STEP_MEASURE;
}

void measure()
{
    reading.humidity = bme.readHumidity();
    reading.pressure_hPa = bme.readPressure() / 100.0F;
    reading.temperature_c = bme.readTemperature();

    reading.pressure_mmHg = (reading.pressure_hPa * 0.7500637554);
    reading.pressure_inHg = (reading.pressure_hPa * 0.02952998057228486);
    reading.temperature_f = ((((reading.temperature_c * 9) + 3) / 5) + 32);
    reading.dewpoint_c = reading.temperature_c - ((100 - reading.humidity) / 5);
    reading.dewpoint_f = ((((reading.dewpoint_c * 9) + 3) / 5) + 32);
  
  step = STEP_SEND;
}

void send()
{
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

  char url[1024];
  sprintf(url, "%s&ID=%s&PASSWORD=%s&apiKey=%s&dateutc=%s&action=%s&format=%s&humidity=%.2f&tempf=%.2f&baromin=%.2f&dewptf=%.2f",
    WUNDERGROUND_API_BASE_URL,
    WUNDERGROUND_DEVICE_ID,
    WUNDERGROUND_DEVICE_PASSWORD,
    WUNDERGROUND_API_KEY,
    "now",
    "updateraw",
    "json",
    reading.humidity, 
    reading.temperature_f,
    reading.pressure_inHg,
    reading.dewpoint_f
  );

  Serial.println(url);

  http.begin(url);
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
  
  Serial.printf("T: %.2f%s", reading.temperature_f, "F");
  Serial.printf("P: %.2f%s", reading.pressure_inHg, "inHg");
  Serial.printf("H: %.2f%s",  reading.humidity, "%");
  Serial.printf("D: %.2f%s", reading.dewpoint_f, "F");

  WiFi.disconnect();
  // Send over WiFi
  step = STEP_DONE;
}

void done()
{
  Serial.println("Done");
  step = STEP_START;

  // Sleep for 60s
  deepSleep(60000000LL);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
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