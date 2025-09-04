/*
    Author: Liam McAuliffe
    Description: An ASCII Globe that rotates and renders colors from a real earth map.
    Created: September 3, 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> // For usleep

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Image tools

// screen dimensions
#define WIDTH 50
#define HEIGHT 14

/*  BEST SIZES (Have to adjust terminal zoom to get proper ratios)
    1600x400
    1200x300
    700x150
    400x115
    200x50
    100x25
    50x14
*/

// Circle calculations
#define SPHERE_RADIUS 1
#define PI 3.14159265359
#define CHAR_ASPECT_RATIO 2.0 // To account for font widths

// animation
#define TARGET_FPS 120
#define ROTATION_SPEED 0.04

// Colors for terminal - consistent naming with regular and dark variants
#define OCEAN "\033[38;5;19m"
#define OCEAN_DARK "\033[38;5;17m"
#define WATER "\033[38;5;33m"
#define GRASSLAND "\033[38;5;70m"
#define FOREST "\033[38;5;28m"
#define FOREST_DARK "\033[38;5;22m"
#define DESERT "\033[38;5;179m"
#define DESERT_LIGHT "\033[38;5;222m"
#define MOUNTAIN "\033[38;5;94m"
#define SNOW "\033[38;5;15m"
#define RESET "\033[0m"

typedef struct
{
    float x, y, z;
} Vec3;

typedef struct
{
    char character;
    const char *color;
} Pixel;

typedef struct
{
    unsigned char r, g, b;
} Color;

typedef struct
{
    unsigned char *data;
    int width;
    int height;
    int channels;
} Texture;

// complete TerrainProfile struct
typedef struct
{
    const char *name;
    float intensity_min;
    float intensity_max;
    float r_ratio_min;
    float r_ratio_max;
    float g_ratio_min;
    float g_ratio_max;
    float b_ratio_min;
    float b_ratio_max;
    float color_balance_tol;
    const char *color_code;
    const char *dark_color_code;
} TerrainProfile;

// Terrain profiles with consistent naming and values from second version
TerrainProfile terrain_profiles[] = {
    // Snow - bright white areas
    {"Snow", 130.0, 255.0, 0.30, 1.0, 0.30, 1.0, 0.30, 1.0, 0.10, SNOW, NULL},

    // Water - deep ocean (very dark blue)
    {"Ocean", 0.0, 30.0, 0.0, 0.4, 0.0, 0.4, 0.4, 1.0, 0.15, OCEAN_DARK, NULL},

    // Water - regular ocean
    {"Ocean", 30.0, 100.0, 0.0, 0.4, 0.0, 0.4, 0.4, 1.0, 0.15, OCEAN, NULL},

    // Water - shallow/coastal water
    {"Water", 100.0, 160.0, 0.0, 0.4, 0.0, 0.4, 0.25, 1.0, 0.15, WATER, NULL},

    // Desert - light desert for bright areas
    {"Desert", 180.0, 255.0, 0.1, 1.0, 0.28, 1.0, 0.0, 0.3, 1.0, DESERT_LIGHT, NULL},

    // Desert - regular desert
    {"Desert", 140.0, 180.0, 0.1, 1.0, 0.28, 1.0, 0.0, 0.3, 1.0, DESERT, NULL},

    // Forest - dark forest for low intensity
    {"Forest", 0.0, 70.0, 0.0, 1.0, 0.38, 1.0, 0.0, 1.0, 1.0, FOREST_DARK, NULL},

    // Forest - regular forest
    {"Forest", 70.0, 255.0, 0.0, 1.0, 0.38, 1.0, 0.0, 1.0, 1.0, FOREST, NULL},

    // Mountain
    {"Mountain", 80.0, 180.0, 0.30, 1.0, 0.1, 1.0, 0.0, 0.50, 1.0, MOUNTAIN, NULL},

    // Grassland - default fallback
    {"Grassland", 0.0, 255.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0, GRASSLAND, NULL}};

int num_terrain_profiles = sizeof(terrain_profiles) / sizeof(terrain_profiles[0]);

// Initialize states
int frame_count = 0;
float rotation_angle = 0;
Pixel *screen_buffer;
Texture earth_texture;

// utilities
float clamp_float(float value, float min, float max)
{
    return (value < min) ? min : (value > max) ? max
                                               : value;
}

int clamp_int(int value, int min, int max)
{
    return (value < min) ? min : (value > max) ? max
                                               : value;
}

float calculate_intensity(Color color)
{
    return (color.r + color.g + color.b) / 3.0;
}

bool colors_are_balanced(float r_ratio, float g_ratio, float b_ratio, float tolerance)
{
    // Absolute value of single precision floating point number
    return fabsf(r_ratio - g_ratio) < tolerance && fabsf(r_ratio - b_ratio) < tolerance;
}

Vec3 apply_rotation(Vec3 pos, float angle)
{
    float ca = cos(-angle);
    float sa = sin(-angle);

    return (Vec3){
        .x = pos.x * ca - pos.z * sa,
        .y = -pos.y,
        .z = pos.x * sa + pos.z * ca};
}

void clear_screen_buffer()
{
    // sets screen_buffer memory to 0
    memset(screen_buffer, 0, sizeof(Pixel) * WIDTH * HEIGHT);
}

void set_pixel(int x, int y, Pixel pixel)
{
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    {
        int index = y * WIDTH + x;
        screen_buffer[index] = pixel;
    }
}

bool ray_sphere_intersection(float screen_x, float screen_y, Vec3 *hit_point)
{
    float x = -(screen_x - WIDTH / 2.0) / (WIDTH / 2.0) * CHAR_ASPECT_RATIO; // Scale x for font
    float y = (screen_y - HEIGHT / 2.0) / (HEIGHT / 2.0);

    // if the calculated radius doesn't match the sphere radius they don't intersect
    float radius_squared = x * x + y * y;
    if (radius_squared > SPHERE_RADIUS * SPHERE_RADIUS)
    {
        return false;
    }

    float z = sqrt(SPHERE_RADIUS * SPHERE_RADIUS - radius_squared);

    hit_point->x = x;
    hit_point->y = y;
    hit_point->z = z;

    return true;
}

bool load_texture(char *filename)
{
    earth_texture.data = stbi_load(filename, &earth_texture.width, &earth_texture.height, &earth_texture.channels, 0);
    if (earth_texture.data == NULL)
    {
        return false;
    }
    return true;
}

void world_to_texture_coords(Vec3 world_pos, int *tex_u, int *tex_v)
{
    float longitude = atan2(world_pos.z, world_pos.x);

    float latitude = asin(clamp_float(world_pos.y / SPHERE_RADIUS, -1.0, 1.0));

    float tex_u_f = (longitude + PI) / (2.0 * PI);

    float tex_v_f = (latitude + PI / 2.0) / PI;

    tex_u_f = clamp_float(tex_u_f, 0.0, 1.0);
    tex_v_f = clamp_float(tex_v_f, 0.0, 1.0);

    *tex_u = (int)(tex_u_f * (earth_texture.width - 1));
    *tex_v = (int)(tex_v_f * (earth_texture.height - 1));
}

Color sample_texture(int tex_u, int tex_v)
{
    Color color = {0, 0, 0}; // Default black

    if (earth_texture.data == NULL || tex_u < 0 || tex_u >= earth_texture.width ||
        tex_v < 0 || tex_v >= earth_texture.height)
    {
        return color;
    }

    int index = (tex_v * earth_texture.width + tex_u) * earth_texture.channels;

    color.r = earth_texture.data[index];
    if (earth_texture.channels > 1)
        color.g = earth_texture.data[index + 1];
    if (earth_texture.channels > 2)
        color.b = earth_texture.data[index + 2];

    return color;
}

// Terrain detection functions
bool is_snow(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    bool colors_balanced = colors_are_balanced(r_ratio, g_ratio, b_ratio, 0.10);

    bool is_bright_snow = intensity > 140.0 &&
                          r_ratio > 0.30 &&
                          g_ratio > 0.30 &&
                          b_ratio > 0.30 &&
                          colors_balanced;

    bool is_dark_snow = intensity > 130.0 &&
                        r_ratio > 0.31 &&
                        g_ratio > 0.31 &&
                        b_ratio > 0.31 &&
                        colors_balanced;

    return is_bright_snow || is_dark_snow;
}

bool is_water(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float b_ratio = color.b / total;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float intensity = calculate_intensity(color);

    if (intensity < 80.0 && b_ratio > 0.35)
        return true;

    if (intensity >= 80.0 && intensity < 160.0 && b_ratio > 0.30 && b_ratio > r_ratio && b_ratio > g_ratio)
        return true;

    if (intensity >= 160.0 && b_ratio > 0.25 && b_ratio > r_ratio * 0.9)
        return true;

    return false;
}

bool is_desert(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return intensity > 140.0 &&
           r_ratio > 0.1 &&
           g_ratio > 0.28 &&
           b_ratio < 0.3;
}

bool is_forest(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float g_ratio = color.g / total;
    float r_ratio = color.r / total;
    float b_ratio = color.b / total;

    return g_ratio > 0.38 &&
           g_ratio > r_ratio * 1.2 &&
           g_ratio > b_ratio * 1.2 &&
           !is_snow(color);
}

bool is_mountain(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return r_ratio > 0.30 &&
           r_ratio > g_ratio * 1.1 &&
           r_ratio > b_ratio * 1.2 &&
           g_ratio > 0.1 &&
           b_ratio < 0.50 &&
           intensity > 80.0 &&
           intensity < 180.0 &&
           !is_water(color) &&
           !is_snow(color);
}

Pixel classify_terrain(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float b_ratio = color.b / total;

    // Default pixel
    Pixel pixel = {'#', GRASSLAND};

    if (is_snow(color))
    {
        pixel.color = SNOW;
    }
    else if (is_water(color))
    {
        if (intensity < 30.0)
        {
            pixel.color = OCEAN_DARK;
        }
        else if (intensity > 30.0)
        {
            pixel.color = OCEAN;
        }
        else
        {
            pixel.color = WATER;
        }
    }
    else if (is_desert(color))
    {
        pixel.color = (intensity > 180) ? DESERT_LIGHT : DESERT;
    }
    else if (is_forest(color))
    {
        pixel.color = (intensity < 70) ? FOREST_DARK : FOREST;
    }
    else if (is_mountain(color))
    {
        pixel.color = MOUNTAIN;
    }

    return pixel;
}

void render_globe()
{
    clear_screen_buffer();

    for (int screen_y = 0; screen_y < HEIGHT; screen_y++)
    {
        for (int screen_x = 0; screen_x < WIDTH; screen_x++)
        {
            Vec3 hit_point;

            if (ray_sphere_intersection(screen_x, screen_y, &hit_point))
            {
                Vec3 rotated_pos = apply_rotation(hit_point, rotation_angle);

                int tex_u, tex_v;
                world_to_texture_coords(rotated_pos, &tex_u, &tex_v);

                Color color = sample_texture(tex_u, tex_v);

                Pixel pixel = classify_terrain(color);

                int display_y = HEIGHT - 1 - screen_y;
                set_pixel(screen_x, display_y, pixel);
            }
        }
    }
}

void display_screen()
{
    // Reset cursor to top left
    printf("\033[H");

    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            int index = y * WIDTH + x;
            Pixel pixel = screen_buffer[index];

            if (pixel.color)
            {
                printf("%s%c", pixel.color, pixel.character);
            }
            else
            {
                printf(" ");
            }
        }
        printf("%s\n", RESET);
    }
}

int main()
{
    load_texture("assets/images/earth_map.jpg");

    screen_buffer = malloc(sizeof(Pixel) * WIDTH * HEIGHT);

    // Clear terminal and hide cursor
    printf("\033[2J\033[?25l");

    while (true)
    {
        render_globe();

        display_screen();

        rotation_angle += ROTATION_SPEED;
        frame_count++;

        usleep(1000000 / TARGET_FPS);
    }

    // Reset
    printf("\033[?25h%s", RESET);
    free(screen_buffer);

    return 0;
}