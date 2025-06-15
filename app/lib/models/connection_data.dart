class ConnectionData {
  final String deviceName;
  final String macAddress;

  ConnectionData({
    required this.deviceName,
    required this.macAddress,
  });

  @override
  String toString() {
    return 'ConnectionData(deviceName: $deviceName, macAddress: $macAddress)';
  }
}