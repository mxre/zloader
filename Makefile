ARCH         ?= x86_64
CC            = clang
LD            = lld
OBJCOPY       = llvm-objcopy
RC            = llvm-rc

# -DUSE_EFI_LOAD_IMAGE -DEFLLIB_USE_EFI_SET_MEM
INCLUDES      = -Iinclude
OPTIMIZATION ?= -Og -DSHUTDOWN -DDEBUG -DEFILIB_DEBUG -DPRINT_MESSAGES -DEFILIB_STALL_ON_EXIT=5000000
CFLAGS       += -mno-stack-arg-probe -fno-stack-protector -ffreestanding -mno-red-zone -std=c2x
CFLAGS       += -Wall -Werror -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-variable-sized-type-not-at-end
LDFLAGS       = -flavor link -subsystem:efi_application -tsaware:no -entry:efi_main
LDFLAGS      += -version:0.1

#   NONE           #              speed 160ms  221MiB/s   inputsize 34 MiB
USE_LZ4 	  = 1  # exe +10 KiB, speed 488ms  729MiB/s   inputsize 15 MiB
USE_ZSTD 	  = 1  # exe +35 KiB, speed 989ms  360MiB/s   inputsize 10 MiB

SRCS          = src/main.c src/decompress.c src/util.c src/systemd.c
SRCS         += src/pe.c src/pe_loader.c src/initrd.c
SRCS         += lib/efilib/efilib.c lib/efilib/efirtlib.c lib/efilib/string.c lib/efilib/efiprint.c lib/efilib/efifprt.c lib/efilib/guid.c
SRCS         += lib/xxhash.c

SRCS         += lib/zstd/common/zstd_common.c lib/zstd/common/entropy_common.c lib/zstd/common/error_private.c lib/zstd/common/fse_decompress.c
SRCS         += lib/zstd/decompress/huf_decompress.c lib/zstd/decompress/zstd_ddict.c lib/zstd/decompress/zstd_decompress_block.c lib/zstd/decompress/zstd_decompress.c
CFLAGS       += -DUSE_ZSTD

SRCS         += lib/lz4/lz4.c lib/lz4/lz4frame.c lib/lz4/lz4hc.c
CFLAGS       += -DUSE_LZ4

OBJS          = $(SRCS:.c=.o) src/main.res
DEPS          = $(SRCS:.c=.dep)

VMA_FIXER     = tools/vma_fixer

.PHONY: all clean distclean x86_64 aarch64 aa64 x64

aa64: aarch64
aarch64: bootaa64.efi

x64: x86_64
x86_64: bootx64.efi

bootx64.efi: ARCH=x86_64
bootx64.efi: TARGET=bootx64.efi

bootaa64.efi: ARCH=aarch64
bootaa64.efi: TARGET=bootaa64.efi

all: x86_64

distclean: clean
	$(RM) $(VMA_FIXER)

clean:
	$(RM) *.efi *.efi.stub $(OBJS) $(DEPS)

-include $(DEPS)

%.efi: $(VMA_FIXER)
%.efi: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -out:$@.stub
	$(OBJCOPY) --add-section .osrel=loader/os-release --add-section .cmdline=loader/cmdline --add-section .linux=loader/Image.lz4 --add-section .initrd=loader/initramfs-linux.img $@.stub $@
	$(VMA_FIXER) $@
#	install -D -t build $@
	sbsign --key ../espressobin/keys/sb/db.key --cert ../espressobin/keys/sb/db.crt --output build/$@ $@

%.res: %.rc
	$(RC) -FO $@ $<

%.o: %.c
	$(CC) -target $(ARCH)-unknown-windows $(INCLUDES) $(OPTIMIZATION) $(CFLAGS) -MMD -MF $(<:.c=.dep) -c $< -o $@

# compile floating point runtime without optimization
lib/efi/efifprt.o: lib/efi/efifprt.c
	$(CC) -target $(ARCH)-unknown-windows $(INCLUDES) -Os $(CFLAGS) -MMD -MF $(<:.c=.dep) -c $< -o $@

$(VMA_FIXER): tools/vma_fixer.c
	$(CC) -std=c2x -Wall -Werror -pedantic $< -o $@
tools/vma_fixer.c: tools/pe.h tools/compiler.h
tools/compiler.h: include/efi/compiler.h
	@ln -fs ../$< $@
tools/pe.h: include/efi/pe.h
	@ln -fs ../$< $@
