# Assimp Setup for Orbital Ring

## Quick Setup

To enable the orbital ring feature, you need to install Assimp:

```bash
cd external/assimp
git clone --depth 1 --branch v5.3.1 https://github.com/assimp/assimp.git .
cd ../..
```

Then rebuild the project:

```bash
cd build
cmake ..
cmake --build . --config Release
```

## What This Enables

Once Assimp is installed, you can load 3D models with PBR textures for the orbital ring.

### Required Files

Place your model and textures in: `source code/orbital_ring/`

```
source code/orbital_ring/
├── model.obj          # Your 3D model (.obj or .gltf)
├── albedo.png         # Base color texture
├── roughness.png      # Surface roughness map
├── metallic.png       # Metallic map
├── ao.png            # Ambient occlusion
├── normal.png        # Normal map for surface detail
└── emissive.png      # Emissive/glow map
```

All textures support both .png and .jpg formats.

## Features

- Full PBR (Physically Based Rendering) with Cook-Torrance BRDF
- Normal mapping for surface detail
- Metallic-roughness workflow
- ACES tone mapping
- Ring positioned at 3.5× Earth scale with 15° tilt
- Independent rotation at 5°/second

## Troubleshooting

If the orbital ring doesn't appear:
1. Check that Assimp was found during CMake configuration
2. Verify model files exist in `source code/orbital_ring/`
3. Check console output for loading errors
4. Ensure textures are in .png or .jpg format
