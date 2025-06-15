import 'package:flutter/material.dart';
import '../models/connection_data.dart';
import '../services/ble_service.dart';
import 'color_control_screen.dart';

class ConnectionScreen extends StatefulWidget {
  final ConnectionData connectionData;

  const ConnectionScreen({Key? key, required this.connectionData}) : super(key: key);

  @override
  _ConnectionScreenState createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends State<ConnectionScreen> {
  final BLEService _bleService = BLEService();
  String _connectionStatus = 'Connecting...';
  bool _isConnecting = true;

  @override
  void initState() {
    super.initState();
    _connectToDevice();
  }

  Future<void> _connectToDevice() async {
    try {
      setState(() {
        _connectionStatus = 'Scanning for device...';
      });

      bool connected = await _bleService.connectToDevice(widget.connectionData.deviceName);
      
      if (connected) {
        setState(() {
          _connectionStatus = 'Connected successfully!';
          _isConnecting = false;
        });
        
        // Wait a moment then navigate to control screen
        await Future.delayed(Duration(seconds: 1));
        
        if (mounted) {
          Navigator.of(context).pushReplacement(
            MaterialPageRoute(
              builder: (context) => ColorControlScreen(bleService: _bleService),
            ),
          );
        }
      } else {
        setState(() {
          _connectionStatus = 'Failed to connect';
          _isConnecting = false;
        });
      }
    } catch (e) {
      setState(() {
        _connectionStatus = 'Error: ${e.toString()}';
        _isConnecting = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Connecting to Device'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (_isConnecting) ...[
              CircularProgressIndicator(),
              SizedBox(height: 32),
            ] else ...[
              Icon(
                _connectionStatus.contains('success') 
                  ? Icons.check_circle 
                  : Icons.error,
                size: 80,
                color: _connectionStatus.contains('success') 
                  ? Colors.green 
                  : Colors.red,
              ),
              SizedBox(height: 32),
            ],
            Text(
              'Device: ${widget.connectionData.deviceName}',
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.bold,
              ),
            ),
            if (widget.connectionData.macAddress.isNotEmpty) ...[
              SizedBox(height: 8),
              Text(
                'MAC: ${widget.connectionData.macAddress}',
                style: TextStyle(
                  fontSize: 14,
                  color: Colors.grey[600],
                ),
              ),
            ],
            SizedBox(height: 32),
            Text(
              _connectionStatus,
              style: TextStyle(fontSize: 16),
              textAlign: TextAlign.center,
            ),
            SizedBox(height: 32),
            if (!_isConnecting && !_connectionStatus.contains('success')) ...[
              ElevatedButton(
                onPressed: () {
                  setState(() {
                    _isConnecting = true;
                  });
                  _connectToDevice();
                },
                child: Text('Retry Connection'),
              ),
              SizedBox(height: 16),
              TextButton(
                onPressed: () => Navigator.of(context).pop(),
                child: Text('Go Back'),
              ),
            ],
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    super.dispose();
  }
}