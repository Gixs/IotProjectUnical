import 'dart:convert';

class CmdValue {
  String actuator;
  String value;

  CmdValue ({
    required this.actuator,
    required this.value,
  });

  CmdValue.fromJson(Map<String, dynamic> json):
      actuator = json['actuator'],
      value = json['value'];

  Map<String, dynamic> toJson(){
    return {
      'actuator' : actuator,
      'value' : value,
    };
  }



}