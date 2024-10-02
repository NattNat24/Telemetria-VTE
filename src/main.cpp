#include <Arduino.h>
#include "secrets.hpp"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include "WiFi.h"
#include <SoftwareSerial.h>

#define RXD2 16
#define TXD2 17
#define MONITOR_SPEED 9600

// put function declarations here:

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXD2, TXD2);

/**
 * Para modificar la conexion a internet ir al archivo secrets.hpp
 * Solo editar WIFI_SSID y WIFI_PASSWORD
 */

/**
 * Inicio del Setup de Conexion a AWS IoT Core
 * No toca modificar nada aquí
 */
// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "charting"
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  // Create a message handler
  // client.onMessage(messageHandler);
  Serial.print("Connecting to AWS IOT");
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_PUBLISH_TOPIC);
  Serial.println("AWS IoT Connected!");
}

void publishMesssage(float speed, float lat, float lng)
{
  // See https://arduinojson.org/
  JsonDocument doc;
  doc["speed"] = speed;
  doc["lat"] = lat;
  doc["lng"] = lng;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to mqtt_client

  // Publish to the topic
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Sent a message");
}
/**
 * Fin del Setup de Conexión a AWS IoT Core
 */

/**
 * A partir de aqui inyecten todo el codigo que tenian antes de los sensores
 */

void setup()
{
  Serial.begin(MONITOR_SPEED);
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(MONITOR_SPEED);
  connectAWS();
}

void loop()
{
  Serial.println("Entrando a loop");
  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
    Serial.println("Entre aqui a gpsSerialAvailable");
  }
  Serial.println("Pude salir de gpsSerial loop");

  if (gps.location.isUpdated())
  {
    /**
     * Ajustar para colocar los valores de los sensores en estas tres variables
     */
    float speed = 0; // valor resultado de la medicion de la velocidad
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    Serial.print("Latitud = ");
    Serial.println(lat, 6);
    Serial.print("Longitud = ");
    Serial.println(lng, 6);
    publishMesssage(speed, lat, lng);
    client.loop();
  }
  Serial.println("Finalize por aqui, antes de delay");
  delay(1000);
}
