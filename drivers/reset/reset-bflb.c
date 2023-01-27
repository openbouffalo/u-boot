// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <reset-uclass.h>
#include <asm/io.h>
#include <clk/bflb.h>
#include <linux/bitops.h>

static const struct bflb_reset_data *
bflb_reset_get_data(const struct bflb_clk_plat *plat,
		    const struct reset_ctl *reset_ctl)
{
	const struct bflb_clk_desc *desc = plat->desc;

	if (reset_ctl->id >= desc->num_resets)
		return NULL;

	return &desc->resets[reset_ctl->id];
}

static int bflb_reset_request(struct reset_ctl *reset_ctl)
{
	const struct bflb_clk_plat *plat = dev_get_plat(reset_ctl->dev);

	return bflb_reset_get_data(plat, reset_ctl) ? 0 : -EINVAL;
}

static int bflb_reset_set(struct reset_ctl *reset_ctl, bool assert)
{
	const struct bflb_clk_plat *plat = dev_get_plat(reset_ctl->dev);
	const struct bflb_reset_data *data = bflb_reset_get_data(plat, reset_ctl);
	u32 mask = BIT(data->bit);

	clrsetbits_le32(plat->base + data->reg, mask, assert ? mask : 0);

	return 0;
}

static int bflb_reset_assert(struct reset_ctl *reset_ctl)
{
	return bflb_reset_set(reset_ctl, true);
}

static int bflb_reset_deassert(struct reset_ctl *reset_ctl)
{
	return bflb_reset_set(reset_ctl, false);
}

static int bflb_reset_status(struct reset_ctl *reset_ctl)
{
	const struct bflb_clk_plat *plat = dev_get_plat(reset_ctl->dev);
	const struct bflb_reset_data *data = bflb_reset_get_data(plat, reset_ctl);
	u32 mask = BIT(data->bit);

	return !!(readl(plat->base + data->reg) & mask);
}

static const struct reset_ops bflb_reset_ops = {
	.request	= bflb_reset_request,
	.rst_assert	= bflb_reset_assert,
	.rst_deassert	= bflb_reset_deassert,
	.rst_status	= bflb_reset_status,
};

U_BOOT_DRIVER(bflb_reset) = {
	.name		= "bflb_reset",
	.id		= UCLASS_RESET,
	.ops		= &bflb_reset_ops,
};
