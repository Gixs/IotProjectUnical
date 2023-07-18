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

enum enumActionsChar
{
  c_ALARM_ON = '0',
  c_ALARM_OFF = '1',
  c_BUZZER_ON = '2',
  c_BUZZER_OFF = '3',
  c_FAN_ON = '4',
  c_FAN_OFF = '5'
};

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
Iotlow::ConnectionConfig config;

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

  Iotlow::connect(config);
  Iotlow::setMqttClient(Iotlow::mqttClientRef());
}

void loop()
{
  if (!Iotlow::isConnected())
  {
    Iotlow::reconnect(config);
  }
  Iotlow::client->loop();
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    String json = Serial.readStringUntil('\n');

    // Serial.println("JSON ricevuto: " + json);

    String jsonToSend;
    if (json.length() > 3)
    {
      jsonToSend = json;
    }
    else
    {
      char jsonToChar = json.charAt(0);
      switch (jsonToChar)
      {
      case c_ALARM_ON:
        jsonToSend = "{\"actuator\":\"alarm\",\"value\":\"on\"}";
        break;
      case c_ALARM_OFF:
        jsonToSend = "{\"actuator\":\"alarm\",\"value\":\"off\"}";
        break;
      case c_BUZZER_ON:
        jsonToSend = "{\"actuator\":\"buzzer\",\"value\":\"on\"}";
        break;
      case c_BUZZER_OFF:
        jsonToSend = "{\"actuator\":\"buzzer\",\"value\":\"off\"}";
        break;
      case c_FAN_ON:
        jsonToSend = "{\"actuator\":\"fan\",\"value\":\"on\"}";
        break;
      case c_FAN_OFF:
        jsonToSend = "{\"actuator\":\"fan\",\"value\":\"off\"}";
        break;

      default:
        break;
      }
    }
    Iotlow::client->publish("STAT/D001", jsonToSend.c_str());
  }
}
