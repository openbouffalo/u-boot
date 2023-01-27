// SPDX-License-Identifier: GPL-2.0+

#ifndef _CLK_BFLB_H_
#define _CLK_BFLB_H_

/* Boundary between internal specifier numbers and OF consumer IDs */
#define FW_PARENT_BASE			0xc0

/* Used for gaps in selector values */
#define NO_PARENT			0xff

struct bflb_clk_data {
	const char	*name;
	const u8 	*parents;
	ulong		rate;
	u32		sel_mask;
	u32		div_mask;
	u32		en_mask;
	u16		sel_reg;
	u16		div_reg;
	u16		en_reg;
	u16 		fixed_div;
};

struct bflb_reset_data {
	u16		reg;
	u8		bit;
};

struct bflb_clk_desc {
	const struct bflb_clk_data	*clks;
	const struct bflb_reset_data	*resets;
	const char *const		*fw_parents;
	u8				num_clks;
	u8				num_resets;
	u8				num_fw_parents;
};

struct bflb_clk_plat {
	void __iomem			*base;
	const struct bflb_clk_desc	*desc;
};

#endif /* _CLK_BFLB_H_ */
