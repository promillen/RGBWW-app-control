#!/usr/bin/env python3
"""
QR Code Generator for Festival LED Controllers
Generates QR codes that link to the web app with device-specific parameters
"""

import qrcode
import os
from PIL import Image, ImageDraw, ImageFont

def generate_qr_codes(base_url, num_devices=10):
    """Generate QR codes for multiple LED devices"""
    
    # Create output directory
    output_dir = "qr_codes"
    os.makedirs(output_dir, exist_ok=True)
    
    for i in range(1, num_devices + 1):
        device_id = f"RGBW_LED_{i:03d}"  # RGBW_LED_001, RGBW_LED_002, etc.
        
        # Generate a fake MAC address for demo (in real use, get from ESP32)
        mac = f"AA:BB:CC:DD:EE:{i:02X}"
        
        # Create URL with device parameters
        url = f"{base_url}?device={device_id}&mac={mac}"
        
        # Generate QR code
        qr = qrcode.QRCode(
            version=1,  # Controls size (1 is smallest)
            error_correction=qrcode.constants.ERROR_CORRECT_L,
            box_size=10,
            border=4,
        )
        qr.add_data(url)
        qr.make(fit=True)
        
        # Create QR code image
        qr_img = qr.make_image(fill_color="black", back_color="white")
        
        # Create a larger image with text labels
        img_width = 400
        img_height = 500
        img = Image.new('RGB', (img_width, img_height), 'white')
        
        # Resize QR code to fit
        qr_size = 300
        qr_img = qr_img.resize((qr_size, qr_size))
        
        # Paste QR code centered
        qr_x = (img_width - qr_size) // 2
        qr_y = 50
        img.paste(qr_img, (qr_x, qr_y))
        
        # Add text labels
        draw = ImageDraw.Draw(img)
        
        try:
            # Try to use a nice font
            title_font = ImageFont.truetype("arial.ttf", 24)
            subtitle_font = ImageFont.truetype("arial.ttf", 16)
        except:
            # Fallback to default font
            title_font = ImageFont.load_default()
            subtitle_font = ImageFont.load_default()
        
        # Device title
        title_text = f"LED Controller {i}"
        title_bbox = draw.textbbox((0, 0), title_text, font=title_font)
        title_width = title_bbox[2] - title_bbox[0]
        title_x = (img_width - title_width) // 2
        draw.text((title_x, 10), title_text, fill="black", font=title_font)
        
        # Device ID
        device_text = device_id
        device_bbox = draw.textbbox((0, 0), device_text, font=subtitle_font)
        device_width = device_bbox[2] - device_bbox[0]
        device_x = (img_width - device_width) // 2
        draw.text((device_x, 370), device_text, fill="black", font=subtitle_font)
        
        # Instructions
        instruction_text = "Scan to control this LED"
        instr_bbox = draw.textbbox((0, 0), instruction_text, font=subtitle_font)
        instr_width = instr_bbox[2] - instr_bbox[0]
        instr_x = (img_width - instr_width) // 2
        draw.text((instr_x, 400), instruction_text, fill="gray", font=subtitle_font)
        
        # MAC address
        mac_text = f"MAC: {mac}"
        mac_bbox = draw.textbbox((0, 0), mac_text, font=subtitle_font)
        mac_width = mac_bbox[2] - mac_bbox[0]
        mac_x = (img_width - mac_width) // 2
        draw.text((mac_x, 430), mac_text, fill="gray", font=subtitle_font)
        
        # Save image
        filename = f"{output_dir}/{device_id}_qr.png"
        img.save(filename)
        print(f"Generated: {filename}")
        print(f"URL: {url}")
        print()

def main():
    # Configuration
    BASE_URL = "https://your-domain.com/control"  # Change this to your web app URL
    NUM_DEVICES = 10  # Number of LED controllers
    
    print("Festival LED Controller QR Generator")
    print("=" * 40)
    print(f"Base URL: {BASE_URL}")
    print(f"Generating QR codes for {NUM_DEVICES} devices...")
    print()
    
    generate_qr_codes(BASE_URL, NUM_DEVICES)
    
    print("✅ QR code generation complete!")
    print(f"📁 Check the 'qr_codes' folder for {NUM_DEVICES} QR code images")
    print()
    print("Next steps:")
    print("1. Host your web app at the configured URL")
    print("2. Print the QR codes and attach to each LED controller")
    print("3. Flash firmware with matching device names")

if __name__ == "__main__":
    # Check dependencies
    try:
        import qrcode
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("❌ Missing dependencies. Install with:")
        print("pip install qrcode[pil] pillow")
        exit(1)
    
    main()