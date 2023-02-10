#include <WiFiMulti.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_LIS3DH.h>
#include "Adafruit_LTR329_LTR303.h"
#include <Adafruit_DotStar.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <secrets.h>

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define NUMPIXELS 1
#define DATAPIN    33
#define CLOCKPIN   21
#define CLICKTHRESHHOLD 80

WiFiMulti wifiMulti;
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point climate("climate");
Point acceleration("acceleration");
Point light("light");
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();
Adafruit_LTR329 ltr329 = Adafruit_LTR329();
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  climate.addTag("device", DEVICE);
  acceleration.addTag("device", DEVICE);
  light.addTag("device", DEVICE);
  timeSync(TZ_INFO, "no.pool.ntp.org");
  strip.begin();            
  strip.setBrightness(20);
  strip.setPixelColor(0,255,0,0);
  strip.show();   

  Serial.println("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (!client.validateConnection()) {
    Serial.println("InfluxDB connection failed");
    Serial.println(client.getLastErrorMessage());
    while (1) delay(1);
  }
  Serial.println("Connected to InfluxDB");

  if (! sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31 sensor");
    while (1) delay(1);
  }
  Serial.println("SHT31 found");

  if (! lis3dh.begin(0x18)) {
    Serial.println("Couldn't find LIS3DH sensor");
    while (1) delay(1);
  }
  Serial.println("LIS3DH found");

  if ( ! ltr329.begin() ) {
    Serial.println("Couldn't find LTR329 sensor");
    while (1) delay(10);
  }
  Serial.println("Found found");

  ltr329.setGain(LTR3XX_GAIN_4);
  ltr329.setIntegrationTime(LTR3XX_INTEGTIME_50);
  ltr329.setMeasurementRate(LTR3XX_MEASRATE_50);
}

void loop() {
  strip.setPixelColor(0,255,0,0);
  strip.show();   
  climate.clearFields();
  acceleration.clearFields();
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  bool valid;
  uint16_t visible_plus_ir, infrared;

  if (ltr329.newDataAvailable()) {
    valid = ltr329.readBothChannels(visible_plus_ir, infrared);
    if (valid) {
    }
  }

  climate.addField("temperature", t);
  climate.addField("humidity", h);
  lis3dh.read();
  acceleration.addField("X", lis3dh.x);
  acceleration.addField("Y", lis3dh.y);
  acceleration.addField("Z", lis3dh.z);
  light.addField("visible_plus_ir", visible_plus_ir);
  light.addField("infrared", infrared);

  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
    strip.setPixelColor(0,0,255,0);
    strip.show();   
  }

  Serial.println("Writing: ");
  Serial.println(climate.toLineProtocol());
  Serial.println(acceleration.toLineProtocol());
  Serial.println(light.toLineProtocol());

  if (!client.writePoint(climate) || !client.writePoint(acceleration) || !client.writePoint(light)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    strip.setPixelColor(0,0,255,0);
    strip.show();   
  }
  
  strip.setPixelColor(0,0,0,255);
  strip.show(); 
  delay(1000);
}
