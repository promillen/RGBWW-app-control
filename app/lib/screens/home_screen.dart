import 'package:flutter/material.dart';
import 'connection_screen.dart';
import '../models/connection_data.dart';

class HomeScreen extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('RGBW LED Controller'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.lightbulb_outline,
              size: 120,
              color: Colors.orange,
            ),
            SizedBox(height: 40),
            Text(
              'RGBW LED Controller',
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(height: 20),
            Text(
              'Scan the QR code on your ESP32 device\nwith your camera app to connect',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 16,
                color: Colors.grey[600],
              ),
            ),
            SizedBox(height: 40),
            ElevatedButton.icon(
              onPressed: () {
                // Manual connection option
                _showManualConnectionDialog(context);
              },
              icon: Icon(Icons.bluetooth),
              label: Text('Manual Connection'),
              style: ElevatedButton.styleFrom(
                padding: EdgeInsets.symmetric(horizontal: 32, vertical: 16),
              ),
            ),
            SizedBox(height: 20),
            TextButton(
              onPressed: () {
                _showInstructions(context);
              },
              child: Text('How to connect?'),
            ),
          ],
        ),
      ),
    );
  }

  void _showManualConnectionDialog(BuildContext context) {
    final deviceNameController = TextEditingController(text: 'RGBW_LED');
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Manual Connection'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: deviceNameController,
                decoration: InputDecoration(
                  labelText: 'Device Name',
                  hintText: 'RGBW_LED',
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                final deviceName = deviceNameController.text.trim();
                if (deviceName.isNotEmpty) {
                  Navigator.of(context).pop();
                  Navigator.of(context).push(
                    MaterialPageRoute(
                      builder: (context) => ConnectionScreen(
                        connectionData: ConnectionData(
                          deviceName: deviceName,
                          macAddress: '',
                        ),
                      ),
                    ),
                  );
                }
              },
              child: Text('Connect'),
            ),
          ],
        );
      },
    );
  }

  void _showInstructions(BuildContext context) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('How to Connect'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('1. Power on your RGBW LED controller'),
              SizedBox(height: 8),
              Text('2. Connect to WiFi network "RGBW_Setup"'),
              SizedBox(height: 8),
              Text('3. Open browser and go to 192.168.4.1'),
              SizedBox(height: 8),
              Text('4. Scan the QR code with your camera app'),
              SizedBox(height: 8),
              Text('5. Your phone will open this app automatically'),
            ],
          ),
          actions: [
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text('Got it!'),
            ),
          ],
        );
      },
    );
  }
}