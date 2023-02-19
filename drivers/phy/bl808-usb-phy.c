// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <generic-phy.h>
#include <asm/io.h>
#include <bl808/pds_reg.h>
#include <linux/delay.h>

struct bl808_usb_phy_plat {
	void *base;
};

static int bl808_usb_phy_init(struct phy *phy)
{
	struct bl808_usb_phy_plat *plat = dev_get_plat(phy->dev);

	clrbits_le32(plat->base + PDS_USB_PHY_CTRL_OFFSET,
		     PDS_REG_USB_PHY_XTLSEL_MSK);

	setbits_le32(plat->base + PDS_USB_PHY_CTRL_OFFSET,
		     PDS_REG_PU_USB20_PSW_MSK);

	setbits_le32(plat->base + PDS_USB_PHY_CTRL_OFFSET,
		     PDS_REG_USB_PHY_PONRST_MSK);
	udelay(1);

	clrbits_le32(plat->base + PDS_USB_CTL_OFFSET,
		     PDS_REG_USB_SW_RST_N_MSK);
	udelay(1);

	setbits_le32(plat->base + PDS_USB_CTL_OFFSET,
		     PDS_REG_USB_EXT_SUSP_N_MSK);
	mdelay(3);

	setbits_le32(plat->base + PDS_USB_CTL_OFFSET,
		     PDS_REG_USB_SW_RST_N_MSK);
	mdelay(2);

	return 0;
}

static int bl808_usb_phy_exit(struct phy *phy)
{
	struct bl808_usb_phy_plat *plat = dev_get_plat(phy->dev);

	clrbits_le32(plat->base + PDS_USB_PHY_CTRL_OFFSET,
		     PDS_REG_USB_PHY_PONRST_MSK);

	clrbits_le32(plat->base + PDS_USB_PHY_CTRL_OFFSET,
		     PDS_REG_PU_USB20_PSW_MSK);

	return 0;
}

static const struct phy_ops bl808_usb_phy_ops = {
	.init		= bl808_usb_phy_init,
	.exit		= bl808_usb_phy_exit,
};

static int bl808_usb_phy_of_to_plat(struct udevice *dev)
{
	struct bl808_usb_phy_plat *plat = dev_get_plat(dev);

	plat->base = dev_read_addr_ptr(dev);
	if (!plat->base)
		return -ENOMEM;

	plat->base -= PDS_USB_CTL_OFFSET;

	return 0;
}

static const struct udevice_id bl808_usb_phy_ids[] = {
	{ .compatible = "bflb,bl808-usb-phy" },
	{ }
};

U_BOOT_DRIVER(bl808_usb_phy) = {
	.name		= "bl808_usb_phy",
	.id		= UCLASS_PHY,
	.of_match	= bl808_usb_phy_ids,
	.of_to_plat	= bl808_usb_phy_of_to_plat,
	.plat_auto	= sizeof(struct bl808_usb_phy_plat),
	.ops		= &bl808_usb_phy_ops,
};
