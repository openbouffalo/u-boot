// SPDX-License-Identifier: GPL-2.0+

#include <bl808/glb_reg.h>
#include <bl808/hbn_reg.h>
#include <bl808/mm_glb_reg.h>
#include <bl808/pds_reg.h>
#include <clk/bflb.h>
#include <dt-bindings/clock/bl808-glb.h>
#include <dt-bindings/clock/bl808-hbn.h>
#include <dt-bindings/clock/bl808-mm-glb.h>
#include <dt-bindings/clock/bl808-pds.h>
#include <dt-bindings/reset/bl808-glb.h>
#include <dt-bindings/reset/bl808-mm-glb.h>
#include <linux/bitops.h>

#define PRNTS(...) (const u8[]) { __VA_ARGS__ }

enum {
	/* Fixed clocks */
	FW_EXT_XTAL		= FW_PARENT_BASE,
	FW_EXT_XTAL32K,
	/* Provided by GLB */
	FW_BCLK,
	FW_DIG_32K,
	FW_AUPLL_DIV1,
	FW_CPUPLL_400M,
	FW_DSPPLL,
	FW_MUXPLL_160M,
	FW_MUXPLL_240M,
	FW_MUXPLL_320M,
	FW_WIFIPLL_240M,
	FW_WIFIPLL_320M,
	/* Provided by HBN */
	FW_XCLK,
	FW_HBN_ROOT,
	FW_HBN_UART,
	FW_XTAL,
	/* Provided by PDS */
	FW_RC32M,
	FW_PDS_PLL,

	FW_PARENT_MAX,
};

static const char *const bl808_fw_parents[FW_PARENT_MAX-FW_PARENT_BASE] = {
	/* Fixed clocks */
	"ext_xtal",
	"ext_xtal32k",
	/* Provided by GLB */
	"bclk",
	"dig_32k",
	"aupll_div1",
	"cpupll_400m",
	"dsppll",
	"muxpll_160m",
	"muxpll_240m",
	"muxpll_320m",
	"wifipll_240m",
	"wifipll_320m",
	/* Provided by HBN */
	"xclk",
	"hbn_root",
	"hbn_uart",
	"xtal",
	/* Provided by PDS */
	"rc32m",
	"pds_pll",
};

static const struct bflb_clk_data bl808_glb_clks[] = {
	/*
	 * NOTE: This list of clocks is incomplete.
	 * The others in ths file are complete.
	 */
	[CLK_CPU] = {
		.name		= "CPU",
		.parents	= PRNTS(FW_HBN_ROOT),
		.div_reg	= GLB_SYS_CFG0_OFFSET,
		.div_mask	= GLB_REG_HCLK_DIV_MSK,
	},
	[CLK_HCLK] = {
		.name		= "HCLK",
		.parents	= PRNTS(CLK_CPU),
		.en_reg		= GLB_SYS_CFG0_OFFSET,
		.en_mask	= GLB_REG_HCLK_EN_MSK,
	},
	[CLK_BCLK] = {
		.name		= "BCLK",
		.parents	= PRNTS(CLK_CPU),
		/* Must set GLB_REG_BCLK_DIV_ACT_PULSE to update */
		.div_reg	= GLB_SYS_CFG0_OFFSET,
		.div_mask	= GLB_REG_BCLK_DIV_MSK,
		.en_reg		= GLB_SYS_CFG0_OFFSET,
		.en_mask	= GLB_REG_BCLK_EN_MSK,
	},
	[CLK_UART] = {
		.name		= "UART",
		.parents	= PRNTS(FW_HBN_UART),
		.div_reg	= GLB_UART_CFG0_OFFSET,
		.div_mask	= GLB_UART_CLK_DIV_MSK,
		.en_reg		= GLB_UART_CFG0_OFFSET,
		.en_mask	= GLB_UART_CLK_EN_MSK,
	},
	[CLK_DSPPLL] = {
		.name		= "DSPPLL",
	},
	[CLK_MM_MUXPLL_160M] = {
		.name		= "MM_MUXPLL_160M",
		.rate		= 160000000,
	},
	[CLK_MM_MUXPLL_240M] = {
		.name		= "MM_MUXPLL_240M",
		.rate		= 240000000,
	},
	[CLK_MM_MUXPLL_320M] = {
		.name		= "MM_MUXPLL_320M",
		.rate		= 320000000,
	},
	[CLK_TOP_MUXPLL_160M] = {
		.name		= "TOP_MUXPLL_160M",
		.rate		= 160000000,
	},
	[CLK_SDH] = {
		.name		= "SDH",
		.parents	= PRNTS(CLK_WIFIPLL_DIV5,
					CLK_TOP_CPUPLL_100M),
		.sel_reg	= GLB_SDH_CFG0_OFFSET,
		.sel_mask	= GLB_REG_SDH_CLK_SEL_MSK,
		.div_reg	= GLB_SDH_CFG0_OFFSET,
		.div_mask	= GLB_REG_SDH_CLK_DIV_MSK,
		.en_reg		= GLB_SDH_CFG0_OFFSET,
		.en_mask	= GLB_REG_SDH_CLK_EN_MSK,
	},
	[CLK_TOP_CPUPLL_100M] = {
		.name		= "TOP_CPUPLL_100M",
		.rate		= 100000000,
	},
	[CLK_TOP_CPUPLL_400M] = {
		.name		= "TOP_CPUPLL_400M",
		.rate		= 400000000,
	},
	[CLK_TOP_WIFIPLL_240M] = {
		.name		= "TOP_WIFIPLL_240M",
		.rate		= 240000000,
	},
	[CLK_TOP_WIFIPLL_320M] = {
		.name		= "TOP_WIFIPLL_320M",
		.rate		= 320000000,
	},
	[CLK_WIFIPLL] = {
		.name		= "WIFIPLL",
		.rate		= 480000000,
	},
	[CLK_WIFIPLL_DIV5] = {
		.name		= "WIFIPLL_DIV5",
		.parents	= PRNTS(CLK_WIFIPLL),
		.en_reg		= GLB_WIFI_PLL_CFG8_OFFSET,
		.en_mask	= GLB_WIFIPLL_EN_DIV5_MSK,
		.fixed_div	= 5,
	},
	[CLK_USBPLL] = {
		.name		= "USBPLL",
		.parents	= PRNTS(CLK_WIFIPLL),
		.en_reg		= GLB_WIFI_PLL_CFG10_OFFSET,
		.en_mask	= GLB_PU_USBPLL_MMDIV_MSK,
		.rst_reg	= GLB_WIFI_PLL_CFG10_OFFSET,
		.rst_mask	= GLB_USBPLL_RSTB_MSK,
	},
	[CLK_BUS_USB] = {
		.name		= "BUS_USB",
		.en_reg		= GLB_CGEN_CFG1_OFFSET,
		.en_mask	= GLB_CGEN_S1_RSVD13_MSK,
	},
	[CLK_BUS_SDH] = {
		.name		= "BUS_SDH",
		.en_reg		= GLB_CGEN_CFG2_OFFSET,
		.en_mask	= GLB_CGEN_S1_EXT_SDH_MSK,
	},
	[CLK_BUS_EMAC] = {
		.name		= "BUS_EMAC",
		.en_reg		= GLB_CGEN_CFG2_OFFSET,
		.en_mask	= GLB_CGEN_S1_EXT_EMAC_MSK,
	},
};

const struct bflb_clk_desc bl808_glb_clk_desc = {
	.clks		= bl808_glb_clks,
	.fw_parents	= bl808_fw_parents,
	.num_clks	= ARRAY_SIZE(bl808_glb_clks),
	.num_fw_parents	= ARRAY_SIZE(bl808_fw_parents),
};

static const struct bflb_clk_data bl808_hbn_clks[] = {
	[CLK_XCLK] = {
		.name		= "XCLK",
		.parents	= PRNTS(FW_RC32M,
					CLK_XTAL),
		.sel_reg	= HBN_GLB_OFFSET,
		.sel_mask	= BIT(0), /* HBN_ROOT_CLK_SEL[0] */
	},
	[CLK_HBN_ROOT] = {
		.name		= "HBN_ROOT",
		.parents	= PRNTS(CLK_XCLK,
					FW_PDS_PLL),
		.sel_reg	= HBN_GLB_OFFSET,
		.sel_mask	= BIT(1), /* HBN_ROOT_CLK_SEL[1] */
	},
	[CLK_HBN_UART_SEL] = {
		.name		= "HBN_UART_SEL",
		.parents	= PRNTS(FW_BCLK,
					FW_MUXPLL_160M),
		.sel_reg	= HBN_GLB_OFFSET,
		.sel_mask	= HBN_UART_CLK_SEL_MSK,
	},
	[CLK_F32K] = {
		.name		= "F32K",
		.parents	= PRNTS(CLK_RC32K,
					CLK_XTAL32K,
					FW_DIG_32K,
					NO_PARENT),
		.sel_reg	= HBN_GLB_OFFSET,
		.sel_mask	= HBN_F32K_SEL_MSK,
	},
	[CLK_HBN_UART] = {
		.name		= "HBN_UART",
		.parents	= PRNTS(CLK_HBN_UART_SEL,
					CLK_XCLK),
		.sel_reg	= HBN_GLB_OFFSET,
		.sel_mask	= HBN_UART_CLK_SEL2_MSK,
	},
	[CLK_RC32K] = {
		.name		= "RC32K",
		.rate		= 32000,
	},
	[CLK_XTAL32K] = {
		.name		= "XTAL32K",
		.parents	= PRNTS(FW_EXT_XTAL32K),
	},
	[CLK_XTAL] = {
		.name		= "XTAL",
		.parents	= PRNTS(FW_EXT_XTAL),
	},
};

const struct bflb_clk_desc bl808_hbn_clk_desc = {
	.clks		= bl808_hbn_clks,
	.fw_parents	= bl808_fw_parents,
	.num_clks	= ARRAY_SIZE(bl808_hbn_clks),
	.num_fw_parents	= ARRAY_SIZE(bl808_fw_parents),
};

static const struct bflb_clk_data bl808_mm_glb_clks[] = {
	[CLK_MM_UART] = {
		.name		= "MM_UART",
		.parents	= PRNTS(CLK_MM_BCLK1X,
					FW_MUXPLL_160M,
					CLK_MM_XCLK,
					NO_PARENT),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_UART_CLK_SEL_MSK,
	},
	[CLK_MM_I2C] = {
		.name		= "MM_I2C",
		.parents	= PRNTS(CLK_MM_BCLK1X,
					CLK_MM_XCLK),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_I2C_CLK_SEL_MSK,
	},
	[CLK_MM_SPI] = {
		.name		= "MM_SPI",
		.parents	= PRNTS(FW_MUXPLL_160M,
					CLK_MM_XCLK),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_SPI_CLK_SEL_MSK,
	},
	[CLK_MM_MUXPLL] = {
		.name		= "MM_MUXPLL",
		.parents	= PRNTS(FW_MUXPLL_240M,
					FW_MUXPLL_320M,
					FW_CPUPLL_400M,
					FW_CPUPLL_400M),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_CPU_CLK_SEL_MSK,
	},
	[CLK_MM_XCLK] = {
		.name		= "MM_XCLK",
		.parents	= PRNTS(FW_RC32M,
					FW_XTAL),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_XCLK_CLK_SEL_MSK,
	},
	[CLK_MM_CPU] = {
		.name		= "MM_CPU",
		.parents	= PRNTS(CLK_MM_XCLK,
					CLK_MM_MUXPLL),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_CPU_ROOT_CLK_SEL_MSK,
		.div_reg	= MM_GLB_MM_CLK_CPU_OFFSET,
		.div_mask	= MM_GLB_REG_CPU_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.en_mask	= MM_GLB_REG_MMCPU0_CLK_EN_MSK,
	},
	[CLK_MM_BCLK1X] = {
		.name		= "MM_BCLK1X",
		.parents	= PRNTS(CLK_MM_XCLK,
					CLK_MM_XCLK,
					FW_MUXPLL_160M,
					FW_MUXPLL_240M),
		.sel_reg	= MM_GLB_MM_CLK_CTRL_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_BCLK1X_SEL_MSK,
		.div_reg	= MM_GLB_MM_CLK_CPU_OFFSET,
		.div_mask	= MM_GLB_REG_BCLK1X_DIV_MSK,
	},
	[CLK_MM_BCLK2X] = {
		.name		= "MM_BCLK2X",
		.parents	= PRNTS(CLK_MM_CPU),
		/* Must set MM_GLB_REG_BCLK2X_DIV_ACT_PULSE to update */
		.div_reg	= MM_GLB_MM_CLK_CPU_OFFSET,
		.div_mask	= MM_GLB_REG_BCLK2X_DIV_MSK,
	},
	[CLK_MM_CNN] = {
		.name		= "MM_CNN",
		.parents	= PRNTS(FW_MUXPLL_160M,
					FW_MUXPLL_240M,
					FW_MUXPLL_320M,
					FW_MUXPLL_320M),
		.sel_reg	= MM_GLB_MM_CLK_CPU_OFFSET,
		.sel_mask	= MM_GLB_REG_CNN_CLK_SEL_MSK,
		.div_reg	= MM_GLB_MM_CLK_CPU_OFFSET,
		.div_mask	= MM_GLB_REG_CNN_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CPU_OFFSET,
		.en_mask	= MM_GLB_REG_CNN_CLK_DIV_EN_MSK,
	},
	[CLK_MM_DSP] = {
		.name		= "MM_DSP",
		.parents	= PRNTS(FW_MUXPLL_160M,
					FW_MUXPLL_240M,
					FW_CPUPLL_400M,
					CLK_MM_XCLK),
		.sel_reg	= MM_GLB_DP_CLK_OFFSET,
		.sel_mask	= MM_GLB_REG_CLK_SEL_MSK,
		.div_reg	= MM_GLB_DP_CLK_OFFSET,
		.div_mask	= MM_GLB_REG_CLK_DIV_MSK,
		.en_reg		= MM_GLB_DP_CLK_OFFSET,
		.en_mask	= MM_GLB_REG_CLK_DIV_EN_MSK,
	},
	[CLK_MM_DSP_DP] = {
		.name		= "MM_DSP_DP",
		.parents	= PRNTS(FW_DSPPLL,
					CLK_MM_XCLK),
		.sel_reg	= MM_GLB_DP_CLK_OFFSET,
		.sel_mask	= MM_GLB_REG_DP_CLK_SEL_MSK,
		.div_reg	= MM_GLB_DP_CLK_OFFSET,
		.div_mask	= MM_GLB_REG_DP_CLK_DIV_MSK,
		.en_reg		= MM_GLB_DP_CLK_OFFSET,
		.en_mask	= MM_GLB_REG_DP_CLK_DIV_EN_MSK,
	},
	[CLK_MM_H264] = {
		.name		= "MM_H264",
		.parents	= PRNTS(FW_MUXPLL_160M,
					FW_MUXPLL_240M,
					FW_MUXPLL_320M,
					FW_MUXPLL_320M),
		.sel_reg	= MM_GLB_CODEC_CLK_OFFSET,
		.sel_mask	= MM_GLB_REG_H264_CLK_SEL_MSK,
		.div_reg	= MM_GLB_CODEC_CLK_OFFSET,
		.div_mask	= MM_GLB_REG_H264_CLK_DIV_MSK,
		.en_reg		= MM_GLB_CODEC_CLK_OFFSET,
		.en_mask	= MM_GLB_REG_H264_CLK_DIV_EN_MSK,
	},
	[CLK_MM_IC20] = {
		.name		= "MM_IC20",
		.parents	= PRNTS(CLK_MM_I2C),
		.div_reg	= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.div_mask	= MM_GLB_REG_I2C0_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.en_mask	= MM_GLB_REG_I2C0_CLK_DIV_EN_MSK |
				  MM_GLB_REG_I2C0_CLK_EN_MSK,
	},
	[CLK_MM_UART0] = {
		.name		= "MM_UART0",
		.parents	= PRNTS(CLK_MM_UART),
		.div_reg	= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.div_mask	= MM_GLB_REG_UART0_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.en_mask	= MM_GLB_REG_UART0_CLK_DIV_EN_MSK,
	},
	[CLK_MM_SPI0] = {
		.name		= "MM_SPI0",
		.parents	= PRNTS(CLK_MM_SPI),
		.div_reg	= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.div_mask	= MM_GLB_REG_SPI_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_PERI_OFFSET,
		.en_mask	= MM_GLB_REG_SPI_CLK_DIV_EN_MSK,
	},
	[CLK_MM_I2C1] = {
		.name		= "MM_I2C1",
		.parents	= PRNTS(CLK_MM_I2C),
		.div_reg	= MM_GLB_MM_CLK_CTRL_PERI3_OFFSET,
		.div_mask	= MM_GLB_REG_I2C1_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_PERI3_OFFSET,
		.en_mask	= MM_GLB_REG_I2C1_CLK_DIV_EN_MSK,
	},
	[CLK_MM_UART1] = {
		.name		= "MM_UART1",
		.parents	= PRNTS(CLK_MM_UART),
		.div_reg	= MM_GLB_MM_CLK_CTRL_PERI3_OFFSET,
		.div_mask	= MM_GLB_REG_UART1_CLK_DIV_MSK,
		.en_reg		= MM_GLB_MM_CLK_CTRL_PERI3_OFFSET,
		.en_mask	= MM_GLB_REG_UART1_CLK_DIV_EN_MSK,
	},
};

static const struct bflb_reset_data bl808_mm_glb_resets[] = {
	[RST_MM_CPU]	= { MM_GLB_MM_SW_SYS_RESET_OFFSET,
			    MM_GLB_REG_CTRL_MMCPU0_RESET_POS },
};

const struct bflb_clk_desc bl808_mm_glb_clk_desc = {
	.clks		= bl808_mm_glb_clks,
	.resets		= bl808_mm_glb_resets,
	.fw_parents	= bl808_fw_parents,
	.num_clks	= ARRAY_SIZE(bl808_mm_glb_clks),
	.num_resets	= ARRAY_SIZE(bl808_mm_glb_resets),
	.num_fw_parents	= ARRAY_SIZE(bl808_fw_parents),
};

static const struct bflb_clk_data bl808_pds_clks[] = {
	[CLK_PDS_PLL] = {
		.name		= "PDS_PLL",
		.parents	= PRNTS(FW_CPUPLL_400M,
					FW_AUPLL_DIV1,
					FW_WIFIPLL_240M,
					FW_WIFIPLL_320M),
		.sel_reg	= PDS_CPU_CORE_CFG1_OFFSET,
		.sel_mask	= PDS_REG_PLL_SEL_MSK,
		.en_reg		= PDS_CPU_CORE_CFG1_OFFSET,
		.en_mask	= PDS_REG_MCU1_CLK_EN_MSK,
	},
	[CLK_RC32M] = {
		.name		= "RC32M",
		.rate		= 32000000,
	},
};

const struct bflb_clk_desc bl808_pds_clk_desc = {
	.clks		= bl808_pds_clks,
	.fw_parents	= bl808_fw_parents,
	.num_clks	= ARRAY_SIZE(bl808_pds_clks),
	.num_fw_parents	= ARRAY_SIZE(bl808_fw_parents),
};
