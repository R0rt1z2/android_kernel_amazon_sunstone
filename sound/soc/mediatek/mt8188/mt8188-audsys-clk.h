/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8188-audsys-clk.h  --  Mediatek 8188 audsys clock definition
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Chun-Chia Chiu <chun-chia.chiu@mediatek.com>
 */

#ifndef _MT8188_AUDSYS_CLK_H_
#define _MT8188_AUDSYS_CLK_H_

int mt8188_audsys_clk_register(struct mtk_base_afe *afe);
void mt8188_audsys_clk_unregister(struct mtk_base_afe *afe);

#endif
