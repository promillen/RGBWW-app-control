import 'package:flutter/material.dart';
import 'package:flex_color_picker/flex_color_picker.dart';
import '../services/ble_service.dart';

class ColorControlScreen extends StatefulWidget {
  final BLEService bleService;

  const ColorControlScreen({Key? key, required this.bleService}) : super(key: key);

  @override
  _ColorControlScreenState createState() => _ColorControlScreenState();
}

class _ColorControlScreenState extends State<ColorControlScreen> {
  Color selectedColor = Colors.white;
  double warmWhite = 0.0;
  double brightness = 1.0;
  bool _isUpdating = false;

  @override
  void initState() {
    super.initState();
    _updateLED();
  }

  Future<void> _updateLED() async {
    if (_isUpdating) return;
    _isUpdating = true;
    
    // Apply brightness to RGB values
    int r = ((selectedColor.red * brightness).round()).clamp(0, 255);
    int g = ((selectedColor.green * brightness).round()).clamp(0, 255);
    int b = ((selectedColor.blue * brightness).round()).clamp(0, 255);
    int w = ((warmWhite * brightness * 255).round()).clamp(0, 255);
    
    await widget.bleService.setRGBW(r, g, b, w);
    
    _isUpdating = false;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('RGBW Control'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: [
          IconButton(
            icon: Icon(Icons.bluetooth_disabled),
            onPressed: () async {
              await widget.bleService.disconnect();
              Navigator.of(context).popUntil((route) => route.isFirst);
            },
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: EdgeInsets.all(16),
        child: Column(
          children: [
            // LED Preview
            Container(
              width: double.infinity,
              height: 100,
              decoration: BoxDecoration(
                color: Color.fromRGBO(
                  (selectedColor.red * brightness).round(),
                  (selectedColor.green * brightness).round(),
                  (selectedColor.blue * brightness).round(),
                  1.0,
                ),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: Colors.grey.shade300),
              ),
              child: Center(
                child: Text(
                  'LED Preview',
                  style: TextStyle(
                    color: brightness > 0.5 ? Colors.black : Colors.white,
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            ),
            
            SizedBox(height: 24),
            
            // Color Picker
            Card(
              child: Padding(
                padding: EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'RGB Color',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    SizedBox(height: 16),
                    ColorPicker(
                      color: selectedColor,
                      onColorChanged: (Color color) {
                        setState(() {
                          selectedColor = color;
                        });
                        _updateLED();
                      },
                      width: 44,
                      height: 44,
                      borderRadius: 22,
                      spacing: 5,
                      runSpacing: 5,
                      wheelDiameter: 200,
                      heading: Text(
                        'Select color',
                        style: Theme.of(context).textTheme.titleSmall,
                      ),
                      subheading: Text(
                        'Select color shade',
                        style: Theme.of(context).textTheme.titleSmall,
                      ),
                      wheelSubheading: Text(
                        'Selected color and its shades',
                        style: Theme.of(context).textTheme.titleSmall,
                      ),
                      showMaterialName: true,
                      showColorName: true,
                      showColorCode: true,
                      copyPasteBehavior: const ColorPickerCopyPasteBehavior(
                        longPressMenu: true,
                      ),
                      materialNameTextStyle: Theme.of(context).textTheme.bodySmall,
                      colorNameTextStyle: Theme.of(context).textTheme.bodySmall,
                      colorCodeTextStyle: Theme.of(context).textTheme.bodySmall,
                      pickersEnabled: const <ColorPickerType, bool>{
                        ColorPickerType.both: false,
                        ColorPickerType.primary: true,
                        ColorPickerType.accent: true,
                        ColorPickerType.bw: false,
                        ColorPickerType.custom: true,
                        ColorPickerType.wheel: true,
                      },
                    ),
                  ],
                ),
              ),
            ),
            
            SizedBox(height: 16),
            
            // Warm White Control
            Card(
              child: Padding(
                padding: EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Warm White: ${(warmWhite * 100).round()}%',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    SizedBox(height: 8),
                    Slider(
                      value: warmWhite,
                      onChanged: (value) {
                        setState(() {
                          warmWhite = value;
                        });
                        _updateLED();
                      },
                      divisions: 100,
                      label: '${(warmWhite * 100).round()}%',
                    ),
                  ],
                ),
              ),
            ),
            
            SizedBox(height: 16),
            
            // Brightness Control
            Card(
              child: Padding(
                padding: EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Brightness: ${(brightness * 100).round()}%',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    SizedBox(height: 8),
                    Slider(
                      value: brightness,
                      onChanged: (value) {
                        setState(() {
                          brightness = value;
                        });
                        _updateLED();
                      },
                      divisions: 100,
                      label: '${(brightness * 100).round()}%',
                    ),
                  ],
                ),
              ),
            ),
            
            SizedBox(height: 24),
            
            // Preset Colors
            Card(
              child: Padding(
                padding: EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Quick Presets',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    SizedBox(height: 16),
                    Wrap(
                      spacing: 8,
                      runSpacing: 8,
                      children: [
                        _presetButton('Off', Colors.black, 0.0, 0.0),
                        _presetButton('White', Colors.white, 1.0, 0.0),
                        _presetButton('Warm', Colors.orange.shade100, 1.0, 1.0),
                        _presetButton('Red', Colors.red, 1.0, 0.0),
                        _presetButton('Green', Colors.green, 1.0, 0.0),
                        _presetButton('Blue', Colors.blue, 1.0, 0.0),
                        _presetButton('Purple', Colors.purple, 1.0, 0.0),
                        _presetButton('Yellow', Colors.yellow, 1.0, 0.0),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _presetButton(String name, Color color, double bright, double warm) {
    return ElevatedButton(
      onPressed: () {
        setState(() {
          selectedColor = color;
          brightness = bright;
          warmWhite = warm;
        });
        _updateLED();
      },
      child: Text(name),
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        foregroundColor: color.computeLuminance() > 0.5 ? Colors.black : Colors.white,
      ),
    );
  }

   @override
 void dispose() {
   widget.bleService.disconnect();
   super.dispose();
 }
}