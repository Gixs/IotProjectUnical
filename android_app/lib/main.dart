import 'package:flutter/material.dart';
import 'MqttHandler.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: MyStatefulWidget(),
    );
  }
}

class MyStatefulWidget extends StatefulWidget {
  const MyStatefulWidget({super.key});

  @override
  State<MyStatefulWidget> createState() => _MyStatefulWidgetState();
}

class _MyStatefulWidgetState extends State<MyStatefulWidget> {
  int _count = 0;
  MqttHandler mqttHandler = MqttHandler();


  @override
  void initState() {
    super.initState();
    mqttHandler.connect();
  }

  @override
  Widget build(BuildContext context) {
    return  MaterialApp(
      home: Scaffold(
      appBar: AppBar(
        title: const Text('IoT Projec Unical'),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            const Text('Air quality:',
                style: TextStyle(color: Colors.black, fontSize: 25)),
            ValueListenableBuilder<String>(
              builder: (BuildContext context, String value, Widget? child) {
                return Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: <Widget>[
                    Text('$value',
                        style: TextStyle(
                            color: Colors.deepPurpleAccent, fontSize: 35))
                  ],
                );
              },
              valueListenable: mqttHandler.dataAir,
            ),
            const Text('Temperature:',
                style: TextStyle(color: Colors.black, fontSize: 25)),
            ValueListenableBuilder<String>(
              builder: (BuildContext context, String value, Widget? child) {
                return Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: <Widget>[
                    Text('$value°',
                        style: TextStyle(
                            color: Colors.deepPurpleAccent, fontSize: 35)),
                  ],

                );
              },
              valueListenable: mqttHandler.dataTemp,
            ),
            Row(
                mainAxisSize: MainAxisSize.min,
              children: [
                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      mqttHandler.alarm.value = !mqttHandler.alarm.value;
                      mqttHandler.alarmToggle();
                    });
                  },
                  child: ValueListenableBuilder<bool>(
                    valueListenable: mqttHandler.alarm,
                    builder: (BuildContext context, bool value, Widget? child) {
                      return Icon(
                        value ? Icons.gpp_good : Icons.upcoming,
                      );
                    },
                  ),
                ),
                SizedBox(width: 10),
                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      mqttHandler.buzzer.value = !mqttHandler.buzzer.value;
                      mqttHandler.buzzerToggle();
                    });
                  },
                  child: ValueListenableBuilder<bool>(
                    valueListenable: mqttHandler.buzzer,
                    builder: (BuildContext context, bool value, Widget? child) {
                      return Icon(
                        value ? Icons.volume_off : Icons.campaign,
                      );
                    },
                  ),
                ),
                SizedBox(width: 10),

                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      mqttHandler.fan.value = !mqttHandler.fan.value;
                      mqttHandler.fanToggle();
                    });
                  },
                  child: ValueListenableBuilder<bool>(
                    valueListenable: mqttHandler.fan,
                    builder: (BuildContext context, bool value, Widget? child) {
                      return Icon(
                        value ? Icons.mode_fan_off : Icons.heat_pump,
                      );
                    },
                  ),
                ),

              ],
            ),

          ],
        ),
      ),
    )
    );
  }
}
