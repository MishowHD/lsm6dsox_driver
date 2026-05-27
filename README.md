# LSM6DSOX IIO Linux Driver - Educational Project

This repository contains a Linux kernel driver for the **ST LSM6DSOX** IMU (Accelerometer + Gyroscope), developed as an educational project for the **Advanced Operating Systems (AOS)** course.

## Recommended Reading Path

If you are new to Linux kernel driver development, follow this path in order.
Each step builds on the previous one.

**Step 1 — Understand the hardware registers**
Read [`lsm6dsox.h`](./lsm6dsox.h) first.
It is short (~40 lines) and defines every register address the driver uses.
Understanding what `WHO_AM_I`, `CTRL1_XL`, and `OUTX_L_G` mean makes the driver code
immediately readable. Each register now includes a datasheet reference.

**Step 2 — Read the driver source with the tutorial open side by side**
Read [`lsm6dsox_driver.c`](./lsm6dsox_driver.c) in this order:
- `lsm6dsox_regmap_config` — how the I2C bus is abstracted
- `lsm6dsox_channels[]` — how sensor axes are declared to the IIO framework
- `lsm6dsox_probe()` — the initialization sequence (hardware verify → IIO setup → register)
- `lsm6dsox_read_raw()` — the on-demand sysfs read path
- `lsm6dsox_trigger_handler()` — the high-speed buffered streaming path

**Step 3 — Understand the build system**
Read [`TUTORIAL.md`](./TUTORIAL.md) section 1 (Kbuild) and section 0 (Prerequisites).
Then set up your environment and compile with `make`.

**Step 4 — Test the driver step by step**
Follow [`command_list.txt`](./command_list.txt).
Each command now includes an "Expected output" comment so you know immediately
whether each step succeeded.

**Step 5 — Visualize the architecture**
Open [`slides.pdf`](./slides.pdf) for sequence diagrams of:
- The probe function call sequence
- The raw data read path (sysfs → driver → I2C → sensor)
- The triggered buffer data flow

**Step 6 — Read the full technical report**
[`report_lsm6dsox_en.pdf`](./report_lsm6dsox_en.pdf) covers the design decisions,
comparison with the official kernel driver, and lessons learned.

## Project Structure

- `lsm6dsox_driver.c`: The main driver source code (IIO + Regmap).
- `lsm6dsox.h`: Register definitions for the LSM6DSOX sensor.
- `command_list.txt`: A quick-reference cheat sheet for testing the driver in a virtual machine.
- `report_lsm6dsox_en.pdf`: The detailed technical report (LaTeX).
- `slides.pdf`: The project presentation with sequence diagrams.

## Key Features

- **IIO Direct Mode**: Real-time reading of raw and scaled data via `sysfs`.
- **Triggered Buffers**: High-speed binary streaming using `kfifo`.
- **Regmap Abstraction**: Clean hardware-software interfacing with built-in locking.
- **Managed Resources (`devm`)**: Robust memory and resource management.

## Ideas for Extension

This driver is intentionally minimal. If you want to go further, here are concrete directions:

- **Runtime ODR/FS configuration:** Expose `in_accel_sampling_frequency` as a writable
  sysfs attribute using `iio_info.write_raw`. The sensor supports ODRs from 12.5 Hz to 6.66 kHz.
- **Hardware FIFO:** The LSM6DSOX has an on-chip 3 kB FIFO. Using watermark interrupts
  instead of per-sample triggers is significantly more power-efficient. See the official
  `st_lsm6dsx` driver for reference.
- **Temperature sensor:** Register `OUT_TEMP_L` (0x20) provides ambient temperature.
  Adding an `IIO_TEMP` channel requires only a new entry in `lsm6dsox_channels[]`
  and a new case in `lsm6dsox_read_raw()`.
- **SPI support:** Regmap makes this straightforward. Add `devm_regmap_init_spi()`,
  a new `spi_driver` struct, and a new `MODULE_DEVICE_TABLE(spi, ...)`.
- **Device Tree binding:** Replace the manual `new_device` instantiation with a proper
  DT node, making the driver usable on embedded boards (Raspberry Pi, BeagleBone, etc.).

---
Developed by: Giacomo Di Clerico & Lorenzo D'Ortona
Politecnico di Milano - A.Y. 2025/2026
