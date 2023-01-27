// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <bl808/glb_reg.h>
#include <bl808/mm_misc_reg.h>

#define GLB_BASE			(void *)0x20000000
#define MM_MISC_BASE			(void *)0x30000000

DECLARE_GLOBAL_DATA_PTR;

void board_debug_uart_init(void)
{
	/* Mux GPIO14-15 to UART0 TXD/RXD, GPIO16-17 to UART1 TXD/RXD. */
	writel((7 << GLB_UART_SIG_5_SEL_POS) |
	       (6 << GLB_UART_SIG_4_SEL_POS) |
	       (3 << GLB_UART_SIG_3_SEL_POS) |
	       (2 << GLB_UART_SIG_2_SEL_POS),
	       GLB_BASE + GLB_UART_CFG1_OFFSET);

	if (!IS_ENABLED(CONFIG_DEBUG_UART))
		return;

	/* Enable the UART clock. */
	setbits_le32(GLB_BASE + GLB_UART_CFG0_OFFSET, GLB_UART_CLK_EN_MSK);

	if (IS_ENABLED(CONFIG_BL808_CPU_M0)) {
		u32 val = (7 << GLB_REG_GPIO_0_FUNC_SEL_POS) |
			  GLB_REG_GPIO_0_IE_MSK;

		/* Enable GPIO14-15 and set their function to UART. */
		writel(val, GLB_BASE + GLB_GPIO_CFG14_OFFSET);
		writel(val, GLB_BASE + GLB_GPIO_CFG15_OFFSET);
	} else if (IS_ENABLED(CONFIG_BL808_CPU_D0)) {
		u32 val = (21 << GLB_REG_GPIO_0_FUNC_SEL_POS) |
			  GLB_REG_GPIO_0_IE_MSK;

		/* Enable GPIO16-17 and set their function to MM_UART. */
		writel(val, GLB_BASE + GLB_GPIO_CFG16_OFFSET);
		writel(val, GLB_BASE + GLB_GPIO_CFG17_OFFSET);
	}
}

int board_early_init_f(void)
{
	board_debug_uart_init();

	if (IS_ENABLED(CONFIG_BL808_CPU_M0)) {
		/* Give M0 access to the full SRAM at 0x7ef80000. */
		u32 val = MM_MISC_REG_BLAI_SRAM_REL_MSK |
			  MM_MISC_REG_SUB_SRAM_REL_MSK |
			  MM_MISC_REG_VRAM_SRAM_REL_MSK |
			  MM_MISC_REG_H2PF_SRAM_REL_MSK;

		writel(val, MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);
		val |= MM_MISC_REG_SYSRAM_SET_MSK;
		writel(val, MM_MISC_BASE + MM_MISC_VRAM_CTRL_OFFSET);
	}

	return 0;
}

void *board_fdt_blob_setup(int *err)
{
	*err = 0;

	return (void *)(ulong)gd->arch.firmware_fdt_addr;
}

phys_size_t board_get_usable_ram_top(phys_size_t total_size)
{
	/* The devicetree describes PSRAM, but force M0 to relocate to SRAM. */
	if (IS_ENABLED(CONFIG_BL808_CPU_M0)) {
		gd->ram_base = 0x7ef80000;
		gd->ram_top  = 0x7f000000;
	}

	return gd->ram_top;
}

int board_init(void)
{
	int ret;

	/* M0 does not actually use PSRAM, so initialize it late. */
	if (IS_ENABLED(CONFIG_BL808_CPU_M0) && CONFIG_IS_ENABLED(RAM)) {
		ret = uclass_get_device(UCLASS_RAM, 0, NULL);
		if (ret) {
			printf("DRAM init failed: %d\n", ret);
			return ret;
		}
	}

	return 0;
}
