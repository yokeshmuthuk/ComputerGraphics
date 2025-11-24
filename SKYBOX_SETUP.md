# Galaxy Skybox Setup Guide

Your Earth application now supports **real skybox images** with automatic fallback to procedural generation!

## Quick Setup (3 Steps)

### 1. Create the skybox directory
```bash
mkdir -p "/home/user/ComputerGraphics/source code/skybox"
```

### 2. Download skybox images

You need **6 images** (one for each cube face). Download from any of these sources:

#### Option A: Free Skybox Resources
1. **OpenGameArt.org** - Search "space skybox" or "galaxy cubemap"
   - https://opengameart.org/
   - Look for "Space Skybox" or "Milky Way Cubemap"

2. **Poly Haven** (High quality, free)
   - https://polyhaven.com/hdris
   - Search for "space" or "nebula"
   - Download and convert to 6 separate images (see conversion guide below)

3. **LearnOpenGL Resources**
   - https://learnopengl.com/Advanced-OpenGL/Cubemaps
   - Example skybox textures available for testing

4. **Spacescape** (Generate your own!)
   - Free tool to create custom space skyboxes
   - http://alexcpeterson.com/spacescape/
   - Export as 6 separate faces

#### Option B: Quick Download Example
Try these pre-made space skybox sets:
- Search Google Images: "space skybox cubemap 6 faces"
- Look for sets labeled: right, left, top, bottom, front, back

### 3. Place the images with exact names

The images **MUST** be named exactly as shown below:

```
/home/user/ComputerGraphics/source code/skybox/
├── right.jpg    (or right.png)  ← +X face
├── left.jpg     (or left.png)   ← -X face
├── top.jpg      (or top.png)    ← +Y face
├── bottom.jpg   (or bottom.png) ← -Y face
├── front.jpg    (or front.png)  ← +Z face
└── back.jpg     (or back.png)   ← -Z face
```

**IMPORTANT:** File names are case-sensitive! Use lowercase names exactly as shown.

## Supported Formats
- ✅ JPEG (.jpg)
- ✅ PNG (.png)
- All 6 images must use the same format

## How It Works

The application will:
1. **First**: Try to load skybox images from the `skybox/` folder
2. **If not found**: Automatically use procedural starfield generation (current behavior)

## Testing Your Setup

After placing the images:

```bash
cd /home/user/ComputerGraphics/build
./lab4_main
```

Look for console output:
- ✅ **Success**: `"✓ Loaded skybox face: skybox/right.jpg (1024x1024)"`
- ⚠️ **Fallback**: `"Skybox images not found - using procedural generation"`

## Cubemap Face Orientation

Understanding which image goes where:

```
       [top]
[left] [front] [right] [back]
      [bottom]
```

- **Right** (+X): Looking right from center
- **Left** (-X): Looking left from center
- **Top** (+Y): Looking up from center
- **Bottom** (-Y): Looking down from center
- **Front** (+Z): Looking forward from center
- **Back** (-Z): Looking backward from center

## Converting HDRI to Cubemap

If you download an HDRI (equirectangular) image, convert it to 6 cube faces:

### Using Online Tools:
1. **HDRI to Cubemap** - https://jaxry.github.io/panorama-to-cubemap/
2. Upload your HDRI
3. Set output resolution (1024x1024 recommended)
4. Download all 6 faces
5. Rename them according to the naming scheme above

### Using ImageMagick (Command line):
```bash
# Install imagemagick if needed
sudo apt-get install imagemagick

# Convert HDRI to cubemap (example)
convert panorama.hdr -background black -virtual-pixel background \
  +distort Polar "0,0,512,512 0" cubemap_%d.jpg
```

## Recommended Image Resolution

For best performance/quality balance:
- **1024x1024** pixels per face (recommended)
- **2048x2048** pixels per face (high quality)
- **512x512** pixels per face (low memory)

All 6 faces should have the same resolution.

## Troubleshooting

### Images not loading?
1. Check file names are **exactly**: `right.jpg`, `left.jpg`, etc. (lowercase)
2. Check files are in: `/home/user/ComputerGraphics/source code/skybox/`
3. Verify image format is JPG or PNG
4. Check console output when running the application

### Mixed results (some faces load, others don't)?
- All 6 images must be present
- If any face fails to load, the app falls back to procedural generation
- Check console for specific error messages

### Images look wrong or distorted?
- Make sure you're using cubemap faces (not equirectangular)
- Verify face orientation matches the layout above
- Try swapping left/right or front/back if orientation is wrong

## Example Download Workflow

```bash
# 1. Create directory
mkdir -p "/home/user/ComputerGraphics/source code/skybox"

# 2. Download example skybox (fictional example)
cd "/home/user/ComputerGraphics/source code/skybox"
wget https://example.com/space_skybox/right.jpg
wget https://example.com/space_skybox/left.jpg
wget https://example.com/space_skybox/top.jpg
wget https://example.com/space_skybox/bottom.jpg
wget https://example.com/space_skybox/front.jpg
wget https://example.com/space_skybox/back.jpg

# 3. Verify files
ls -la

# 4. Run the application
cd /home/user/ComputerGraphics/build
./lab4_main
```

## No Images? No Problem!

If you don't want to download images, the application will automatically use the **realistic procedural starfield** with:
- 2000-3000 stars per face
- Varied star colors (white, blue, yellow)
- Natural brightness variation
- Subtle nebula wisps

Just run the application without placing any images and it will work perfectly with the fallback.

## Credits

- STB Image library for texture loading
- OpenGL for cubemap rendering
- Free texture sources: NASA, Poly Haven, OpenGameArt.org
