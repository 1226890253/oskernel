CFLAGS:= -m32 # 32 位的程序
CFLAGS+= -fno-builtin	# 不需要 gcc 内置函数
CFLAGS+= -nostdinc		# 不需要标准头文件
CFLAGS+= -fno-pic		# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie		# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib		# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -Ioskernel/include
CFLAGS:=$(strip ${CFLAGS})

SRCDIR:= oskernel
SRC := $(shell find $(SRCDIR) -type f -name '*.c')
BUILD := ./build
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRC))
OBJS := $(patsubst ${BUILD}/oskernel/%,$(BUILD)/%,$(OBJS))

HD_IMG_NAME := "hd.img"

all: ${BUILD}/boot/boot.o ${BUILD}/boot/setup.o ${BUILD}/kernel.bin
	$(shell rm -rf $(BUILD)/$(HD_IMG_NAME))
	bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $(BUILD)/${HD_IMG_NAME}
	dd if=${BUILD}/boot/boot.o of=$(BUILD)/${HD_IMG_NAME}  bs=512 seek=0  count=1 conv=notrunc
	dd if=${BUILD}/boot/setup.o of=$(BUILD)/${HD_IMG_NAME} bs=512 seek=1  count=2 conv=notrunc
	dd if=${BUILD}/kernel.bin of=$(BUILD)/$(HD_IMG_NAME) bs=512 seek=3 count=60 conv=notrunc

${BUILD}/kernel.bin: ${BUILD}/kernel.elf
	objcopy -O binary ${BUILD}/kernel.elf ${BUILD}/kernel.bin
	nm ${BUILD}/kernel.elf | sort > ${BUILD}/kernel.map

${BUILD}/kernel.elf : ${BUILD}/boot/head.o ${OBJS}
	ld -m elf_i386 $^ -o $@ -Ttext 0x1200

${BUILD}/%.o : oskernel/%.c | ${BUILD}
	@mkdir -p $(@D)
	gcc ${CFLAGS} -g -c $^ -o $@

${BUILD}/boot/head.o: oskernel/boot/head.asm
	nasm -f elf32 -g $< -o $@

${BUILD}/boot/%.o : oskernel/boot/%.asm
	$(shell mkdir -p ${BUILD}/boot)
	nasm $< -o $@

${BUILD}:
	mkdir -p $@

clean:
	$(shell rm -rf ${BUILD})

bochs: all
	bochs -q -f bochsrc

qemug: all
	qemu-system-i386 \
	-m 32M \
	-boot d \
	-hda ./build/hd.img -s -S

qemu: all
	qemu-system-x86_64 \
	-m 32M \
	-boot c \
	-hda ./build/hd.img