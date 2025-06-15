import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEService {
  static const String RGBW_SERVICE_UUID = "00ff";
  static const String RED_CHAR_UUID = "ff01";
  static const String GREEN_CHAR_UUID = "ff02";
  static const String BLUE_CHAR_UUID = "ff03";
  static const String WARM_WHITE_CHAR_UUID = "ff04";

  BluetoothDevice? connectedDevice;
  BluetoothCharacteristic? redCharacteristic;
  BluetoothCharacteristic? greenCharacteristic;
  BluetoothCharacteristic? blueCharacteristic;
  BluetoothCharacteristic? warmWhiteCharacteristic;

  Future<bool> _requestPermissions() async {
    Map<Permission, PermissionStatus> statuses = await [
      Permission.bluetooth,
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    return statuses.values.every((status) => status.isGranted);
  }

  Future<bool> connectToDevice(String deviceName) async {
    try {
      // Request permissions
      bool permissionsGranted = await _requestPermissions();
      if (!permissionsGranted) {
        throw Exception('Bluetooth permissions not granted');
      }

      // Check if Bluetooth is on
      if (await FlutterBluePlus.isOn == false) {
        throw Exception('Bluetooth is not enabled');
      }

      // Start scanning
      await FlutterBluePlus.startScan(timeout: Duration(seconds: 10));

      // Listen for scan results
      await for (List<ScanResult> results in FlutterBluePlus.scanResults) {
        for (ScanResult result in results) {
          if (result.device.name == deviceName) {
            // Found the device, stop scanning
            await FlutterBluePlus.stopScan();
            
            // Connect to device
            await result.device.connect();
            connectedDevice = result.device;
            
            // Discover services
            List<BluetoothService> services = await result.device.discoverServices();
            
            // Find RGBW service and characteristics
            for (BluetoothService service in services) {
              if (service.uuid.toString().toLowerCase().contains(RGBW_SERVICE_UUID)) {
                for (BluetoothCharacteristic char in service.characteristics) {
                  String charUuid = char.uuid.toString().toLowerCase();
                  
                  if (charUuid.contains(RED_CHAR_UUID)) {
                    redCharacteristic = char;
                  } else if (charUuid.contains(GREEN_CHAR_UUID)) {
                    greenCharacteristic = char;
                  } else if (charUuid.contains(BLUE_CHAR_UUID)) {
                    blueCharacteristic = char;
                  } else if (charUuid.contains(WARM_WHITE_CHAR_UUID)) {
                    warmWhiteCharacteristic = char;
                  }
                }
              }
            }
            
            return true;
          }
        }
      }
      
      // Device not found
      await FlutterBluePlus.stopScan();
      return false;
      
    } catch (e) {
      await FlutterBluePlus.stopScan();
      throw e;
    }
  }

  Future<void> setRGBW(int r, int g, int b, int w) async {
    if (connectedDevice == null) return;
    
    try {
      List<Future> writes = [];
      
      if (redCharacteristic != null) {
        writes.add(redCharacteristic!.write([r], withoutResponse: true));
      }
      if (greenCharacteristic != null) {
        writes.add(greenCharacteristic!.write([g], withoutResponse: true));
      }
      if (blueCharacteristic != null) {
        writes.add(blueCharacteristic!.write([b], withoutResponse: true));
      }
      if (warmWhiteCharacteristic != null) {
        writes.add(warmWhiteCharacteristic!.write([w], withoutResponse: true));
      }
      
      await Future.wait(writes);
    } catch (e) {
      print('Error setting RGBW: $e');
    }
  }

  Future<void> disconnect() async {
    if (connectedDevice != null) {
      await connectedDevice!.disconnect();
      connectedDevice = null;
      redCharacteristic = null;
      greenCharacteristic = null;
      blueCharacteristic = null;
      warmWhiteCharacteristic = null;
    }
  }

  bool get isConnected => connectedDevice != null;
}