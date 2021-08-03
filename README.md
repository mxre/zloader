zloader
=======

zloader is a Linux kernel bootstub (like the systemd boot stub) that tries to
overcome some of the shortcomings of the Linux UEFI boot process on the Aarch64
platform:

 * Linux for AArch64 does not have Kernel Image compression support
 * systemd-boot stub does not support AArch64 (only x86 specific EFI handover)

It does so by providing a boot stub (a drop in replacement for the systemd-boot
stub) wih support for LZ4 compressed and/or ZSTD comressed kernel images. The
Linux kernel itself must have an EFI stub (which is usually the case in
modern distributions)

Additionally two UEFI tools are provided:

 * `lockdown`: which installs SecureBoot public keys that are provided in files
 * `fit`: is a stub designed for inclusion in a UBoot FIT image

Compiling this project requires
 * `clang`: with support for Windows targets (tested version 12.0.1)
 * `lld-link`: Microsoft compatible LLVM linker
 * `llvm-rc`: LLVM Resource compiler
 * `llvm-objcopy`:LLVM's version of objcopy, which support PE for all machine
   types in contrast to binutil's objcopy which only does so for x86.
 * `cmake`: Build configuration

On Arch Linux these tools are provided by the packages `clang`, `lld`, `llvm`
and `cmake`. Crosscompilation is automatically provided.

Building
--------

Like any C project using CMake the project can be built with
```
$ mkdir build
$ cmake -GNinja ..
$ ninja
```

The CMake build process knows the following options which can be set using the
`-D` parameter for `cmake` or using the `cmake-gui` tool.

`LOADER_TARGET` (default: x86_64)
:   Target Machine for the UEFI executable, can either be `aarch64` or `x86_64`

`LOADER_USE_LZ4` (on)
:   Build with support for LZ4 compressed Kernels

`LOADER_USE_ZSTD` (off)
:   Build with support for ZSTD compressed Kernels

`LOADER_PRINT_MESSAGES` (off)
:   Print status/debug messages (default is to be silent except for errors)

`LOADER_USE_EFI_LOAD_IMAGE` (off)
:   Use UEFI `LoadImage` and `StartImage` to process and execute the
    decompressed kernel image. The default is to use an internal loader and
    relocator, because when using SecureBoot the embedded kernel has to be
    signed and when using a TPM it will be hashed to the PCR. Since the kernel
    is embedded in the zloader PE image, it would be processed twice.

-------------------------------------------------------------------------------
The included EFI runtime support library has also options, which usually don't
need to be set from anything different than the default.

`EFILIB_SHUTDOWN` (off)
:   Issue a PowerOff on program exit, this is only usefull for debugging
    purposes, when running the program inside of Qemu

`EFILIB_DEBUG` (off)
:   Write library messages to console and enable library internal asserts

`EFILIB_USE_EFI_SET_MEM` (off)
:   Use UEFI BootServices `SetMem` for as the `memset` implementation instead
    of our own

`EFILIB_USE_EFI_COPY_MEM` (off)
:   Use UEFI BootServices `CopyMem` for as the `memcpy` and `memmove`
    implementation instead of our own

`EFILIB_USE_DEVICE_PATH_TO_TEXT_PROTOCOL` (on)
:   Use UEFI optional DevicePathToTextProtocol to print device paths in
    messages. Although this protocol is optional it is included in common UEFI
    implementations since the UEFI shell also requires it. UBoot allows for
    building UEFI without support for this in such a case this option can be
    enable to provide a very incomplete DevicePath to text implementation

`EFILIB_STALL_ON_EXIT` (5000000)
:   Many UEFI tools (like systemd-boot) stall for a few seconds after exiting
    on an error condition so that the user can actually read the error message.
    This option sets the timeout value for stall in microseconds. Values
    smaller than 10000 (0.01 seconds) disable this feature.

Using zloader stub
------------------

To embed the Linux Kernel and the initrd in the stub use objcopy in the same way
as with systemd-boot stub but since binutil's objcopy dows not support Aarch64
we use LLVM's version.

When compressing the Linux kernel using LZ4 always use the `--content-size`
option. zloader requires this to be set when decompressing in order to allocate
the appropriate amount of memory.

```
lz4 --content-size --best --favor-decSpeed vmlinux kernel.lz4
```

Note that only the `.linux` section is required `.osrel` is usefull for enabling
systemd-boot autodiscovert, `.cmdline` is the default cmdline that is passed on
and `.initrd` is the ramdisk and `.fdt` is a device tree binary, UBoot fixups wull
be applied if the EFI-UBoot-fixup protocol is found.

(when built for x86_64 replace `aa64` with `x64`)
```
llvm-objcopy \
	--add-section .osrel="/etc/os-release" \
	--add-section .cmdline="/proc/cmdline" \
	--add-section .linux="kernel.lz4"      \
	--add-section .initrd="initrd.img"     \
	--add-section .fdt="devicetree.dtb"    \
	"zloaderaa64.efi.stub" "bootaa64.efi"
```
Since LLVM objopy does not set the Virtual Memory Address (VMA) we need to do
that ourselves with a small fixup tool.
```
tools/pe_fixup --file "bootaa64.efi"
```
Now bootaa64.efi can either be copied to `<ESP>/efi/boot/bootaa64.efi` to use
the default boot behaviour or when using systemd-boot it can be copied to
`<ESP>/efi/Linux` where the Kernel can than be discovered automatically
(if os-release was embedded).

Using UBoot FIT
---------------

Using a DeviceTree embedded in the zloader image only allows for static compile
`.dtb` files. Sometimes it is necessary to have several configurations and
modify the DeviceTree prior to booting. This is done using DeviceTree Overlays,
usually stored in `.dtbo` files. The securest and easiest way to load
DeviceTrees with overlay is using FIT images.

UBoot supports signed FIT images (which are basically DeviceTree files with
binaries embedded in them), so the solution is to create a FIT images containing
an EFI executable and the DeviceTree file and optional Overlays.

Then UBoot has to be configured to boot this FIT image with the `bootm` command.

The `fitaa64.efi` executable just copies the basic EFI boot process of reading
the `BootNext`, `BootOrder`, `BootXXXX` variables and if that failes booting
`/efi/boot/bootaa64.efi`. UBoot would do this process by itself but not from
inside a FIT image.

To create a signed FIT image please refer to the UBoot documentation, an example
`.its` file for creating such an image follows:
```
/dts-v1/;

/ {
	description = "EFI Firmware with DeviceTree";
	#address-cells = <1>;

    images {
		efi {
			description = "EFI Firmware";
			data = /incbin/("fitaa64.efi");
			type = "kernel";
			arch = "aarch64";
			os = "efi";
			compression = "none";
			load = <0x7000000>;
			entry = <0x7000000>;
			hash-1 {
				algo = "sha256";
			};
		};

		fdt-base {
			description = "Base DTB";
			data = /incbin/("base.dtb");
			type = "flat_dt";
			arch = "aarch64";
			compression = "none";
            load = <0x6f00000>;
			hash-1 {
				algo = "sha256";
			};
		};

        fdt-base {
			description = "DTB Overlay";
			data = /incbin/("overlay.dtbo");
			type = "flat_dt";
			arch = "aarch64";
			compression = "none";
            load = <0x6fc0000>;
			hash-1 {
				algo = "sha256";
			};
		};
	};

	configurations {
		default = "config-base";

		config-base {
			description = "Base Config";
			kernel = "efi";
			fdt = "fdt-base", "fdt-overlay;
			signature-1 {
				algo = "sha256,rsa2048";
				key-name-hint = "boot";
				sign-images = "kernel", "fdt";
			};
		};
    };
};
```

Using with SecureBoot
---------------------
(Or read any other SecureBoot setup description)

First create a GUID for the Keys
```
echo /proc/sys/kernel/random/uuid > guid.txt
```

Now create a new Platform Key (PK) using OpenSSL, append it to a new
EFI certificate list (ESL) and sign it to crate a authenticated list.
```
openssl req -x509 -sha256 -newkey rsa:2048 -subj /CN=TEST_PK/ \
    -keyout PK.key -out PK.crt -nodes -days 365
cert-to-efi-sig-list -g $(<guid.txt) PK.crt PK.esl;
sign-efi-sig-list -c PK.crt -k PK.key PK PK.esl PK.auth
```

Repeat for the Key Exchange Key (KEK) sign the list with the PK
```
openssl req -x509 -sha256 -newkey rsa:2048 -subj /CN=TEST_KEK/ \
    -keyout KEK.key -out KEK.crt -nodes -days 365
cert-to-efi-sig-list -g $(<guid.txt) KEK.crt KEK.esl
sign-efi-sig-list -c PK.crt -k PK.key KEK KEK.esl KEK.auth
```
Again for the actual SecureBoot certificate (db) and sign it with the KEK
```
openssl req -x509 -sha256 -newkey rsa:2048 -subj /CN=TEST_db/ \
    -keyout db.key -out db.crt -nodes -days 365
cert-to-efi-sig-list -g $(<guid.txt) db.crt db.esl
sign-efi-sig-list -c KEK.crt -k KEK.key db db.esl db.auth
```

To use the lockdown utility in order to install the keys just copy
`lockdownaa64.efi` `PK.auth` `KEK.auth` and `db.auth` to an EFI bootable
partition. This is especially usefull for platform where Linux can't write EFI
variables (like UBoot).

The lockdown utility just sets the variables `PK`, `KEK` and `db` in order to
activate SecureBoot.

To actually sign the Kernel together with the stub just use `sbsign`

```
sbsign --key "db.key" --cert "db.crt" --out "bootaa64.efi" "bootaa64.efi"
```

