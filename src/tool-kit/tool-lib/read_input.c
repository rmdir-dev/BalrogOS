//
// Created by rmdir on 10/20/23.
//

#include "toolkit/tool_read_keyboard.h"

static uint8_t keys[255] = {};
static char* _qwertyuiop = "qwertyuiop[]QWERTYUIOP{}";
static char* _asdfghjkl = "asdfghjkl;'\\ASDFGHJKL:\"|";
static char* _zxcvbnm = "zxcvbnm,./ZXCVBNM<>?";
static char* _num = "1234567890-=!@#$%^&*()_+";
static int shift = 0;
static int ctrl = 0;
struct input_event prev_key_ev = {-1, -1, -1 };
static int cursor_blink_counter = -1;
char* cursor = NULL;
char* cursor_color = "97";

int process_input(struct input_event* input, char* buffer, uint32_t* buf_idx, uint8_t print,
        int (*manage_special_keys)(uint16_t special, uint16_t pressed, uint8_t* ky)) {
    if(
        print && cursor
        && input->code == prev_key_ev.code
        && input->type == prev_key_ev.type
        && input->value == prev_key_ev.value
    )
    {
        if(cursor_blink_counter == 0) {
            puts(cursor);
            cursor_blink_counter++;
        } else if(cursor_blink_counter == 1000000 / 2) {
            puts("\b");
            cursor_blink_counter++;
        } else if(cursor_blink_counter > 1000000) {
            cursor_blink_counter = 0;
        } else {
            cursor_blink_counter++;
        }
    } else if(print && cursor) {
        if(cursor_blink_counter < 1000000 / 2 && cursor_blink_counter > 0) {
            puts("\b");
        }
        cursor_blink_counter = -1;
    }

    if(input->type == EV_KEY)
    {
        if(input->value && keys[input->code] != input->value)
        {
            shift = keys[KEY_RIGHTSHIFT] || keys[KEY_LEFTSHIFT];

            if((keys[KEY_LEFTCTRL] || keys[KEY_RIGHTCTRL]))
            {
                if(manage_special_keys) {
                    return manage_special_keys(KEY_RIGHTCTRL, input->code, keys);
                }
                return 0;
            }

            if((keys[KEY_LEFTALT] || keys[KEY_RIGHTALT]))
            {
                if(manage_special_keys) {
                    return manage_special_keys(KEY_RIGHTALT, input->code, keys);
                }
                return 0;
            }

            if(!buffer) {
                keys[input->code] = input->value;
                return 0;
            }

            uint8_t bckspace = 0;
            if(input->code == KEY_ENTER)
            {
                buffer[*buf_idx] = 0;
                return -1;
            } else
            if(input->code == KEY_SPACE)
            {
                buffer[*buf_idx] = ' ';
            } else
            if(input->code == KEY_BACKSPACE)
            {
                bckspace = 1;
                if(*buf_idx > 0)
                {
                    buffer[*buf_idx - 1] = 0;
                    buffer[*buf_idx] = '\b';
                } else
                {
                    buffer[*buf_idx] = 0;
                }
            } else
            if(input->code >= KEY_1 && input->code <= KEY_0 + 2)
            {
                buffer[*buf_idx] = _num[(input->code - KEY_1) + (shift * 12)];
            } else
            if (input->code >= KEY_Q && input->code <= KEY_P + 2)
            {
                buffer[*buf_idx] = _qwertyuiop[(input->code - KEY_Q) + (shift * 12)];
            } else
            if (input->code >= KEY_A && input->code <= KEY_L + 3)
            {
                buffer[*buf_idx] = _asdfghjkl[(input->code - KEY_A) + (shift * 12)];
            } else
            if (input->code >= KEY_Z && input->code <= KEY_M + 3)
            {
                buffer[*buf_idx] = _zxcvbnm[(input->code - KEY_Z) + (shift * 10)];
            } else
                // up, down, left, right
            if (input->code == 72 || input->code == 80 || input->code == 75 || input->code == 77)
            {
                keys[input->code] = input->value;
                if(manage_special_keys) {
                    return manage_special_keys(input->code, 0, keys);
                }
            }
            // for testing arrow key code values
//            printf("arrow key pressed : %d\n", input->code);

            if(buffer[*buf_idx] != 0)
            {
                if(print) {
                    putchar(buffer[*buf_idx]);
                }
                if(bckspace == 0)
                {
                    (*buf_idx)++;
                } else
                {
                    (*buf_idx)--;
                }
            }
        }

        prev_key_ev.code = input->code;
        prev_key_ev.type = input->type;
        prev_key_ev.value = input->value;

        keys[input->code] = input->value;
    }
    return 0;
}
