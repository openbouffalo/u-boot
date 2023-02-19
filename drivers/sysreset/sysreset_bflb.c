// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <sysreset.h>
#include <asm/io.h>
#include <bl808/glb_reg.h>

struct bflb_sysreset_plat {
	void *base;
};

static int bflb_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	struct bflb_sysreset_plat *plat = dev_get_plat(dev);

	setbits_le32(plat->base + GLB_SWRST_CFG2_OFFSET,
		     GLB_REG_CTRL_PWRON_RST_MSK);

	return -EINPROGRESS;
}

static struct sysreset_ops bflb_sysreset_ops = {
	.request = bflb_sysreset_request,
};

static int bflb_sysreset_of_to_plat(struct udevice *dev)
{
	struct bflb_sysreset_plat *plat = dev_get_plat(dev);

	plat->base = dev_remap_addr(dev);
	if (!plat->base)
		return -ENOMEM;

	return 0;
}

static const struct udevice_id bflb_sysreset_ids[] = {
	{ .compatible = "bflb,bl808-glb-sysreset" },
	{ }
};

U_BOOT_DRIVER(bflb_sysreset) = {
	.name		= "bflb_sysreset",
	.id		= UCLASS_SYSRESET,
	.of_match	= bflb_sysreset_ids,
	.of_to_plat	= bflb_sysreset_of_to_plat,
	.plat_auto	= sizeof(struct bflb_sysreset_plat),
	.ops		= &bflb_sysreset_ops,
};
