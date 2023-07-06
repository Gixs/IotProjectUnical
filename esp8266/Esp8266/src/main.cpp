#include <Arduino.h>

#include <Service.h>
#include <ConnectionManager.h>

#include <stdint.h>

#define TO_STR(str) _TO_STR(str)
#define _TO_STR(str) #str

#ifndef FINGERPRINT
#define FINGERPRINT ""
#endif

#ifndef DOMAIN_NAME
#define DOMAIN_NAME ""
#endif

#ifndef PORT
#define PORT 0
#endif

#ifndef USERNAME
#define USERNAME ""
#endif

#ifndef PASSWORD
#define PASSWORD ""
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef KEY
#define KEY ""
#endif

#ifndef CLIENTID
#define CLIENTID ""
#endif

/* Connection defines */
const char ssid[] = TO_STR(WIFI_SSID);
const char key[] = TO_STR(KEY);

/* MQTT Server defines */
const char serverFingerprint[] = TO_STR(FINGERPRINT);
const char clientId[] = TO_STR(CLIENTID);
const char username[] = TO_STR(USERNAME);
const char password[] = TO_STR(PASSWORD);
const char domain[] = TO_STR(DOMAIN_NAME);
constexpr uint16_t port = PORT;
Iotsec::ConnectionConfig config;

constexpr int poolPeriod = 500;
constexpr int loopPeriod = 2000;

void setup()
{

  Serial.begin(9600);

  config.clientId = clientId;
  config.domain = domain;
  config.key = key;
  config.password = password;
  config.port = port;
  config.serverFingerprint = serverFingerprint;
  config.ssid = ssid;
  config.username = username;

  Iotsec::connect(config);
  Iotsec::setMqttClient(Iotsec::mqttClientRef());
}

void loop()
{
  if (!Iotsec::isConnected())
  {
    Iotsec::reconnect(config);
  }

  if (Serial.available() > 0)
  {
    // read the incoming byte:
    String json = Serial.readStringUntil('\n');
    Serial.println("JSON ricevuto: " + json);

    Iotsec::client->publish("D001", json.c_str());
  }
}
