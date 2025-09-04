# ASCII Globe 🌍

A terminal-based 3D Earth renderer written in C that displays a spinning, textured globe using ASCII characters and terminal colors.

## What it does

This program renders a 3D Earth in your terminal that:

- Spins continuously with smooth animation
- Uses real Earth texture mapping (if you have `earth_map.jpg`)
- Classifies terrain types (ocean, forest, desert, mountains, snow) with different colors
- Renders using ASCII characters with varying intensity based on terrain
- Runs at 120 FPS for smooth animation

## Features

- **3D Ray-Sphere Intersection**: Mathematical ray casting to project 3D sphere onto 2D screen
- **Texture Mapping**: Maps 2D Earth texture onto 3D sphere surface
- **Terrain Classification**: Automatically detects and colors different terrain types
- **Real-time Animation**: Smooth rotation with configurable speed
- **Terminal Colors**: Uses ANSI color codes for realistic terrain representation
- **Fallback Rendering**: Works even without texture file using default colors

## Requirements

- C compiler (gcc recommended)
- Math library (`-lm` flag)
- `stb_image.h` library (included)
- Terminal with color support
- Optional: `earth_map.jpg` for realistic Earth texture

## How to compile and run

```bash
gcc -o globe globe.c -lm
./globe
```

## Controls

- **Ctrl+C**: Exit the program

## Technical Details

### What I learned building this:

- **3D Mathematics**: Ray-sphere intersection, 3D rotations, coordinate transformations
- **Computer Graphics**: Texture mapping, screen projection, color space conversion
- **Image Processing**: Color analysis, terrain classification algorithms
- **Terminal Graphics**: ANSI escape codes, screen buffering, real-time rendering
- **C Programming**: Structs, memory management, modular code organization

### Key algorithms:

1. **Ray-Sphere Intersection**: Projects screen coordinates to 3D sphere surface
2. **Texture Coordinate Mapping**: Converts 3D world coordinates to 2D texture UV coordinates
3. **Terrain Classification**: Analyzes RGB values to identify terrain types
4. **Screen Buffering**: Double-buffered rendering for smooth animation

## File Structure

```
Globe/
├── globe.c          # Main program
├── stb_image.h      # Image loading library
├── earth_map.jpg    # Earth texture (optional)
└── README.md        # This file
```

## Customization

You can modify these constants in the code:

- `WIDTH`, `HEIGHT`: Screen dimensions
- `TARGET_FPS`: Animation speed
- `ROTATION_SPEED`: How fast the Earth spins
- Terrain detection thresholds for different biomes

## Notes

This is a practice project for learning C programming and computer graphics concepts. The terrain classification is tuned for a specific Earth texture image, but the program includes fallback rendering for any texture or no texture at all.

---

_Built as a learning exercise in C programming and 3D graphics_ 🚀
