#!/usr/bin/env python3
"""Create icons for the three C++ plugin commands"""

from PIL import Image, ImageDraw, ImageFont
import os

def create_command_icon(text, color, size, output_path):
    """Create a command icon with text and color"""
    # Create image with light gray background
    img = Image.new('RGBA', (size, size), (240, 240, 240, 255))
    draw = ImageDraw.Draw(img)
    
    # Draw a colored circle
    margin = size // 8
    draw.ellipse([margin, margin, size-margin, size-margin], 
                 fill=color, outline=(50, 50, 50, 255))
    
    # Add text (simplified)
    text_size = size // 4 if len(text) <= 2 else size // 6
    text_bbox = draw.textbbox((0, 0), text)
    text_width = text_bbox[2] - text_bbox[0]
    text_height = text_bbox[3] - text_bbox[1]
    text_pos = ((size - text_width) // 2, (size - text_height) // 2 - 2)
    draw.text(text_pos, text, fill=(255, 255, 255, 255))
    
    # Save
    img.save(output_path)
    print(f"Created {output_path}")

def create_icons_for_command(command_name, text, color):
    """Create all icon sizes for a command"""
    sizes = [
        (16, f"resources/{command_name}/16x16.png"),
        (32, f"resources/{command_name}/32x32.png"),
        (32, f"resources/{command_name}/16x16@2x.png"),
        (64, f"resources/{command_name}/32x32@2x.png")
    ]
    
    for size, path in sizes:
        create_command_icon(text, color, size, path)

# Create icons for each command
create_icons_for_command("import", "IMP", (100, 150, 200, 255))    # Blue
create_icons_for_command("generate", "GEN", (150, 100, 200, 255))  # Purple  
create_icons_for_command("test", "TEST", (200, 150, 100, 255))     # Orange

print("All command icons created successfully!")