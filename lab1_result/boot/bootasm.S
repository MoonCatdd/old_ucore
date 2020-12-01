#include <asm.h>

# Start the CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector
.set CR0_PE_ON,             0x1                     # protected mode enable flag

# start address should be 0:7c00, in real mode, the beginning address of the running bootloader
/*使用x86汇编语言编写，接下来将根据源码分析进入保护模式的过程：
bootloader入口为start, 根据bootloader的相关知识可知，bootloader会被BIOS加载到内存的0x7c00处，
此时cs=0, eip=0x7c00，在刚进入bootloader的时候，
最先执行的操作分别为关闭中断、清除EFLAGS的DF位以及将ax, ds, es, ss寄存器初始化为0；
https://www.jianshu.com/p/2f95d38afa1d
*/
.globl start
start:
.code16                                             # Assemble for 16-bit mode
    cli                                             # Disable interrupts
    cld                                             # String operations increment

    # Set up the important data segment registers (DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero
    movw %ax, %ds                                   # -> Data Segment
    movw %ax, %es                                   # -> Extra Segment
    movw %ax, %ss                                   # -> Stack Segment

    # Enable A20:A20开启，进入保护模式之后可以充分使用4G的寻址能力
    #  For backwards compatibility with the earliest PCs, physical
    #  address line 20 is tied low, so that addresses higher than
    #  1MB wrap around to zero by default. This code undoes this.
    /*接下来为了使得CPU进入保护模式之后能够充分使用32位的寻址能力，需要开启A20，关闭“回卷”机制；
    该过程主要分为等待8042控制器Inpute Buffer为空，发送P2命令到Input Buffer，等待Input Buffer为空，
    将P2得到的第二个位（A20选通）置为1，写回Input Buffer；接下来对应上述步骤分析bootasm中的汇编代码：

首先是从0x64内存地址中（映射到8042的status register）中读取8042的状态，
直到读取到的该字节第二位（input buffer是否有数据）为0，此时input buffer中无数据；
*/
seta20.1:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.1

//接下来往0x64写入0xd1命令，表示修改8042的P2 port；
    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port

seta20.2://接下来继续等待input buffer为空：
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.2

//接下来往0x60端口写入0xDF，表示将P2 port的第二个位（A20）选通置为1
    movb $0xdf, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1

    # Switch from real to protected mode, using a bootstrap GDT
    # and segment translation that makes virtual addresses
    # identical to physical addresses, so that the
    # effective memory map does not change during the switch.
    因此在完成A20开启之后，
    只需要使用命令lgdt gdtdesc即可载入全局描述符表；接下来只需要将cr0寄存器的PE位置1，
    即可从实模式切换到保护模式：
    lgdt gdtdesc
    movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0

    # Jump to next instruction, but in 32-bit code segment.
    # Switches processor into 32-bit mode.
    ljmp $PROT_MODE_CSEG, $protcseg
    接下来则使用一个长跳转指令，将cs修改为32位段寄存器，以及跳转到protcseg这一32位代码入口处，此时CPU进入32位模式：

.code32                                             # Assemble for 32-bit mode
protcseg:
    # Set up the protected-mode data segment registers
    movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment
/*接下来执行的32位代码功能为：设置ds、es, fs, gs, ss这几个段寄存器，
然后初始化栈的frame pointer和stack pointer，然后调用使用C语言编写的bootmain函数，
进行操作系统内核的加载，至此，bootloader已经完成了从实模式进入到保护模式的任务；
*/
    # Set up the stack pointer and call into C. The stack region is from 0--start(0x7c00)
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain

    # If bootmain returns (it shouldn't), loop.
spin:
    jmp spin

# Bootstrap GDT
/*接下来需要设置GDT（全局描述符表），在bootasm.S中已经静态地描述了一个简单的GDT，
如下所示; 值得注意的是GDT中将代码段和数据段的base均设置为了0，而limit设置为了2^32-1即4G，
此时就使得逻辑地址等于线性地址，方便后续对于内存的操作；*/
.p2align 2                                          # force 4 byte alignment
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel

gdtdesc:
    .word 0x17                                      # sizeof(gdt) - 1
    .long gdt                                       # address gdt
