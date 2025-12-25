// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/of.h>

#define DRIVER_NAME "pm_hooks_demo"

static int pm_demo_probe(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "PM Demo Driver Probed\n");
    return 0;
}

static int pm_demo_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "PM Demo Driver Removed\n");
    return 0;
}

/* -------- Power Management Hooks -------- */
static int pm_demo_suspend(struct device *dev)
{
    dev_info(dev, "System is suspending... Saving context, disabling HW\n");
    /* turn off clocks / interrupts / power rails here in real drivers */
    return 0;
}

static int pm_demo_resume(struct device *dev)
{
    dev_info(dev, "System resumed! Restoring hardware state...\n");
    /* reinitialize clocks / restore registers / power on hardware */
    return 0;
}

static const struct dev_pm_ops pm_demo_ops = {
    .suspend = pm_demo_suspend,
    .resume  = pm_demo_resume,
};

/* -------- Device Tree Match Table -------- */
static const struct of_device_id pm_demo_of_match[] = {
    { .compatible = "demo,pm-hooks" },
    {},
};
MODULE_DEVICE_TABLE(of, pm_demo_of_match);

/* -------- Platform Driver Structure -------- */
static struct platform_driver pm_demo_driver = {
    .probe  = pm_demo_probe,
    .remove = pm_demo_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = pm_demo_of_match,
        .pm = &pm_demo_ops,  // <-- attaching PM hooks here
    },
};

module_platform_driver(pm_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Satya Prakash Rout");
MODULE_DESCRIPTION("Power Management Hooks Demo Driver");
MODULE_VERSION("1.0");
