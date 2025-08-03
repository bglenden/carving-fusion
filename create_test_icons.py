#!/usr/bin/env python3
"""Create simple test icons for the C++ plugin"""

from PIL import Image, ImageDraw, ImageFont
import os

def create_icon(text, size, output_path):
    """Create a simple icon with text"""
    # Create image with light gray background
    img = Image.new('RGBA', (size, size), (240, 240, 240, 255))
    draw = ImageDraw.Draw(img)
    
    # Draw a circle
    margin = size // 8
    draw.ellipse([margin, margin, size-margin, size-margin], 
                 fill=(100, 150, 200, 255), outline=(50, 100, 150, 255))
    
    # Add text (simplified - just use default font)
    text_size = size // 3
    text_pos = (size // 2 - text_size // 2, size // 2 - text_size // 2)
    draw.text(text_pos, text, fill=(255, 255, 255, 255))
    
    # Save
    img.save(output_path)
    print(f"Created {output_path}")

# Create icon directory
icon_dir = "resources/hello"
os.makedirs(icon_dir, exist_ok=True)

# Create icons in different sizes
create_icon("C++", 16, f"{icon_dir}/16x16.png")
create_icon("C++", 32, f"{icon_dir}/32x32.png")

# For retina displays
create_icon("C++", 32, f"{icon_dir}/16x16@2x.png")
create_icon("C++", 64, f"{icon_dir}/32x32@2x.png")

print("Icons created successfully!")