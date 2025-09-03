#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int WIDTH = 1600;
int HEIGHT = 400;
float SPHERE_RADIUS = 1.0;
float PI = 3.14159265359;
float CHAR_ASPECT_RATIO = 2.0;
float ROTATION_SPEED = 0.02;
int TARGET_FPS = 60;

float DEEP_OCEAN_INTENSITY = 30.0;
float OCEAN_INTENSITY = 100.0;
float WATER_BLUE_RATIO = 0.4;
float WATER_BLUE_RATIO_MULTIPLIER = 1.5;
float COASTAL_BLUE_RATIO = 0.25;
float COASTAL_INTENSITY_MAX = 160.0;
float COASTAL_COLOR_BALANCE = 0.15;
float DESERT_INTENSITY = 140.0;
float DESERT_RED_RATIO = 0.1;
float DESERT_GREEN_RATIO = 0.28;
float DESERT_BLUE_RATIO = 0.3;
float FOREST_GREEN_RATIO = 0.38;
float FOREST_GREEN_MULTIPLIER = 1.2;
float GRASSLAND_GREEN_RATIO = 1.0;
float GRASSLAND_GREEN_MULTIPLIER = 1.2;
float GRASSLAND_GREEN_RED_MULTIPLIER = 1.1;
float MOUNTAIN_RED_RATIO = 0.30;
float MOUNTAIN_GREEN_RATIO = 0.1;
float MOUNTAIN_BLUE_RATIO = 0.50;
float MOUNTAIN_INTENSITY_MIN = 80.0;
float MOUNTAIN_INTENSITY_MAX = 180.0;
float SNOW_PRIMARY_INTENSITY = 140.0;
float SNOW_SECONDARY_INTENSITY = 130.0;
float SNOW_PRIMARY_COLOR_RATIO = 0.30;
float SNOW_SECONDARY_COLOR_RATIO = 0.31;
float SNOW_COLOR_DIFFERENCE_TOLERANCE = 0.10;

char *deep_ocean = "\033[38;5;17m";
char *ocean = "\033[38;5;19m";
char *shallow_water = "\033[38;5;33m";
char *grassland = "\033[38;5;70m";
char *forest = "\033[38;5;28m";
char *forest_dark = "\033[38;5;22m";
char *desert = "\033[38;5;179m";
char *desert_light = "\033[38;5;222m";
char *mountain = "\033[38;5;94m";
char *snow = "\033[38;5;15m";
char *reset = "\033[0m";

typedef struct
{
    float x, y, z;
} Vec3;

typedef struct
{
    char character;
    char *color;
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
    bool loaded;
} Texture;

Pixel *screen_buffer;
Texture earth_texture;
float rotation_angle = 0;
int frame_count = 0;

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
    return fabsf(r_ratio - g_ratio) < tolerance && fabsf(r_ratio - b_ratio) < tolerance;
}

bool load_texture(char *filename)
{
    earth_texture.data = stbi_load(filename, &earth_texture.width, &earth_texture.height, &earth_texture.channels, 0);
    earth_texture.loaded = (earth_texture.data != NULL);

    if (earth_texture.loaded)
    {
        printf("Texture loaded: %dx%d (channels: %d)\n", earth_texture.width, earth_texture.height, earth_texture.channels);
    }
    else
    {
        printf("Warning: Could not load '%s'. Using default colors.\n", filename);
        earth_texture.width = 100;
        earth_texture.height = 50;
        earth_texture.channels = 3;
    }

    return earth_texture.loaded;
}

Color sample_texture(int u, int v)
{
    Color default_color = {0, 0, 0};

    if (!earth_texture.loaded || !earth_texture.data)
    {
        return default_color;
    }

    u = clamp_int(u % earth_texture.width, 0, earth_texture.width - 1);
    v = clamp_int(v % earth_texture.height, 0, earth_texture.height - 1);

    int index = (v * earth_texture.width + u) * earth_texture.channels;

    Color color = {earth_texture.data[index], earth_texture.data[index + 1], earth_texture.data[index + 2]};

    return color;
}

Color get_default_color(int tex_u, int tex_v)
{
    Color color;

    if (tex_v < earth_texture.height / 3)
    {
        color = (Color){0, 100, 200};
    }
    else if (tex_v < earth_texture.height * 2 / 3)
    {
        color = (Color){0, 150, 0};
    }
    else
    {
        color = (Color){200, 200, 200};
    }

    return color;
}

bool is_snow(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    bool colors_balanced = colors_are_balanced(r_ratio, g_ratio, b_ratio, SNOW_COLOR_DIFFERENCE_TOLERANCE);

    bool is_bright_snow = intensity > SNOW_PRIMARY_INTENSITY &&
                          r_ratio > SNOW_PRIMARY_COLOR_RATIO &&
                          g_ratio > SNOW_PRIMARY_COLOR_RATIO &&
                          b_ratio > SNOW_PRIMARY_COLOR_RATIO &&
                          colors_balanced;

    bool is_dark_snow = intensity > SNOW_SECONDARY_INTENSITY &&
                        r_ratio > SNOW_SECONDARY_COLOR_RATIO &&
                        g_ratio > SNOW_SECONDARY_COLOR_RATIO &&
                        b_ratio > SNOW_SECONDARY_COLOR_RATIO &&
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

    bool is_deep_water = b_ratio > WATER_BLUE_RATIO &&
                         b_ratio > r_ratio * WATER_BLUE_RATIO_MULTIPLIER &&
                         b_ratio > g_ratio * WATER_BLUE_RATIO_MULTIPLIER;

    bool is_coastal = b_ratio > COASTAL_BLUE_RATIO &&
                      b_ratio > r_ratio &&
                      b_ratio > g_ratio &&
                      intensity < COASTAL_INTENSITY_MAX &&
                      colors_are_balanced(r_ratio, g_ratio, b_ratio, COASTAL_COLOR_BALANCE);

    return is_deep_water || is_coastal;
}

bool is_desert(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return intensity > DESERT_INTENSITY &&
           r_ratio > DESERT_RED_RATIO &&
           g_ratio > DESERT_GREEN_RATIO &&
           b_ratio < DESERT_BLUE_RATIO;
}

bool is_forest(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float g_ratio = color.g / total;
    float r_ratio = color.r / total;
    float b_ratio = color.b / total;

    return g_ratio > FOREST_GREEN_RATIO &&
           g_ratio > r_ratio * FOREST_GREEN_MULTIPLIER &&
           g_ratio > b_ratio * FOREST_GREEN_MULTIPLIER &&
           !is_snow(color);
}

bool is_grassland(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float g_ratio = color.g / total;
    float r_ratio = color.r / total;
    float b_ratio = color.b / total;

    return g_ratio > GRASSLAND_GREEN_RATIO &&
           g_ratio > b_ratio * GRASSLAND_GREEN_MULTIPLIER &&
           g_ratio > r_ratio * GRASSLAND_GREEN_RED_MULTIPLIER &&
           !is_snow(color);
}

bool is_mountain(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return r_ratio > MOUNTAIN_RED_RATIO &&
           r_ratio > g_ratio * 1.1 &&
           r_ratio > b_ratio * 1.2 &&
           g_ratio > MOUNTAIN_GREEN_RATIO &&
           b_ratio < MOUNTAIN_BLUE_RATIO &&
           intensity > MOUNTAIN_INTENSITY_MIN &&
           intensity < MOUNTAIN_INTENSITY_MAX &&
           !is_water(color) &&
           !is_snow(color);
}

Pixel classify_terrain(Color color)
{
    Pixel pixel = {'#', grassland};
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float b_ratio = color.b / total;

    if (is_snow(color))
    {
        pixel.color = snow;
    }
    else if (is_water(color))
    {
        if (intensity < DEEP_OCEAN_INTENSITY || b_ratio > WATER_BLUE_RATIO * 1.2)
        {
            pixel.color = deep_ocean;
        }
        else if (intensity < OCEAN_INTENSITY)
        {
            pixel.color = ocean;
        }
        else
        {
            pixel.color = shallow_water;
        }
    }
    else if (is_desert(color))
    {
        pixel.color = (intensity > 180) ? desert_light : desert;
    }
    else if (is_forest(color))
    {
        pixel.color = (intensity < 70) ? forest_dark : forest;
    }
    else if (is_grassland(color))
    {
        pixel.color = grassland;
    }
    else if (is_mountain(color))
    {
        pixel.color = mountain;
    }

    return pixel;
}

void clear_screen_buffer()
{
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

Vec3 apply_rotation(Vec3 pos, float angle)
{
    float ca = cos(-angle);
    float sa = sin(-angle);

    return (Vec3){
        .x = pos.x * ca - pos.z * sa,
        .y = -pos.y,
        .z = pos.x * sa + pos.z * ca};
}

bool ray_sphere_intersection(float screen_x, float screen_y, Vec3 *hit_point)
{
    float x = -(screen_x - WIDTH / 2.0) / (WIDTH / 2.0) * CHAR_ASPECT_RATIO;
    float y = (screen_y - HEIGHT / 2.0) / (HEIGHT / 2.0);

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

void render_earth()
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

                Color color;
                if (earth_texture.loaded)
                {
                    color = sample_texture(tex_u, tex_v);
                }
                else
                {
                    color = get_default_color(tex_u, tex_v);
                }

                Pixel pixel = classify_terrain(color);

                int display_y = HEIGHT - 1 - screen_y;
                set_pixel(screen_x, display_y, pixel);
            }
        }
    }
}

void display_screen()
{
    printf("\033[H");
    printf("ASCII Earth Renderer - Frame %d\n\n", frame_count);

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
        printf("%s\n", reset);
    }
}

int main()
{
    printf("ASCII Earth Renderer - Starting...\n");

    load_texture("earth_map.jpg");

    screen_buffer = malloc(sizeof(Pixel) * WIDTH * HEIGHT);
    if (!screen_buffer)
    {
        printf("Error: Could not allocate screen buffer.\n");
        return 1;
    }

    printf("Screen size: %dx%d\n", WIDTH, HEIGHT);
    printf("Starting animation... Press Ctrl+C to exit.\n\n");
    sleep(1);

    printf("\033[2J\033[?25l");

    while (true)
    {
        render_earth();
        display_screen();

        rotation_angle += ROTATION_SPEED;
        frame_count++;

        usleep(1000000 / TARGET_FPS);
    }

    printf("\033[?25h%s", reset);
    free(screen_buffer);

    return 0;
}