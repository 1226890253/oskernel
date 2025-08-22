CFLAGS:= -m32 # 32 位的程序
CFLAGS+= -fno-builtin	# 不需要 gcc 内置函数
CFLAGS+= -nostdinc		# 不需要标准头文件
CFLAGS+= -fno-pic		# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie		# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib		# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -Ioskernel/include
CFLAGS += -mno-red-zone
CFLAGS:=$(strip ${CFLAGS})

CFLAGS64 := -m64
CFLAGS64 += -mcmodel=large
CFLAGS64 += -fno-builtin	# 不需要 gcc 内置函数
CFLAGS64 += -nostdinc		# 不需要标准头文件
CFLAGS64 += -fno-pic		# 不需要位置无关的代码  position independent code
CFLAGS64 += -fno-pie		# 不需要位置无关的可执行程序 position independent executable
CFLAGS64 += -nostdlib		# 不需要标准库
CFLAGS64 += -fno-stack-protector	# 不需要栈保护
CFLAGS64 += -Ioskernel64/include
CFLAGS64 += -mno-red-zone
CFLAGS64:=$(strip ${CFLAGS64})

SRCDIR:= oskernel
SRC := $(shell find $(SRCDIR) -type f -name '*.c')
BUILD := ./build
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRC))
#OBJS := $(patsubst ${BUILD}/oskernel/%,$(BUILD)/%,$(OBJS))

SRCDIR64:= oskernel64
SRC64 := $(shell find $(SRCDIR64) -type f -name '*.c')
OBJS64 := $(patsubst %.c,$(BUILD)/%.o,$(SRC64))
#OBJS := $(patsubst ${BUILD}/oskernel/%,$(BUILD)/%,$(OBJS))

HD_IMG_NAME := "hd.img"
clean:
	$(shell rm -rf ${BUILD})

all: ${BUILD}/oskernel/boot/boot.o ${BUILD}/oskernel/boot/setup.o ${BUILD}/oskernel/kernel.bin ${BUILD}/oskernel64/kernel64.bin
	$(shell rm -rf $(BUILD)/$(HD_IMG_NAME))
	bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $(BUILD)/${HD_IMG_NAME}
	dd if=${BUILD}/oskernel/boot/boot.o of=$(BUILD)/$(HD_IMG_NAME)  bs=512 seek=0  count=1 conv=notrunc
	dd if=${BUILD}/oskernel/boot/setup.o of=$(BUILD)/$(HD_IMG_NAME) bs=512 seek=1  count=2 conv=notrunc
	dd if=${BUILD}/oskernel/kernel.bin of=$(BUILD)/$(HD_IMG_NAME) bs=512 seek=3 count=30 conv=notrunc
	dd if=${BUILD}/oskernel64/kernel64.bin of=$(BUILD)/$(HD_IMG_NAME) bs=512 seek=41 count=5000 conv=notrunc

${BUILD}/oskernel64/kernel64.bin : ${BUILD}/oskernel64/kernel64.elf
	objcopy -O binary ${BUILD}/oskernel64/kernel64.elf ${BUILD}/oskernel64/kernel64.bin
	nm ${BUILD}/oskernel64/kernel64.elf | sort > ${BUILD}/oskernel64/system64.map

${BUILD}/oskernel64/kernel64.elf : ${BUILD}/oskernel64/boot/head.o ${OBJS64} ${BUILD}/oskernel64/kernel/isr.o
	ld -b elf64-x86-64 $^ -o $@ -Ttext 0x100000

${OBJS64}: ${BUILD}/%.o : %.c
	@mkdir -p $(@D)
	gcc ${CFLAGS64} -g -c $^ -o $@

${BUILD}/oskernel64/kernel/isr.o: oskernel64/kernel/isr.asm
	@mkdir -p $(@D)
	nasm -f elf64 -g $< -o $@

${BUILD}/oskernel64/boot/head.o: oskernel64/boot/head.asm
	@mkdir -p $(@D)
	nasm -f elf64 -g $< -o $@

#下面为32位内核编译
${BUILD}/oskernel/kernel.bin: ${BUILD}/oskernel/kernel.elf
	objcopy -O binary ${BUILD}/oskernel/kernel.elf ${BUILD}/oskernel/kernel.bin
	nm ${BUILD}/oskernel/kernel.elf | sort > ${BUILD}/oskernel/kernel.map

${BUILD}/oskernel/kernel.elf : ${BUILD}/oskernel/boot/head.o ${OBJS} ${BUILD}/oskernel/init/enter_x64.o
	ld -m elf_i386 $^ -o $@ -Ttext 0x1200

${OBJS} : ${BUILD}/%.o : %.c
	@mkdir -p $(@D)
	gcc ${CFLAGS} -g -c $^ -o $@

${BUILD}/oskernel/boot/head.o: oskernel/boot/head.asm
	@mkdir -p $(@D)
	nasm -f elf32 -g $< -o $@

${BUILD}/oskernel/boot/%.o : oskernel/boot/%.asm
	@mkdir -p $(@D)
	nasm $< -o $@

${BUILD}/oskernel/init/enter_x64.o : oskernel/init/enter_x64.asm
	@mkdir -p $(@D)
	nasm -f elf32 -g $< -o $@

.PHONY: print bochs qemug qemu clean
print:
	@echo ${OBJS}
	@echo ${SRC}
	@echo ${OBJS64}
	@echo ${SRC64}

bochs: all
	bochs -q -f bochsrc
#qemu-system-i386
qemug: all
	qemu-system-x86_64 \
	-d int -D qemu-debug.log -no-reboot \
	-cpu Haswell -smp cores=4,threads=8 \
	-m 9G \
	-boot c \
	-hda ./build/hd.img -s -S

qemu: all
	qemu-system-x86_64 \
		-d int -D qemu-debug.log -no-reboot\
		-cpu Haswell -smp cores=4,threads=8 \
		-m 9G \
		-boot c \
		-hda ./build/hd.img \


#-hda ./build/hd.img \
-enable-kvm -cpu host \
-drive file=./build/hd.img,format=raw