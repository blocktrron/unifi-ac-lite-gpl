/*
 * Driver for the Solomon SSD1317Z OLED controller
 *
 * Copyright 2017 Ubiquiti Networks
 *
 * Licensed under the GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>

#include "ssd1317fb-splash.h"

#define SSD1317FB_DATA			0x1
#define SSD1317FB_COMMAND		0x0

#define SSD1307FB_SET_ADDRESS_MODE  0x20
#define SSD1307FB_SET_ADDRESS_MODE_HORIZONTAL (0x00)
#define SSD1307FB_SET_ADDRESS_MODE_VERTICAL (0x01)
#define SSD1307FB_SET_ADDRESS_MODE_PAGE   (0x02)
#define SSD1307FB_SET_COL_RANGE   0x21
#define SSD1307FB_SET_PAGE_RANGE  0x22
#define SSD1307FB_CONTRAST    0x81
#define SSD1307FB_CHARGE_PUMP   0x8d
#define SSD1307FB_SEG_REMAP_ON    0xa1
#define SSD1307FB_DISPLAY_OFF   0xae
#define SSD1307FB_SET_MULTIPLEX_RATIO 0xa8
#define SSD1307FB_DISPLAY_ON    0xaf
#define SSD1307FB_START_PAGE_ADDRESS  0xb0
#define SSD1307FB_SET_DISPLAY_OFFSET  0xd3
#define SSD1307FB_SET_CLOCK_FREQ  0xd5
#define SSD1307FB_SET_PRECHARGE_PERIOD  0xd9
#define SSD1307FB_SET_COM_PINS_CONFIG 0xda
#define SSD1307FB_SET_VCOMH   0xdb

/*
#define SSD1317FB_SET_ADDRESS_MODE	0x20
#define SSD1317FB_SET_ADDRESS_MODE_HORIZONTAL	(0x00)
#define SSD1317FB_SET_ADDRESS_MODE_VERTICAL	(0x01)
#define SSD1317FB_SET_ADDRESS_MODE_PAGE		(0x02)
#define SSD1317FB_SET_COL_RANGE		0x21
#define SSD1317FB_SET_PAGE_RANGE	0x22
#define SSD1317FB_CONTRAST		0x81
#define SSD1317FB_SEG_REMAP_ON		0xa1
#define SSD1317FB_DISPLAY_OFF		0xae
#define SSD1317FB_SET_MULTIPLEX_RATIO	0xa8
#define SSD1317FB_DISPLAY_ON		0xaf
#define	SSD1317FB_SET_VCOMH		0xbe
*/

#define SSD1317FB_DATAW_SIZE		0x50

struct ssd1317fb_par;

struct ssd1317fb_ops {
	int (*init)(struct ssd1317fb_par *);
	int (*splash)(struct ssd1317fb_par *);
	int (*remove)(struct ssd1317fb_par *);
};

#if 0
static const u8 ssd1317fb_init_seq[] = {
	/* Turn off the display */
	SSD1317FB_DISPLAY_OFF,
	/* Set column address */
	0x15,
	0x00,
	0x4f,
	/* Set row address */
	0x75,
	0x00,
	0x3f,
	/* Set the contrast control */
	SSD1317FB_CONTRAST,
	0xff,
	/* Set remap */
	0xa0,
	0xc1,
	/* Set display start line */
	SSD1317FB_SEG_REMAP_ON,
	0x00,
	/* Set display offset */
	0xa2,
	0x00,
	/* Set Vertical scroll area */
	0xa3,
	0x00,
	0x40,
	/* Set display mode (normal display) */
	0xa4,
	/* Set MUX ratio */
	SSD1317FB_SET_MULTIPLEX_RATIO,
	0x3f,
	/* Function selection A, 1v8 */
	0xab,
	0x00,
	/* External/Internal iREF selection */
	0xad,
	/* Select external iREF */
	0x8e,
	/* Set phase length */
	0xb1,
	0x82,
	/* Set front clock divider/oscillator frequency */
	0xb3,
	0xa0,
	/* Set second precharge period */
	0xb6,
	0x04,
	/* Linear LUT */
	0xb9,
	/* Set pre-charge voltage */
	0xbc,
	0x04,
	/* Pre-charge voltage capacitor selection */
	0xbd,
	0x01,
	/* Set VCOMH */
	SSD1317FB_SET_VCOMH,
	0x05,
	/* Turn on the display */
	SSD1317FB_DISPLAY_ON,
};
#endif

static u8 ssd1317fb_update_cmd[] = {
	SSD1307FB_DISPLAY_ON,
};

struct ssd1317fb_deviceinfo {
  u32 default_vcomh;
  u32 default_dclk_div;
  u32 default_dclk_frq;
  int need_pwm;
  int need_chargepump;
};

struct ssd1317fb_par {
  u32 com_invdir;
  u32 com_lrremap;
  u32 com_offset;
  u32 com_seq;
  u32 contrast;
  u32 dclk_div;
  u32 dclk_frq;
  struct ssd1317fb_deviceinfo *device_info;
	struct spi_device *spi;
  u32 height;
  struct fb_info *info;
  u32 page_offset;
  u32 prechargep1;
  u32 prechargep2;
  struct pwm_device *pwm;
  u32 pwm_period;
  int reset;
  u32 seg_remap;
  u32 vcomh;
  u32 width;
	struct ssd1317fb_ops *ops;
	int data_comm;
	int boost_en;
};

static struct fb_fix_screeninfo ssd1317fb_fix = {
	.id		= "SSD1317FB-SPI",
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_MONO10,
	.xpanstep	= 0,
	.ypanstep	= 0,
	.ywrapstep	= 0,
	.accel		= FB_ACCEL_NONE,
};

static struct fb_var_screeninfo ssd1317fb_var = {
	.bits_per_pixel	= 1,
};

/* spi write command */
struct ssd1317fb_array {
  u8  type;
  u8  data[0];
};

static struct ssd1317fb_array *ssd1317fb_alloc_array(u32 len, u8 type)
{
  struct ssd1317fb_array *array;

  array = kzalloc(sizeof(struct ssd1317fb_array) + len, GFP_KERNEL);
  if (!array)
    return NULL;

  array->type = type;

  return array;
}

static int ssd1317fb_write_array(struct ssd1317fb_par *par,
         struct ssd1317fb_array *array, u32 len)
{
  int ret,i;

	gpio_set_value(par->data_comm, SSD1317FB_COMMAND);
	spi_write(par->spi, ssd1317fb_update_cmd, ARRAY_SIZE(ssd1317fb_update_cmd));

	gpio_set_value(par->data_comm, SSD1317FB_DATA);

	spi_write(par->spi, array->data, 1024);

  return 0;
}

static inline int ssd1317fb_write_cmd(struct ssd1317fb_par *par, u8 cmd)
{
  int ret;

	gpio_set_value(par->data_comm, SSD1317FB_COMMAND);
	ret = spi_write(par->spi, &cmd, sizeof(cmd));

  return ret;
}

static void ssd1317fb_update_display(struct ssd1317fb_par *par)
{
printk("%s:%d\n",__FUNCTION__,__LINE__);
 struct ssd1317fb_array *array;
 u8 *vmem = par->info->screen_base;
 int i, j, k;
#if 0

 array = ssd1317fb_alloc_array(par->width * par->height / 8,
             SSD1317FB_DATA);
 if (!array)
   return;

 /*
  * The screen is divided in pages, each having a height of 8
  * pixels, and the width of the screen. When sending a byte of
  * data to the controller, it gives the 8 bits for the current
  * column. I.e, the first byte are the 8 bits of the first
  * column, then the 8 bits for the second column, etc.
  *
  *
  * Representation of the screen, assuming it is 5 bits
  * wide. Each letter-number combination is a bit that controls
  * one pixel.
  *
  * A0 A1 A2 A3 A4
  * B0 B1 B2 B3 B4
  * C0 C1 C2 C3 C4
  * D0 D1 D2 D3 D4
  * E0 E1 E2 E3 E4
  * F0 F1 F2 F3 F4
  * G0 G1 G2 G3 G4
  * H0 H1 H2 H3 H4
  *
  * If you want to update this screen, you need to send 5 bytes:
  *  (1) A0 B0 C0 D0 E0 F0 G0 H0
  *  (2) A1 B1 C1 D1 E1 F1 G1 H1
  *  (3) A2 B2 C2 D2 E2 F2 G2 H2
  *  (4) A3 B3 C3 D3 E3 F3 G3 H3
  *  (5) A4 B4 C4 D4 E4 F4 G4 H4
  */
 for (i = 0; i < (par->height / 8); i++) {
   for (j = 0; j < par->width; j++) {
     u32 array_idx = i * par->width + j;
     array->data[array_idx] = 0;
     for (k = 0; k < 8; k++) {
       u32 page_length = par->width * i;
       u32 index = page_length + (par->width * k + j) / 8;
       u8 byte = *(vmem + index);
       u8 bit = byte & (1 << (j % 8));
       bit = bit >> (j % 8);
       array->data[array_idx] |= bit << k;
     }
   }
 }
 for (i = 0; i < 1024; i++)
       array->data[i] = 0xff;

 ssd1317fb_write_array(par, array, par->width * par->height / 8);
 kfree(array);
#endif


 spi_write(par->spi, vmem, par->width * par->height / 8);
 /*
	for (i = 0; i < par->height; i++) {
		spi_write(par->spi, vmem + (i * SSD1317FB_DATAW_SIZE), SSD1317FB_DATAW_SIZE);
	}
	*/
}


static ssize_t ssd1317fb_write(struct fb_info *info, const char __user *buf,
		size_t count, loff_t *ppos)
{
	struct ssd1317fb_par *par = info->par;
	unsigned long total_size;
	unsigned long p = *ppos;
	u8 __iomem *dst;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EINVAL;

	if (count + p > total_size)
		count = total_size - p;

	if (!count)
		return -EINVAL;

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		return -EFAULT;

	ssd1317fb_update_display(par);

	*ppos += count;

	return count;
}

static void ssd1317fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct ssd1317fb_par *par = info->par;

	sys_fillrect(info, rect);
	ssd1317fb_update_display(par);
}

static void ssd1317fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	struct ssd1317fb_par *par = info->par;

	sys_copyarea(info, area);
	ssd1317fb_update_display(par);
}

static void ssd1317fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	struct ssd1317fb_par *par = info->par;
	sys_imageblit(info, image);
	ssd1317fb_update_display(par);
}

static struct fb_ops ssd1317fb_ops = {
	.owner		= THIS_MODULE,
	.fb_read	= fb_sys_read,
	.fb_write	= ssd1317fb_write,
	.fb_fillrect	= ssd1317fb_fillrect,
	.fb_copyarea	= ssd1317fb_copyarea,
	.fb_imageblit	= ssd1317fb_imageblit,
};

static void ssd1317fb_deferred_io(struct fb_info *info,
		struct list_head *pagelist)
{
	ssd1317fb_update_display(info->par);
}

static struct fb_deferred_io ssd1317fb_defio = {
	.delay		= HZ,
	.deferred_io = ssd1317fb_deferred_io,
};

static int ssd1317fb_panel_init(struct ssd1317fb_par *par)
{
int ret;
	printk("%s:%d\n",__FUNCTION__,__LINE__);
u32 precharge, dclk, com_invdir, compins;
  /* Set initial contrast */
ret = ssd1317fb_write_cmd(par, SSD1307FB_CONTRAST);
if (ret < 0)
  return ret;

ret = ssd1317fb_write_cmd(par, par->contrast);
if (ret < 0)
  return ret;

/* Set segment re-map */
if (par->seg_remap) {
  ret = ssd1317fb_write_cmd(par, SSD1307FB_SEG_REMAP_ON);
  if (ret < 0)
    return ret;
};

/* Set COM direction */
com_invdir = 0xc0 | (par->com_invdir & 0x1) << 3;
ret = ssd1317fb_write_cmd(par,  com_invdir);
if (ret < 0)
  return ret;

/* Set multiplex ratio value */
ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_MULTIPLEX_RATIO);
if (ret < 0)
  return ret;

ret = ssd1317fb_write_cmd(par, par->height - 1);
if (ret < 0)
  return ret;

/* set display offset value */
ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_DISPLAY_OFFSET);
if (ret < 0)
  return ret;

ret = ssd1317fb_write_cmd(par, par->com_offset);
if (ret < 0)
  return ret;

/* Set clock frequency */
ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_CLOCK_FREQ);
if (ret < 0)
  return ret;

dclk = ((par->dclk_div - 1) & 0xf) | (par->dclk_frq & 0xf) << 4;
ret = ssd1317fb_write_cmd(par, dclk);
if (ret < 0)
  return ret;

/* Set precharge period in number of ticks from the internal clock */
ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_PRECHARGE_PERIOD);
if (ret < 0)
  return ret;

   precharge = (par->prechargep1 & 0xf) | (par->prechargep2 & 0xf) << 4;
   ret = ssd1317fb_write_cmd(par, precharge);
   if (ret < 0)
     return ret;

   /* Set COM pins configuration */
   ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_COM_PINS_CONFIG);
   if (ret < 0)
     return ret;

   compins = 0x02 | !(par->com_seq & 0x1) << 4
            | (par->com_lrremap & 0x1) << 5;
   ret = ssd1317fb_write_cmd(par, compins);
   if (ret < 0)
     return ret;

   /* Set VCOMH */
   ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_VCOMH);
   if (ret < 0)
     return ret;

   ret = ssd1317fb_write_cmd(par, par->vcomh);
   if (ret < 0)
     return ret;

   /* Turn on the DC-DC Charge Pump */
   ret = ssd1317fb_write_cmd(par, SSD1307FB_CHARGE_PUMP);
   if (ret < 0)
     return ret;

   ret = ssd1317fb_write_cmd(par,
    (1 & 0x1 << 2) & 0x14);
   if (ret < 0)
     return ret;

   /* Switch to horizontal addressing mode */
   ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_ADDRESS_MODE);
   if (ret < 0)
     return ret;

   ret = ssd1317fb_write_cmd(par,
           SSD1307FB_SET_ADDRESS_MODE_VERTICAL);
   if (ret < 0)
     return ret;

   /* Set column range */
   ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_COL_RANGE);
   if (ret < 0)
     return ret;

   ret = ssd1317fb_write_cmd(par, 0x0);
   if (ret < 0)
     return ret;


ret = ssd1317fb_write_cmd(par, par->width - 1);
if (ret < 0)
  return ret;

/* Set page range */
ret = ssd1317fb_write_cmd(par, SSD1307FB_SET_PAGE_RANGE);
if (ret < 0)
  return ret;

ret = ssd1317fb_write_cmd(par, 0x0);
if (ret < 0)
  return ret;

ret = ssd1317fb_write_cmd(par,
        par->page_offset + (par->height / 8) - 1);
if (ret < 0)
  return ret;

/* Turn on the display */
ret = ssd1317fb_write_cmd(par, SSD1307FB_DISPLAY_ON);
if (ret < 0)
  return ret;

return 0;
}

static int ssd1317fb_panel_splash(struct ssd1317fb_par *par)
{
	int i, ret;

	gpio_set_value(par->data_comm, SSD1317FB_COMMAND);
	spi_write(par->spi, ssd1317fb_update_cmd, ARRAY_SIZE(ssd1317fb_update_cmd));

	gpio_set_value(par->data_comm, SSD1317FB_DATA);

	ret = spi_write(par->spi, ssd1317z_splash, par->width * par->height / 8 );

	return ret;
}

static int ssd1317fb_panel_remove(struct ssd1317fb_par *par)
{
	return 0;
}

static struct ssd1317fb_ops panel_ops = {
	.init	= ssd1317fb_panel_init,
	.splash	= ssd1317fb_panel_splash,
	.remove	= ssd1317fb_panel_remove,
};

static const struct of_device_id ssd1317fb_of_match[] = {
	{
		.compatible = "solomon,ssd1317fb-spi",
		.data = (void *)&panel_ops,
	},
	{},
};
MODULE_DEVICE_TABLE(of, ssd1317fb_of_match);

static int ssd1317fb_probe(struct spi_device *spi)
{
	struct fb_info *info;
	struct device_node *node = spi->dev.of_node;
	u32 vmem_size;
	struct ssd1317fb_par *par;
	u8 *vmem;
	int ret;

	info = framebuffer_alloc(sizeof(struct ssd1317fb_par), &spi->dev);
	if (!info) {
		dev_err(&spi->dev, "Couldn't allocate framebuffer.\n");
		return -ENOMEM;
	}

	par = info->par;
	par->info = info;
	par->spi = spi;

	par->ops = (struct ssd1317fb_ops *) &panel_ops;

	if(par->ops == NULL) {
		printk("Meow\n");
		return -ENOMEM;
	}

	par->reset = 21;
	if (!gpio_is_valid(par->reset)) {
		ret = -EINVAL;
		goto fb_alloc_error;
	}
	par->data_comm = 20;
	if (!gpio_is_valid(par->data_comm)) {
		ret = -EINVAL;
		goto fb_alloc_error;
	}

	/* Ignore device tree and use hard code here */
	par->width = 128;
	par->height = 64;
	par->page_offset = 0;
  par->prechargep1 = 2;
  par->prechargep2 = 2;
  par->contrast = 127;
  par->vcomh = 0x30;
  par->dclk_div = 1;
  par->dclk_frq = 10;
  par->com_offset= 0x50;
  

	vmem_size = ((par->width * par->height )/8);

	vmem = vzalloc(vmem_size);

	if (!vmem) {
		dev_err(&spi->dev, "Couldn't allocate graphical memory.\n");
		ret = -ENOMEM;
		goto fb_alloc_error;
	}

	info->fbops = &ssd1317fb_ops;
	info->fix = ssd1317fb_fix;
	info->fix.line_length = par->width / 8;
	info->fbdefio = &ssd1317fb_defio;

	info->var = ssd1317fb_var;
	info->var.xres = par->width;
	info->var.xres_virtual = par->width;
	info->var.yres = par->height;
	info->var.yres_virtual = par->height;

	info->var.red.length = 1;
	info->var.red.offset = 0;
	info->var.green.length = 1;
	info->var.green.offset = 0;
	info->var.blue.length = 1;
	info->var.blue.offset = 0;

	info->screen_base = (u8 __force __iomem *)vmem;
	info->fix.smem_start = (unsigned long)vmem;
	info->fix.smem_len = vmem_size;

	fb_deferred_io_init(info);

#if 0
	ret = devm_gpio_request_one(&spi->dev, par->boost_en,
			GPIOF_OUT_INIT_HIGH,
			"oled-boost-en");
	if (ret) {
		dev_err(&spi->dev,
				"failed to request gpio %d: %d\n",
				par->reset, ret);
		goto vmem_error;
	}

	/* Enable boost voltage */
	gpio_set_value(par->boost_en, 1);
	udelay(2);

	ret = devm_gpio_request_one(&spi->dev, par->reset,
			GPIOF_OUT_INIT_HIGH,
			"oled-reset");
	if (ret) {
		dev_err(&spi->dev,
				"failed to request gpio %d: %d\n",
				par->reset, ret);
		goto reset_oled_error;
	}

	ret = devm_gpio_request_one(&spi->dev, par->data_comm,
			GPIOF_OUT_INIT_LOW,
			"oled-data-com");
#endif

	spi->bits_per_word = 8;
	spi_set_drvdata(spi, info);

	/* Reset the screen */
	gpio_set_value(par->reset, 0);
	udelay(4);
	gpio_set_value(par->reset, 1);
	udelay(4);

	if (par->ops->init) {
		ret = par->ops->init(par);
		if (ret)
			goto reset_oled_error;
	}

	if (par->ops->splash) {
		ret = par->ops->splash(par);
		if (ret)
			goto reset_oled_error;
	}

	ret = register_framebuffer(info);
	if (ret) {
		dev_err(&spi->dev, "Couldn't register the framebuffer\n");
		goto panel_init_error;
	}

	printk("%s:%d\n",__FUNCTION__,__LINE__);
	dev_info(&spi->dev, "fb: %s framebuffer device registered, using %d bytes of video memory\n"
			,info->fix.id, vmem_size);

	return 0;

panel_init_error:
	if (par->ops->remove) {
		par->ops->remove(par);
	}
reset_oled_error:
	fb_deferred_io_cleanup(info);
	vfree(vmem);
fb_alloc_error:
	framebuffer_release(info);
	return ret;
}

static int ssd1317fb_remove(struct spi_device *spi)
{
	struct fb_info *info = spi_get_drvdata(spi);
	struct ssd1317fb_par *par = info->par;

	return 0;
	unregister_framebuffer(info);
	if (par->ops->remove)
		par->ops->remove(par);
	fb_deferred_io_cleanup(info);
	vfree((void __force *)info->screen_base);
	framebuffer_release(info);

	return 0;
}

static struct spi_driver ssd1317fb_driver = {
	.probe = ssd1317fb_probe,
	.remove = ssd1317fb_remove,
	.driver = {
		.name = "solomon,ssd1317fb-spi",
		.of_match_table = ssd1317fb_of_match,
		.owner = THIS_MODULE,
	},
};

module_spi_driver(ssd1317fb_driver);

MODULE_DESCRIPTION("Framebuffer driver for the Solomon SSD1317Z OLED controller");
MODULE_AUTHOR("Matt Hsu <matt.hsu@ubnt.com>");
MODULE_LICENSE("GPL");
