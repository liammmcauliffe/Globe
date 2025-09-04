// ASCII Globe - renders a spinning earth in terminal

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// structs
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

// screen dimensions
int WIDTH = 800;
int HEIGHT = 200;

// sphere numbers
float SPHERE_RADIUS = 1.0;
float PI = 3.14159265359; 
float CHAR_ASPECT_RATIO = 2.0;

// animation
int TARGET_FPS = 120;
float ROTATION_SPEED = 0.04;

// global vars
Pixel *screen_buffer;
float rotation_angle = 0;
int frame_count = 0;
Texture earth_texture;

// terrain detection constants (tuned to my terminal and my image xD)
float DEEP_OCEAN = 30.0;
float OCEAN = 100.0;
float WATER_BLUE = 0.4;
float WATER_BLUE_MUL = 1.5;
float COASTAL_BLUE = 0.25;
float COASTAL_MAX = 160.0;
float COASTAL_BAL = 0.15;

float DESERT = 140.0;
float DESERT_R = 0.1;
float DESERT_G = 0.28;
float DESERT_B = 0.3;

float FOREST_G = 0.38;
float FOREST_G_MUL = 1.2;

float GRASS_G = 1.0;
float GRASS_G_MUL = 1.2;
float GRASS_R_MUL = 1.1;

float MOUNT_R = 0.30;
float MOUNT_G = 0.1;
float MOUNT_B = 0.50;
float MOUNT_MIN = 80.0;
float MOUNT_MAX = 180.0;

float SNOW_BRIGHT = 140.0;
float SNOW_DARK = 130.0;
float SNOW_BRIGHT_RATIO = 0.30;
float SNOW_DARK_RATIO = 0.31;
float SNOW_TOL = 0.10;

// colors for terminal
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

// utility functions
float clamp_float(float value, float min, float max)
{
    return (value < min) ? min : (value > max) ? max : value;
}

int clamp_int(int value, int min, int max)
{
    return (value < min) ? min : (value > max) ? max : value;
}

float calculate_intensity(Color color)
{
    return (color.r + color.g + color.b) / 3.0;
}

bool colors_are_balanced(float r_ratio, float g_ratio, float b_ratio, float tolerance)
{
    return fabsf(r_ratio - g_ratio) < tolerance && fabsf(r_ratio - b_ratio) < tolerance;
}

Vec3 apply_rotation(Vec3 pos, float angle)
{
    float ca = cos(-angle);
    float sa = sin(-angle);

    return (Vec3){
        .x = pos.x * ca - pos.z * sa,
        .y = -pos.y,
        .z = pos.x * sa + pos.z * ca
    };
}

// screen buffer
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

// ray sphere intersection
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

// texture loading
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

    Color color = {
        earth_texture.data[index],
        earth_texture.data[index + 1],
        earth_texture.data[index + 2]
    };

    return color;
}

Color get_default_color(int tex_u, int tex_v)
{
    Color color;

    if (tex_v < earth_texture.height / 3)
    {
        color = (Color){0, 100, 200};  // blue for water
    }
    else if (tex_v < earth_texture.height * 2 / 3)
    {
        color = (Color){0, 150, 0};    // green for land
    }
    else
    {
        color = (Color){200, 200, 200}; // white for ice
    }

    return color;
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

// terrain classification
bool is_snow(Color color)
{
    float intensity = calculate_intensity(color);
    
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    bool colors_balanced = colors_are_balanced(r_ratio, g_ratio, b_ratio, SNOW_TOL);

    bool is_bright_snow = intensity > SNOW_BRIGHT &&
                          r_ratio > SNOW_BRIGHT_RATIO &&
                          g_ratio > SNOW_BRIGHT_RATIO &&
                          b_ratio > SNOW_BRIGHT_RATIO &&
                          colors_balanced;

    bool is_dark_snow = intensity > SNOW_DARK &&
                        r_ratio > SNOW_DARK_RATIO &&
                        g_ratio > SNOW_DARK_RATIO &&
                        b_ratio > SNOW_DARK_RATIO &&
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

    bool is_deep_water = b_ratio > WATER_BLUE &&
                         b_ratio > r_ratio * WATER_BLUE_MUL &&
                         b_ratio > g_ratio * WATER_BLUE_MUL;

    bool is_coastal = b_ratio > COASTAL_BLUE &&
                      b_ratio > r_ratio &&
                      b_ratio > g_ratio &&
                      intensity < COASTAL_MAX &&
                      colors_are_balanced(r_ratio, g_ratio, b_ratio, COASTAL_BAL);

    return is_deep_water || is_coastal;
}

bool is_desert(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return intensity > DESERT &&
           r_ratio > DESERT_R &&
           g_ratio > DESERT_G &&
           b_ratio < DESERT_B;
}

bool is_forest(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float g_ratio = color.g / total;
    float r_ratio = color.r / total;
    float b_ratio = color.b / total;

    return g_ratio > FOREST_G &&
           g_ratio > r_ratio * FOREST_G_MUL &&
           g_ratio > b_ratio * FOREST_G_MUL &&
           !is_snow(color);
}

bool is_grassland(Color color)
{
    float total = color.r + color.g + color.b + 1;
    float g_ratio = color.g / total;
    float r_ratio = color.r / total;
    float b_ratio = color.b / total;

    return g_ratio > GRASS_G &&
           g_ratio > b_ratio * GRASS_G_MUL &&
           g_ratio > r_ratio * GRASS_R_MUL &&
           !is_snow(color);
}

bool is_mountain(Color color)
{
    float intensity = calculate_intensity(color);
    float total = color.r + color.g + color.b + 1;
    float r_ratio = color.r / total;
    float g_ratio = color.g / total;
    float b_ratio = color.b / total;

    return r_ratio > MOUNT_R &&
           r_ratio > g_ratio * 1.1 &&
           r_ratio > b_ratio * 1.2 &&
           g_ratio > MOUNT_G &&
           b_ratio < MOUNT_B &&
           intensity > MOUNT_MIN &&
           intensity < MOUNT_MAX &&
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
        if (intensity < DEEP_OCEAN || b_ratio > WATER_BLUE * 1.2)
        {
            pixel.color = deep_ocean;
        }
        else if (intensity < OCEAN)
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

// rendering functions
void render_sphere()
{
    clear_screen_buffer();

    for (int screen_y = 0; screen_y < HEIGHT; screen_y++)
    {
        for (int screen_x = 0; screen_x < WIDTH; screen_x++)
        {
            Vec3 hit_point;

            if (ray_sphere_intersection(screen_x, screen_y, &hit_point))
            {
                Pixel pixel = {'#', NULL};
                
                int display_y = HEIGHT - 1 - screen_y;
                set_pixel(screen_x, display_y, pixel);
            }
        }
    }
}

void render_rotating_sphere()
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
                
                Pixel pixel = {'#', NULL};
                
                int display_y = HEIGHT - 1 - screen_y;
                set_pixel(screen_x, display_y, pixel);
            }
        }
    }
}

void render_textured_sphere()
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

                float intensity = (color.r + color.g + color.b) / 3.0;
                char character = (intensity > 200) ? '#' :
                                 (intensity > 150) ? '*' :
                                 (intensity > 100) ? '+' :
                                 (intensity > 50) ? '.' : ' ';
                
                Pixel pixel = {character, NULL};
                
                int display_y = HEIGHT - 1 - screen_y;
                set_pixel(screen_x, display_y, pixel);
            }
        }
    }
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

// display functions
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

            if (pixel.character)
            {
                printf("%c", pixel.character);
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
    }
}

void display_screen_colored()
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
        
        display_screen_colored();

        rotation_angle += ROTATION_SPEED;
        frame_count++;

        usleep(1000000 / TARGET_FPS);
    }

    printf("\033[?25h%s", reset);
    free(screen_buffer);

    return 0;
}