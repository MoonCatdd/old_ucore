//首先在Makefile的最开始是对各种常量的初始化：
PROJ	:= challenge
EMPTY	:=
SPACE	:= $(EMPTY) $(EMPTY)
SLASH	:= /

V       := @

# try to infer the correct GCCPREFX
/*接下来部分则用于推断环境中调用所安装的gcc应当使用的命令：
在本部分，如果为定义GCCPREFIX变量，则利用了linux bash中的技巧来推断所使用的gcc命令是什么, 
在本部分首先猜测gcc命令的前缀是i386-elf-，因此执行i386-elf-objdump -i命令，2>&1表示将错误输出一起输出到标准输出里，
然后通过管道的方式传递给下一条bash命令grep '^elf32-i386$$' >/dev/null 2>&1;，>/dev/null这部分表示将标准输出输
出到一个空设备里，而输入上一条命令发送给grep的标准输出（作为grep的输入）中可以匹配到'^elf32-i386$$'的话，
则说明i386-elf-objdump这一命令是存在的，那么条件满足，由echo输出'i386-elf-'，由于是在$()里的bash命令，
这个输出会作为值被赋给GCCPREFIX变量；如果i386-elf-objdump命令不存在，则猜测使用的gcc命令不包含其他前缀，
则继续按照上述方法，测试objdump这条命令是否存在，如果存在则GCCPREFIX为空串，否则之间报错，
要求显示地提供gcc的前缀作为GCCPREFIX变量的数值（可以在环境变量中指定）
*/
ifndef GCCPREFIX
GCCPREFIX := $(shell if i386-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# try to infer the correct QEMU
/*接下来部分与上述方法一致，利用bash命令来推断qemu的命令，因此具体细节不再赘述*/
ifndef QEMU
QEMU := $(shell if which qemu-system-i386 > /dev/null; \
	then echo 'qemu-system-i386'; exit; \
	elif which i386-elf-qemu > /dev/null; \
	then echo 'i386-elf-qemu'; exit; \
	else \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# eliminate default suffix rules
.SUFFIXES: .c .S .h
/*该段Makefile代码中所设计的所有gcc编译选项和链接器ld选项的作用分别如下：

-g：在编译中加入调试信息，便于之后使用gdb进行调试；
-Wall：使能所有编译警告，便于发现潜在的错误；
-O2: 开启O2编译优化；
-fno-builtin: 不承认所有不是以builtin为开头的内建函数；
-ggdb 产生gdb所需要的调试信息（与-g的区别是ggdb的调试信息是专门为gdb而生成的）；
-m32: 32位模式；
-gstabs：以stabs格式输出调试信息，不包括gdb拓展；
-nostdinc: 不搜索默认路径头文件；
-fno-stack-protector: 禁用堆栈保护;
-nostdlib: 该链接器选项表示不链接任何系统标准启动文件和标准库文件，
这是因为编译操作系统内核和bootloader是不需要这些启动文件和库就应该能够执行的；
其他涉及到的bash命令选项为：

mkdir -p: 允许创建嵌套子目录；
touch -c: 不创建已经存在的文件；
rm -f: 无视任何确认提示；
*/
# delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# define compiler and flags

HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

CC		:= $(GCCPREFIX)gcc
CFLAGS	:= -fno-builtin -fno-PIC -Wall -ggdb -m32 -gstabs -nostdinc $(DEFS)
CFLAGS	+= $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
CTYPE	:= c S

LD      := $(GCCPREFIX)ld
LDFLAGS	:= -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)
LDFLAGS	+= -nostdlib

OBJCOPY := $(GCCPREFIX)objcopy
OBJDUMP := $(GCCPREFIX)objdump

COPY	:= cp
MKDIR   := mkdir -p
MV		:= mv
RM		:= rm -f
AWK		:= awk
SED		:= sed
SH		:= sh
TR		:= tr
TOUCH	:= touch -c

OBJDIR	:= obj
BINDIR	:= bin

ALLOBJS	:=
ALLDEPS	:=
TARGETS	:=

include tools/function.mk
//总之以下内容就是一些
listf_cc = $(call listf,$(1),$(CTYPE))

# for cc
add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(3),$(2),$(4))
create_target_cc = $(call create_target,$(1),$(2),$(3),$(CC),$(CFLAGS))

# for hostcc
add_files_host = $(call add_files,$(1),$(HOSTCC),$(HOSTCFLAGS),$(2),$(3))
create_target_host = $(call create_target,$(1),$(2),$(3),$(HOSTCC),$(HOSTCFLAGS))

cgtype = $(patsubst %.$(2),%.$(3),$(1))
objfile = $(call toobj,$(1))
asmfile = $(call cgtype,$(call toobj,$(1)),o,asm)
outfile = $(call cgtype,$(call toobj,$(1)),o,out)
symfile = $(call cgtype,$(call toobj,$(1)),o,sym)

# for match pattern
match = $(shell echo $(2) | $(AWK) '{for(i=1;i<=NF;i++){if(match("$(1)","^"$$(i)"$$")){exit 1;}}}'; echo $$?)

# >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# include kernel/user

INCLUDE	+= libs/

CFLAGS	+= $(addprefix -I,$(INCLUDE))

LIBDIR	+= libs

$(call add_files_cc,$(call listf_cc,$(LIBDIR)),libs,)
/*，含义是寻找libs目录下的所有具有.c, .s后缀的文件，并生成相应的.o文件，放置在obj/libs/文件夹下，
具体生成的文件是printfmt.o, string.o文件，与此同时，该文件夹下还生成了.d文件，
这是Makefile自动生成的依赖文件列表所存放的位置，比如打开string.d文件可以发现，
string.o文件的生成依赖于string.c, string.h, x86.h, defs.h四个文件，
这与我们对于代码的观察是一致的；这部分编译所使用的编译选项保存在CFLAGS变量下，
关于具体每一个使用到的gcc编译选项的含义，将在下文具体分析Makefile中定义CFLAGS变量的部分进行详细描述；
作者：AmadeusChan
链接：https://www.jianshu.com/p/2f95d38afa1d   */

# -------------------------------------------------------------------
# kernel

KINCLUDE	+= kern/debug/ \
			   kern/driver/ \
			   kern/trap/ \
			   kern/mm/

KSRCDIR		+= kern/init \
			   kern/libs \
			   kern/debug \
			   kern/driver \
			   kern/trap \
			   kern/mm

KCFLAGS		+= $(addprefix -I,$(KINCLUDE))

$(call add_files_cc,$(call listf_cc,$(KSRCDIR)),kernel,$(KCFLAGS))
/*这一段代码将用于生成kernel的所有子目录下包含的CTYPE文件（.s, .c文件）所对应的.o文件以及.d文件，
这段代码与上述生成obj/libs/*.o文件的代码类似，区别仅在于其还新指定了若干gcc编译选项，存放在KCFLAGS变量中，
具体为制定了若干存放在KINCLUDE变量下的头文件；具体而言，该命令最终生成的文件为obj/kern下子目录里的以stdio, 
readline, panic, kdebug, kmonitor, clock, console, picirq, intr, trap, vector, trapentry, pmm为前缀的.d, .o文件；
作者：AmadeusChan
链接：https://www.jianshu.com/p/2f95d38afa1d
*/

KOBJS	= $(call read_packet,kernel libs)

# create kernel target
kernel = $(call totarget,kernel)

$(kernel): tools/kernel.ld
/*表示/bin/kernel文件依赖于tools/kernel.ld文件，并且没有指定生成规则，
也就是说如果没有预先准备好kernel.ld，就会在make的时候产生错误；
之后的$(kernel): $(KOBJS)表示kernel文件的生成还依赖于上述生成的obj/libs, obj/kernels下的.o文件，
并且生成规则为使用ld链接器将这些.o文件连接成kernel文件，其中ld的-T表示指定使用kernel.ld来替代默认的链接器脚本；
关于LDFLAGS中的选项含义，将在下文中描述LDFLAGS变量定义的时候进行描述；之后还使用objdump反汇编出kernel的汇编代码，
-S表示将源代码与汇编代码混合展示出来，这部分代码最终保存在kernel.asm文件中；-t表示打印出文件的符号表表项，
然后通过管道将带有符号表的反汇编结果作为sed命令的标准输入进行处理，最终将符号表信息保存到kernel.sym文件中；
*/

$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)

$(call create_target,kernel)

# -------------------------------------------------------------------

# create bootblock
/*生成bootblock文件：
首先是$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc)这一段代码，
表示将boot/文件夹下的bootasm.S, bootmain.c两个文件编译成相应的.o文件，并且生成依赖文件.d；
其中涉及到的两个gcc编译选项含义如下所示：
-nostdinc: 不搜索默认路径头文件；
-0s: 针对生成代码的大小进行优化，这是因为bootloader的总大小被限制为不大于512-2=510字节；
接下来由代码$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign可知，
bootblock依赖于bootasm.o, bootmain.o文件与sign文件，其中两个.o文件由以下规则生成：
$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock): 
使用ld链接器将依赖的.o文件链接成bootblock.o文件，该文件中除了$(LDFLAGS)之外的其他选项含义如下：
-N：将代码段和数据段设置为可读可写；
-e：设置入口；
-Ttext：设置起始地址为0X7C00；
@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock): 
使用objdump将编译结果反汇编出来，保存在bootclock.asm中，-S表示将源代码与汇编代码混合表示；
@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock): 
使用objcopy将bootblock.o二进制拷贝到bootblock.out，其中：
-S：表示移除符号和重定位信息；
-O：表示指定输出格式；
@$(call totarget,sign) $(call outfile,bootblock) $(bootblock): 使用sign程序, 利用bootblock.out生成bootblock;
$(call add_files_host,tools/sign.c,sign,sign: 
利用tools/sing.c生成sign.o, $(call create_target_host,sign,sign)则利用sign.o生成sign，
至此bootblock所依赖的文件均生成完毕；
*/

bootfiles = $(call listf_cc,boot)
$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))


bootblock = $(call totarget,bootblock)

$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJDUMP) -t $(call objfile,bootblock) | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)

$(call create_target,bootblock)

# -------------------------------------------------------------------

# create 'sign' tools
$(call add_files_host,tools/sign.c,sign,sign)
$(call create_target_host,sign,sign)

# -------------------------------------------------------------------

# create ucore.img
/*最后一个部分是利用dd命令使用bootblock, kernel文件来生成ucore.img文件：
$(V)dd if=/dev/zero of=$@ count=10000 命令表示从/dev/zero文件中获取10000个block，
每一个block为512字节，并且均为空字符，并且输出到目标文件ucore.img中；
$(V)dd if=$(bootblock) of=$@ conv=notrunc 命令表示从bootblock文件中获取数据，
并且输出到目标文件ucore.img中，-notruct选项表示不要对数据进行删减；
$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc 命令表示从kernel文件中获取数据，
并且输出到目标文件ucore.img中, 并且seek = 1表示跳过第一个block，输出到第二个块；
至此，关于生成ucore.img文件的主要的Makefile命令分析完成；
*/

UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)

# >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

$(call finish_all)

IGNORE_ALLDEPS	= clean \
				  dist-clean \
				  grade \
				  touch \
				  print-.+ \
				  handin

ifeq ($(call match,$(MAKECMDGOALS),$(IGNORE_ALLDEPS)),0)
-include $(ALLDEPS)
endif

# files for grade script

TARGETS: $(TARGETS)
all: $(TARGETS)
.DEFAULT_GOAL := TARGETS

.PHONY: qemu qemu-nox debug debug-nox
lab1-mon: $(UCOREIMG)
	$(V)$(TERMINAL) -e "$(QEMU) -S -s -d in_asm -D $(BINDIR)/q.log -monitor stdio -hda $< -serial null"
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -x tools/lab1init"
debug-mon: $(UCOREIMG)
#	$(V)$(QEMU) -S -s -monitor stdio -hda $< -serial null &
	$(V)$(TERMINAL) -e "$(QEMU) -S -s -monitor stdio -hda $< -serial null"
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -x tools/moninit"
qemu-mon: $(UCOREIMG)
	$(V)$(QEMU) -monitor stdio -hda $< -serial null
qemu: $(UCOREIMG)
	$(V)$(QEMU) -parallel stdio -hda $< -serial null

qemu-nox: $(UCOREIMG)
	$(V)$(QEMU) -serial mon:stdio -hda $< -nographic
TERMINAL        :=gnome-terminal
gdb: $(UCOREIMG)
	$(V)$(QEMU) -S -s -parallel stdio -hda $< -serial null
debug: $(UCOREIMG)
	$(V)$(QEMU) -S -s -parallel stdio -hda $< -serial null &
	$(V)sleep 2
	$(V)$(TERMINAL)  -e "cgdb -q -x tools/gdbinit"
	
debug-nox: $(UCOREIMG)
	$(V)$(QEMU) -S -s -serial mon:stdio -hda $< -nographic &
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -x tools/gdbinit"

.PHONY: grade touch

GRADE_GDB_IN	:= .gdb.in
GRADE_QEMU_OUT	:= .qemu.out
HANDIN			:= proj$(PROJ)-handin.tar.gz

TOUCH_FILES		:= kern/trap/trap.c

MAKEOPTS		:= --quiet --no-print-directory

grade:
	$(V)$(MAKE) $(MAKEOPTS) clean
	$(V)$(SH) tools/grade.sh

touch:
	$(V)$(foreach f,$(TOUCH_FILES),$(TOUCH) $(f))

print-%:
	@echo $($(shell echo $(patsubst print-%,%,$@) | $(TR) [a-z] [A-Z]))

.PHONY: clean dist-clean handin packall
clean:
	$(V)$(RM) $(GRADE_GDB_IN) $(GRADE_QEMU_OUT)
	-$(RM) -r $(OBJDIR) $(BINDIR)

dist-clean: clean
	-$(RM) $(HANDIN)

handin: packall
	@echo Please visit http://learn.tsinghua.edu.cn and upload $(HANDIN). Thanks!

packall: clean
	@$(RM) -f $(HANDIN)
	@tar -czf $(HANDIN) `find . -type f -o -type d | grep -v '^\.*$$' | grep -vF '$(HANDIN)'`

