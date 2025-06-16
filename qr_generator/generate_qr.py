#!/usr/bin/env python3
"""
QR Code Generator for Festival LED Controllers
Generates QR codes that link directly to the web app with device names
"""

import qrcode
import os
from PIL import Image, ImageDraw, ImageFont
import argparse

def generate_qr_codes(base_url, num_devices=10, device_prefix="RGBW_LED", start_num=1, auto_connect=False):
    """Generate QR codes for multiple LED devices"""
    
    # Create output directory
    output_dir = "qr_codes"
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"Generating {num_devices} QR codes...")
    print(f"Base URL: {base_url}")
    print(f"Device naming: {device_prefix}_{start_num:03d} onwards")
    print(f"Auto-connect: {'Yes' if auto_connect else 'No'}")
    print("-" * 50)
    
    for i in range(start_num, start_num + num_devices):
        device_id = f"{device_prefix}_{i:03d}"  # RGBW_LED_001, RGBW_LED_002, etc.
        
        # Create URL with device parameter
        url = f"{base_url}?device={device_id}"
        if auto_connect:
            url += "&autoconnect=true"
        
        # Generate QR code
        qr = qrcode.QRCode(
            version=1,  # Controls size (1 is smallest)
            error_correction=qrcode.constants.ERROR_CORRECT_M,  # Medium error correction
            box_size=8,  # Box size for good scanning
            border=4,
        )
        qr.add_data(url)
        qr.make(fit=True)
        
        # Create QR code image
        qr_img = qr.make_image(fill_color="black", back_color="white")
        
        # Create a larger image with text labels
        img_width = 350
        img_height = 420
        img = Image.new('RGB', (img_width, img_height), 'white')
        
        # Resize QR code to fit
        qr_size = 250
        qr_img = qr_img.resize((qr_size, qr_size), Image.Resampling.LANCZOS)
        
        # Paste QR code centered
        qr_x = (img_width - qr_size) // 2
        qr_y = 60
        img.paste(qr_img, (qr_x, qr_y))
        
        # Add text labels
        draw = ImageDraw.Draw(img)
        
        try:
            # Try to use system fonts
            title_font = ImageFont.truetype("arial.ttf", 22)
            subtitle_font = ImageFont.truetype("arial.ttf", 16)
            small_font = ImageFont.truetype("arial.ttf", 12)
        except (OSError, IOError):
            try:
                # Try alternative font names
                title_font = ImageFont.truetype("/System/Library/Fonts/Arial.ttf", 22)
                subtitle_font = ImageFont.truetype("/System/Library/Fonts/Arial.ttf", 16)
                small_font = ImageFont.truetype("/System/Library/Fonts/Arial.ttf", 12)
            except (OSError, IOError):
                # Fallback to default font
                title_font = ImageFont.load_default()
                subtitle_font = ImageFont.load_default()
                small_font = ImageFont.load_default()
        
        # Device title
        title_text = f"LED Controller #{i}"
        title_bbox = draw.textbbox((0, 0), title_text, font=title_font)
        title_width = title_bbox[2] - title_bbox[0]
        title_x = (img_width - title_width) // 2
        draw.text((title_x, 15), title_text, fill="black", font=title_font)
        
        # Device ID
        device_text = device_id
        device_bbox = draw.textbbox((0, 0), device_text, font=subtitle_font)
        device_width = device_bbox[2] - device_bbox[0]
        device_x = (img_width - device_width) // 2
        draw.text((device_x, 325), device_text, fill="black", font=subtitle_font)
        
        # Instructions
        if auto_connect:
            instruction_text = "Scan to auto-connect & control"
        else:
            instruction_text = "Scan with phone to control"
        instr_bbox = draw.textbbox((0, 0), instruction_text, font=small_font)
        instr_width = instr_bbox[2] - instr_bbox[0]
        instr_x = (img_width - instr_width) // 2
        draw.text((instr_x, 355), instruction_text, fill="gray", font=small_font)
        
        # Website URL (for reference)
        url_parts = base_url.replace("https://", "").replace("http://", "")
        url_text = url_parts
        if len(url_text) > 30:
            url_text = url_text[:27] + "..."
        
        url_bbox = draw.textbbox((0, 0), url_text, font=small_font)
        url_width = url_bbox[2] - url_bbox[0]
        url_x = (img_width - url_width) // 2
        draw.text((url_x, 380), url_text, fill="lightgray", font=small_font)
        
        # Save image
        filename = f"{output_dir}/{device_id}_qr.png"
        img.save(filename, "PNG", quality=95, optimize=True)
        print(f"✅ Generated: {filename}")
        
    print(f"\n🎉 Successfully generated {num_devices} QR codes!")
    return output_dir

def generate_batch_sheet(output_dir, cols=3):
    """Generate a single sheet with multiple QR codes for easy printing"""
    qr_files = [f for f in os.listdir(output_dir) if f.endswith('_qr.png')]
    qr_files.sort()
    
    if not qr_files:
        print("No QR code files found for batch sheet generation")
        return
    
    # Calculate sheet dimensions
    qr_width = 350
    qr_height = 420
    margin = 15
    
    rows = (len(qr_files) + cols - 1) // cols
    sheet_width = cols * qr_width + (cols + 1) * margin
    sheet_height = rows * qr_height + (rows + 1) * margin
    
    # Create large sheet
    sheet = Image.new('RGB', (sheet_width, sheet_height), 'white')
    
    for idx, qr_file in enumerate(qr_files):
        row = idx // cols
        col = idx % cols
        
        x = margin + col * (qr_width + margin)
        y = margin + row * (qr_height + margin)
        
        qr_img = Image.open(os.path.join(output_dir, qr_file))
        sheet.paste(qr_img, (x, y))
    
    # Save batch sheet
    batch_filename = f"{output_dir}/batch_sheet_{cols}x{rows}.png"
    sheet.save(batch_filename, "PNG", quality=95, optimize=True)
    print(f"📄 Generated batch sheet: {batch_filename}")

def main():
    parser = argparse.ArgumentParser(description='Generate QR codes for LED controllers')
    parser.add_argument('--url', default='https://yourusername.github.io/led-control', 
                       help='Base URL for the web app')
    parser.add_argument('--count', type=int, default=10, 
                       help='Number of QR codes to generate')
    parser.add_argument('--prefix', default='RGBW_LED', 
                       help='Device name prefix')
    parser.add_argument('--start', type=int, default=1, 
                       help='Starting device number')
    parser.add_argument('--batch', action='store_true', 
                       help='Generate a batch sheet for printing')
    parser.add_argument('--auto-connect', action='store_true',
                       help='Add auto-connect parameter to URLs (future feature)')
    
    args = parser.parse_args()
    
    print("🎨 Festival LED Controller QR Generator")
    print("=" * 50)
    
    # Generate individual QR codes
    output_dir = generate_qr_codes(
        base_url=args.url,
        num_devices=args.count,
        device_prefix=args.prefix,
        start_num=args.start,
        auto_connect=args.auto_connect
    )
    
    # Generate batch sheet if requested
    if args.batch:
        print(f"\n📄 Generating batch sheet...")
        generate_batch_sheet(output_dir)
    
    print(f"\n📁 All files saved to: {output_dir}/")
    print(f"\n📋 Next steps:")
    print(f"1. Deploy your web app to: {args.url}")
    print(f"2. Print the QR codes (individual or batch sheet)")
    print(f"3. Flash ESP32 devices with names: {args.prefix}_001, {args.prefix}_002, etc.")
    print(f"4. Attach QR codes to corresponding devices")
    print(f"\n💡 Test by scanning a QR code with your phone!")
    
    if args.auto_connect:
        print(f"\n⚠️  Auto-connect feature is not yet implemented in the web app")

if __name__ == "__main__":
    # Check dependencies
    try:
        import qrcode
        from PIL import Image, ImageDraw, ImageFont
    except ImportError as e:
        print("❌ Missing dependencies. Install with:")
        print("pip install qrcode[pil] pillow")
        print(f"Error: {e}")
        exit(1)
    
    main()