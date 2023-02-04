// SPDX-License-Identifier: GPL-2.0+

#include <cpu_func.h>
#include <asm/cache.h>
#include <asm/csr.h>
#include <linux/bitops.h>

#include "thead_csr.h"

#define THEAD_SYNC_I		".long 0x01a0000b"
#define THEAD_DCACHE_CIALL	".long 0x0030000b"
#define THEAD_DCACHE_CIPA_A0	".long 0x02b5000b"
#define THEAD_DCACHE_IALL	".long 0x0020000b"
#define THEAD_DCACHE_IPA_A0	".long 0x02a5000b"
#define THEAD_ICACHE_IPA_A0	".long 0x0385000b"

static inline void sync_i(void)
{
	asm volatile (THEAD_SYNC_I ::: "memory");
}

void flush_dcache_all(void)
{
	asm volatile (THEAD_DCACHE_CIALL ::: "memory");
	sync_i();
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	register ulong addr asm ("a0") = start & -CONFIG_SYS_CACHELINE_SIZE;

	for (; addr < end; addr += CONFIG_SYS_CACHELINE_SIZE)
		asm volatile (THEAD_DCACHE_CIPA_A0 :: "r" (addr) : "memory");
	sync_i();
}

void invalidate_dcache_all(void)
{
	asm volatile (THEAD_DCACHE_IALL ::: "memory");
	sync_i();
}

void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	register ulong addr asm ("a0") = start & -CONFIG_SYS_CACHELINE_SIZE;

	for (; addr < end; addr += CONFIG_SYS_CACHELINE_SIZE)
		asm volatile (THEAD_DCACHE_IPA_A0 :: "r" (addr) : "memory");
	sync_i();
}

void invalidate_icache_range(unsigned long start, unsigned long end)
{
	register ulong addr asm ("a0") = start & -CONFIG_SYS_CACHELINE_SIZE;

	for (; addr < end; addr += CONFIG_SYS_CACHELINE_SIZE)
		asm volatile (THEAD_ICACHE_IPA_A0 :: "r" (addr) : "memory");
	sync_i();
}

#if CONFIG_IS_ENABLED(RISCV_MMODE)

void icache_enable(void)
{
	invalidate_icache_all();
	csr_set(CSR_MHCR, MHCR_IE);
}

void icache_disable(void)
{
	csr_clear(CSR_MHCR, MHCR_IE);
}

int icache_status(void)
{
	return csr_read(CSR_MHCR) & MHCR_IE;
}

void dcache_enable(void)
{
	invalidate_dcache_all();
	csr_set(CSR_MHCR, MHCR_DE);
}

void dcache_disable(void)
{
	flush_dcache_all();
	csr_clear(CSR_MHCR, MHCR_DE);
}

int dcache_status(void)
{
	return csr_read(CSR_MHCR) & MHCR_DE;
}

#endif
