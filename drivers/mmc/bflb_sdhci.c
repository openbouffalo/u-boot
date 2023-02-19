// SPDX-License-Identifier: GPL-2.0+

#include <clk.h>
#include <dm.h>
#include <mmc.h>
#include <sdhci.h>

struct bflb_sdhci_plat {
	struct mmc_config	cfg;
	struct mmc		mmc;
};

static int bflb_sdhci_bind(struct udevice *dev)
{
	struct bflb_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int bflb_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct bflb_sdhci_plat *plat = dev_get_plat(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	struct clk clk;
	int ret;

	host->name = dev->name;
	host->ioaddr = dev_remap_addr(dev);
	if (!host->ioaddr)
		return -ENOMEM;

	ret = clk_get_by_name(dev, "mod", &clk);
	if (ret)
		return ret;

	ret = clk_enable(&clk);
	if (ret)
		return ret;

	host->max_clk = clk_get_rate(&clk);
	if (IS_ERR_VALUE(host->max_clk)) {
		ret = host->max_clk;
		goto err_clk_disable;
	}

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;
	upriv->mmc = &plat->mmc;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 0);
	if (ret)
		goto err_clk_disable;

	ret = sdhci_probe(dev);
	if (ret)
		goto err_clk_disable;

	return 0;

err_clk_disable:
	clk_disable(&clk);

	return ret;
}

static const struct udevice_id bflb_sdhci_match[] = {
	{ .compatible = "bflb,bl808-sdhci" },
	{ }
};

U_BOOT_DRIVER(bflb_sdhci) = {
	.name		= "bflb_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= bflb_sdhci_match,
	.bind		= bflb_sdhci_bind,
	.probe		= bflb_sdhci_probe,
	.priv_auto	= sizeof(struct sdhci_host),
	.plat_auto	= sizeof(struct bflb_sdhci_plat),
	.ops		= &sdhci_ops,
};
