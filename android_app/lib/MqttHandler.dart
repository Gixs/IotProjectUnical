
import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
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
  //final ValueNotifier<String> alarm = ValueNotifier<String>("");

  final ValueNotifier<bool> alarm = ValueNotifier<bool>(false);
  final ValueNotifier<bool> buzzer = ValueNotifier<bool>(false);
  final ValueNotifier<bool> fan = ValueNotifier<bool>(false);

  late MqttServerClient client;
  //late SecurityContext securityContext;
  //late MqttServerSecureConnection secureConnection;

  static const topicHeader = 'Unical2023IoT/IoTProject/';
  static const topicPrefixStat = 'Stat';
  static const topicPrefixCommand = 'Cmd';

  static const topicAlarm = topicHeader+'Alarm';

  static const topicStat = 'STAT/D001';
  static const topicAir = topicHeader+topicPrefixStat+'Air';

  static const topicfan = topicHeader+topicPrefixCommand+'fan';
  static const topicBuzzer = topicHeader+topicPrefixCommand+'Buzzer';


  bool stateFan = false;
  bool stateBuzzer = false;
  bool stateAlarm = false;


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

      Map <String, dynamic> map = mapJson(pt);
      String jsonType = whichTypeJson(map);
      if (jsonType.compareTo('actuator') == 0){
        CmdValue actuator = parseCmd(map);

        if (actuator.actuator == 'alarm'){
          if (actuator.value == 'on'){
            stateAlarm = true;
            alarm.value = true;
            notifyListeners();
          }
          else if (actuator.value == 'off'){
            stateAlarm = false;
            alarm.value = false;
            notifyListeners();
          }
        }
        else if (actuator.actuator == 'buzzer'){
          if (actuator.value == 'on'){
            stateBuzzer = true;
            buzzer.value = true;
            notifyListeners();
          }
          else if (actuator.value == 'off'){
            stateBuzzer = false;
            buzzer.value = false;
            notifyListeners();
          }
        }
        else if (actuator.actuator == 'fan'){
          if (actuator.value == 'on'){
            stateFan = true;
            fan.value = true;
            notifyListeners();
          }
          else if (actuator.value == 'off'){
            stateFan = false;
            fan.value = false;
            notifyListeners();
          }
        }

      }
      else if (jsonType.compareTo('sensor') == 0){
        SensorValue sensor = parseSensor(map);

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
    const pubTopic = 'CMD/D001';
    final builder = MqttClientPayloadBuilder();
    builder.addString(message);

    if (client.connectionStatus?.state == MqttConnectionState.connected) {
      client.publishMessage(pubTopic, MqttQos.atLeastOnce, builder.payload!);
    }

  }

  Map <String, dynamic> mapJson (String responseBody){
    Map<String, dynamic> map = jsonDecode(responseBody);
    return map;
  }

  String whichTypeJson (Map <String, dynamic> map){
    return map.keys.first;
  }

  SensorValue parseSensor(Map<String, dynamic> map) {

    SensorValue sensor = SensorValue.fromJson(map);
    return sensor;
  }

  CmdValue parseCmd(Map<String, dynamic> map) {

    CmdValue cmd = CmdValue.fromJson(map);
    return cmd;
  }

  void alarmToggle () {
    if (stateAlarm == true) {
      stateAlarm = false;
      publishMessage('{\"actuator\":\"alarm\",\"value\":\"off\"}');
    }
    else {
      stateAlarm = true;
      publishMessage('{\"actuator\":\"alarm\",\"value\":\"on\"}');
    }
  }

  void buzzerToggle () {
    if (stateBuzzer == true) {
      stateBuzzer = false;
      publishMessage('{\"actuator\":\"buzzer\",\"value\":\"off\"}');
    }
    else {
      stateBuzzer = true;
      publishMessage('{\"actuator\":\"buzzer\",\"value\":\"on\"}');
    }
  }

  void fanToggle () {
    if (stateFan == true) {
      stateFan = false;
      publishMessage('{\"actuator\":\"fan\",\"value\":\"off\"}');
    }
    else {
      stateFan = true;
      publishMessage('{\"actuator\":\"fan\",\"value\":\"on\"}');
    }
  }




}

