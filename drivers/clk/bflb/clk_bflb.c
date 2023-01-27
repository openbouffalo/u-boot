// SPDX-License-Identifier: GPL-2.0+

#include <clk-uclass.h>
#include <dm.h>
#include <asm/io.h>
#include <clk/bflb.h>
#include <dm/device-internal.h>
#include <dm/lists.h>

extern U_BOOT_DRIVER(bflb_reset);

static struct clk *
bflb_clk_get_current_parent(const struct clk *clk)
{
	struct clk *current_parents = dev_get_priv(clk->dev);

	return &current_parents[clk->id];
}

static const struct bflb_clk_data *
bflb_clk_get_data(const struct bflb_clk_plat *plat, const struct clk *clk)
{
	const struct bflb_clk_desc *desc = plat->desc;

	if (clk->id >= desc->num_clks)
		return NULL;

	return &desc->clks[clk->id];
}

static int bflb_clk_request(struct clk *clk)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);

	return bflb_clk_get_data(plat, clk) ? 0 : -EINVAL;
}

static u32 bflb_clk_count_parents(const struct bflb_clk_data *data)
{
	if (!data->sel_mask)
		return 1;

	return data->sel_mask / (data->sel_mask & -data->sel_mask) + 1;
}

static int bflb_clk_resolve_parent(struct udevice *dev,
				   const struct bflb_clk_data *data,
				   u32 sel, struct clk *out)
{
	const struct bflb_clk_plat *plat = dev_get_plat(dev);
	struct clk parent = {};
	u8 id;

	if (!data->parents)
		return -ENOENT;

	id = data->parents[sel];
	if (id == NO_PARENT) {
		return -ENOENT;
	} else if (id < FW_PARENT_BASE) {
		parent.dev = dev;
		parent.id = id;
	} else if (id < FW_PARENT_BASE + plat->desc->num_fw_parents) {
		const char *name = plat->desc->fw_parents[id - FW_PARENT_BASE];
		int ret;

		ret = clk_get_by_name(dev, name, &parent);
		if (ret)
			return ret;
	} else {
		return -EINVAL;
	}

	*out = parent;

	return 0;
}

static ulong bflb_clk_calc_rate(struct clk *clk, ulong req,
				u32 *best_sel, u32 *best_div)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);
	u32 num_sels = bflb_clk_count_parents(data), num_divs = 1;
	ulong best_rate = 0;

	if (data->div_mask) {
		u32 div_lsb = data->div_mask & -data->div_mask;

		num_divs = data->div_mask / div_lsb + 1;
	}

	for (u32 sel = 0; sel < num_sels; ++sel) {
		struct clk parent;
		ulong parent_rate;
		int ret;

		ret = bflb_clk_resolve_parent(clk->dev, data, sel, &parent);
		if (ret)
			continue;

		parent_rate = clk_get_rate(&parent);
		if (IS_ERR_VALUE(parent_rate))
			continue;

		if (data->fixed_div)
			parent_rate /= data->fixed_div;

		for (u32 div = 0; div < num_divs; ++div) {
			ulong rate = parent_rate / (1 + div);

			if (rate > req || rate <= best_rate)
				continue;

			*best_sel = sel;
			*best_div = div;
			best_rate = rate;
		}
	}

	return best_rate;
}

static ulong bflb_clk_round_rate(struct clk *clk, ulong req)
{
	u32 sel, div;

	return bflb_clk_calc_rate(clk, req, &sel, &div);
}

static ulong bflb_clk_get_rate(struct clk *clk)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);
	ulong rate;

	if (data->rate)
		return data->rate;

	rate = clk_get_parent_rate(clk);
	if (IS_ERR_VALUE(rate))
		return rate;

	if (data->fixed_div)
		rate /= data->fixed_div;

	if (data->div_mask) {
		u32 div = readl(plat->base + data->div_reg);

		div &= data->div_mask;
		div /= data->div_mask & -data->div_mask;

		rate /= 1 + div;
	}

	return rate;
}

static ulong bflb_clk_set_rate(struct clk *clk, ulong req)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);
	u32 sel, div;
	ulong rate;

	rate = bflb_clk_calc_rate(clk, req, &sel, &div);
	if (!rate)
		return rate;

	if (data->sel_mask) {
		u32 sel_lsb = data->sel_mask & -data->sel_mask;

		clrsetbits_le32(plat->base + data->sel_reg,
				data->sel_mask, sel * sel_lsb);
	}

	if (data->div_mask) {
		u32 div_lsb = data->div_mask & -data->div_mask;

		clrsetbits_le32(plat->base + data->div_reg,
				data->div_mask, div * div_lsb);
	}

	return 0;
}

static struct clk *bflb_clk_get_parent(struct clk *clk)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);
	struct clk *current_parent = bflb_clk_get_current_parent(clk);

	if (!clk_valid(current_parent)) {
		u32 sel = 0;
		int ret;

		if (data->sel_mask) {
			sel = readl(plat->base + data->sel_reg);
			sel &= data->sel_mask;
			sel /= data->sel_mask & -data->sel_mask;
		}

		ret = bflb_clk_resolve_parent(clk->dev, data, sel,
					      current_parent);
		if (ret)
			return ERR_PTR(ret);
	}

	return current_parent;
}

static int bflb_clk_set_parent(struct clk *clk, struct clk *parent)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);
	struct clk *current_parent = bflb_clk_get_current_parent(clk);
	u32 num_sels = bflb_clk_count_parents(data);
	u32 sel, sel_lsb;
	struct clk p;
	int ret;

	/* Find the selector value corresponding to the requested parent. */
	for (sel = 0; sel < num_sels; ++sel) {
		ret = bflb_clk_resolve_parent(clk->dev, data, sel, &p);
		if (!ret && p.dev == parent->dev && p.id == parent->id)
			break;
	}
	if (sel == num_sels)
		return -ENOENT;

	if (data->sel_mask)
		clrsetbits_le32(plat->base + data->sel_reg,
				data->sel_mask, sel * sel_lsb);

	*current_parent = p;

	return 0;
}

static int bflb_clk_set_gate(struct clk *clk, bool enable)
{
	const struct bflb_clk_plat *plat = dev_get_plat(clk->dev);
	const struct bflb_clk_data *data = bflb_clk_get_data(plat, clk);

	clrsetbits_le32(plat->base + data->en_reg,
			data->en_mask, enable ? data->en_mask : 0);

	return 0;
}

static int bflb_clk_enable(struct clk *clk)
{
	struct clk *parent;
	int ret;

	parent = bflb_clk_get_parent(clk);
	if (clk_valid(parent)) {
		ret = clk_enable(parent);
		if (ret)
			return ret;
	}

	return bflb_clk_set_gate(clk, true);
}

static int bflb_clk_disable(struct clk *clk)
{
	return bflb_clk_set_gate(clk, false);
}

static struct clk_ops bflb_clk_ops = {
	.request	= bflb_clk_request,
	.round_rate	= bflb_clk_round_rate,
	.get_rate	= bflb_clk_get_rate,
	.set_rate	= bflb_clk_set_rate,
	.get_parent	= bflb_clk_get_parent,
	.set_parent	= bflb_clk_set_parent,
	.enable		= bflb_clk_enable,
	.disable	= bflb_clk_disable,
};

static int bflb_clk_bind(struct udevice *dev)
{
	if (IS_ENABLED(CONFIG_RESET_BFLB)) {
		device_bind(dev, DM_DRIVER_REF(bflb_reset), "reset",
			    dev_get_plat(dev), dev_ofnode(dev), NULL);
	}

	return 0;
}

static int bflb_clk_probe(struct udevice *dev)
{
	const struct bflb_clk_plat *plat = dev_get_plat(dev);
	struct clk *current_parents;

	current_parents = calloc(sizeof(struct clk), plat->desc->num_clks);
	if (!current_parents)
		return -ENOMEM;

	dev_set_priv(dev, current_parents);

	return 0;
}

static int bflb_clk_of_to_plat(struct udevice *dev)
{
	struct bflb_clk_plat *plat = dev_get_plat(dev);

	plat->base = dev_remap_addr(dev);
	if (!plat->base)
		return -ENOMEM;

	plat->desc = (const struct bflb_clk_desc *)dev_get_driver_data(dev);
	if (!plat->desc)
		return -EINVAL;

	return 0;
}

static const struct udevice_id bflb_clk_ids[] = {
	{ }
};

U_BOOT_DRIVER(bflb_clk) = {
	.name		= "bflb_clk",
	.id		= UCLASS_CLK,
	.of_match	= bflb_clk_ids,
	.bind		= bflb_clk_bind,
	.probe		= bflb_clk_probe,
	.of_to_plat	= bflb_clk_of_to_plat,
	.plat_auto	= sizeof(struct bflb_clk_plat),
	.ops		= &bflb_clk_ops,
};
