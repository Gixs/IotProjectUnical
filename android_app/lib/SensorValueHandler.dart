import 'dart:convert';

class SensorValue {
  String sensor;
  double value;

  SensorValue ({
    required this.sensor,
    required this.value,
  });

  SensorValue.fromJson(Map<String, dynamic> json):
      sensor = json['sensor'],
      value = json['value'];

  Map<String, dynamic> toJson(){
    return {
      'sensor' : sensor,
      'value' : value,
    };


  }



}