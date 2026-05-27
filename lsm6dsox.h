/*
 * lsm6dsox.h - Register map for the ST LSM6DSOX 6-axis IMU (Accel + Gyro)
 *
 * Register addresses and bit definitions are taken from:
 *   ST LSM6DSOX Datasheet, Revision 8 (DocID031160)
 *   Table 18: Register address map (pages 55–56)
 *   Available at: https://www.st.com/resource/en/datasheet/lsm6dsox.pdf
 *
 * Only the registers used by this educational driver are defined here.
 * The full register map contains additional registers for temperature,
 * step counting, Machine Learning Core (MLC), and Finite State Machines (FSM)
 * which are out of scope for this implementation.
 */

#ifndef LSM6DSOX_H
#define LSM6DSOX_H

/* Register to control the physical INT1 pin */
#define LSM6DSOX_REG_INT1_CTRL 0x0D  /* Datasheet Table 18, pg.55 - INT1 pin control */
/* Bit to route Accelerometer Data Ready to INT1 */
#define LSM6DSOX_INT1_DRDY_XL  BIT(0) /* Datasheet Table 64, pg.84 - bit 0: XL DRDY on INT1 */
/* Bit to route Gyroscope Data Ready to INT1 */
#define LSM6DSOX_INT1_DRDY_G   BIT(1) /* Datasheet Table 64, pg.84 - bit 1: G DRDY on INT1 */

#define LSM6DSOX_REG_WHO_AM_I      0x0F   /* Datasheet Table 18, pg.55 - fixed value: 0x6C */
#define LSM6DSOX_WHO_AM_I_VAL      0x6C   /* Datasheet Table 19, pg.57 */

#define LSM6DSOX_REG_CTRL1_XL      0x10   /* Datasheet Table 18, pg.55 - Accel ODR and FS */
#define LSM6DSOX_CTRL1_XL_104HZ    0x40   /* ODR_XL[3:0]=0100 → 104 Hz, FS_XL[1:0]=00 → 2g */

#define LSM6DSOX_REG_CTRL2_G       0x11   /* Datasheet Table 18, pg.55 - Gyro ODR and FS */
#define LSM6DSOX_CTRL2_G_104HZ     0x40   /* ODR_G[3:0]=0100 → 104 Hz, FS_G[1:0]=00 → 250 dps */

#define LSM6DSOX_REG_CTRL3_C       0x12   /* Datasheet Table 18, pg.55 - general device control */
#define LSM6DSOX_CTRL3_C_BDU       0x44   /* bit6=BDU: block data update; bit2=IF_INC: register auto-increment on burst read */

#define LSM6DSOX_REG_OUTX_L_G      0x22   /* Datasheet Table 18, pg.56 - Gyro X output, low byte */
#define LSM6DSOX_REG_OUTY_L_G      0x24   /* Datasheet Table 18, pg.56 - Gyro Y output, low byte */
#define LSM6DSOX_REG_OUTZ_L_G      0x26   /* Datasheet Table 18, pg.56 - Gyro Z output, low byte */

#define LSM6DSOX_REG_OUTX_L_A      0x28   /* Datasheet Table 18, pg.56 - Accel X output, low byte */
#define LSM6DSOX_REG_OUTY_L_A      0x2A   /* Datasheet Table 18, pg.56 - Accel Y output, low byte */
#define LSM6DSOX_REG_OUTZ_L_A      0x2C   /* Datasheet Table 18, pg.56 - Accel Z output, low byte */

#endif /* LSM6DSOX_H */
