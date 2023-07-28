#include "ConnectionManager.h"
#include "certs.h"

#include <ArduinoJson.h>

#include <ESP8266WiFi.h>

#include <string.h>

namespace Iotlow
{

	static WiFiClientSecure wifiClient;
	static PubSubClient mqttClient;
	static X509List caList;

	void callback(char *topic, byte *payload, unsigned int length)
	{
		DynamicJsonDocument doc(1024);

		char json[length];

		for (int i = 0; i < length; i++)
		{
			json[i] = (char)payload[i];
		}

		deserializeJson(doc, json);

		const char *actuator = doc["actuator"];
		const char *value = doc["value"];

		if (strcmp(actuator, "alarm") == 0)
		{
			if (strcmp(value, "on") == 0)
				Serial.print("0\n");
			if (strcmp(value, "off") == 0)
				Serial.print("1\n");
		}
		if (strcmp(actuator, "buzzer") == 0)
		{
			if (strcmp(value, "on") == 0)
				Serial.print("2\n");
			if (strcmp(value, "off") == 0)
				Serial.print("3\n");
		}
		if (strcmp(actuator, "fan") == 0)
		{
			if (strcmp(value, "on") == 0)
				Serial.print("4\n");
			if (strcmp(value, "off") == 0)
				Serial.print("5\n");
		}
	}

	void connect(ConnectionConfig &config)
	{
		Serial.begin(9600);
		caList.append(cert1);
		caList.append(cert2);
		wifiClient.setTrustAnchors(&caList);
		wifiClient.setFingerprint(config.serverFingerprint.c_str());

		mqttClient.setServer(config.domain.c_str(), config.port);
		mqttClient.setClient(wifiClient);
		mqttClient.setCallback(callback);

		WiFi.mode(WIFI_STA);
		WiFi.begin(config.ssid, config.key);

		int attempts = 0;
		while (WiFi.status() != WL_CONNECTED)
		{
			attempts++;
			delay(500);
		}

		attempts = 0;
		while (!mqttClient.connect(config.clientId.c_str(), config.username.c_str(), config.password.c_str()))
		{
			attempts++;
			delay(500);
		}

		boolean subscrive = mqttClient.subscribe("CMD/D001");
		Serial.println(subscrive);
	}

	bool isConnected(void)
	{
		return WiFi.isConnected() && mqttClient.connected();
	}

	void reconnect(ConnectionConfig &config)
	{
		int attempts = 0;
		while (WiFi.status() != WL_CONNECTED)
		{
			WiFi.reconnect();
			attempts++;
			delay(500);
		}

		attempts = 0;
		while (!mqttClient.connect(config.clientId.c_str(), config.username.c_str(), config.password.c_str()))
		{
			attempts++;
			delay(2000);
		}
		mqttClient.subscribe("CMD/D001");
	}

	PubSubClient &mqttClientRef()
	{
		return mqttClient;
	}

}
