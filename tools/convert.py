from PIL import Image
import numpy as np

# Define image dimensions
width = 320
height = 480

# Read the raw RGB565 file
with open("screenshot.raw", "rb") as f:
    raw_data = f.read()

# Convert the raw data to an array of 16-bit values (RGB565)
pixels = np.frombuffer(raw_data, dtype=np.uint16)

# Convert RGB565 to RGB888
def rgb565_to_rgb888(pixel):
    r = (pixel & 0xF800) >> 8
    g = (pixel & 0x07E0) >> 3
    b = (pixel & 0x001F) << 3
    return (r, g, b)

# Apply the conversion to all pixels
rgb_data = np.array([rgb565_to_rgb888(pixel) for pixel in pixels], dtype=np.uint8)

# Reshape to the correct dimensions
rgb_data = rgb_data.reshape((height, width, 3))

# Create an image from the RGB data
image = Image.fromarray(rgb_data)
image.save("screenshot.png")
