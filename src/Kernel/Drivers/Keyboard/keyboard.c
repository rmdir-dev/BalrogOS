#include "BalrogOS/Drivers/Keyboard/keyboard.h"
#include "BalrogOS/CPU/Interrupts/irq.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Ports/ports.h"

/**
 * @brief enable the keyboard found in keyboard.asm
 * 
 */
extern void _EnableKeyboard();

struct input_event key_event = { 0, 0, 0 };

/**
 * @brief the keyboard interrupt handler
 * 
 * @param stack_frame the content of the interrupt stack frame.
 * @return interrupt_regs* return the stack_frame
 */
static interrupt_regs* keyboard_int_handler(interrupt_regs* stack_frame)
{
    if(in_byte(0x64) & 1)
    {
        unsigned char key = in_byte(0x60);
        key_event.type = EV_KEY;
        key_event.code = key % 128;
        key_event.value = key < 128 ? 1 : 0;
    }
    irq_end(INT_IRQ_1);
    return stack_frame;
}

void init_keyboard()
{
    register_interrupt_handler(INT_IRQ_1, keyboard_int_handler);

    irq_pic_toggle_mask_bit(INT_IRQ_1);
}

void keyboard_read(struct input_event* event)
{
    event->code = key_event.code;
    event->type = key_event.type;
    event->value = key_event.value;
}