
import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';


import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'CmdValueHandler.dart';
import 'SensorValueHandler.dart';

class MqttHandler with ChangeNotifier {
  final ValueNotifier<String> dataTemp = ValueNotifier<String>("");
  final ValueNotifier<String> dataAir = ValueNotifier<String>("");
  final ValueNotifier<String> alarm = ValueNotifier<String>("");
  late MqttServerClient client;
  //late SecurityContext securityContext;
  //late MqttServerSecureConnection secureConnection;

  static const topicHeader = 'Unical2023IoT/IoTProject/';
  static const topicPrefixStat = 'Stat';
  static const topicPrefixCommand = 'Cmd';

  static const topicAlarm = topicHeader+'Alarm';

  static const topicStat = 'D001';
  static const topicAir = topicHeader+topicPrefixStat+'Air';

  static const topicVent = topicHeader+topicPrefixCommand+'Vent';
  static const topicBuzzer = topicHeader+topicPrefixCommand+'Buzzer';


  bool vent = false;


  Future<Object> connect() async {
    client = MqttServerClient.withPort(
        'iotprojectunical.me', 'telefono_di_gixs1', 8883);
    client.logging(on: true);
    client.onConnected = onConnected;
    client.onDisconnected = onDisconnected;
    client.onUnsubscribed = onUnsubscribed;
    client.onSubscribed = onSubscribed;
    client.onSubscribeFail = onSubscribeFail;
    client.pongCallback = pong;
    client.keepAlivePeriod = 60;
    client.logging(on: true);
    client.secure = true;

    /// Set the correct MQTT protocol for mosquito
    client.setProtocolV311();

    final connMessage = MqttConnectMessage()
        .withWillTopic('Gixs/testiamo/esp8266')
        .withWillMessage('Will message')
        .startClean()
        .withWillQos(MqttQos.atLeastOnce);

    print('MQTT_LOGS::Mosquitto client connecting....');

    client.connectionMessage = connMessage;

    try {
      await client.connect('mqttandroid', '123123');
    } catch (e) {
      print('Exception: $e');
      client.disconnect();
    }

    if (client.connectionStatus!.state == MqttConnectionState.connected) {
      print('MQTT_LOGS::Mosquitto client connected');
    } else {
      print(
          'MQTT_LOGS::ERROR Mosquitto client connection failed - disconnecting, status is ${client.connectionStatus}');
      client.disconnect();
      return -1;
    }

    print('MQTT_LOGS::Subscribing to the test/lol topic');


    print('MQTT_LOGS::Subscribing to the <${topicStat}> topic');
    client.subscribe(topicStat, MqttQos.atMostOnce);
    //print('MQTT_LOGS::Subscribing to the <${topicAir}> topic');
    //client.subscribe(topicAir, MqttQos.atMostOnce);

    client.updates!.listen((List<MqttReceivedMessage<MqttMessage?>>? c) {
      final recMess = c![0].payload as MqttPublishMessage;
      final pt =
      MqttPublishPayload.bytesToStringAsString(recMess.payload.message);


      if (c[0].topic == topicAlarm){
        alarm.value = "on";
        notifyListeners();
        print(
            'MQTT_LOGS:: Vecchio log: New data arrived: topic is <${c[0]
                .topic}>, payload is $pt');

      }

      else if (c[0].topic == topicStat){
        SensorValue sensor = parseSensor(pt);


        if (sensor.sensor == 'temperature') {
          dataTemp.value = sensor.value.toString();
          notifyListeners();
          print(
              'MQTT_LOGS:: Vecchio log: New data arrived: topic is <${c[0]
                  .topic}>, payload is $pt');
          print(
              'MQTT_LOGS:: New data arrived: sensor is <${sensor
                  .sensor}>, payload is $sensor.sensorValue');
          print('');
        }
        else if (sensor.sensor == 'air') {
          dataAir.value = sensor.value.toString();
          notifyListeners();
          print(
              'MQTT_LOGS:: Vecchio log: New data arrived: topic is <${c[0]
                  .topic}>, payload is $pt');
          print(
              'MQTT_LOGS:: Vecchio log: New data arrived: topic is <${sensor
                  .sensor}>, payload is $sensor.sensorValue');
          print('');
        }
      }


    });

    return client;
  }

  void onConnected() {
    print('MQTT_LOGS:: Connected');
  }

  void onDisconnected() {
    print('MQTT_LOGS:: Disconnected');
  }

  void onSubscribed(String topic) {
    print('MQTT_LOGS:: Subscribed topic: $topic');
  }

  void onSubscribeFail(String topic) {
    print('MQTT_LOGS:: Failed to subscribe $topic');
  }

  void onUnsubscribed(String? topic) {
    print('MQTT_LOGS:: Unsubscribed topic: $topic');
  }

  void pong() {
    print('MQTT_LOGS:: Ping response client callback invoked');
  }

  void publishMessage(String message) {
    const pubTopic = 'Gixs/testiamo/esp8266';
    final builder = MqttClientPayloadBuilder();
    builder.addString(message);

    if (client.connectionStatus?.state == MqttConnectionState.connected) {
      client.publishMessage(pubTopic, MqttQos.atMostOnce, builder.payload!);
    }

    }

  void stopBuzzer(){
    final builder = MqttClientPayloadBuilder();
    String message = 'off';
    builder.addString(message);

    client.publishMessage(topicBuzzer, MqttQos.atLeastOnce, builder.payload!);

  }

  void changeStateVent(){
    final builder = MqttClientPayloadBuilder();
    String message;
    if (vent == false) {
      message = 'on';
      vent = true;
    }
    else {
      message = 'off';
      vent = false;
    }
    builder.addString(message);

    client.publishMessage(topicBuzzer, MqttQos.atLeastOnce, builder.payload!);

  }

  bool getStateVent (){
    return vent;
  }

  SensorValue parseSensor(String responseBody) {
    Map<String, dynamic> map = jsonDecode(responseBody);

    SensorValue sensor = SensorValue.fromJson(map);
    return sensor;
  }

  CmdValue parseCmd(String responseBody) {
    Map<String, dynamic> map = jsonDecode(responseBody);

    CmdValue cmd = CmdValue.fromJson(map);
    return cmd;
  }




}

