// SPDX-License-Identifier: GPL-2.0+

#include <clk.h>
#include <dm.h>
#include <remoteproc.h>
#include <reset.h>
#include <asm/io.h>
#include <bl808/mm_misc_reg.h>

struct bflb_rproc_plat {
	void __iomem		*base;
	struct clk		clk;
	struct reset_ctl	reset;
};

static int bflb_rproc_init(struct udevice *dev)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);

	clk_set_rate(&plat->clk, 240000000);

	return 0;
}

static int bflb_rproc_load(struct udevice *dev, ulong addr, ulong size)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);

	/* Set the CPU entry point. */
	writel(rproc_elf_get_boot_addr(dev, addr),
	       plat->base + MM_MISC_CPU0_BOOT_OFFSET);

	return rproc_elf_load_image(dev, addr, size);
}

static int bflb_rproc_start(struct udevice *dev)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);
	int ret;

	reset_assert(&plat->reset);

	ret = clk_enable(&plat->clk);
	if (ret)
		return ret;

	/* Set up the CPU time base. */
	u32 divider = clk_get_rate(&plat->clk) / 1000000 - 1;
	writel(divider | MM_MISC_C906_RTC_RST_MSK,
	       plat->base + MM_MISC_CPU_RTC_OFFSET);
	writel(divider | MM_MISC_C906_RTC_EN_MSK,
	       plat->base + MM_MISC_CPU_RTC_OFFSET);

	ret = reset_deassert(&plat->reset);
	if (ret)
		goto err_disable_clk;

	return 0;

err_disable_clk:
	clk_disable(&plat->clk);

	return ret;
}

static int bflb_rproc_stop(struct udevice *dev)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);

	reset_assert(&plat->reset);
	clk_disable(&plat->clk);

	return 0;
}

static int bflb_rproc_reset(struct udevice *dev)
{
	bflb_rproc_stop(dev);

	return bflb_rproc_start(dev);
}

static int bflb_rproc_is_running(struct udevice *dev)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);

	return reset_status(&plat->reset);
}

static const struct dm_rproc_ops bflb_rproc_ops = {
	.init		= bflb_rproc_init,
	.load		= bflb_rproc_load,
	.start		= bflb_rproc_start,
	.stop		= bflb_rproc_stop,
	.reset		= bflb_rproc_reset,
	.is_running	= bflb_rproc_is_running,
};

static int bflb_rproc_of_to_plat(struct udevice *dev)
{
	struct bflb_rproc_plat *plat = dev_get_plat(dev);
	int ret;

	plat->base = dev_remap_addr(dev);
	if (!plat->base)
		return -ENOMEM;

	ret = clk_get_by_index(dev, 0, &plat->clk);
	if (ret)
		return ret;

	ret = reset_get_by_index(dev, 0, &plat->reset);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id bflb_rproc_ids[] = {
	{ .compatible = "bflb,bl808-mm-misc" },
	{ }
};

U_BOOT_DRIVER(bflb_rproc) = {
	.name		= "bflb_rproc",
	.id		= UCLASS_REMOTEPROC,
	.of_match	= bflb_rproc_ids,
	.of_to_plat	= bflb_rproc_of_to_plat,
	.plat_auto	= sizeof(struct bflb_rproc_plat),
	.ops		= &bflb_rproc_ops,
};
