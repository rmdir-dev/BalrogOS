#include "BalrogOS/CPU/PIT/pit.h"
#include "BalrogOS/CPU/Ports/ports.h"

void init_pit(uint32_t frequency)
{
    // calculate the divisor to get the correct frequency
    // 1193180 is the tick rate
    uint32_t divisor = 1193180 / frequency;

    /* Send mode command :
            - 0b00110110 = 0x36
            0b
            00  CHANNEL (using chanel 0)
            11  ACCESS MODE (send low byte then hight byte)
            011 OPERATING MODE (MODE 3 square wave generator)
            0   BINARY MODE (16 bit binary)
    */
    out_byte(0x43, 0x36);

    // Set the low and high byte to be sent to the PIT
    uint8_t low = (uint8_t)(divisor & 0xff);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xff);

    // Send the low then the high byte to the PIT
    out_byte(0x40, low);
    out_byte(0x40, high);
}