# RGBW LED Controller

A professional BLE-controlled 100W RGBW COB LED controller for ESP32-C3, with optimizations for different LED driver ICs, board configurations, and a complete web-based control interface.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Supported Hardware](#supported-hardware)
- [Firmware](#firmware)
  - [Quick Start](#quick-start)
  - [Configuration](#configuration)
  - [Building Different Device Variants](#building-different-device-variants)
  - [BLE Service Specification](#ble-service-specification)
- [Web Application](#web-application)
  - [Features](#web-app-features)
  - [Usage](#web-app-usage)
  - [QR Code Integration](#qr-code-integration)
  - [Accessing the Web App](#accessing-the-web-app)
- [QR Code Generator](#qr-code-generator)
  - [Purpose](#qr-code-purpose)
  - [Usage](#qr-code-usage)
  - [QR Code Format](#qr-code-format)
- [Development](#development)
- [Hardware Design Notes](#hardware-design-notes)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Overview

This project provides a complete solution for controlling 100W RGBW COB LEDs via Bluetooth Low Energy (BLE), including:

- **ESP32-C3 Firmware** - Optimized for different LED driver ICs
- **Web Application** - Browser-based controller with modern UI
- **QR Code Generator** - For easy device discovery and labeling
- **Professional Configuration** - Using ESP-IDF Kconfig system

## Features

### Firmware Features
- **Bluetooth Low Energy (BLE) Control** - Control via smartphone apps or web browser
- **Multiple Light Effects** - Static colors, smooth fades, breathing, lightning, candle flicker, and more
- **Driver-Specific Optimizations** - Optimized for AL8860 and LM3414 LED drivers
- **Board Configuration Support** - Easy configuration for different hardware variants
- **Auto-Discovery Mode** - Smooth color cycling when no device is connected
- **High-Resolution PWM** - 8-bit (AL8860) or 12-bit (LM3414) resolution

### Web Application Features
- **Modern UI** - Responsive design with glassmorphism effects
- **Real-time Control** - Live connection status and instant feedback
- **Color Picker** - HSV color wheel with brightness control
- **Effect Selection** - Visual effect picker with previews
- **Device Discovery** - Automatic BLE device scanning
- **QR Code Scanner** - Quick device connection via QR codes
- **Mobile Optimized** - Works on smartphones and tablets

### QR Code System
- **Device Identification** - Unique QR codes for each device
- **Quick Connection** - Scan to connect instantly
- **Label Generation** - Professional labels for device identification
- **Easy Deployment** - Simplified device management for installations

## Supported Hardware

### ESP32-C3 with OLED Display (AL8860 Driver)
- **LED Driver**: AL8860 (hysteretic control)
- **Max Current**: 1500mA per channel
- **PWM Resolution**: 8-bit (256 levels)
- **PWM Frequency**: 1000Hz (optimized for AL8860)
- **GPIO Mapping**: R=10, G=9, B=8, WW=7
- **Special Effects**: Pulse Wave, Soft Transition

### ESP32-C3 without OLED Display (LM3414 Driver)
- **LED Driver**: LM3414 (high precision)
- **Max Current**: 1000mA per channel
- **PWM Resolution**: 12-bit (4096 levels)
- **PWM Frequency**: 5000Hz (optimized for LM3414)
- **GPIO Mapping**: R=5, G=6, B=7, WW=8
- **Special Effects**: Precision Fade, Fast Strobe

## Firmware

### Quick Start

#### Prerequisites
- ESP-IDF v5.4.1 or later
- ESP32-C3 development board
- 100W RGBWW COB LED module
- Appropriate LED driver circuit (AL8860 or LM3414)
- Adequate heatsink and thermal management
- High-current power supply (24V/36V recommended)

#### Build and Flash

```bash
git clone <your-repo-url>
cd rgbw-led-controller/firmware

# Configure your build
idf.py menuconfig

# Build and flash
idf.py build
idf.py flash monitor
```

### Configuration

Use the ESP-IDF configuration menu to set up your board:

```bash
idf.py menuconfig
```

Navigate to **"RGBW LED Controller Configuration"** and configure:

- **Board Type**: Select your hardware variant
  - ESP32-C3 with OLED (AL8860 driver)
  - ESP32-C3 without OLED (LM3414 driver)
- **BLE Device Name**: Set unique name (e.g., "RGBW_LED_001")
- **GPIO Pins**: Adjust if using custom pin mapping
- **LED Driver Settings**: Fine-tune current limits and frequencies

### Building Different Device Variants

#### Method 1: Using Menuconfig (Recommended)

```bash
idf.py menuconfig
# Navigate to "RGBW LED Controller Configuration"
# Change "BLE Device Name" to "RGBW_LED_002", etc.
idf.py build
```

#### Method 2: Using Configuration Files

```bash
# Configure for device 001
idf.py menuconfig
cp sdkconfig sdkconfig.device001

# Build specific device
cp sdkconfig.device001 sdkconfig
idf.py build
```

#### Method 3: Build Profiles

```bash
# Create configuration files
echo "CONFIG_DEVICE_NAME=\"RGBW_LED_001\"" > sdkconfig.dev001

# Build with specific config
idf.py -D SDKCONFIG_DEFAULTS=sdkconfig.dev001 build
```

### BLE Service Specification

#### Service UUID: `0x00FF`

| Characteristic | UUID | Access | Description |
|----------------|------|--------|-------------|
| Red Channel | 0xFF01 | R/W | Red brightness (0-255) |
| Green Channel | 0xFF02 | R/W | Green brightness (0-255) |
| Blue Channel | 0xFF03 | R/W | Blue brightness (0-255) |
| White Channel | 0xFF04 | R/W | White brightness (0-255) |
| Effect Mode | 0xFF05 | R/W | Light effect (see table below) |
| Brightness | 0xFF06 | R/W | Master brightness (0-255) |
| Speed | 0xFF07 | R/W | Effect speed (0-255) |

#### Light Effects

| Value | Effect | Description |
|-------|--------|-------------|
| 0 | OFF | All LEDs off |
| 1 | STATIC | Static color (manual control) |
| 2 | SMOOTH_FADE | Smooth RGB color cycling (default) |
| 3 | RGB_CYCLE | Hard RGB transitions |
| 4 | BREATHING | Breathing effect |
| 5 | TWINKLE_PULSE | Random color flickers |
| 6 | LIGHTNING_FLASH | Lightning storm effect |
| 7 | CANDLE_FLICKER | Warm candle flame |

**Board-Specific Effects:**

*AL8860 Boards (ESP32-C3 with OLED):*
| Value | Effect | Description |
|-------|--------|-------------|
| 8 | PULSE_WAVE | Optimized for hysteretic control |
| 9 | SOFT_TRANSITION | Leverages soft-start capability |

*LM3414 Boards (ESP32-C3 without OLED):*
| Value | Effect | Description |
|-------|--------|-------------|
| 8 | PRECISION_FADE | High-resolution 12-bit fading |
| 9 | FAST_STROBE | High-frequency strobe effects |

## Web Application

### Web App Features

The web application provides a modern, responsive interface for controlling RGBW LED devices:

- **🎨 Intuitive Color Control** - HSV color wheel with live preview
- **✨ Effect Gallery** - Visual effect selection with descriptions
- **📱 Mobile Optimized** - Touch-friendly interface for smartphones
- **🔗 Real-time Connection** - Live status indicators and connection management
- **📷 QR Code Scanner** - Built-in camera scanner for quick device connection
- **🌈 Live Preview** - See color changes in real-time
- **⚡ Fast Response** - Optimized BLE communication for instant feedback

### Web App Usage

#### Accessing the Controller

1. **Open the web app** in a modern browser (Chrome, Edge, Safari)
2. **Enable Bluetooth** when prompted
3. **Scan for devices** or use QR code scanner
4. **Connect** to your RGBW LED device

#### Using the Interface

**Color Control:**
- Use the **color wheel** to select hue and saturation
- Adjust **brightness** with the vertical slider
- **Individual channel control** for precise adjustments
- **White channel** for brightness and color temperature adjustment

**Effect Control:**
- **Effect gallery** with visual previews
- **Speed control** for dynamic effects
- **Real-time switching** between effects
- **Custom effect parameters** where applicable

**Device Management:**
- **Connection status** indicator
- **Device information** display
- **Reconnection** handling
- **Multiple device** support (switch between devices)

### QR Code Integration

The web app includes a built-in QR code scanner for quick device connection:

1. **Click the QR scanner button** in the device selection screen
2. **Allow camera access** when prompted
3. **Point camera at device QR code**
4. **Automatic connection** to the scanned device

### Accessing the Web App

#### Option 1: GitHub Pages (Recommended)
```
https://your-username.github.io/rgbw-led-controller/
```

#### Option 2: Local Development
```bash
cd web-app
# Open index.html in a modern browser
# or use a local server:
python -m http.server 8080
# Navigate to http://localhost:8080
```

#### Browser Requirements
- **Chrome 56+** (recommended)
- **Edge 79+**
- **Safari 15+** (iOS 15+)
- **Firefox 104+** (with flag enabled)

*Note: Web Bluetooth requires HTTPS in production environments*

## QR Code Generator

### QR Code Purpose

The QR code generator creates professional labels for RGBW LED devices, enabling:

- **Quick Device Identification** - Visual device labeling
- **Instant Connection** - Scan to connect workflow
- **Professional Deployment** - Clean, labeled installations
- **Easy Maintenance** - Technicians can quickly identify and connect to devices

### QR Code Usage

#### Generating QR Codes

```bash
cd qr-generator
python generate_qr.py
```

**Interactive Mode:**
1. Enter device name (e.g., "RGBW_LED_001")
2. Select label size (small, medium, large)
3. Choose output format (PNG, PDF, SVG)
4. Generate and save labels

**Batch Mode:**
```bash
python generate_qr.py --batch devices.txt --format pdf --size medium
```

#### Label Options

**Sizes:**
- **Small** (2" x 1") - For compact installations
- **Medium** (3" x 2") - Standard size, good readability
- **Large** (4" x 3") - High visibility, easy scanning

**Formats:**
- **PNG** - For digital display or basic printing
- **PDF** - Professional printing with vector graphics
- **SVG** - Scalable vector format for any size

**Information Included:**
- Device name (human-readable)
- BLE connection data (encoded)
- Device type identifier
- Optional: Installation date, location, notes

### QR Code Format

QR codes contain JSON data for device connection:

```json
{
  "type": "rgbw_cob_led",
  "device_name": "RGBW_LED_001",
  "service_uuid": "00ff",
  "power_rating": "100W",
  "channels": 4,
  "version": "2.0"
}
```

**Usage in Web App:**
1. Scan QR code with web app
2. Parse device information
3. Automatic BLE connection attempt
4. Display device-specific interface

## Development

### Project Structure

```
rgbw-led-controller/
├── firmware/                    # ESP32-C3 firmware
│   ├── main/
│   │   ├── Kconfig             # Configuration definitions
│   │   ├── main.c              # Main application
│   │   ├── ble_server.c/.h     # BLE GATT server
│   │   ├── pwm_control.c/.h    # PWM/LED control
│   │   ├── light_effects.c/.h  # Light effect engine
│   │   └── CMakeLists.txt
│   ├── CMakeLists.txt          # Root build configuration
│   └── sdkconfig               # Generated configuration
├── web-app/                    # Web application
│   ├── index.html              # Main interface
│   ├── css/
│   │   └── style.css          # Modern UI styles
│   ├── js/
│   │   ├── app.js             # Main application logic
│   │   ├── ble-controller.js  # BLE communication
│   │   └── qr-scanner.js      # QR code scanning
│   └── assets/                # Images and icons
├── qr-generator/               # QR code generation
│   ├── generate_qr.py         # Main generator script
│   ├── templates/             # Label templates
│   └── output/                # Generated labels
└── docs/                      # Documentation
```

### Adding New Features

#### New Light Effects (Firmware)
1. Add effect to `light_effect_t` enum in `light_effects.h`
2. Implement effect function in `light_effects.c`
3. Add case to switch statement in `effects_task()`
4. Update web app effect gallery

#### Web App Features
1. Modify `js/app.js` for new UI elements
2. Update `css/style.css` for styling
3. Test across different browsers and devices

#### QR Code Features
1. Extend `generate_qr.py` for new data formats
2. Add new templates in `templates/`
3. Update web app QR parser

### Testing

#### Firmware Testing
```bash
cd firmware
idf.py build flash monitor
```

#### Web App Testing
- Test in multiple browsers
- Verify BLE functionality
- Check mobile responsiveness
- Test QR code scanning

#### QR Code Testing
- Verify QR code readability
- Test with different label sizes
- Check web app integration

## Hardware Design Notes

### AL8860 Integration
- **Hysteretic Control** - Better efficiency for LED drivers
- **Lower Switching Frequency** - Reduces EMI and heat
- **Built-in Soft-start** - Reduces inrush current
- **Optimized Effects** - Designed for hysteretic behavior

### LM3414 Integration  
- **High Switching Frequency** - Enables smaller inductors
- **Better Regulation** - Higher accuracy for precise color control
- **Higher PWM Resolution** - Smoother effects and better color mixing
- **Fast Response** - Suitable for high-frequency effects

### 100W COB LED Design Considerations
- **High Power Density** - 100W total (25W per channel) in compact package
- **Multiple LED Drivers** - Requires 4 separate driver circuits (one per channel)
- **Thermal Management** - Critical for performance and longevity
- **Current Control** - Precise regulation for color accuracy and thermal protection
- **Color Mixing** - Physical LED die placement affects color uniformity
- **Power Distribution** - High current paths for each channel
- **Isolation** - Electrical isolation between channels may be required

### Thermal Management (Critical for 100W Operation)
- **Heatsink Requirements** - Minimum 0.5°C/W thermal resistance
- **Thermal Interface** - High-quality thermal compound or thermal pads required
- **Temperature Monitoring** - Thermal protection circuits strongly recommended
- **Active Cooling** - Fans or liquid cooling required for continuous operation
- **Thermal Derating** - Automatic current reduction at high temperatures
- **Heat Distribution** - Ensure even heat spreading across COB surface
- **Ambient Considerations** - Derate power in high ambient temperature environments

### Power Supply Requirements
- **Voltage**: 36V or 48V recommended for efficiency
- **Current**: Minimum 3A (for 100W operation)
- **Regulation**: ±1% voltage regulation recommended
- **Ripple**: <100mV peak-to-peak for LED driver stability
- **Protection**: Overcurrent and overvoltage protection required

## Troubleshooting

### Firmware Issues

**Device Not Advertising:**
- Check BLE is enabled in sdkconfig
- Verify GPIO pins don't conflict with internal peripherals
- Check serial output for error messages

**LEDs Not Working:**
- Verify GPIO configuration matches your hardware
- Check PWM frequency is appropriate for your LED drivers
- Ensure adequate power supply for 100W COB LED (36V/3A minimum)
- Check thermal protection - LED may shut down if overheating
- Verify heatsink is properly installed with thermal interface
- Check individual channel drivers - one channel failure affects only that color
- Monitor LED temperature - COB LEDs are sensitive to overheating

**Configuration Issues:**
- Run `idf.py fullclean` if changing board types
- Verify all Kconfig options are set correctly
- Check for GPIO conflicts in menuconfig

### Web App Issues

**Cannot Connect to Device:**
- Ensure browser supports Web Bluetooth
- Check if device is advertising (check firmware logs)
- Try refreshing the page and scanning again
- Verify HTTPS is used (required for Web Bluetooth)

**QR Scanner Not Working:**
- Ensure adequate lighting for QR code
- Try different camera angles
- Check if QR code is properly generated

**Poor Performance:**
- Check Bluetooth signal strength
- Close other BLE connections
- Refresh the page to reset connection
- Try from a different device

### QR Code Issues

**QR Code Won't Scan:**
- Ensure sufficient contrast (dark on light background)
- Check label print quality
- Verify QR code size is adequate for scanning distance
- Test with different QR scanner apps

**Generated Labels Look Poor:**
- Use vector formats (PDF, SVG) for printing
- Ensure printer resolution is adequate
- Check template compatibility with label stock

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on both AL8860 and LM3414 configurations
5. Test web app across different browsers
6. Submit a pull request

## Version History

- **v2.0.0** - Complete system with web app, QR codes, and Kconfig
- **v1.0.0** - Initial firmware release with basic BLE control