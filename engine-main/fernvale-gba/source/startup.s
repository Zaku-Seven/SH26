/* Minimal ARM7TDMI cartridge entry and C runtime startup. */
.syntax unified
.cpu arm7tdmi
.arm
.section .gba_header,"ax",%progbits
.global _start
_start:
    b reset
    .space 188, 0  /* build.py fills the required 192-byte cartridge header */

.section .text.startup,"ax",%progbits
.align 2
reset:
    /* Set a separate interrupt stack, then the normal system stack. */
    mov r0, #0xd2
    msr cpsr_c, r0
    ldr sp, =0x03007fa0
    mov r0, #0xdf
    msr cpsr_c, r0
    ldr sp, =0x03007f00

    /* Copy initialized writable globals from ROM to work RAM. */
    ldr r0, =__data_load
    ldr r1, =__data_start
    ldr r2, =__data_end
1:
    cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 1b

    /* Zero all uninitialized C globals. */
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
2:
    cmp r1, r2
    strlo r3, [r1], #4
    blo 2b

    /* BX supports ARM or Thumb C code depending on the compiler flags. */
    ldr r0, =main
    mov lr, pc
    bx r0
3:
    b 3b
.pool
