/*
 * wm8350-i2c.c  --  Generic I2C driver for Wolfson WM8350 PMIC
 *
 * Copyright 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood
 *         linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Modifications 2009 : Intrinsyc Software, Inc on behalf of Barnes and Noble
 * Portions of this code copyright (c) 2009 Barnes and Noble, Inc
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <plat/iic.h>
#include <linux/platform_device.h>
#include <linux/mfd/wm8350/core.h>

static int wm8350_i2c_read_device(struct wm8350 *wm8350, char reg,
				  int bytes, void *dest)
{
	int ret;

	ret = i2c_master_send(wm8350->i2c_client, &reg, 1);
	if (ret < 0)
		return ret;
	ret = i2c_master_recv(wm8350->i2c_client, dest, bytes);
	if (ret < 0)
		return ret;
	if (ret != bytes)
		return -EIO;
	return 0;
}

static int wm8350_i2c_write_device(struct wm8350 *wm8350, char reg,
				   int bytes, void *src)
{
	/* we add 1 byte for device register */
	u8 msg[(WM8350_MAX_REGISTER << 1) + 1];
	int ret;

	if (bytes > ((WM8350_MAX_REGISTER << 1) + 1))
		return -EINVAL;

	msg[0] = reg;
	memcpy(&msg[1], src, bytes);
	ret = i2c_master_send(wm8350->i2c_client, msg, bytes + 1);
	if (ret < 0)
		return ret;
	if (ret != bytes + 1)
		return -EIO;
	return 0;
}

static int wm8350_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct wm8350 *wm8350;
	int ret = 0;
#ifdef CONFIG_MACH_BRAVO_I2C_MULTICLK
	struct s3c_i2c_client_platdata *client_pdata = (struct s3c_i2c_client_platdata *)(i2c->dev.platform_data);
#endif
	wm8350 = kzalloc(sizeof(struct wm8350), GFP_KERNEL);
	if (wm8350 == NULL) {
		kfree(i2c);
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, wm8350);
	wm8350->dev = &i2c->dev;
	wm8350->i2c_client = i2c;
	wm8350->read_dev = wm8350_i2c_read_device;
	wm8350->write_dev = wm8350_i2c_write_device;

#ifdef CONFIG_MACH_BRAVO_I2C_MULTICLK
	ret = wm8350_device_init(wm8350, i2c->irq, client_pdata->client_owner_platdata);
#else
	ret = wm8350_device_init(wm8350, i2c->irq, i2c->dev.platform_data);
#endif

	if (ret < 0)
		goto err;

	return ret;

err:
	kfree(wm8350);
	return ret;
}

static int wm8350_i2c_remove(struct i2c_client *i2c)
{
	struct wm8350 *wm8350 = i2c_get_clientdata(i2c);

	wm8350_device_exit(wm8350);
	kfree(wm8350);

	return 0;
}

static const struct i2c_device_id wm8350_i2c_id[] = {
       { "wm8350", 0 },
       { "wm8351", 0 },
       { "wm8352", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, wm8350_i2c_id);


static struct i2c_driver wm8350_i2c_driver = {
	.driver = {
		   .name = "wm8350",
		   .owner = THIS_MODULE,
	},
	.probe = wm8350_i2c_probe,
	.remove = wm8350_i2c_remove,
	.id_table = wm8350_i2c_id,
};

static int __init wm8350_i2c_init(void)
{
	return i2c_add_driver(&wm8350_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall_sync(wm8350_i2c_init);

static void __exit wm8350_i2c_exit(void)
{
	i2c_del_driver(&wm8350_i2c_driver);
}
module_exit(wm8350_i2c_exit);

MODULE_DESCRIPTION("I2C support for the WM8350 AudioPlus PMIC");
MODULE_LICENSE("GPL");
