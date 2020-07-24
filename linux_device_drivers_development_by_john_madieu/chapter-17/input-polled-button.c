/* ************ LDDD: chapter-17: input-polled-button.c ************ */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>                   /* For DT*/
#include <linux/platform_device.h>      /* For platform devices */
#include <linux/gpio/consumer.h>        /* For GPIO Descriptor interface */
#include <linux/input.h>
#include <linux/input-polldev.h>


struct poll_btn_data {
	struct gpio_desc *btn_gpiod;
	struct input_polled_dev *poll_dev;
};

static void polled_btn_open(struct input_polled_dev *poll_dev)
{
	/* struct poll_btn_data *priv = poll_dev->private; */
	pr_info("polled device opened()\n");
}

static void polled_btn_close(struct input_polled_dev *poll_dev)
{
	/* struct poll_btn_data *priv = poll_dev->private; */
	pr_info("polled device closed()\n");
}

static void polled_btn_poll(struct input_polled_dev *poll_dev)
{
	struct poll_btn_data *priv = poll_dev->private;

	/* One can comment these two line for testing purpose on a PC */
	input_report_key(poll_dev->input, BTN_0, gpiod_get_value(priv->btn_gpiod) & 1);
	input_sync(poll_dev->input);
}

static const struct of_device_id btn_dt_ids[] = {
	{ .compatible = "packt,input-polled-button", },
	{ /* sentinel */ }
};

static int polled_btn_probe(struct platform_device *pdev)
{
	struct poll_btn_data *priv;
	struct input_polled_dev *poll_dev;
	struct input_dev *input_dev;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	poll_dev = input_allocate_polled_device();
	if (!poll_dev)
		return -ENOMEM;

	/* We assume this GPIO is active high */
	priv->btn_gpiod = gpiod_get(&pdev->dev, "button", GPIOD_IN);

	poll_dev->private = priv ;
	poll_dev->poll_interval = 200; /* Poll every 200ms */
	poll_dev->poll = polled_btn_poll;
	poll_dev->open = polled_btn_open;
	poll_dev->close = polled_btn_close;
	priv->poll_dev = poll_dev;

	input_dev = poll_dev->input;
	input_dev->name = "Packt input polled Btn";
	input_dev->dev.parent = &pdev->dev;

	/* Declare the events generated by this driver */
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_0, input_dev->keybit); /* buttons */

	ret = input_register_polled_device(priv->poll_dev);
	if (ret) {
		pr_err("Failed to register input polled device\n");
		input_free_polled_device(poll_dev);
		devm_kfree(&pdev->dev, priv);
		return ret;
	}

	platform_set_drvdata(pdev, priv);
	return 0;
}

static int polled_btn_remove(struct platform_device *pdev)
{
	struct poll_btn_data *priv = platform_get_drvdata(pdev);
	input_unregister_polled_device(priv->poll_dev);
	input_free_polled_device(priv->poll_dev);
	gpiod_put(priv->btn_gpiod);
	return 0;
}

static struct platform_driver mypdrv = {
	.probe      = polled_btn_probe,
	.remove     = polled_btn_remove,
	.driver     = {
		.name     = "input-polled-button",
		.of_match_table = of_match_ptr(btn_dt_ids),  
		.owner    = THIS_MODULE,
	},
};
module_platform_driver(mypdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_DESCRIPTION("Polled input device");