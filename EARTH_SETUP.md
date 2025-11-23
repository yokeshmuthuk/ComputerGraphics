# Earth 3D Model - Setup Guide

This project renders a spinning Earth in 3D space with advanced Phong lighting and texture mapping.

## Features Implemented

✅ **3D Earth Sphere** - Procedurally generated UV sphere with 64x64 resolution
✅ **Texture Mapping** - Supports loading Earth color textures
✅ **Advanced Lighting** - Phong lighting model with ambient, diffuse, and specular components
✅ **Black Space Background** - Realistic space environment
✅ **Smooth Rotation** - Earth spins continuously around Y-axis
✅ **Fallback Texture** - Automatic procedural texture if image file not found

## Quick Start

### Option 1: Run with Procedural Texture (Works Immediately)

```bash
cd /home/user/ComputerGraphics/build
./lab4_main
```

The application will automatically create a blue/green Earth-like procedural texture.

### Option 2: Add Real NASA Earth Texture

1. **Download NASA Blue Marble texture:**
   - Visit: https://visibleearth.nasa.gov/collection/1484/blue-marble
   - Or: https://www.solarsystemscope.com/textures/
   - Download any Earth color map (2K or 4K recommended)

2. **Save the texture:**
   ```bash
   # Save downloaded image as:
   cp ~/Downloads/earth_texture.jpg /home/user/ComputerGraphics/source\ code/earth_color.jpg
   ```

3. **Rebuild and run:**
   ```bash
   cd /home/user/ComputerGraphics/build
   cmake ..
   make -j4
   ./lab4_main
   ```

## Direct Download Examples

Try these direct download commands (some may require browser download):

```bash
cd /home/user/ComputerGraphics/source\ code/

# Option 1: NASA Earth Observatory (if accessible)
wget -O earth_color.jpg "https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73909/world.topo.bathy.200412.3x5400x2700.jpg"

# Option 2: Use any Earth texture from the web
# Just save it as: earth_color.jpg
```

## Lighting Parameters

The application uses **Phong lighting** with:
- **Ambient**: 0.2 (20% base illumination)
- **Diffuse**: 0.8 (80% directional lighting)
- **Specular**: 0.3 (30% shininess)
- **Shininess**: 32.0 (smooth surface)
- **Light Position**: (10, 10, 10) in world space
- **Light Color**: White (1.0, 1.0, 1.0)

## Technical Details

### Sphere Generation
- Radius: 2.0 units
- Rings: 64
- Sectors: 64
- Total vertices: ~4,096
- Indexed rendering with element buffers

### Shaders
- **Vertex Shader**: `simpleVertexShader.txt`
  - World space lighting calculations
  - Texture coordinate passing
- **Fragment Shader**: `simpleFragmentShader.txt`
  - Phong lighting (ambient + diffuse + specular)
  - Texture sampling
  - Per-pixel lighting

### Controls
- **Close window**: Exit application
- Earth rotates automatically at 15°/second

## File Structure

```
ComputerGraphics/
├── source code/
│   ├── main.cpp                      # Main application
│   ├── maths_funcs.h/.cpp           # Math library
│   ├── sphere_generator.h/.cpp      # UV sphere generation
│   ├── stb_image.h                  # Texture loading
│   ├── simpleVertexShader.txt       # Vertex shader
│   ├── simpleFragmentShader.txt     # Fragment shader
│   └── earth_color.jpg              # [Optional] Earth texture
├── build/
│   └── lab4_main                    # Compiled executable
└── CMakeLists.txt                   # Build configuration
```

## Troubleshooting

### "Failed to load texture" message
- The app will automatically use a procedural fallback texture
- Download a real Earth texture and place it as `source code/earth_color.jpg`

### Window doesn't open
- Ensure you have a display server running (X11 or Wayland)
- On headless systems, use Xvfb or similar virtual display

### Build errors
- Make sure OpenGL development libraries are installed:
  ```bash
  apt-get install libgl1-mesa-dev libglu1-mesa-dev
  ```

## Credits

- NASA for free Earth textures (Public Domain)
- GLM for mathematics library
- GLFW for window management
- GLAD for OpenGL loading
- STB for image loading
