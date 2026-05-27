# LSM6DSOX IIO Driver - Educational Tutorial

This guide explains how to compile, load, and interact with the LSM6DSOX Linux kernel driver. It is designed to be a starting point for students interested in Industrial I/O (IIO) and Linux kernel development.

---

## 0. Prerequisites and Environment Setup

Before starting with the compilation, ensure your environment is correctly configured and the hardware is reachable.

### 0.1 Required packages
To compile and test the driver, you need several system tools:
```bash
sudo apt install build-essential linux-headers-$(uname -r) i2c-tools bc
```
* **build-essential**: provides `gcc` and `make`, the core tools for C development.
* **linux-headers-$(uname -r)**: provides the kernel header files needed by Kbuild to compile a module against your currently running kernel version.
* **i2c-tools**: provides `i2cdetect` and other utilities to verify the sensor is correctly detected on the I2C bus.
* **bc**: an arbitrary precision calculator used in scripts to convert raw sensor values (like 16-bit integers) to physical units (like m/s²).

### 0.2 Verifying hrtimer support
The software trigger used in this project (`iio-trig-hrtimer`) is a loadable kernel module that provides high-resolution timing for periodic sampling. It may not be present or enabled on all distributions.

Check if the module is available:
```bash
modinfo iio-trig-hrtimer
```
* **Expected output**: A block of module information (filename, license, description).
* **Failure**: `modinfo: ERROR: Module iio-trig-hrtimer not found`.

This project was tested on **Debian 13 (Trixie)**. Older kernels (e.g., the default kernel in Ubuntu 22.04) may lack this module or require a manual kernel recompilation.

### 0.3 Verifying MCP2221 USB-to-I2C bridge
The MCP2221A is the USB adapter used to bridge your PC's USB port to the sensor's I2C pins.

1. **Check USB Connection**:
   ```bash
   lsusb | grep -i microchip
   ```
   Expect an entry like: `Bus ... Device ... ID 04d8:00dd Microchip Technology, Inc. MCP2221 USB-I2C/UART Combo`.

2. **Check I2C Bus**:
   ```bash
   ls /dev/i2c-*
   ```
   You should see at least `/dev/i2c-0` and `/dev/i2c-1`. The bridge usually appears as the highest numbered index.

3. **Scan the Bus**:
   ```bash
   sudo i2cdetect -y 1
   ```
   (Replace `1` with the correct bus number from the previous step if necessary).
   Expect to see address **0x6a** (the LSM6DSOX default) appearing as `6a` in the grid. If it does not appear, check your wiring: the **SDO** pin must be pulled to GND for address 0x6a, or the cables might be loose.

### 0.4 Project file structure
The repository is organized as follows:
```text
.
├── lsm6dsox_driver.c   # Main driver source: IIO registration and Regmap logic
├── lsm6dsox.h          # Hardware register definitions and bitmasks
├── Makefile            # Kbuild configuration to automate compilation
├── command_list.txt    # Step-by-step shell commands for testing and validation
├── TUTORIAL.md         # This file: compilation guide and architecture explanation
├── README.md           # Project overview and navigation guide
└── report_lsm6dsox_en.pdf  # Full technical report with deep-dives into the code
```

---

## 1. Demystifying Compilation (The Kbuild System)

Compiling a Linux Kernel Module (LKM) is different from a standard C program. We don't use `gcc` directly; instead, we use the **Kbuild system**.

### The Makefile Logic
Look at our `Makefile`. The most important line is:
```makefile
$(MAKE) -C $(KDIR) M=$(PWD) modules
```
- `-C $(KDIR)`: Changes the directory to the kernel source/headers. The kernel's own Makefile contains the rules for building modules.
- `M=$(PWD)`: Tells the kernel Makefile to return to our current folder to find the source code.
- `obj-m`: Tells Kbuild that we want to create a module (`.ko`) from the object file.

### Compilation Artifacts
After running `make`, you will see several files:
- `lsm6dsox_driver.o`: The intermediate object file.
- `lsm6dsox_driver.mod`: Metadata for the module.
- **`lsm6dsox_driver.ko`**: The actual **Kernel Object**. This is the driver you load into the kernel.

### 1.3 Why You Cannot Use `gcc` Directly

It is tempting to try:
```bash
gcc -o lsm6dsox_driver.ko lsm6dsox_driver.c   # This will NOT work
```
This fails for several fundamental reasons:

1. **Kernel headers vs. libc:** Kernel modules cannot include standard C library headers
   (`<stdio.h>`, `<stdlib.h>`). They use kernel-internal headers only (`<linux/module.h>`,
   `<linux/i2c.h>`, etc.). The `gcc` invocation above would not find these headers.

2. **Symbol versioning (Module.symvers):** The kernel exports symbols (functions, globals)
   to modules via a versioned symbol table. Each exported symbol has a CRC checksum computed
   at kernel build time. When you load a `.ko`, the kernel verifies that the CRC of each
   symbol the module uses matches the running kernel's table. If you compile outside Kbuild,
   this table is not consulted and the module will be rejected with `version magic mismatch`.

3. **Kernel ABI:** The kernel is compiled with specific flags (`-ffreestanding`, `-mno-red-zone`,
   `-fno-stack-protector`, etc.) that disable features incompatible with kernel execution context.
   Kbuild applies all of these automatically.

The Kbuild system exists precisely to handle all of this transparently.

### 1.4 What Is Inside the `.ko` File?

A `.ko` file is an ELF (Executable and Linkable Format) relocatable object, similar to a
`.o` file but with additional ELF sections that the kernel module loader uses:

| ELF Section       | Purpose                                                     |
|-------------------|-------------------------------------------------------------|
| `.text`           | Compiled code (your functions: probe, read_raw, etc.)       |
| `.rodata`         | Read-only data (strings, const structs like regmap_config)  |
| `__ksymtab`       | Symbols this module exports to other modules (if any)       |
| `__modinfo`       | Module metadata: author, license, description, vermagic     |
| `.gnu.linkonce.*` | Device table entries (`MODULE_DEVICE_TABLE`)                |

You can inspect these with standard tools:
```bash
# See all ELF sections
readelf -S lsm6dsox_driver.ko

# See module metadata (author, license, kernel version it was built for)
modinfo lsm6dsox_driver.ko

# See all symbols (functions) defined in the module
nm lsm6dsox_driver.ko | grep " T "
```

The `vermagic` field in `modinfo` output is critical: it must exactly match the running
kernel's version string (`uname -r`). This is the first check `insmod` performs.

### 1.5 Verify the Module Before Loading

Before running `insmod`, it is good practice to verify the module:

```bash
# 1. Check that the module was built for the running kernel
modinfo lsm6dsox_driver.ko | grep vermagic
uname -r
# These must match.

# 2. Check module dependencies (should be empty for this driver,
#    as all dependencies are built into the kernel or loaded separately)
modinfo lsm6dsox_driver.ko | grep depends

# 3. Check the module is not already loaded
lsmod | grep lsm6dsox
# Empty output = not loaded. If it appears, rmmod it first.

# 4. Dry-run symbol resolution (checks if all required symbols are available)
sudo modprobe --dry-run --verbose lsm6dsox_driver.ko 2>&1 | head -20
```


---

## 2. Driver Architecture: From Hardware to User-space

The driver follows the **IIO (Industrial I/O)** framework. Here is how data flows:

### A. Initialization (Probe)
When the driver is loaded (`insmod`), the `lsm6dsox_probe` function:
1. Initializes **Regmap** (abstracts the I2C bus).
2. Verifies the hardware (`WHO_AM_I` check).
3. Configures the sensor (ODR, Full Scale).
4. Registers the device with the IIO subsystem.

> 📍 **Code Reference — `lsm6dsox_driver.c`**
> The entire probe sequence lives in `lsm6dsox_probe()`. Follow it top to bottom:
> - `devm_regmap_init_i2c()` — initializes the regmap abstraction over the I2C bus. The `lsm6dsox_regmap_config` struct (defined just above probe) sets register/value width to 8 bits and the max addressable register to 0x7F.
> - `regmap_read(regmap, LSM6DSOX_REG_WHO_AM_I, &val)` — reads register 0x0F and compares it to 0x6C. If it doesn't match, probe returns `-ENODEV` immediately.
> - `devm_iio_device_alloc()` — allocates the IIO device struct plus private data (`lsm6dsox_data`) in one managed allocation.
> - `regmap_write(regmap, LSM6DSOX_REG_CTRL1_XL, ...)` and `CTRL2_G` — power on accelerometer and gyroscope at 104 Hz. These values come from `lsm6dsox.h`.
> - `devm_iio_triggered_buffer_setup()` — wires up the kfifo buffer and registers `lsm6dsox_trigger_handler` as the callback.
> - `devm_iio_device_register()` — the final call; after this the device appears under `/sys/bus/iio/devices/`.

### B. Direct Mode (On-demand reading)
When you `cat` a file in `/sys/bus/iio/devices/iio:device0/in_accel_x_raw`:
1. The kernel calls `lsm6dsox_read_raw`.
2. The driver performs a single I2C read.
3. The raw value is returned to user-space.

> 📍 **Code Reference — `lsm6dsox_driver.c`**
> The on-demand read path is entirely in `lsm6dsox_read_raw()`:
> - The function receives a `mask` argument from the IIO core. The `switch(mask)` handles two cases:
>   - `IIO_CHAN_INFO_RAW`: calls `regmap_bulk_read()` with `chan->address` (the register address stored in the channel spec, defined via the `LSM6DSOX_CHAN_ACCEL` / `LSM6DSOX_CHAN_GYRO` macros). Converts the result with `le16_to_cpu()` and returns `IIO_VAL_INT`.
>   - `IIO_CHAN_INFO_SCALE`: returns hardcoded constants. No I2C transaction occurs. Returns `IIO_VAL_INT_PLUS_NANO`, meaning the physical value is `val + val2/1e9`.
> - The channel addresses (e.g. `LSM6DSOX_REG_OUTX_L_A = 0x28`) are defined in `lsm6dsox.h` and assigned in the `lsm6dsox_channels[]` array.

### C. Triggered Buffer (High-speed streaming)
For continuous data:
1. A **trigger** (like a software timer or hardware interrupt) fires.
2. The `lsm6dsox_trigger_handler` is called.
3. The driver reads all 6 axes in a **bulk burst** (faster than individual reads).
4. Data is pushed into a **kfifo** (a kernel-space buffer).
5. User-space reads the binary stream from `/dev/iio:device0`.

> 📍 **Code Reference — `lsm6dsox_driver.c`**
> The high-speed path is in `lsm6dsox_trigger_handler()`:
> - The `scan` local struct is critical: it contains `s16 channels[6]` (12 bytes for all axes) and `s64 ts __aligned(8)` for the timestamp. The `__aligned(8)` is mandatory — the IIO core requires the timestamp field to be 8-byte aligned.
> - `regmap_bulk_read(data->regmap, LSM6DSOX_REG_OUTX_L_G, scan.channels, 12)` — reads all 6 axes in a single I2C burst starting from the Gyro X low register (0x22). This works because the sensor's output registers are contiguous in memory.
> - `iio_push_to_buffers_with_timestamp()` — pushes the scan struct into the kfifo and adds a kernel timestamp. User-space reads this via `/dev/iio:device0`.
> - `iio_trigger_notify_done()` — mandatory final call to release the trigger and allow the next event to fire.

---

## 3. Practical Commands (Testing Workflow)

Refer to `command_list.txt` for the full list. Here is the "Golden Path":

### Load and Instantiate
```bash
sudo insmod lsm6dsox_driver.ko
# Manually tell the I2C bus about the sensor (Address 0x6a)
echo lsm6dsox 0x6a | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
```

### Read Raw Data
```bash
# Get raw acceleration and apply scale manually
RAW=$(cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw)
SCALE=$(cat /sys/bus/iio/devices/iio:device0/in_accel_scale)
echo "Accel X: $(echo "$RAW * $SCALE" | bc) m/s^2"
```

### Enable Streaming
```bash
# 1. Attach a software trigger
echo finto | sudo tee /sys/bus/iio/devices/iio:device0/trigger/current_trigger
# 2. Enable channels (Accel X and Gyro Z)
echo 1 | sudo tee /sys/bus/iio/devices/iio:device0/scan_elements/in_accel_x_en
echo 1 | sudo tee /sys/bus/iio/devices/iio:device0/scan_elements/in_anglvel_z_en
# 3. Start the buffer
echo 1 | sudo tee /sys/bus/iio/devices/iio:device0/buffer/enable
# 4. View the binary data
sudo cat /dev/iio:device0 | hexdump -C
```

### 3.1 Understanding the hexdump Output

When you run `sudo cat /dev/iio:device0 | hexdump -C`, the terminal will print a continuous stream of binary lines. Each line represents one sample captured by the trigger handler.

**Sample output (20 bytes per line with hexdump -C formatting):**
```text
00000000  f2 ff d4 01 3a 00 12 34  56 ff a1 00 00 00 00 00  |....:.4V.......|
00000010  a8 3b 2d 17 94 54 17 18  00 00 00 00              |.;-..T......    |
```

**Packet layout (the driver always reads all 6 axes in one burst):**

| Bytes  | Field      | Type      | Register origin         |
|--------|------------|-----------|-------------------------|
| 0–1    | Gyro X     | s16 LE    | OUTX_L_G (0x22)        |
| 2–3    | Gyro Y     | s16 LE    | OUTY_L_G (0x24)        |
| 4–5    | Gyro Z     | s16 LE    | OUTZ_L_G (0x26)        |
| 6–7    | Accel X    | s16 LE    | OUTX_L_A (0x28)        |
| 8–9    | Accel Y    | s16 LE    | OUTY_L_A (0x2A)        |
| 10–11  | Accel Z    | s16 LE    | OUTZ_L_A (0x2C)        |
| 12–19  | Timestamp  | s64 LE    | Kernel clock (ns)       |

**Key concepts:**
- **Little Endian (LE):** the low byte comes first. So `f2 ff` = 0xFFF2 = -14 in signed 16-bit.
- **s16:** signed 16-bit integer. Range: -32768 to +32767.
- **Timestamp:** 8 bytes, nanoseconds since boot. The 4 zero-bytes before it are alignment padding inserted by the compiler to satisfy the `__aligned(8)` requirement in the driver.

**Converting a raw value to physical units (command line):**
```bash
# Example: Gyro X raw bytes are f2 ff -> little-endian s16 = -14
# Gyro scale = 0.000152716 rad/s per LSB
RAW=-14
SCALE=0.000152716
echo "Gyro X: $(echo "$RAW * $SCALE" | bc -l) rad/s"
# Output: Gyro X: -.00213802 rad/s

# Example: Accel Z raw bytes are a1 00 -> little-endian s16 = 161
# Accel scale = 0.000598205 m/s^2 per LSB (≈ 1g when stationary, expect ~9.8 m/s^2)
RAW=161
SCALE=0.000598205
echo "Accel Z: $(echo "$RAW * $SCALE" | bc -l) m/s^2"
# Output: Accel Z: .09631100 m/s^2
```

**Note on enabled channels:** the `scan_elements` sysfs interface lets you enable only specific channels (e.g., only Accel X and Gyro Z). When channels are disabled, the driver still reads all 6 axes in the burst (see `lsm6dsox_trigger_handler`), but the IIO core will only include enabled channels in the packet sent to user-space. Always check which channels are active with:
```bash
grep -r "" /sys/bus/iio/devices/iio:device0/scan_elements/
```
---

## 4. Key Takeaways for Students
- **Resource Management**: We use `devm_*` functions. These are "managed" resources; the kernel automatically frees them if the probe fails or the driver is removed, preventing memory leaks.
- **Endianness**: Sensors usually use **Little Endian**. We use `le16_to_cpu` to ensure the data is correct regardless of the host CPU architecture.
- **Regmap**: Instead of raw I2C calls, we use Regmap. It handles locking (preventing race conditions) and makes the driver easier to port to SPI in the future.

---

## 5. Troubleshooting — Common Errors and Fixes

This section documents the most common failure modes encountered during development and testing.

### 5.1 Driver loading errors

**Symptom:** `insmod: ERROR: could not insert module lsm6dsox_driver.ko: File exists`
**Cause:** The module (or a previous version) is already loaded in the kernel.
**Fix:**
```bash
sudo rmmod lsm6dsox_driver
sudo insmod /path/to/lsm6dsox_driver.ko
```

**Symptom:** `insmod: ERROR: could not insert module: Invalid module format`
**Cause:** The module was compiled against a different kernel version than the one running.
**Fix:** Recompile. Run `make clean && make` after a kernel update or when switching machines.
Verify the kernel version match with: `uname -r` vs `modinfo lsm6dsox_driver.ko | grep vermagic`

---

### 5.2 Device not appearing in sysfs

**Symptom:** `/sys/bus/iio/devices/iio:device0/` does not exist after `new_device`
**Cause:** The probe function failed. The most common reason is a wrong WHO_AM_I value.
**Diagnosis:**
```bash
dmesg | tail -10
```
Look for: `Invalid WHO_AM_I value: 0xXX` — this means the I2C read succeeded but
returned the wrong device ID.
**Cause A:** The sensor address is wrong. Verify with `sudo i2cdetect -y 1`. The sensor
should appear at 0x6a (SDO pulled to GND) or 0x6b (SDO pulled to VDD).
**Cause B:** The MCP2221 is not on bus 1. Try `i2c-0`: change `i2c-1` to `i2c-0`
in both the `new_device` and `delete_device` commands.

---

### 5.3 Triggered buffer / streaming issues

**Symptom:** `write error: Invalid argument` when attaching the trigger
```bash
echo finto | sudo tee /sys/bus/iio/devices/iio:device0/trigger/current_trigger
```
**Cause:** The hrtimer trigger named `finto` was not created.
**Fix:** Ensure configfs is mounted and the mkdir command was run:
```bash
ls /sys/kernel/config/iio/triggers/hrtimer/   # should show 'finto'
# If empty:
sudo modprobe iio-trig-hrtimer
sudo mkdir -p /sys/kernel/config/iio/triggers/hrtimer/finto
```

**Symptom:** Buffer enabled (returns 1) but `hexdump` shows no output
**Cause:** The trigger is not firing, or no channels are enabled.
**Fix:** Verify both:
```bash
# Check trigger is attached
cat /sys/bus/iio/devices/iio:device0/trigger/current_trigger  # should print: finto
# Check at least one channel is enabled
cat /sys/bus/iio/devices/iio:device0/scan_elements/in_accel_x_en  # should print: 1
```

---

### 5.4 Permission errors

**Symptom:** `Permission denied` when reading sysfs attributes
**Fix:** Use `sudo cat` or configure a udev rule. For quick testing, `sudo` is sufficient.

**Symptom:** virtiofs shared directory not writable from the VM guest
**Cause:** The virtiofs mount options did not propagate the host user's UID/GID.
**Fix:** On the host, ensure the shared directory is owned by the user that the guest
maps to. Alternatively, copy the `.ko` file into the guest's local filesystem with `scp`
or via a shared clipboard mechanism.

---

### 5.5 No hardware interrupt (expected behavior in VM)

**Symptom:** `dmesg` shows:
`lsm6dsox 1-006a: No HW IRQ found. Polling/Sysfs trigger mode only.`
**Cause:** This is **expected** in a virtualized environment. The MCP2221A USB bridge
does not route the sensor's INT1 pin as a Linux IRQ. The driver detects this (`client->irq <= 0`)
and skips the hardware interrupt setup, falling back to software triggers (hrtimer).
**Action required:** None. Use the hrtimer trigger as described in Step 6.
