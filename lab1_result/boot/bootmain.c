#include <defs.h>
#include <x86.h>
#include <elf.h>

/* *********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(bootasm.S and bootmain.c) is the bootloader.
 *    It should be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in bootasm.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 * */
unsigned int    SECTSIZE  =      512 ;
struct elfhdr * ELFHDR    =      ((struct elfhdr *)0x10000) ;     // scratch space

/* waitdisk - wait for disk ready */
/*首先是waitdisk函数，该函数的作用是连续不断地从0x1F7地址读取磁盘的状态，直到磁盘不忙为止；*/
static void
waitdisk(void) {
    while ((inb(0x1F7) & 0xC0) != 0x40)
        /* do nothing */;
}

/* readsect - read a single sector at @secno into @dst */
/*接下来是readsect函数，其基本功能为读取一个磁盘扇区，关于具体代码的含义如下代码中注释所示：*/
static void
readsect(void *dst, uint32_t secno) {
    waitdisk(); // 等待磁盘到不忙为止

    outb(0x1F2, 1);             // 往0X1F2地址中写入要读取的扇区数，由于此处需要读一个扇区，因此参数为1
    outb(0x1F3, secno & 0xFF); // 输入LBA参数的0...7位；
    outb(0x1F4, (secno >> 8) & 0xFF); // 输入LBA参数的8-15位；
    outb(0x1F5, (secno >> 16) & 0xFF); // 输入LBA参数的16-23位；
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0); // 输入LBA参数的24-27位（对应到0-3位），第四位为0表示从主盘读取，其余位被强制置为1；
    outb(0x1F7, 0x20);                      // 向磁盘发出读命令0x20

    waitdisk(); // 等待磁盘直到不忙

    insl(0x1F0, dst, SECTSIZE / 4); // 从数据端口0x1F0读取数据，除以4是因为此处是以4个字节为单位的，这个从指令是以l(long)结尾这点可以推测出来；
}

/*根据上述代码，不妨将读取磁盘扇区的过程总结如下：

等待磁盘直到其不忙；
往0x1F2到0X1F6中设置读取扇区需要的参数，包括读取扇区的数量以及LBA参数；
往0x1F7端口发送读命令0X20；
等待磁盘完成读取操作；
从数据端口0X1F0读取出数据到指定内存中；

作者：AmadeusChan
链接：https://www.jianshu.com/p/2f95d38afa1d
*/

/* *
 * readseg - read @count bytes at @offset from kernel into virtual address @va,
 * might copy more than asked.
 * 在bootmain.c中还有另外一个与读取磁盘相关的函数readseg，
 * 其功能为将readsect进行进一步封装，提供能够从磁盘第二个扇区起（kernel起始位置）offset个位置处，
 * 读取count个字节到指定内存中，由于上述readsect函数只能就整个扇区进行读取，因此在readseg中，
 * 不得不连不完全包括了指定数据的首尾扇区内容也要一起读取进来，此处还有一个小技巧就是将va减去了一个offset%512 Byte的偏移量，
 * 这使得就算是整个整个扇区读取，也可以使得要求的读取到的数据在内存中的起始位置恰好是指定的原始的va；
 * */
static void
readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count;

    // round down to sector boundary
    va -= offset % SECTSIZE;

    // translate from bytes to sectors; kernel starts at sector 1
    uint32_t secno = (offset / SECTSIZE) + 1;

    // If this is too slow, we could read lots of sectors at a time.
    // We'd write more to memory than asked, but it doesn't matter --
    // we load in increasing order.
    for (; va < end_va; va += SECTSIZE, secno ++) {
        readsect((void *)va, secno);
    }
}





/////////////////////////////////////////////////////////////////////////
/* bootmain - the entry of bootloader */
void
bootmain(void) {
    // read the 1st page off diskbootloader加载ELF格式的OS的代码位于bootmain.c中的bootmain函数中，
    //接下来不妨分析这部分代码来描述加载ELF格式OS的过程：
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);
/*首先，从磁盘的第一个扇区（第零个扇区为bootloader）中读取OS kenerl最开始的4kB代码，
然后判断其最开始四个字节是否等于指定的ELF_MAGIC，用于判断该ELF header是否合法*/
    // is this a valid ELF?
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

/*接下来从ELF头文件中获取program header表的位置，以及该表的入口数目，然后遍历该表的每一项，
且从每一个program header中获取到段应该被加载到内存中的位置（Load Address，虚拟地址），
以及段的大小，然后调用readseg函数将每一个段加载到内存中，至此完成了将OS加载到内存中的操作；
*/
    // load each program segment (ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // call the entry point from the ELF header
    // note: does not returnbootloader所需要完成的最后一个步骤就是从ELF header中查询到OS kernel的入口地址，
    //然后使用函数调用的方式跳转到该地址上去；至此，完整地分析了bootloader加载ELF格式的OS kernel的过程。
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);

    /* do nothing */
    while (1);
}

