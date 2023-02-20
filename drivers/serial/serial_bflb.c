// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Samuel Holland <samuel@sholland.org>
 */

#include <clk.h>
#include <debug_uart.h>
#include <dm.h>
#include <serial.h>
#include <asm/io.h>
#include <linux/bitfield.h>

#define UART_TX_CFG			0x00
#define UART_TX_CFG_TX_BIT_CNT_P		GENMASK(12, 11)
#define UART_TX_CFG_TX_BIT_CNT_D		GENMASK(10,  8)
#define UART_TX_CFG_TX_FRM_EN			BIT(2)
#define UART_TX_CFG_TX_EN			BIT(0)
#define UART_RX_CFG			0x04
#define UART_RX_CFG_RX_BIT_CNT_D		GENMASK(10,  8)
#define UART_RX_CFG_RX_EN			BIT(0)
#define UART_BIT_PRD			0x08
#define UART_BIT_PRD_RX_BIT_PRD			GENMASK(31, 16)
#define UART_BIT_PRD_TX_BIT_PRD			GENMASK(15,  0)
#define UART_FIFO_CFG0			0x80
#define UART_FIFO_CFG0_RX_FIFO_CLR		BIT(3)
#define UART_FIFO_CFG0_TX_FIFO_CLR		BIT(2)
#define UART_FIFO_CFG1			0x84
#define UART_FIFO_CFG1_RX_FIFO_CNT		GENMASK(13,  8)
#define UART_FIFO_CFG1_TX_FIFO_CNT		GENMASK( 5,  0)
#define UART_FIFO_WDATA			0x88
#define UART_FIFO_RDATA			0x8c

#define UART_FIFO_SIZE			32

struct bflb_serial_plat {
	void		*base;
	struct clk	clk;
	ulong		clock;
};

static int _bflb_serial_rx_avail(void *base)
{
	u32 reg = readl(base + UART_FIFO_CFG1);

	return FIELD_GET(UART_FIFO_CFG1_RX_FIFO_CNT, reg);
}

static int _bflb_serial_tx_avail(void *base)
{
	u32 reg = readl(base + UART_FIFO_CFG1);

	return FIELD_GET(UART_FIFO_CFG1_TX_FIFO_CNT, reg);
}

static int _bflb_serial_getc(void *base)
{
	if (!_bflb_serial_rx_avail(base))
		return -EAGAIN;

	return readl(base + UART_FIFO_RDATA);
}

static int _bflb_serial_putc(void *base, char c)
{
	if (!_bflb_serial_tx_avail(base))
		return -EAGAIN;

	writel(c, base + UART_FIFO_WDATA);

	return 0;
}

static int _bflb_serial_setbrg(void *base, ulong clock, ulong baud)
{
	u32 period, reg;

	period = clock / baud - 1;
	reg = FIELD_PREP(UART_BIT_PRD_RX_BIT_PRD, period) |
	      FIELD_PREP(UART_BIT_PRD_TX_BIT_PRD, period);

	writel(reg, base + UART_BIT_PRD);

	return 0;
}

static int _bflb_serial_init(void *base)
{
	u32 reg;

	reg = FIELD_PREP(UART_TX_CFG_TX_BIT_CNT_P, 2) |
	      FIELD_PREP(UART_TX_CFG_TX_BIT_CNT_D, 7) |
	      UART_TX_CFG_TX_FRM_EN |
	      UART_TX_CFG_TX_EN;
	writel(reg, base + UART_TX_CFG);

	reg = FIELD_PREP(UART_RX_CFG_RX_BIT_CNT_D, 7) |
	      UART_RX_CFG_RX_EN;
	writel(reg, base + UART_RX_CFG);

	return 0;
}

static int bflb_serial_setbrg(struct udevice *dev, int baud)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	return _bflb_serial_setbrg(plat->base, plat->clock, baud);
}

static int bflb_serial_getc(struct udevice *dev)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	return _bflb_serial_getc(plat->base);
}

static int bflb_serial_putc(struct udevice *dev, char c)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	return _bflb_serial_putc(plat->base, c);
}

static int bflb_serial_pending(struct udevice *dev, bool input)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	if (input)
		return _bflb_serial_rx_avail(plat->base);
	else
		return UART_FIFO_SIZE - _bflb_serial_tx_avail(plat->base);
}

static int bflb_serial_clear(struct udevice *dev)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);
	u32 reg;

	reg = readl(plat->base + UART_FIFO_CFG0);
	reg |= UART_FIFO_CFG0_RX_FIFO_CLR | UART_FIFO_CFG0_TX_FIFO_CLR;
	writel(reg, plat->base + UART_FIFO_CFG0);

	return 0;
}

static const struct dm_serial_ops bflb_serial_ops = {
	.setbrg		= bflb_serial_setbrg,
	.getc		= bflb_serial_getc,
	.putc		= bflb_serial_putc,
	.pending	= bflb_serial_pending,
	.clear		= bflb_serial_clear,
};

static int bflb_serial_probe(struct udevice *dev)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	clk_enable(&plat->clk);

	return _bflb_serial_init(plat->base);
}

static int bflb_serial_of_to_plat(struct udevice *dev)
{
	struct bflb_serial_plat *plat = dev_get_plat(dev);

	plat->base = dev_remap_addr(dev);
	if (!plat->base)
		return -ENOMEM;

	if (!clk_get_by_index(dev, 0, &plat->clk)) {
		plat->clock = clk_get_rate(&plat->clk);
		if (IS_ERR_VALUE(plat->clock))
			return plat->clock;
	} else {
		u32 clock = 0;

		dev_read_u32(dev, "clock-frequency", &clock);
		plat->clock = clock;
	}

	return 0;
}

static const struct udevice_id bflb_serial_ids[] = {
	{ .compatible = "bflb,bl808-uart" },
	{ }
};

U_BOOT_DRIVER(bflb_serial) = {
	.name		= "bflb_serial",
	.id		= UCLASS_SERIAL,
	.of_match	= bflb_serial_ids,
	.probe		= bflb_serial_probe,
	.of_to_plat	= bflb_serial_of_to_plat,
	.plat_auto	= sizeof(struct bflb_serial_plat),
	.ops		= &bflb_serial_ops,
	.flags		= DM_FLAG_PRE_RELOC,
};

#ifdef CONFIG_DEBUG_UART_BFLB
static inline void _debug_uart_init(void)
{
	void *base = (void *)CONFIG_VAL(DEBUG_UART_BASE);

	_bflb_serial_init(base);
	_bflb_serial_setbrg(base, CONFIG_DEBUG_UART_CLOCK, CONFIG_BAUDRATE);
}

static inline void _debug_uart_putc(char c)
{
	void *base = (void *)CONFIG_VAL(DEBUG_UART_BASE);

	while (_bflb_serial_putc(base, c) == -EAGAIN)
		schedule();
}

DEBUG_UART_FUNCS
#endif
