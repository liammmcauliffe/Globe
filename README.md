# ASCII Globe 🌍

A stunning terminal-based 3D Earth renderer written in C that displays a spinning, textured globe using ASCII characters and terminal colors.

![ASCII Globe Animation](assets/gifs/globe.gif)

## What it does

This program renders a 3D Earth in your terminal that:

- ✨ Spins continuously with smooth animation
- 🗺️ Uses real Earth texture mapping (if you have `earth_map.jpg`)
- 🌍 Classifies terrain types (ocean, forest, desert, mountains, snow) with different colors
- 🎨 Renders using ASCII characters with varying intensity based on terrain
- ⚡ Runs at 120 FPS for smooth animation

### Different Aspect Ratios

| Small (40x10)                                 | Medium (700x150)                                | Large (1600x400)                              |
| --------------------------------------------- | ----------------------------------------------- | --------------------------------------------- |
| ![Small Globe](assets/images/globe-small.png) | ![Medium Globe](assets/images/globe-medium.png) | ![Large Globe](assets/images/globe-large.png) |

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

## 🚀 Quick Start

```bash
# Compile and run
gcc -O3 -o globe src/globe.c -lm
./globe
```

**See it in action:** The globe spins smoothly at 120 FPS with realistic terrain colors!

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

## 📁 Project Structure

```
Globe/
├── src/
│   ├── globe.c          # Main program
│   └── stb_image.h      # Image loading library
├── assets/
│   ├── images/
│   │   ├── earth_map.jpg        # Earth texture (optional)
│   │   ├── globe-small.png      # Small demo screenshot
│   │   ├── globe-medium.png     # Medium demo screenshot
│   │   └── globe-large.png      # Large demo screenshot
│   └── gifs/
│       └── globe.gif            # Animated demo
└── README.md            # This file
```

## 🎯 Perfect for Showcasing

This project is ideal for:

- **Portfolio demos** - Impressive visual output that stands out
- **Technical interviews** - Demonstrates 3D math and graphics skills
- **GitHub profiles** - Eye-catching README with live demos
- **Learning C** - Great example of graphics programming
- **Computer graphics** - Shows ray casting and texture mapping

## Customization

You can modify these constants in the code:

- `WIDTH`, `HEIGHT`: Screen dimensions
- `TARGET_FPS`: Animation speed
- `ROTATION_SPEED`: How fast the Earth spins
- Terrain detection thresholds for different biomes

## 🎬 Demo Gallery

The ASCII Globe renders beautifully at different terminal sizes and aspect ratios. Each screenshot shows the smooth animation and realistic terrain colors.

## Notes

This is a practice project for learning C programming and computer graphics concepts. The terrain classification is tuned for a specific Earth texture image, but the program includes fallback rendering for any texture or no texture at all.

## 🤝 Contributing

Feel free to fork, modify, and submit pull requests! This is a great project for:

- Adding new features (lighting, shadows, different planets)
- Optimizing performance
- Improving the terrain classification
- Adding more demo modes

## 📄 License

MIT License - feel free to use this in your own projects!

---

_Built as a learning exercise in C programming and 3D graphics_ 🚀

**Star this repo if you found it helpful!** ⭐
