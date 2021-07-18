ARCH         := x86_64
CC           := clang
LD           := lld

INCLUDES      = -Iinclude
OPTIMIZATION ?= -Og -DSHUTDOWN -DDEBUG -DEFILIB_DEBUG
CFLAGS       += -mno-stack-arg-probe -fno-stack-protector -ffreestanding -mno-red-zone -std=c2x
CFLAGS       += -Wall -Werror -pedantic -Wno-gnu-zero-variadic-macro-arguments
LDFLAGS       = -flavor link -subsystem:efi_application -entry:efi_main
LDFLAGS      += -version:0.1

ifeq ($(ARCH),x86_64)
EFI_ARCH      = x64
CFLAGS       += 
else ifeq ($(ARCH),aarch64)
EFI_ARCH      = aa64
CFLAGS       +=
endif

SRCS          = src/main.c src/decompress.c src/util.c
SRCS         += src/pe.c src/initrd.c
SRCS         += lib/efilib/efilib.c lib/efilib/efirtlib.c lib/efilib/efiprint.c lib/efilib/efifprt.c lib/efilib/guid.c
SRCS         += lib/xxhash.c
ifdef USE_ZSTD # exe +35 KiB, speed 989ms  360MiB/s   inputsize 10 MiB
SRCS         += lib/zstd/common/zstd_common.c lib/zstd/common/entropy_common.c lib/zstd/common/error_private.c lib/zstd/common/fse_decompress.c
SRCS         += lib/zstd/decompress/huf_decompress.c lib/zstd/decompress/zstd_ddict.c lib/zstd/decompress/zstd_decompress_block.c lib/zstd/decompress/zstd_decompress.c
CFLAGS       += -DUSE_ZSTD
endif
ifdef USE_LZ4  # exe +10 KiB, speed 488ms  729MiB/s   inputsize 15 MiB
SRCS         += lib/lz4/lz4.c lib/lz4/lz4frame.c lib/lz4/lz4hc.c
CFLAGS       += -DUSE_LZ4
endif
#   NONE       #              speed 160ms  221MiB/s   inputsize 34 MiB

OBJS          = $(SRCS:.c=.o)
DEPS          = $(SRCS:.c=.dep)
TARGET        = boot$(EFI_ARCH).efi

.PHONY: all clean

all: $(TARGET)
	install -D -t build $(TARGET)

clean:
	$(RM) -rf $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -out:$@

%.o: %.c
	$(CC) -target $(ARCH)-unknown-windows $(INCLUDES) $(OPTIMIZATION) $(CFLAGS) -MMD -MF $(<:.c=.dep) -c $< -o $@

# compile floating point runtime without optimization
lib/efi/efifprt.o: lib/efi/efifprt.c
	$(CC) -target $(ARCH)-unknown-windows $(INCLUDES) -Os $(CFLAGS) -MMD -MF $(<:.c=.dep) -c $< -o $@
