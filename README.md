# LSM6DSOX Linux IIO Driver

A Linux kernel driver for the STMicroelectronics LSM6DSOX Inertial Measurement Unit (IMU), developed as part of the Advanced Operating Systems (AOS) course at Politecnico di Milano.

**Note: This project received a maximum grade of 10/10.**

## Overview

This project implements a custom Linux driver for the LSM6DSOX sensor using the Industrial I/O (IIO) framework. The driver provides support for:
- 3-axis Accelerometer and 3-axis Gyroscope raw data access via sysfs.
- Regmap-based I/O management for I2C communication.
- Triggered buffer support for continuous data streaming using software triggers (hrtimer).
- Kernel-standard resource management (devm).

## Hardware Setup

The development and testing were performed using the following hardware:
- **Sensor**: ST LSM6DSOX (Adafruit breakout board).
- **Interface**: MCP2221A USB-to-I2C bridge.
- **Environment**: Debian 13 virtualized via QEMU/KVM with the sensor connected over USB.

## Features

- **IIO Core**: Integration with the Linux Industrial I/O subsystem.
- **Triggered Buffers**: Support for high-speed data acquisition through kfifo.
- **Regmap**: Efficient register access abstraction.
- **Scalability**: While currently optimized for a specific educational setup, the driver follows kernel best practices for portability.

## Usage

### 1. Build and Load
Compile the driver using the provided Makefile:
```bash
make
sudo insmod lsm6dsox_driver.ko
```

### 2. Device Instantiation
Manually instantiate the device on the I2C bus:
```bash
echo lsm6dsox 0x6a | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
```

### 3. Data Access
Read raw sensor data and scale factors:
```bash
cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw
cat /sys/bus/iio/devices/iio:device0/in_accel_scale
```

### 4. Continuous Streaming
Set up a software trigger and enable the buffer:
```bash
sudo modprobe iio-trig-hrtimer
sudo mkdir -p /sys/kernel/config/iio/triggers/hrtimer/finto
echo 100 | sudo tee /sys/bus/iio/devices/trigger0/sampling_frequency
echo finto | sudo tee /sys/bus/iio/devices/iio:device0/trigger/current_trigger
echo 1 | sudo tee /sys/bus/iio/devices/iio:device0/buffer/enable
sudo cat /dev/iio:device0 | hexdump -C
```

## Project Structure
- `lsm6dsox_driver.c`: Core driver implementation.
- `lsm6dsox.h`: Register definitions and macros.
- `report_lsm6dsox_en.tex`: Detailed technical report source.
- `command_list.txt`: Handy list of commands for testing.

## Authors
- **Giacomo Di Clerico** - [GitHub Profile](https://github.com/MishowHD)
- **Lorenzo D'Ortona** - [GitHub Profile](https://github.com/LorenzoDOrtona)

## License
This project was originally developed for academic purposes.
