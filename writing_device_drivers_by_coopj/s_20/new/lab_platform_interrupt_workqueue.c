/* ************ LDD:2.0 s_20/new/lab_platform_interrupt_workqueue.c ************ */
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

#include <linux/module.h>
#include <linux/platform_device.h> 	
#include <linux/interrupt.h> 		
#include <linux/gpio/consumer.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>
#include <linux/delay.h>

static int delay = 0;
module_param(delay, int, S_IRUGO);
static atomic_t counter_bh, counter_th;

struct my_dat {
	unsigned long jiffies;  /* used for timestamp */
	struct work_struct work;    /* used in dynamic workqueue solution */
};
static struct my_dat my_data;

static void w_fun(struct work_struct *w_arg)
{
	struct my_dat *data = container_of(w_arg, struct my_dat, work);
	atomic_inc(&counter_bh);
	pr_info("In BH: counter_th = %d, counter_bh = %d, jiffies=%ld, %ld\n",
			atomic_read(&counter_th), atomic_read(&counter_bh),
			data->jiffies, jiffies);
}

static irqreturn_t my_interrupt(int irq, void *dev_id)
{
	struct my_dat *data = (struct my_dat *)dev_id;
	atomic_inc(&counter_th);
	data->jiffies = jiffies;
	schedule_work(&data->work);
	mdelay(delay);      /* hoke up a delay to try to cause pileup */
	return IRQ_NONE;    /* we return IRQ_NONE because we are just observing */
}

static struct miscdevice my_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mydev",
};

static int my_probe(struct platform_device *pdev)
{	
	int ret_val, irq;
	struct device *dev = &pdev->dev;
	struct gpio_desc *btn;
	struct my_dat *data = (struct my_dat *)&my_data;

	atomic_set(&counter_bh, 0);
	atomic_set(&counter_th, 0);

	INIT_WORK(&data->work, w_fun);

	btn = devm_gpiod_get_optional(dev, "mybtn", GPIOD_IN);
	if (IS_ERR(btn)) {
		dev_err(dev, "Failed to retrieve/request reset gpio: %ld\n", PTR_ERR(btn));
		return PTR_ERR(btn);
	}

	irq = gpiod_to_irq(btn);
	if (irq < 0)
		return irq;
	dev_info(dev, "The irq number is %d.\n", irq);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "irq is not available.\n");
		return -EINVAL;
	}
	dev_info(dev, "platform_get_irq(): %d\n", irq);

	atomic_set(&counter_bh, 0);
	atomic_set(&counter_th, 0);

	ret_val = devm_request_irq(dev, irq,
			my_interrupt, IRQF_TRIGGER_FALLING, "my_interrupt", &my_data);
	if (ret_val) {
		dev_err(dev, "Failed to reserve irq %d(error: %d)\n", irq, ret_val);
		return ret_val;
	}

	ret_val = misc_register(&my_miscdevice);
	if (ret_val != 0) {
		dev_err(dev, "Could not register the misc device.\n");
		return ret_val;
	}

	dev_info(dev, "Successfully loading ISR handler.\n");
	return 0;
}

static int my_remove(struct platform_device *pdev)
{	
	misc_deregister(&my_miscdevice);
	dev_info(&pdev->dev, "Successfully unloading.\n");
	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "eagle,int-button"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "int-button",
		.of_match_table = of_match_ptr(my_of_ids),
		.owner = THIS_MODULE,
	}
};

module_platform_driver(my_platform_driver);

MODULE_AUTHOR("Chunghan Yi");
MODULE_DESCRIPTION("LDD:2.0 s_20/new/lab_platform_interrupt_workqueue.c");
MODULE_LICENSE("GPL v2");
