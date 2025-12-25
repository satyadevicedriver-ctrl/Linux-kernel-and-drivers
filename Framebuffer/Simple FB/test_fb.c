// test_fb.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <string.h>
#include <stdint.h>

#define FB_DEVICE "/dev/fb0"

struct framebuffer {
    int fd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    uint32_t *buffer;
    size_t buffer_size;
};

int fb_init(struct framebuffer *fb) {
    // Open framebuffer device
    fb->fd = open(FB_DEVICE, O_RDWR);
    if (fb->fd < 0) {
        perror("Error opening framebuffer device");
        return -1;
    }
    
    // Get variable screen info
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo) < 0) {
        perror("Error reading variable screen info");
        close(fb->fd);
        return -1;
    }
    
    // Get fixed screen info
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->finfo) < 0) {
        perror("Error reading fixed screen info");
        close(fb->fd);
        return -1;
    }
    
    printf("Framebuffer Info:\n");
    printf("  Resolution: %dx%d\n", fb->vinfo.xres, fb->vinfo.yres);
    printf("  Bits per pixel: %d\n", fb->vinfo.bits_per_pixel);
    printf("  Line length: %d bytes\n", fb->finfo.line_length);
    printf("  Buffer size: %d bytes\n", fb->finfo.smem_len);
    
    // Calculate buffer size
    fb->buffer_size = fb->vinfo.xres * fb->vinfo.yres * 
                     (fb->vinfo.bits_per_pixel / 8);
    
    // Map framebuffer to user space
    fb->buffer = mmap(0, fb->buffer_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fb->fd, 0);
    if (fb->buffer == MAP_FAILED) {
        perror("Error mapping framebuffer");
        close(fb->fd);
        return -1;
    }
    
    printf("Framebuffer mapped successfully\n");
    return 0;
}

void fb_cleanup(struct framebuffer *fb) {
    munmap(fb->buffer, fb->buffer_size);
    close(fb->fd);
}

// Draw a pixel
void draw_pixel(struct framebuffer *fb, int x, int y, uint32_t color) {
    if (x >= 0 && x < fb->vinfo.xres && y >= 0 && y < fb->vinfo.yres) {
        fb->buffer[y * fb->vinfo.xres + x] = color;
    }
}

// Fill rectangle
void fill_rect(struct framebuffer *fb, int x, int y, int width, int height,
               uint32_t color) {
    int i, j;
    for (j = y; j < y + height && j < fb->vinfo.yres; j++) {
        for (i = x; i < x + width && i < fb->vinfo.xres; i++) {
            draw_pixel(fb, i, j, color);
        }
    }
}

// Clear screen
void clear_screen(struct framebuffer *fb, uint32_t color) {
    int i;
    for (i = 0; i < fb->vinfo.xres * fb->vinfo.yres; i++) {
        fb->buffer[i] = color;
    }
}

// Draw line (Bresenham's algorithm)
void draw_line(struct framebuffer *fb, int x0, int y0, int x1, int y1,
               uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        draw_pixel(fb, x0, y0, color);
        
        if (x0 == x1 && y0 == y1)
            break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw circle
void draw_circle(struct framebuffer *fb, int cx, int cy, int radius,
                uint32_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    while (x <= y) {
        draw_pixel(fb, cx + x, cy + y, color);
        draw_pixel(fb, cx - x, cy + y, color);
        draw_pixel(fb, cx + x, cy - y, color);
        draw_pixel(fb, cx - x, cy - y, color);
        draw_pixel(fb, cx + y, cy + x, color);
        draw_pixel(fb, cx - y, cy + x, color);
        draw_pixel(fb, cx + y, cy - x, color);
        draw_pixel(fb, cx - y, cy - x, color);
        
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

// Test patterns
void test_color_bars(struct framebuffer *fb) {
    int bar_width = fb->vinfo.xres / 8;
    uint32_t colors[] = {
        0xFFFFFFFF, // White
        0xFFFFFF00, // Yellow
        0xFF00FFFF, // Cyan
        0xFF00FF00, // Green
        0xFFFF00FF, // Magenta
        0xFFFF0000, // Red
        0xFF0000FF, // Blue
        0xFF000000  // Black
    };
    
    printf("Drawing color bars...\n");
    
    for (int i = 0; i < 8; i++) {
        fill_rect(fb, i * bar_width, 0, bar_width, fb->vinfo.yres, colors[i]);
    }
}

void test_gradient(struct framebuffer *fb) {
    printf("Drawing gradient...\n");
    
    for (int y = 0; y < fb->vinfo.yres; y++) {
        for (int x = 0; x < fb->vinfo.xres; x++) {
            uint8_t r = (x * 255) / fb->vinfo.xres;
            uint8_t g = (y * 255) / fb->vinfo.yres;
            uint8_t b = 128;
            uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
            draw_pixel(fb, x, y, color);
        }
    }
}

void test_shapes(struct framebuffer *fb) {
    printf("Drawing shapes...\n");
    
    clear_screen(fb, 0xFF000000); // Black background
    
    // Red rectangle
    fill_rect(fb, 50, 50, 200, 150, 0xFFFF0000);
    
    // Green rectangle
    fill_rect(fb, 300, 100, 150, 200, 0xFF00FF00);
    
    // Blue circle
    draw_circle(fb, 400, 300, 80, 0xFF0000FF);
    
    // Yellow lines
    draw_line(fb, 0, 0, fb->vinfo.xres - 1, fb->vinfo.yres - 1, 0xFFFFFF00);
    draw_line(fb, 0, fb->vinfo.yres - 1, fb->vinfo.xres - 1, 0, 0xFFFFFF00);
    
    // White border
    draw_line(fb, 0, 0, fb->vinfo.xres - 1, 0, 0xFFFFFFFF);
    draw_line(fb, fb->vinfo.xres - 1, 0, fb->vinfo.xres - 1,
             fb->vinfo.yres - 1, 0xFFFFFFFF);
    draw_line(fb, fb->vinfo.xres - 1, fb->vinfo.yres - 1, 0,
             fb->vinfo.yres - 1, 0xFFFFFFFF);
    draw_line(fb, 0, fb->vinfo.yres - 1, 0, 0, 0xFFFFFFFF);
}

void test_animation(struct framebuffer *fb) {
    printf("Drawing animation (10 seconds)...\n");
    
    int cx = fb->vinfo.xres / 2;
    int cy = fb->vinfo.yres / 2;
    int max_radius = (fb->vinfo.xres < fb->vinfo.yres) ? 
                     fb->vinfo.xres / 2 - 20 : fb->vinfo.yres / 2 - 20;
    
    for (int frame = 0; frame < 100; frame++) {
        clear_screen(fb, 0xFF000000);
        
        int radius = (frame * max_radius) / 100;
        uint8_t color_val = (frame * 255) / 100;
        uint32_t color = 0xFF000000 | (color_val << 16) | 
                        ((255 - color_val) << 8) | 128;
        
        draw_circle(fb, cx, cy, radius, color);
        
        usleep(100000); // 100ms
    }
}

int main(int argc, char *argv[]) {
    struct framebuffer fb;
    int test_num = 0;
    
    if (argc > 1) {
        test_num = atoi(argv[1]);
    }
    
    printf("Simple Framebuffer Test Application\n");
    printf("===================================\n\n");
    
    if (fb_init(&fb) < 0) {
        return 1;
    }
    
    switch (test_num) {
    case 0:
        printf("Running all tests...\n");
        test_color_bars(&fb);
        sleep(3);
        test_gradient(&fb);
        sleep(3);
        test_shapes(&fb);
        sleep(3);
        test_animation(&fb);
        break;
        
    case 1:
        test_color_bars(&fb);
        break;
        
    case 2:
        test_gradient(&fb);
        break;
        
    case 3:
        test_shapes(&fb);
        break;
        
    case 4:
        test_animation(&fb);
        break;
        
    default:
        printf("Unknown test number\n");
        printf("Usage: %s [test_number]\n", argv[0]);
        printf("  0 - All tests (default)\n");
        printf("  1 - Color bars\n");
        printf("  2 - Gradient\n");
        printf("  3 - Shapes\n");
        printf("  4 - Animation\n");
        break;
    }
    
    fb_cleanup(&fb);
    printf("\nTest complete\n");
    
    return 0;
}