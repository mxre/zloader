#!/bin/sh

KERNEL="/boot/Image"
KERNEL_COMPRESSED="/boot/Image.zst"
OSRELEASE="/etc/os-release"
KEYS=""
BOOTCFG="/etc/boot.bcfg"
DT="/boot/armada-3720-espressobin.dtb"
STUB="/boot/zloaderaa64.efi.stub"

function arch() {
	case $(uname -m) in
		x86_64) echo "x64";;
		i*68) echo "ia32";;
		aarch64) echo "aa64";;
		armv7) echo "arm";;
	esac
}

function install() {
	KERNEL_VERSION="$1"
	EFI_OUT="/efi/EFI/Linux/${KERNEL_VERSION}.efi"

	if [ "${KERNEL}" -nt "${KERNEL_COMPRESSED}" ]; then
		echo "Compressing ${KERNEL} ($KERNEL_VERSION)"
		zstd -f -19 "${KERNEL}" -o ${KERNEL_COMPRESSED} || exit 1
	fi

	INITRD="/boot/initrd.img"

	if [ "${KERNEL}" -nt "${INITRD}" ]; then
		echo "Generating ${INITRD}"
		dracut --quiet --force "${INITRD}" $KERNEL_VERSION || return 1
	fi

	if [ "${BOOTCFG}" -nt "${INITRD}" -o "${INITRD}" -nt "${EFI_OUT}" ]; then
		bootconfig -a "${BOOTCFG}" "${INITRD}" || return 1
	fi

	if [ \
		"${INITRD}" -nt "${EFI_OUT}" -o \
		"${KERNEL_COMPRESSED}" -nt "${EFI_OUT}" -o \
		"${DT}" -nt "${EFI_OUT}" -o \
		"${STUB}" -nt "${EFI_OUT}" \
	   ]; then
        	ARCH=$(arch)
		echo "console=ttyMV0,115200 earlycon=ar3700_uart,0xd0012000 bootconfig" > /tmp/cmdline

		echo "Creating ${KERNEL_VERSION}.efi from $ARCH.efi.stub"
		build_image \
			${OSRELEASE:+--osrel "${OSRELEASE}"} \
			--cmdline "/tmp/cmdline" \
			--linux "${KERNEL_COMPRESSED}" \
			${INITRD:+--initrd "${INITRD}"} \
			${DT:+--dtb "${DT}"} \
			--stub "${STUB}" \
			--outfile "/tmp/zloader.efi"
	
		if [ "${KEYS}" ]; then
			sbsign \
				--key "${KEYS}/db.key" \
				--cert "${KEYS}/db.crt" \
				--out "${EFI_OUT}" "/tmp/zloader.efi" || return 1
			rm -f "/tmp/zloader.efi"
		else
			mv "/tmp/zloader.efi" "${EFI_OUT}" || return 1
		fi

		rm -f "/tmp/cmdline"
	fi
}

KERNEL_VERSION=$(sed -En '/Linux version/ s/Linux version ([^ ]+).*/\1/ p' "${KERNEL}" | tr -d '\0')
if [ "$1" = "remove" ]; then
	EFI_OUT="/efi/EFI/Linux/${KERNEL_VERSION}.efi"
	rm -r "${EFI_OUT}"
elif [ "$1" = "install" ]; then
	install "${KERNEL_VERSION}" || exit 1
fi

