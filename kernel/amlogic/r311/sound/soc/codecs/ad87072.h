#ifndef _AD87072_H
#define _AD87072_H

#define MUTE                             0x02
#define MVOL                             0x03
#define C1VOL                            0x04
#define C2VOL                            0x05

#define CFADDR                           0x14
#define A1CF1                            0x15
#define A1CF2                            0x16
#define A1CF3                            0x17
#define CFUD                             0x24

#define AD87072_REGISTER_COUNT			46
#define AD87072_RAM_TABLE_COUNT         127
#define AD82586D_DRC_RAM_TABLE_COUNT         0x79

struct ad87072_platform_data {
	int reset_pin;
};

#endif
