print('; Interupts service routines.')
print('section .text')
print('''
    extern isr_common
    extern schedule
    extern user_mode_print
    ''')

# A couple of errors here :
# 1) 0 - 255 not 0-254
# 2) interrupt 21, 29 and 30 have error code, so no need to push 0 check https://wiki.osdev.org/Exceptions
#    Before only 8, 10, 11, 12, 13, 14, 17 had that behaviour.
for i in range(256):
    print('''isr{0}:
    cli
    {1}
    push {0}
    jmp isr_common
    '''.format(i, 'push 0' if i not in [8, 10, 11, 12, 13, 14, 17, 21, 29, 30] else 'nop'))

print('''
; ISR VECTOR
section .data
    global isr_table
isr_table:
''')
for i in range(256):
    print('     dq isr{}'.format(i))