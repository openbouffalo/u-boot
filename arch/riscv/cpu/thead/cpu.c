// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <irq_func.h>
#include <asm/cache.h>
#include <asm/csr.h>
#include <linux/bitops.h>

#include "thead_csr.h"

/*
 * cleanup_before_linux() is called just before we call linux
 * it prepares the processor for linux
 *
 * we disable interrupt and caches.
 */
int cleanup_before_linux(void)
{
	disable_interrupts();

	cache_flush();

	return 0;
}

void harts_early_init(void)
{
	if (!CONFIG_IS_ENABLED(RISCV_MMODE))
		return;

	csr_set(CSR_MXSTATUS, MXSTATUS_THEADISAEE | MXSTATUS_MM);
	if (IS_ENABLED(THEAD_C906)) {
		csr_set(CSR_MHCR,
			MHCR_BTB_C906 | MHCR_BPE | MHCR_RS | MHCR_WB | MHCR_WA);
		csr_set(CSR_MHINT, MHINT_IWPE | MHINT_IPLD | MHINT_IPLD);
	}
	if (IS_ENABLED(THEAD_E906)) {
		csr_set(CSR_MHCR,
			MHCR_BTB_E906 | MHCR_BPE | MHCR_RS | MHCR_WB | MHCR_WA);
	}
}
