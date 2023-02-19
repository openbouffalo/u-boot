// SPDX-License-Identifier: GPL-2.0

#include <dm.h>
#include <errno.h>
#include <bl808/glb_reg.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/pinctrl.h>
#include <linux/bitfield.h>

#define BFLB_FUNC_GPIO	11

struct bflb_pinctrl_desc {
	const char *const	*functions;
	u8			num_functions;
	u8			num_pins;
};

struct bflb_pinctrl_plat {
	void __iomem			*base;
	const struct bflb_pinctrl_desc	*desc;
};

static int bflb_gpio_get_value(struct udevice *dev, unsigned offset)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	u32 val;

	val = readl(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * offset);

	return FIELD_GET(GLB_REG_GPIO_0_I_MSK, val);
}

static int bflb_gpio_get_function(struct udevice *dev, unsigned offset)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	u32 val;

	val = readl(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * offset);

	if (!FIELD_GET(GLB_REG_GPIO_0_IE_MSK, val) &&
	    !FIELD_GET(GLB_REG_GPIO_0_OE_MSK, val))
		return GPIOF_UNUSED;

	if (FIELD_GET(GLB_REG_GPIO_0_FUNC_SEL_MSK, val) != BFLB_FUNC_GPIO)
		return GPIOF_FUNC;
	else if (FIELD_GET(GLB_REG_GPIO_0_OE_MSK, val))
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static int bflb_gpio_set_flags(struct udevice *dev, unsigned int offset,
			       ulong flags)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	u32 val;

	val = readl(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * offset);
	val &= ~(GLB_REG_GPIO_0_IE_MSK |
		 GLB_REG_GPIO_0_PU_MSK |
		 GLB_REG_GPIO_0_PD_MSK |
		 GLB_REG_GPIO_0_OE_MSK |
		 GLB_REG_GPIO_0_FUNC_SEL_MSK |
		 GLB_REG_GPIO_0_O_MSK);
	val |= FIELD_PREP(GLB_REG_GPIO_0_FUNC_SEL_MSK, BFLB_FUNC_GPIO);

	if (flags & GPIOD_IS_OUT) {
		val |= GLB_REG_GPIO_0_OE_MSK;

		if (flags & GPIOD_IS_OUT_ACTIVE)
			val |= GLB_REG_GPIO_0_O_MSK;
	} else if (flags & GPIOD_IS_IN) {
		val |= GLB_REG_GPIO_0_IE_MSK;

		if (flags & GPIOD_PULL_UP)
			val |= GLB_REG_GPIO_0_PU_MSK;
		else if (flags & GPIOD_PULL_DOWN)
			val |= GLB_REG_GPIO_0_PD_MSK;
	}

	writel(val, plat->base + GLB_GPIO_CFG0_OFFSET + 4 * offset);

	return 0;
}

static const struct dm_gpio_ops bflb_gpio_ops = {
	.get_value		= bflb_gpio_get_value,
	.get_function		= bflb_gpio_get_function,
	.set_flags		= bflb_gpio_set_flags,
};

static int bflb_gpio_probe(struct udevice *dev)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);

	uc_priv->bank_name = "GPIO";
	uc_priv->gpio_count = plat->desc->num_pins;

	return 0;
}

static struct driver bflb_gpio_driver = {
	.name		= "bflb_gpio",
	.id		= UCLASS_GPIO,
	.probe		= bflb_gpio_probe,
	.ops		= &bflb_gpio_ops,
};

static int bflb_pinctrl_get_pins_count(struct udevice *dev)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);

	return plat->desc->num_pins;
}

static const char *bflb_pinctrl_get_pin_name(struct udevice *dev,
					     uint pin_selector)
{
	static char pin_name[sizeof("GPIO??")] = "GPIO??";

	snprintf(pin_name + 4, sizeof(pin_name) - 4, "%d", pin_selector);

	return pin_name;
}

static int bflb_pinctrl_get_functions_count(struct udevice *dev)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);

	return plat->desc->num_functions;
}

static const char *bflb_pinctrl_get_function_name(struct udevice *dev,
						  uint func_selector)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);

	return plat->desc->functions[func_selector] ?: "";
}

static int bflb_pinctrl_pinmux_set(struct udevice *dev, uint pin_selector,
				    uint func_selector)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);

	clrsetbits_le32(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * pin_selector,
			GLB_REG_GPIO_0_FUNC_SEL_MSK,
			GLB_REG_GPIO_0_IE_MSK |
			FIELD_PREP(GLB_REG_GPIO_0_FUNC_SEL_MSK, func_selector));

	return 0;
}

static const struct pinconf_param bflb_pinctrl_pinconf_params[] = {
	{ "bias-disable",		PIN_CONFIG_BIAS_DISABLE,	 0 },
	{ "bias-pull-down",		PIN_CONFIG_BIAS_PULL_DOWN,	 1 },
	{ "bias-pull-up",		PIN_CONFIG_BIAS_PULL_UP,	 1 },
	{ "drive-strength",		PIN_CONFIG_DRIVE_STRENGTH,	 0 },
	{ "input-disable",		PIN_CONFIG_INPUT_ENABLE,	 0 },
	{ "input-enable",		PIN_CONFIG_INPUT_ENABLE,	 1 },
	{ "input-schmitt-disable",	PIN_CONFIG_INPUT_SCHMITT_ENABLE, 0 },
	{ "input-schmitt-enable",	PIN_CONFIG_INPUT_SCHMITT_ENABLE, 1 },
	{ "output-disable",		PIN_CONFIG_OUTPUT_ENABLE,	 0 },
	{ "output-enable",		PIN_CONFIG_OUTPUT_ENABLE,	 1 },
};

static int bflb_pinctrl_pinconf_set(struct udevice *dev, uint pin_selector,
				     uint param, uint val)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	u32 mask;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		mask = GLB_REG_GPIO_0_PU_MSK |
		       GLB_REG_GPIO_0_PD_MSK;
		val = 0;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		mask = GLB_REG_GPIO_0_PU_MSK |
		       GLB_REG_GPIO_0_PD_MSK;
		if (!val)
			return -EINVAL;
		val = GLB_REG_GPIO_0_PD_MSK;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		mask = GLB_REG_GPIO_0_PU_MSK |
		       GLB_REG_GPIO_0_PD_MSK;
		if (!val)
			return -EINVAL;
		val = GLB_REG_GPIO_0_PU_MSK;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		mask = GLB_REG_GPIO_0_DRV_MSK;
		val <<= GLB_REG_GPIO_0_DRV_POS;
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		mask = GLB_REG_GPIO_0_IE_MSK;
		val = val ? GLB_REG_GPIO_0_IE_MSK : 0;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		mask = GLB_REG_GPIO_0_SMT_MSK;
		val = val ? GLB_REG_GPIO_0_SMT_MSK : 0;
		break;
	case PIN_CONFIG_OUTPUT_ENABLE:
		mask = GLB_REG_GPIO_0_OE_MSK;
		val = val ? GLB_REG_GPIO_0_OE_MSK : 0;
		break;
	default:
		return -EINVAL;
	}

	clrsetbits_le32(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * pin_selector,
			mask, val);

	return 0;
}

static int bflb_pinctrl_get_pin_muxing(struct udevice *dev, uint pin_selector,
				       char *buf, int size)
{
	const struct bflb_pinctrl_plat *plat = dev_get_plat(dev);
	const char *function;
	u32 val;

	val = readl(plat->base + GLB_GPIO_CFG0_OFFSET + 4 * pin_selector);
	val = FIELD_GET(GLB_REG_GPIO_0_FUNC_SEL_MSK, val);

	function = plat->desc->functions[val];
	if (function)
		strlcpy(buf, function, size);
	else
		snprintf(buf, size, "function %d", val);

	return 0;
}

static const struct pinctrl_ops bflb_pinctrl_ops = {
	.get_pins_count		= bflb_pinctrl_get_pins_count,
	.get_pin_name		= bflb_pinctrl_get_pin_name,
	.get_functions_count	= bflb_pinctrl_get_functions_count,
	.get_function_name	= bflb_pinctrl_get_function_name,
	.pinmux_set		= bflb_pinctrl_pinmux_set,
	.pinconf_num_params	= ARRAY_SIZE(bflb_pinctrl_pinconf_params),
	.pinconf_params		= bflb_pinctrl_pinconf_params,
	.pinconf_set		= bflb_pinctrl_pinconf_set,
	.set_state		= pinctrl_generic_set_state,
	.get_pin_muxing		= bflb_pinctrl_get_pin_muxing,
};

static int bflb_pinctrl_bind(struct udevice *dev)
{
	return device_bind(dev, &bflb_gpio_driver, "gpio",
			   dev_get_plat(dev), dev_ofnode(dev), NULL);
}

static int bflb_pinctrl_of_to_plat(struct udevice *dev)
{
	struct bflb_pinctrl_plat *plat = dev_get_plat(dev);

	plat->base = dev_remap_addr(dev);
	if (!plat->base)
		return -ENOMEM;

	plat->desc = (const struct bflb_pinctrl_desc *)dev_get_driver_data(dev);
	if (!plat->desc)
		return -EINVAL;

	return 0;
}

static const char *const bl616_pinctrl_functions[] = {
	[0]	= "sdh",
	[1]	= "spi0",
	[2]	= "flash",
	[3]	= "i2s",
	[4]	= "pdm",
	[5]	= "i2c0",
	[6]	= "i2c1",
	[7]	= "uart",
	[8]	= "emac",
	[9]	= "cam",
	[10]	= "analog",
	[11]	= "gpio",
	[12]	= "sdu",
	[16]	= "pwm0",
	[24]	= "qspi",
	[25]	= "aupwm",
	[26]	= "jtag",
	[27]	= "pec",
	[31]	= "clkout",
};

static const struct bflb_pinctrl_desc bl616_pinctrl_desc = {
	.functions	= bl616_pinctrl_functions,
	.num_functions	= ARRAY_SIZE(bl616_pinctrl_functions),
	.num_pins	= 35,
};

static const char *const bl808_pinctrl_functions[] = {
	[0]	= "sdh",
	[1]	= "spi0",
	[2]	= "flash",
	[3]	= "i2s",
	[4]	= "pdm",
	[5]	= "i2c0",
	[6]	= "i2c1",
	[7]	= "uart",
	[8]	= "emac",
	[9]	= "cam",
	[10]	= "analog",
	[11]	= "gpio",
	[12]	= "sdu",
	[16]	= "pwm0",
	[17]	= "pwm1",
	[18]	= "spi1",
	[19]	= "i2c2",
	[20]	= "i2c3",
	[21]	= "mm_uart",
	[22]	= "dbi_b",
	[23]	= "dbi_c",
	[24]	= "dpi",
	[25]	= "jtag_lp",
	[26]	= "jtag_m0",
	[27]	= "jtag_d0",
	[31]	= "clkout",
};

static const struct bflb_pinctrl_desc bl808_pinctrl_desc = {
	.functions	= bl808_pinctrl_functions,
	.num_functions	= ARRAY_SIZE(bl808_pinctrl_functions),
	.num_pins	= 46,
};

static const struct udevice_id bflb_pinctrl_ids[] = {
	{ .compatible = "bflb,bl616-gpio",
	  .data = (ulong)&bl616_pinctrl_desc },
	{ .compatible = "bflb,bl808-gpio",
	  .data = (ulong)&bl808_pinctrl_desc },
	{ }
};

U_BOOT_DRIVER(bflb_pinctrl) = {
	.name		= "bflb_pinctrl",
	.id		= UCLASS_PINCTRL,
	.of_match	= bflb_pinctrl_ids,
	.bind		= bflb_pinctrl_bind,
	.of_to_plat	= bflb_pinctrl_of_to_plat,
	.plat_auto	= sizeof(struct bflb_pinctrl_plat),
	.ops		= &bflb_pinctrl_ops,
};
