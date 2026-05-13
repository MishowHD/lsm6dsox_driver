/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LSM6DSOX_H
#define LSM6DSOX_H

#define LSM6DSOX_REG_INT1_CTRL 0x0D
#define LSM6DSOX_INT1_DRDY_XL  BIT(0)
#define LSM6DSOX_INT1_DRDY_G   BIT(1)

#define LSM6DSOX_REG_WHO_AM_I      0x0F
#define LSM6DSOX_WHO_AM_I_VAL      0x6C

#define LSM6DSOX_REG_CTRL1_XL      0x10
#define LSM6DSOX_CTRL1_XL_104HZ    0x40

#define LSM6DSOX_REG_CTRL2_G       0x11
#define LSM6DSOX_CTRL2_G_104HZ     0x40

#define LSM6DSOX_REG_CTRL3_C       0x12
#define LSM6DSOX_CTRL3_C_BDU       0x44 /* BDU + IF_INC */

#define LSM6DSOX_REG_OUTX_L_G      0x22
#define LSM6DSOX_REG_OUTY_L_G      0x24
#define LSM6DSOX_REG_OUTZ_L_G      0x26

#define LSM6DSOX_REG_OUTX_L_A      0x28
#define LSM6DSOX_REG_OUTY_L_A      0x2A
#define LSM6DSOX_REG_OUTZ_L_A      0x2C

#endif /* LSM6DSOX_H */
