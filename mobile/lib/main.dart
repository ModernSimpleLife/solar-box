import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: DeviceDiscovery(),
    );
  }
}

class _DeviceDiscoveryState extends State<DeviceDiscovery> {
  @override
  void initState() {
    super.initState();

    debugPrint("Initiated _DeviceDiscoveryState");
    FlutterBlue.instance.startScan(timeout: const Duration(seconds: 4));
  }

  @override
  void deactivate() {
    super.deactivate();
    FlutterBlue.instance.stopScan();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        body: Container(
            padding: const EdgeInsets.fromLTRB(10, 10, 10, 0),
            width: double.maxFinite,
            child: StreamBuilder<List<ScanResult>>(
              stream: FlutterBlue.instance.scanResults,
              builder: (context, snap) {
                debugPrint("Getting scan result: ${snap.data}");

                var data = snap.data ?? [];
                List<Widget> list = data
                    .map((e) => Card(
                        elevation: 5,
                        child: SizedBox(
                            height: 100,
                            child: Center(
                                child: Text(
                              e.device.name,
                              style: TextStyle(fontSize: 20),
                            )))))
                    .cast<Widget>()
                    .toList();

                var loading = SizedBox(
                    height: 100,
                    child: Center(child: CircularProgressIndicator()));
                list.add(loading);

                return ListView(
                  children: list,
                );
              },
            )));
  }
}

class DeviceDiscovery extends StatefulWidget {
  @override
  State<StatefulWidget> createState() => _DeviceDiscoveryState();
}

class _SolarBoxState extends State<SolarBox> {
  @override
  Widget build(BuildContext context) {
    return Text("hi");
  }
}

class SolarBox extends StatefulWidget {
  @override
  State<StatefulWidget> createState() => _SolarBoxState();
}
