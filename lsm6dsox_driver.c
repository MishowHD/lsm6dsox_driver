// SPDX-License-Identifier: GPL-2.0-only
#include "lsm6dsox.h"
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/regmap.h>

/**
 * struct lsm6dsox_data - driver private data
 * @regmap: regmap instance for register access
 * @dev: pointer to the i2c device
 */
struct lsm6dsox_data {
	struct regmap *regmap;
	struct device *dev;
};

static const struct regmap_config lsm6dsox_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7F,
};

static int lsm6dsox_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int *val,
			     int *val2, long mask)
{
	struct lsm6dsox_data *data = iio_priv(indio_dev);
	__le16 raw_val;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = regmap_bulk_read(data->regmap, chan->address, &raw_val,
				       sizeof(raw_val));
		if (ret < 0) {
			dev_err(data->dev, "Failed to read register 0x%02lx\n",
				chan->address);
			return ret;
		}

		*val = (s16)le16_to_cpu(raw_val);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		if (chan->type == IIO_ACCEL) {
			/*
			 * FS = 2g, sensitivity = 0.061 mg/LSB
			 * Scale = (0.061 * 9.80665 / 1000) = 0.000598205 m/s^2
			 */
			*val = 0;
			*val2 = 598205;
			return IIO_VAL_INT_PLUS_NANO;
		} else if (chan->type == IIO_ANGL_VEL) {
			/*
			 * FS = 250 dps, sensitivity = 8.75 mdps/LSB
			 * Scale = (8.75 * pi / 180 / 1000) = 0.000152716 rad/s
			 */
			*val = 0;
			*val2 = 152716;
			return IIO_VAL_INT_PLUS_NANO;
		}
		return -EINVAL;
	default:
		return -EINVAL;
	}
}

static const struct iio_info lsm6dsox_info = {
	.read_raw = lsm6dsox_read_raw,
};

#define LSM6DSOX_CHAN_ACCEL(axis, reg, index)				\
	{								\
		.type = IIO_ACCEL,					\
		.modified = 1,						\
		.channel2 = IIO_MOD_##axis,				\
		.address = reg,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
		.scan_index = index,					\
		.scan_type = {						\
			.sign = 's',					\
			.realbits = 16,					\
			.storagebits = 16,				\
			.endianness = IIO_LE,				\
		},							\
	}

#define LSM6DSOX_CHAN_GYRO(axis, reg, index)				\
	{								\
		.type = IIO_ANGL_VEL,					\
		.modified = 1,						\
		.channel2 = IIO_MOD_##axis,				\
		.address = reg,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
		.scan_index = index,					\
		.scan_type = {						\
			.sign = 's',					\
			.realbits = 16,					\
			.storagebits = 16,				\
			.endianness = IIO_LE,				\
		},							\
	}

static const struct iio_chan_spec lsm6dsox_channels[] = {
	LSM6DSOX_CHAN_GYRO(X, LSM6DSOX_REG_OUTX_L_G, 0),
	LSM6DSOX_CHAN_GYRO(Y, LSM6DSOX_REG_OUTY_L_G, 1),
	LSM6DSOX_CHAN_GYRO(Z, LSM6DSOX_REG_OUTZ_L_G, 2),
	LSM6DSOX_CHAN_ACCEL(X, LSM6DSOX_REG_OUTX_L_A, 3),
	LSM6DSOX_CHAN_ACCEL(Y, LSM6DSOX_REG_OUTY_L_A, 4),
	LSM6DSOX_CHAN_ACCEL(Z, LSM6DSOX_REG_OUTZ_L_A, 5),
	IIO_CHAN_SOFT_TIMESTAMP(6),
};

static irqreturn_t lsm6dsox_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct lsm6dsox_data *data = iio_priv(indio_dev);
	struct {
		s16 channels[6];

		s64 ts __aligned(8);
	} scan;
	int ret;

	/*
	 * Read all 6 axes (12 bytes) in a single burst from Gyro X.
	 * Registers are contiguous in memory.
	 */
	ret = regmap_bulk_read(data->regmap, LSM6DSOX_REG_OUTX_L_G,
			       scan.channels, sizeof(scan.channels));
	if (ret < 0)
		goto out;

	iio_push_to_buffers_with_timestamp(indio_dev, &scan, pf->timestamp);

out:
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int lsm6dsox_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct lsm6dsox_data *data;
	struct regmap *regmap;
	struct iio_trigger *trig;
	unsigned int val;
	int ret;

	regmap = devm_regmap_init_i2c(client, &lsm6dsox_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Failed to initialize regmap\n");
		return PTR_ERR(regmap);
	}

	ret = regmap_read(regmap, LSM6DSOX_REG_WHO_AM_I, &val);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read WHO_AM_I: %d\n", ret);
		return ret;
	}

	if (val != LSM6DSOX_WHO_AM_I_VAL) {
		dev_err(&client->dev, "Invalid WHO_AM_I value: 0x%02x\n", val);
		return -ENODEV;
	}

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	data->regmap = regmap;
	data->dev = &client->dev;

	indio_dev->name = "lsm6dsox";
	indio_dev->info = &lsm6dsox_info;
	indio_dev->channels = lsm6dsox_channels;
	indio_dev->num_channels = ARRAY_SIZE(lsm6dsox_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	/* Enable Block Data Update (BDU) and auto-increment */
	ret = regmap_write(regmap, LSM6DSOX_REG_CTRL3_C, LSM6DSOX_CTRL3_C_BDU);
	if (ret < 0)
		return ret;

	/* Power on Accel: 104Hz, 2g FS */
	ret = regmap_write(regmap, LSM6DSOX_REG_CTRL1_XL,
			   LSM6DSOX_CTRL1_XL_104HZ);
	if (ret < 0)
		return ret;

	/* Power on Gyro: 104Hz, 250 dps FS */
	ret = regmap_write(regmap, LSM6DSOX_REG_CTRL2_G,
			   LSM6DSOX_CTRL2_G_104HZ);
	if (ret < 0)
		return ret;

	ret = devm_iio_triggered_buffer_setup(&client->dev, indio_dev, NULL,
					      lsm6dsox_trigger_handler, NULL);
	if (ret)
		return ret;

	ret = regmap_write(regmap, LSM6DSOX_REG_INT1_CTRL,
			   LSM6DSOX_INT1_DRDY_XL | LSM6DSOX_INT1_DRDY_G);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to route DRDY to INT1\n");
		return ret;
	}

	if (client->irq <= 0) {
		dev_warn(&client->dev,
			 "No HW IRQ found. Polling/Sysfs trigger mode only.\n");
	} else {
		trig = devm_iio_trigger_alloc(&client->dev, "%s-trigger",
					      indio_dev->name);
		if (!trig)
			return -ENOMEM;

		iio_trigger_set_drvdata(trig, indio_dev);

		ret = devm_request_irq(&client->dev, client->irq,
				       iio_trigger_generic_data_rdy_poll,
				       IRQF_TRIGGER_RISING, trig->name, trig);
		if (ret) {
			dev_err(&client->dev, "Failed to request IRQ %d\n",
				client->irq);
			return ret;
		}

		ret = devm_iio_trigger_register(&client->dev, trig);
		if (ret) {
			dev_err(&client->dev, "Failed to register IIO trigger\n");
			return ret;
		}

		indio_dev->trig = iio_trigger_get(trig);
	}

	return devm_iio_device_register(&client->dev, indio_dev);
}

static const struct i2c_device_id lsm6dsox_id[] = {
	{ "lsm6dsox", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lsm6dsox_id);

static struct i2c_driver lsm6dsox_driver = {
	.driver = {
		.name = "lsm6dsox",
	},
	.probe = lsm6dsox_probe,
	.id_table = lsm6dsox_id,
};
module_i2c_driver(lsm6dsox_driver);

MODULE_AUTHOR("Giacomo Di Clerico, Lorenzo D'Ortona");
MODULE_DESCRIPTION("LSM6DSOX IIO Driver (Accel + Gyro)");
MODULE_LICENSE("GPL v2");
