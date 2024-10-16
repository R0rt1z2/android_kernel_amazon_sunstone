/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef SSHEAP_PRIV_H
#define SSHEAP_PRIV_H

/* prefer_align: 0 (use default alignment) */
struct ssheap_buf_info *ssheap_alloc_non_contig(u32 req_size, u32 prefer_align, u8 mem_type);

int ssheap_free_non_contig(struct ssheap_buf_info *info);

uint64_t ssheap_get_used_size(void);

unsigned long mtee_assign_buffer(struct ssheap_buf_info *info, uint8_t mem_type);
unsigned long mtee_unassign_buffer(struct ssheap_buf_info *info, uint8_t mem_type);


#if IS_ENABLED(CONFIG_TEST_MTK_TRUSTED_MEMORY)
extern struct device *ssheap_dev;
extern phys_addr_t ssheap_base;
extern phys_addr_t ssheap_size;
extern atomic64_t ssheap_total_allocated_size;
void create_ssheap_ut_device(void);
#endif

#endif
