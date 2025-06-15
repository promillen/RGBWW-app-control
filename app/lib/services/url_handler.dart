import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../models/connection_data.dart';
import '../screens/connection_screen.dart';

class URLHandler {
  static const platform = MethodChannel('rgbw_controller/url');

  void initialize(BuildContext context) {
    platform.setMethodCallHandler((call) async {
      if (call.method == 'handleURL') {
        final String url = call.arguments;
        _handleDeepLink(context, url);
      }
    });
  }

  void _handleDeepLink(BuildContext context, String url) {
    if (url.startsWith('rgbwled://connect')) {
      final uri = Uri.parse(url);
      final name = uri.queryParameters['name'];
      final mac = uri.queryParameters['mac'];
      
      if (name != null && mac != null) {
        final connectionData = ConnectionData(
          deviceName: name,
          macAddress: mac,
        );
        
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(
            builder: (context) => ConnectionScreen(connectionData: connectionData),
          ),
        );
      }
    }
  }
}