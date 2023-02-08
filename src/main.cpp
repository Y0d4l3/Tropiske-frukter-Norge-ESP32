#include <WiFiMulti.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_DotStar.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "ESP32"
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
#define CLICKTHRESHHOLD 80

WiFiMulti wifiMulti;
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point climate("climate");
Point motion("motion");
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  climate.addTag("device", DEVICE);
  motion.addTag("device", DEVICE);
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

  if (! lis.begin(0x18)) {
    Serial.println("Couldn't find LIS3DH sensor");
    while (1) delay(1);
  }
  Serial.println("LIS3DH found");
}

void loop() {
  strip.setPixelColor(0,255,0,0);
  strip.show();   
  climate.clearFields();
  motion.clearFields();
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  climate.addField("temperature", t);
  climate.addField("humidity", h);
  lis.read();
  motion.addField("X", lis.x);
  motion.addField("Y", lis.y);
  motion.addField("Z", lis.z);

  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
    strip.setPixelColor(0,0,255,0);
    strip.show();   
  }

  Serial.println("Writing: ");
  Serial.println(climate.toLineProtocol());
  Serial.println(motion.toLineProtocol());

  if (!client.writePoint(climate) || !client.writePoint(motion)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    strip.setPixelColor(0,0,255,0);
    strip.show();   
  }
  
  strip.setPixelColor(0,0,0,255);
  strip.show(); 
  delay(1000);
}
