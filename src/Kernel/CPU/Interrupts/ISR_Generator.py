print('; Interupts service routines.')
print('section .text')
print('''
    extern isr_common
    ''')
for i in range(255):
    print('''isr{0}:
    cli
    {1}
    push {0}
    jmp isr_common
    '''.format(i, 'push 0' if i not in [8, 10, 11, 12, 13, 14, 17] else 'nop'))

print('''
; ISR VECTOR
section .data
    global isr_table
isr_table:
''')
for i in range(255):
    print('     dq isr{}'.format(i))