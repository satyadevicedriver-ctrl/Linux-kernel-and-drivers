# Simple Framebuffer Driver

## Overview
A complete framebuffer driver implementation demonstrating the Linux framebuffer subsystem with DMA memory management, color modes, and hardware acceleration stubs.

## Architecture
```
┌─────────────────┐
│  User Space     │
│  Application    │
└────────┬────────┘
         │ /dev/fb0
         │ mmap(), ioctl()
┌────────▼────────┐
│ Framebuffer     │
│ Subsystem       │
│ (fbmem.c)       │
└────────┬────────┘
         │ fb_ops
┌────────▼────────┐
│ Simple FB       │
│ Driver          │
└────────┬────────┘
         │
┌────────▼────────┐
│ DMA Coherent    │
│ Memory          │
│ (Physical RAM)  │
└─────────────────┘
```

## Features

### Core Functionality
- **Resolution**: 800x600 (configurable)
- **Color Depths**: 16-bit (RGB565), 24-bit (RGB888), 32-bit (ARGB8888)
- **DMA Memory**: Coherent memory allocation for framebuffer
- **mmap Support**: Direct memory mapping to userspace

### Framebuffer Operations
1. **check_var**: Validate resolution and color format
2. **set_par**: Configure hardware parameters
3. **setcolreg**: Set color palette/pseudo-palette
4. **blank**: Screen power management
5. **pan_display**: Virtual screen scrolling
6. **fillrect**: Rectangle filling (software fallback)
7. **copyarea**: Area copying (software fallback)
8. **imageblit**: Image blitting (software fallback)
9. **mmap**: Memory mapping
10. **ioctl**: Custom operations

### Test Application Features
- Color bars pattern
- Gradient rendering
- Geometric shapes (rectangles, circles, lines)
- Animation demo
- Direct pixel manipulation

## Build & Test
```bash
# Build driver and test app
make

# Load driver
sudo insmod simple_fb.ko

# Check which framebuffer device was created
ls -l /dev/fb*
dmesg | tail -20

# If simple_fb is /dev/fb1, update test app to use it
# Edit test_fb.c: #define FB_DEVICE "/dev/fb1"

# Run tests
sudo ./test_fb 0    # All tests
sudo ./test_fb 1    # Color bars only
sudo ./test_fb 2    # Gradient only
sudo ./test_fb 3    # Shapes only
sudo ./test_fb 4    # Animation only

# Unload driver
sudo rmmod simple_fb
```

## Color Formats

### RGB565 (16-bit)
```
Bit: 15-11  10-5   4-0
     [RED] [GREEN][BLUE]
      5bit  6bit   5bit
```

### RGB888 (24-bit)
```
Byte:  2      1      0
     [RED] [GREEN][BLUE]
      8bit  8bit   8bit
```

### ARGB8888 (32-bit)
```
Byte:   3      2      1      0
     [ALPHA][RED] [GREEN][BLUE]
       8bit  8bit  8bit   8bit
```

## Key Data Structures

### fb_info
Main framebuffer information structure containing:
- `fbops`: Operations callbacks
- `var`: Variable screen info
- `fix`: Fixed screen info
- `pseudo_palette`: Color lookup
- `screen_base`: Framebuffer virtual address

### fb_var_screeninfo
Variable screen parameters:
- Resolution (xres, yres)
- Virtual resolution
- Bits per pixel
- RGB bit fields

### fb_fix_screeninfo
Fixed parameters:
- Physical address (smem_start)
- Buffer size (smem_len)
- Line length
- Visual type

## Memory Management

### DMA Coherent Allocation
```c
void *virt = dma_alloc_coherent(dev, size, &phys, GFP_KERNEL);
```
**Benefits:**
- Cache-coherent memory
- Suitable for hardware DMA
- No cache management needed

### User Space Mapping
```c
// Kernel
remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT, size, prot);

// User space
buffer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
```

## Performance Considerations

### Write-Combining
```c
vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
```
- Optimizes sequential writes
- Critical for framebuffer performance
- Reduces memory bus transactions

### Hardware Acceleration
Current implementation uses software fallbacks:
- `sys_fillrect()` - Software rectangle fill
- `sys_copyarea()` - Software copy
- `sys_imageblit()` - Software blit

**For Real Hardware:**
Replace with DMA/GPU-accelerated versions.

## Integration with Display Pipeline

### Typical Display Stack
```
Application
    ↓
Framebuffer Driver (this)
    ↓
Display Controller (DPU/LCD Controller)
    ↓
Panel Driver (MIPI DSI/LVDS/eDP)
    ↓
Display Panel
```

### Required for Production
1. **Display Controller Driver**: Configure timing, format
2. **Panel Driver**: Power on, initialization sequence
3. **Backlight Driver**: PWM control
4. **Clock/Power**: Enable display clocks, power domains

## Debugging

### Enable Debug Output
```bash
# Kernel debug messages
echo 8 > /proc/sys/kernel/printk

# Check framebuffer registration
cat /sys/class/graphics/fb0/name
cat /sys/class/graphics/fb0/mode
cat /sys/class/graphics/fb0/bits_per_pixel
```

### Common Issues

**Problem**: Device not created
```bash
# Check if platform device registered
ls /sys/devices/platform/simple_fb
```

**Problem**: Screen stays black
```bash
# Verify memory write
sudo dd if=/dev/urandom of=/dev/fb0 bs=800x600x4 count=1
```

**Problem**: Wrong colors
```bash
# Check RGB bit field configuration in fb_var_screeninfo
fbset -i
```

## Real-World Examples

### Qualcomm DPU
- Multi-layer composition
- Hardware rotation
- Color space conversion
- UBWC compression

### Samsung DECON
- Dual display support
- HDR processing
- AFBC compression

### NVIDIA Tegra
- NVDISPLAY controller
- DSI/eDP interfaces
- Hardware cursor

## Interview Questions Prep

**Q: Difference between framebuffer and DRM?**
- **Framebuffer**: Simple, single-user, legacy
- **DRM/KMS**: Modern, multi-user, atomic modesetting, buffer management

**Q: Why use DMA coherent memory?**
- Cache coherency without manual management
- Hardware can access directly
- No cache flush/invalidate needed

**Q: How to optimize framebuffer performance?**
- Write-combining
- Hardware acceleration
- DMA for large transfers
- Reduced memory copies

**Q: Explain double buffering**
```c
// Allocate 2x framebuffer size
yres_virtual = yres * 2;

// Pan between buffers
var.yoffset = (current_buffer == 0) ? yres : 0;
ioctl(fd, FBIOPAN_DISPLAY, &var);
```

## Extensions

### Add Double Buffering
```c
simple_fb_var.yres_virtual = FB_HEIGHT * 2;
```

### Add VSYNC Wait
```c
static int simple_fb_wait_for_vsync(struct fb_info *info) {
    // Wait for VSYNC interrupt
    return 0;
}
```

### Add Rotation Support
```c
static int simple_fb_set_rotate(struct fb_info *info, int angle) {
    // Configure hardware rotation
    return 0;
}
```

## Learning Points

1. **Framebuffer Subsystem**: Registration, operations
2. **DMA Management**: Coherent allocation, mapping
3. **Memory Mapping**: remap_pfn_range, cache attributes
4. **Color Formats**: RGB565, RGB888, ARGB8888
5. **Display Pipeline**: FB → Display Controller → Panel
6. **Performance**: Write-combining, hardware acceleration
7. **User-Kernel Interface**: ioctl, mmap
8. **Platform Driver Model**: probe/remove lifecycle

## References
- `Documentation/fb/framebuffer.rst`
- `drivers/video/fbdev/`
- `include/linux/fb.h`
- `include/uapi/linux/fb.h`

## License
GPL v2