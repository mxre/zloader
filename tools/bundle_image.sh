#!/bin/sh

KERNEL=$1
INITRD=$2
KEYS=$3
CMDLINE=${4:-"/proc/cmdline"}
OSRELEASE=${5:-"/etc/os-release"}
ARCH=${6:-x64}

KERNEL=${KERNEL:-"/usr/lib/modules/$(uname -r)/vmlinuz" }

if [ "${INITRD}" == "current" ]; then
	objcopy --dump-section=.initrd=initrd-arch-zen.img /efi/EFI/Linux/arch-zen.efi
	INITRD="initrd-arch-zen.img"
fi;

if [ "${CMDLINE}" == "/proc/cmdline" ]; then
	cat "${CMDLINE}" > "/tmp/cmdline"
	CMDLINE="/tmp/cmdline"
fi

llvm-objcopy \
	${OSRELEASE:+--add-section .osrel="${OSRELEASE}"} \
	${CMDLINE:+--add-section .cmdline="${CMDLINE}"} \
	--add-section .linux=${KERNEL} \
	${INITRD:+--add-section .initrd="${INITRD}"} \
	"../zloader${ARCH}.efi.stub" "/tmp/zloader.efi"

rm -f "/tmp/cmdline"

./vma_fixer "/tmp/zloader.efi"

if [ "${KEYS}" ]; then
	sbsign --key "${KEYS}/db.key" --cert "${KEYS}/db.crt" --out "../boot${ARCH}.efi" "/tmp/zloader.efi"
	rm -f "/tmp/zloader.efi"
else
	mv "/tmp/zloader.efi" "../boot${ARCH}.efi"
fi
