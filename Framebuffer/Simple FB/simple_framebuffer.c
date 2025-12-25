// simple_fb.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define DRIVER_NAME "simple_fb"
#define FB_WIDTH  800
#define FB_HEIGHT 600
#define FB_BPP    32  // Bits per pixel
#define FB_DEPTH  24  // Color depth

struct simple_fb_par {
    u32 pseudo_palette[16];
    struct platform_device *pdev;
    void *fb_virt;       // Virtual address of framebuffer
    dma_addr_t fb_phys;  // Physical address
    size_t fb_size;
};

static struct fb_var_screeninfo simple_fb_var = {
    .xres           = FB_WIDTH,
    .yres           = FB_HEIGHT,
    .xres_virtual   = FB_WIDTH,
    .yres_virtual   = FB_HEIGHT,
    .bits_per_pixel = FB_BPP,
    .red            = { 16, 8, 0 },  // Red: bits 23-16
    .green          = { 8,  8, 0 },  // Green: bits 15-8
    .blue           = { 0,  8, 0 },  // Blue: bits 7-0
    .transp         = { 24, 8, 0 },  // Alpha: bits 31-24
    .activate       = FB_ACTIVATE_NOW,
    .height         = -1,
    .width          = -1,
    .vmode          = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo simple_fb_fix = {
    .id             = "SimpleFB",
    .type           = FB_TYPE_PACKED_PIXELS,
    .visual         = FB_VISUAL_TRUECOLOR,
    .accel          = FB_ACCEL_NONE,
    .line_length    = FB_WIDTH * (FB_BPP / 8),
};

/*
 * Framebuffer operations
 */

static int simple_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    pr_info("%s: Checking var\n", DRIVER_NAME);
    
    // Validate resolution
    if (var->xres > FB_WIDTH || var->yres > FB_HEIGHT) {
        pr_err("Resolution too large: %dx%d (max: %dx%d)\n",
               var->xres, var->yres, FB_WIDTH, FB_HEIGHT);
        return -EINVAL;
    }
    
    // Validate bits per pixel
    if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
        var->bits_per_pixel != 32) {
        pr_err("Invalid bpp: %d (supported: 16, 24, 32)\n",
               var->bits_per_pixel);
        return -EINVAL;
    }
    
    // Adjust RGB fields based on bpp
    switch (var->bits_per_pixel) {
    case 16: // RGB565
        var->red.offset    = 11;
        var->red.length    = 5;
        var->green.offset  = 5;
        var->green.length  = 6;
        var->blue.offset   = 0;
        var->blue.length   = 5;
        var->transp.offset = 0;
        var->transp.length = 0;
        break;
        
    case 24: // RGB888
    case 32: // ARGB8888
        var->red.offset    = 16;
        var->red.length    = 8;
        var->green.offset  = 8;
        var->green.length  = 8;
        var->blue.offset   = 0;
        var->blue.length   = 8;
        var->transp.offset = 24;
        var->transp.length = (var->bits_per_pixel == 32) ? 8 : 0;
        break;
    }
    
    return 0;
}

static int simple_fb_set_par(struct fb_info *info)
{
    pr_info("%s: Setting par\n", DRIVER_NAME);
    
    info->fix.line_length = info->var.xres * (info->var.bits_per_pixel / 8);
    
    return 0;
}

static int simple_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
                               unsigned blue, unsigned transp,
                               struct fb_info *info)
{
    struct simple_fb_par *par = info->par;
    
    if (regno >= 16)
        return -EINVAL;
    
    // Store pseudo palette for 16-color mode
    if (info->var.bits_per_pixel == 16) {
        red   >>= (16 - info->var.red.length);
        green >>= (16 - info->var.green.length);
        blue  >>= (16 - info->var.blue.length);
        
        par->pseudo_palette[regno] = 
            (red   << info->var.red.offset)   |
            (green << info->var.green.offset) |
            (blue  << info->var.blue.offset);
    } else {
        red   >>= 8;
        green >>= 8;
        blue  >>= 8;
        transp >>= 8;
        
        par->pseudo_palette[regno] = 
            (transp << info->var.transp.offset) |
            (red    << info->var.red.offset)    |
            (green  << info->var.green.offset)  |
            (blue   << info->var.blue.offset);
    }
    
    return 0;
}

static int simple_fb_blank(int blank_mode, struct fb_info *info)
{
    pr_info("%s: Blank mode: %d\n", DRIVER_NAME, blank_mode);
    
    switch (blank_mode) {
    case FB_BLANK_UNBLANK:
        pr_info("Screen ON\n");
        break;
    case FB_BLANK_NORMAL:
    case FB_BLANK_VSYNC_SUSPEND:
    case FB_BLANK_HSYNC_SUSPEND:
    case FB_BLANK_POWERDOWN:
        pr_info("Screen OFF\n");
        break;
    }
    
    return 0;
}

// Pan display (scrolling)
static int simple_fb_pan_display(struct fb_var_screeninfo *var,
                                 struct fb_info *info)
{
    if (var->xoffset + info->var.xres > info->var.xres_virtual ||
        var->yoffset + info->var.yres > info->var.yres_virtual)
        return -EINVAL;
    
    pr_info("Pan display: xoffset=%d, yoffset=%d\n",
            var->xoffset, var->yoffset);
    
    return 0;
}

// Fill rectangle (hardware acceleration stub)
static void simple_fb_fillrect(struct fb_info *info,
                               const struct fb_fillrect *rect)
{
    pr_debug("fillrect: x=%d, y=%d, width=%d, height=%d, color=0x%x\n",
             rect->dx, rect->dy, rect->width, rect->height, rect->color);
    
    // Use software fallback
    sys_fillrect(info, rect);
}

// Copy area (hardware acceleration stub)
static void simple_fb_copyarea(struct fb_info *info,
                               const struct fb_copyarea *area)
{
    pr_debug("copyarea: sx=%d, sy=%d, dx=%d, dy=%d, width=%d, height=%d\n",
             area->sx, area->sy, area->dx, area->dy,
             area->width, area->height);
    
    // Use software fallback
    sys_copyarea(info, area);
}

// Image blit (hardware acceleration stub)
static void simple_fb_imageblit(struct fb_info *info,
                                const struct fb_image *image)
{
    pr_debug("imageblit: x=%d, y=%d, width=%d, height=%d\n",
             image->dx, image->dy, image->width, image->height);
    
    // Use software fallback
    sys_imageblit(info, image);
}

static int simple_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    struct simple_fb_par *par = info->par;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    
    if (offset + size > par->fb_size)
        return -EINVAL;
    
    pr_info("mmap: offset=0x%lx, size=0x%lx\n", offset, size);
    
    // Map DMA coherent memory
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    
    if (remap_pfn_range(vma, vma->vm_start,
                       par->fb_phys >> PAGE_SHIFT,
                       size, vma->vm_page_prot))
        return -EAGAIN;
    
    return 0;
}

// Custom IOCTL for driver-specific operations
static int simple_fb_ioctl(struct fb_info *info, unsigned int cmd,
                          unsigned long arg)
{
    struct simple_fb_par *par = info->par;
    
    switch (cmd) {
    case 0x4600: // Custom: Get physical address
        if (copy_to_user((void __user *)arg, &par->fb_phys,
                        sizeof(par->fb_phys)))
            return -EFAULT;
        return 0;
        
    case 0x4601: // Custom: Clear screen
        memset(par->fb_virt, 0, par->fb_size);
        pr_info("Screen cleared\n");
        return 0;
        
    default:
        pr_warn("Unknown ioctl: 0x%x\n", cmd);
        return -ENOTTY;
    }
}

static struct fb_ops simple_fb_ops = {
    .owner          = THIS_MODULE,
    .fb_check_var   = simple_fb_check_var,
    .fb_set_par     = simple_fb_set_par,
    .fb_setcolreg   = simple_fb_setcolreg,
    .fb_blank       = simple_fb_blank,
    .fb_pan_display = simple_fb_pan_display,
    .fb_fillrect    = simple_fb_fillrect,
    .fb_copyarea    = simple_fb_copyarea,
    .fb_imageblit   = simple_fb_imageblit,
    .fb_mmap        = simple_fb_mmap,
    .fb_ioctl       = simple_fb_ioctl,
};

/*
 * Platform driver probe/remove
 */

static int simple_fb_probe(struct platform_device *pdev)
{
    struct fb_info *info;
    struct simple_fb_par *par;
    int ret;
    
    pr_info("%s: Probing framebuffer driver\n", DRIVER_NAME);
    
    // Allocate framebuffer info structure
    info = framebuffer_alloc(sizeof(struct simple_fb_par), &pdev->dev);
    if (!info) {
        dev_err(&pdev->dev, "Failed to allocate fb_info\n");
        return -ENOMEM;
    }
    
    par = info->par;
    par->pdev = pdev;
    platform_set_drvdata(pdev, info);
    
    // Calculate framebuffer size
    par->fb_size = FB_WIDTH * FB_HEIGHT * (FB_BPP / 8);
    par->fb_size = PAGE_ALIGN(par->fb_size);
    
    // Allocate DMA coherent memory for framebuffer
    par->fb_virt = dma_alloc_coherent(&pdev->dev, par->fb_size,
                                      &par->fb_phys, GFP_KERNEL);
    if (!par->fb_virt) {
        dev_err(&pdev->dev, "Failed to allocate framebuffer memory\n");
        ret = -ENOMEM;
        goto err_fb_release;
    }
    
    // Clear framebuffer
    memset(par->fb_virt, 0, par->fb_size);
    
    dev_info(&pdev->dev, "Framebuffer: virt=%p, phys=0x%pad, size=0x%zx\n",
             par->fb_virt, &par->fb_phys, par->fb_size);
    
    // Setup fb_info
    info->fbops = &simple_fb_ops;
    info->flags = FBINFO_DEFAULT | FBINFO_HWACCEL_DISABLED;
    info->pseudo_palette = par->pseudo_palette;
    info->var = simple_fb_var;
    info->fix = simple_fb_fix;
    info->fix.smem_start = par->fb_phys;
    info->fix.smem_len = par->fb_size;
    info->screen_base = par->fb_virt;
    info->screen_size = par->fb_size;
    
    // Allocate color map
    ret = fb_alloc_cmap(&info->cmap, 256, 0);
    if (ret) {
        dev_err(&pdev->dev, "Failed to allocate color map\n");
        goto err_dma_free;
    }
    
    // Register framebuffer
    ret = register_framebuffer(info);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register framebuffer\n");
        goto err_dealloc_cmap;
    }
    
    dev_info(&pdev->dev, "Framebuffer registered: fb%d (%s)\n",
             info->node, info->fix.id);
    dev_info(&pdev->dev, "Mode: %dx%d-%d\n",
             info->var.xres, info->var.yres, info->var.bits_per_pixel);
    
    return 0;
    
err_dealloc_cmap:
    fb_dealloc_cmap(&info->cmap);
err_dma_free:
    dma_free_coherent(&pdev->dev, par->fb_size, par->fb_virt, par->fb_phys);
err_fb_release:
    framebuffer_release(info);
    return ret;
}

static int simple_fb_remove(struct platform_device *pdev)
{
    struct fb_info *info = platform_get_drvdata(pdev);
    struct simple_fb_par *par = info->par;
    
    pr_info("%s: Removing framebuffer driver\n", DRIVER_NAME);
    
    unregister_framebuffer(info);
    fb_dealloc_cmap(&info->cmap);
    dma_free_coherent(&pdev->dev, par->fb_size, par->fb_virt, par->fb_phys);
    framebuffer_release(info);
    
    return 0;
}

static struct platform_driver simple_fb_driver = {
    .probe  = simple_fb_probe,
    .remove = simple_fb_remove,
    .driver = {
        .name = DRIVER_NAME,
    },
};

// Create platform device for testing
static struct platform_device *simple_fb_pdev;

static int __init simple_fb_init(void)
{
    int ret;
    
    pr_info("%s: Initializing framebuffer driver\n", DRIVER_NAME);
    
    ret = platform_driver_register(&simple_fb_driver);
    if (ret) {
        pr_err("Failed to register platform driver\n");
        return ret;
    }
    
    // Create test platform device
    simple_fb_pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(simple_fb_pdev)) {
        pr_err("Failed to register platform device\n");
        platform_driver_unregister(&simple_fb_driver);
        return PTR_ERR(simple_fb_pdev);
    }
    
    pr_info("%s: Initialization complete\n", DRIVER_NAME);
    return 0;
}

static void __exit simple_fb_exit(void)
{
    pr_info("%s: Exiting framebuffer driver\n", DRIVER_NAME);
    
    platform_device_unregister(simple_fb_pdev);
    platform_driver_unregister(&simple_fb_driver);
}

module_init(simple_fb_init);
module_exit(simple_fb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Framebuffer Driver");
MODULE_VERSION("1.0");