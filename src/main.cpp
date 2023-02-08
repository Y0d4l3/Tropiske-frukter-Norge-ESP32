#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"

#include <Adafruit_SHT31.h>
#include <Adafruit_DotStar.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "Student"
#define WIFI_PASSWORD "Kristiania1914"
#define INFLUXDB_URL "https://influxdb.guardiansofentity.de/"
#define INFLUXDB_TOKEN "DP8vStYHSSQMKMtrfzUKMPED8Culx0wKsXPs7LVZ0tKdEiveXjL5rj9RMZc9kP0NC0iV25MRrCWaihieiT-xoQ=="
#define INFLUXDB_ORG "Tropiske frukter Norge"
#define INFLUXDB_BUCKET "Sensor_Data"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

#define NUMPIXELS 1
#define DATAPIN    33
#define CLOCKPIN   21

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensor("climate");

Adafruit_SHT31 sht31 = Adafruit_SHT31();

Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  sensor.addTag("device", DEVICE);

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }



  if (! sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31 sensor!");
    while (1) delay(1);
  }

  Serial.println("Found SHT31 sensor!");

  strip.begin();            
  strip.setBrightness(20);
  strip.setPixelColor(0,255,0,0);
  strip.show();   
}

void error() {
  strip.setPixelColor(0,0,255,0);
  strip.show();   
}

void success() {
  strip.setPixelColor(0,0,0,255);
  strip.show();   
}

void loop() {
  strip.setPixelColor(0,255,0,0);
  strip.show();   

  sensor.clearFields();

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  sensor.addField("humidity", h);
  sensor.addField("temperature", t);

  if (! isnan(t)) {
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
    error();
  }
  
  if (! isnan(h)) {
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
    error();
  }

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
    error();
  }

  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    error();
  }
  
  success();
  Serial.println("Wait 10s");
  delay(10000);
}
