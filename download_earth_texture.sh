#!/bin/bash
# Download NASA Earth textures

cd "source code"

echo "Downloading NASA Blue Marble Earth texture..."

# Try NASA's direct image (2K resolution)
curl -L -o earth_color.jpg "https://eoimages.gsfc.nasa.gov/images/imagerecords/74000/74218/world.200412.3x5400x2700.jpg" || \
wget -O earth_color.jpg "https://eoimages.gsfc.nasa.gov/images/imagerecords/74000/74218/world.200412.3x5400x2700.jpg"

if [ -f "earth_color.jpg" ]; then
    echo "✓ Earth texture downloaded successfully!"
    ls -lh earth_color.jpg
else
    echo "✗ Download failed. Please manually download from:"
    echo "  https://visibleearth.nasa.gov/collection/1484/blue-marble"
    echo "  or https://www.solarsystemscope.com/textures/"
fi
