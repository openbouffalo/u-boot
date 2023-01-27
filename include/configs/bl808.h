/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <config_distro_bootcmd.h>

#ifdef CONFIG_BL808_CPU_D0
#define CFG_SYS_INIT_RAM_ADDR	0x50100000
#define CFG_SYS_INIT_RAM_SIZE	0x00100000
#else
#define CFG_SYS_INIT_RAM_ADDR	0x22020000
#define CFG_SYS_INIT_RAM_SIZE	0x00034000
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#ifdef CONFIG_MMC
#define BOOT_TARGET_DEVICES_MMC(func) func(MMC, mmc, 0)
#else
#define BOOT_TARGET_DEVICES_MMC(func)
#endif

#ifdef CONFIG_USB_STORAGE
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifdef CONFIG_CMD_PXE
#define BOOT_TARGET_DEVICES_PXE(func) func(PXE, pxe, na)
#else
#define BOOT_TARGET_DEVICES_PXE(func)
#endif

#ifdef CONFIG_CMD_DHCP
#define BOOT_TARGET_DEVICES_DHCP(func) func(DHCP, dhcp, na)
#else
#define BOOT_TARGET_DEVICES_DHCP(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_MMC(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	BOOT_TARGET_DEVICES_PXE(func) \
	BOOT_TARGET_DEVICES_DHCP(func)

#define CFG_EXTRA_ENV_SETTINGS \
	"fdt_addr_r=0x50120000\0" \
	"fdtoverlay_addr_r=0x50140000\0" \
	"kernel_addr_r=0x50200000\0" \
	"scriptaddr=0x50100000\0" \
	BOOTENV
#endif

#endif /* __CONFIG_H */
