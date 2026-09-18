#include "balrog_os/drivers/keyboard/keyboard.h"

#include <stdint.h>
#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/debug/debug_output.h"

/**
 * @brief enable the keyboard found in keyboard.asm
 * 
 */
extern void _EnableKeyboard();

// Key event queue
struct input_event key_eq[KEYBOARD_QUEUE_SIZE] = {};
static size_t key_eq_head = 0;
static size_t key_eq_tail = 0;
static size_t key_dropped = 0;

static void __keyboard_push_event(uint8_t key)
{
    size_t next = (key_eq_head + 1) % KEYBOARD_QUEUE_SIZE;

    if (next == key_eq_tail)
    {
        if (key_dropped++ == 0)
        {
            kernel_debug_output(KDB_LVL_ERROR, "keyboard : queue is full");
        }
        return;
    }

    key_eq[key_eq_head].type = EV_KEY;
    key_eq[key_eq_head].code = key % 128;
    key_eq[key_eq_head].value = key < 128 ? 1 : 0;

    key_eq_head = next;
}

/**
 * @brief the keyboard interrupt handler
 * 
 * @param stack_frame the content of the interrupt stack frame.
 * @return interrupt_regs* return the stack_frame
 */
static interrupt_regs* keyboard_int_handler(interrupt_regs* stack_frame)
{
    uint8_t status = in_byte(0x64);

    for(int i = 0; (status & 1) && i < 32; i++)
    {
        uint8_t key = in_byte(0x60);

        if(!(status & 0x20))
        {
            // 0xe0 is an extended code.
            if (key != 0xe0)
            {
                __keyboard_push_event(key);
            }

            kernel_debug_output(KDB_LVL_VERBOSE, "keyboard : scan code 0%x, %s",
                    key, key < 128 ? "pressed" : "released");
        } else
        {
            /*  a byte from the touchpad. nobody reads it, and a keyboard that
                stops answering usually stopped on one of these.  */
            kernel_debug_output(KDB_LVL_VERBOSE, "keyboard : 0%x came from the auxiliary port, dropped", key);
        }

        status = in_byte(0x64);
    }

    irq_end(INT_IRQ_1);
    return stack_frame;
}

int init_keyboard()
{
    register_interrupt_handler(INT_IRQ_1, keyboard_int_handler);

    irq_pic_unmask(INT_IRQ_1);

    kernel_debug_output(KDB_LVL_INFO, "keyboard : irq %d unmasked, 8042 status 0%x",
            INT_IRQ_1, in_byte(0x64));

    return 0;
}

int keyboard_read(struct input_event* event)
{
    if (key_eq_head == key_eq_tail)
    {
        return 0;
    }

    *event = key_eq[key_eq_tail];
    key_eq_tail = (key_eq_tail + 1) % KEYBOARD_QUEUE_SIZE;

    return 1;
}