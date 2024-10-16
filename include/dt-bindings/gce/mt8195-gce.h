/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 */

#ifndef _DT_BINDINGS_GCE_MT8195_H
#define _DT_BINDINGS_GCE_MT8195_H

/* assign timeout 0 also means default */
#define CMDQ_NO_TIMEOUT		0xffffffff
#define CMDQ_TIMEOUT_DEFAULT	1000

/* GCE thread priority */
#define CMDQ_THR_PRIO_LOWEST	0
#define CMDQ_THR_PRIO_1		1
#define CMDQ_THR_PRIO_2		2
#define CMDQ_THR_PRIO_3		3
#define CMDQ_THR_PRIO_4		4
#define CMDQ_THR_PRIO_5		5
#define CMDQ_THR_PRIO_6		6
#define CMDQ_THR_PRIO_HIGHEST	7

/* CPR count in 32bit register */
#define GCE_CPR_COUNT		1312

/* GCE subsys table */
#define SUBSYS_1400XXXX		0
#define SUBSYS_1401XXXX		1
#define SUBSYS_1402XXXX		2
#define SUBSYS_1c00XXXX		3
#define SUBSYS_1c01XXXX		4
#define SUBSYS_1c02XXXX		5
#define SUBSYS_1c10XXXX		6
#define SUBSYS_1c11XXXX		7
#define SUBSYS_1c12XXXX		8
#define SUBSYS_14f0XXXX		9
#define SUBSYS_14f1XXXX		10
#define SUBSYS_14f2XXXX		11
#define SUBSYS_1800XXXX		12
#define SUBSYS_1801XXXX		13
#define SUBSYS_1802XXXX		14
#define SUBSYS_1803XXXX		15
#define SUBSYS_1032XXXX		16
#define SUBSYS_1033XXXX		17
#define SUBSYS_1600XXXX		18
#define SUBSYS_1601XXXX		19
#define SUBSYS_14e0XXXX		20
#define SUBSYS_1c20XXXX		21
#define SUBSYS_1c30XXXX		22
#define SUBSYS_1c40XXXX		23
#define SUBSYS_1c50XXXX		24
#define SUBSYS_1c60XXXX		25
#define SUBSYS_NO_SUPPORT	99

/* GCE General Purpose Register (GPR) support
 * Leave note for scenario usage here
 */
/* GCE: write mask */
#define GCE_GPR_R00		0x00
#define GCE_GPR_R01		0x01
/* MDP: P1: JPEG dest */
#define GCE_GPR_R02		0x02
#define GCE_GPR_R03		0x03
/* MDP: PQ color */
#define GCE_GPR_R04		0x04
/* MDP: 2D sharpness */
#define GCE_GPR_R05		0x05
/* DISP: poll esd */
#define GCE_GPR_R06		0x06
#define GCE_GPR_R07		0x07
/* MDP: P4: 2D sharpness dst */
#define GCE_GPR_R08		0x08
#define GCE_GPR_R09		0x09
/* VCU: poll with timeout for GPR timer */
#define GCE_GPR_R10		0x0A
#define GCE_GPR_R11		0x0B
/* CMDQ: debug */
#define GCE_GPR_R12		0x0C
#define GCE_GPR_R13		0x0D
/* CMDQ: P7: debug */
#define GCE_GPR_R14		0x0E
#define GCE_GPR_R15		0x0F

#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_0	1
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_1	2
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_2	3
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_3	4
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_4	5
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_5	6
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_6	7
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_7	8
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_8	9
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_9	10
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_10	11
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_11	12
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_12	13
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_13	14
#define CMDQ_EVENT_CQ_THR_DONE_TRAW0_14	15
#define CMDQ_EVENT_TRAW0_DMA_ERROR_INT	16
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_0	17
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_1	18
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_2	19
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_3	20
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_4	21
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_5	22
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_6	23
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_7	24
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_8	25
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_9	26
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_10	27
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_11	28
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_12	29
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_13	30
#define CMDQ_EVENT_CQ_THR_DONE_TRAW1_14	31
#define CMDQ_EVENT_TRAW1_DMA_ERROR_INT	32

#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_0	65
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_1	66
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_2	67
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_3	68
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_4	69
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_5	70
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_6	71
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_7	72
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_8	73
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_9	74
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_10	75
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_11	76
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_12	77
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_13	78
#define CMDQ_EVENT_DIP0_FRAME_DONE_P2_14	79
#define CMDQ_EVENT_DIP0_DMA_ERR	80
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_0	81
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_1	82
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_2	83
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_3	84
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_4	85
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_5	86
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_6	87
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_7	88
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_8	89
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_9	90
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_10	91
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_11	92
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_12	93
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_13	94
#define CMDQ_EVENT_PQA0_FRAME_DONE_P2_14	95
#define CMDQ_EVENT_PQA0_DMA_ERR	96
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_0	97
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_1	98
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_2	99
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_3	100
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_4	101
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_5	102
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_6	103
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_7	104
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_8	105
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_9	106
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_10	107
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_11	108
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_12	109
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_13	110
#define CMDQ_EVENT_PQB0_FRAME_DONE_P2_14	111
#define CMDQ_EVENT_PQB0_DMA_ERR	112
#define CMDQ_EVENT_DIP0_DUMMY_0	113
#define CMDQ_EVENT_DIP0_DUMMY_1	114
#define CMDQ_EVENT_DIP0_DUMMY_2	115
#define CMDQ_EVENT_DIP0_DUMMY_3	116
#define CMDQ_EVENT_WPE0_EIS_GCE_FRAME_DONE	117
#define CMDQ_EVENT_WPE0_EIS_DONE_SYNC_OUT	118
#define CMDQ_EVENT_WPE0_TNR_GCE_FRAME_DONE	119
#define CMDQ_EVENT_WPE0_TNR_DONE_SYNC_OUT	120
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_0	121
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_1	122
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_2	123
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_3	124
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_4	125
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_5	126
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_6	127
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_7	128
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_8	129
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_9	130
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_10	131
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_11	132
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_12	133
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_13	134
#define CMDQ_EVENT_WPE0_EIS_FRAME_DONE_P2_14	135
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_0	136
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_1	137
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_2	138
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_3	139
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_4	140
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_5	141
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_6	142
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_7	143
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_8	144
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_9	145
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_10	146
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_11	147
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_12	148
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_13	149
#define CMDQ_EVENT_WPE0_TNR_FRAME_DONE_P2_14	150
#define CMDQ_EVENT_WPE0_DUMMY_0	151
#define CMDQ_EVENT_IMGSYS_IPE_DUMMY	152
#define CMDQ_EVENT_IMGSYS_IPE_FDVT_DONE	153
#define CMDQ_EVENT_IMGSYS_IPE_ME_DONE	154
#define CMDQ_EVENT_IMGSYS_IPE_DVS_DONE	155
#define CMDQ_EVENT_IMGSYS_IPE_DVP_DONE	156

#define CMDQ_EVENT_TPR_0	194
#define CMDQ_EVENT_TPR_1	195
#define CMDQ_EVENT_TPR_2	196
#define CMDQ_EVENT_TPR_3	197
#define CMDQ_EVENT_TPR_4	198
#define CMDQ_EVENT_TPR_5	199
#define CMDQ_EVENT_TPR_6	200
#define CMDQ_EVENT_TPR_7	201
#define CMDQ_EVENT_TPR_8	202
#define CMDQ_EVENT_TPR_9	203
#define CMDQ_EVENT_TPR_10	204
#define CMDQ_EVENT_TPR_11	205
#define CMDQ_EVENT_TPR_12	206
#define CMDQ_EVENT_TPR_13	207
#define CMDQ_EVENT_TPR_14	208
#define CMDQ_EVENT_TPR_15	209
#define CMDQ_EVENT_TPR_16	210
#define CMDQ_EVENT_TPR_17	211
#define CMDQ_EVENT_TPR_18	212
#define CMDQ_EVENT_TPR_19	213
#define CMDQ_EVENT_TPR_20	214
#define CMDQ_EVENT_TPR_21	215
#define CMDQ_EVENT_TPR_22	216
#define CMDQ_EVENT_TPR_23	217
#define CMDQ_EVENT_TPR_24	218
#define CMDQ_EVENT_TPR_25	219
#define CMDQ_EVENT_TPR_26	220
#define CMDQ_EVENT_TPR_27	221
#define CMDQ_EVENT_TPR_28	222
#define CMDQ_EVENT_TPR_29	223
#define CMDQ_EVENT_TPR_30	224
#define CMDQ_EVENT_TPR_31	225
#define CMDQ_EVENT_TPR_TIMEOUT_0	226
#define CMDQ_EVENT_TPR_TIMEOUT_1	227
#define CMDQ_EVENT_TPR_TIMEOUT_2	228
#define CMDQ_EVENT_TPR_TIMEOUT_3	229
#define CMDQ_EVENT_TPR_TIMEOUT_4	230
#define CMDQ_EVENT_TPR_TIMEOUT_5	231
#define CMDQ_EVENT_TPR_TIMEOUT_6	232
#define CMDQ_EVENT_TPR_TIMEOUT_7	233
#define CMDQ_EVENT_TPR_TIMEOUT_8	234
#define CMDQ_EVENT_TPR_TIMEOUT_9	235
#define CMDQ_EVENT_TPR_TIMEOUT_10	236
#define CMDQ_EVENT_TPR_TIMEOUT_11	237
#define CMDQ_EVENT_TPR_TIMEOUT_12	238
#define CMDQ_EVENT_TPR_TIMEOUT_13	239
#define CMDQ_EVENT_TPR_TIMEOUT_14	240
#define CMDQ_EVENT_TPR_TIMEOUT_15	241

#define CMDQ_EVENT_VPP0_MDP_RDMA_SOF	256
#define CMDQ_EVENT_VPP0_MDP_FG_SOF	257
#define CMDQ_EVENT_VPP0_STITCH_SOF	258
#define CMDQ_EVENT_VPP0_MDP_HDR_SOF	259
#define CMDQ_EVENT_VPP0_MDP_AAL_SOF	260
#define CMDQ_EVENT_VPP0_MDP_RSZ_IN_RSZ_SOF	261
#define CMDQ_EVENT_VPP0_MDP_TDSHP_SOF	262
#define CMDQ_EVENT_VPP0_DISP_COLOR_SOF	263
#define CMDQ_EVENT_VPP0_DISP_OVL_NOAFBC_SOF	264
#define CMDQ_EVENT_VPP0_VPP_PADDING_IN_PADDING_SOF	265
#define CMDQ_EVENT_VPP0_MDP_TCC_IN_SOF	266
#define CMDQ_EVENT_VPP0_MDP_WROT_SOF	267

#define CMDQ_EVENT_VPP0_WARP0_MMSYS_TOP_RELAY_SOF_PRE	269
#define CMDQ_EVENT_VPP0_WARP1_MMSYS_TOP_RELAY_SOF_PRE	270
#define CMDQ_EVENT_VPP0_VPP1_MMSYS_TOP_RELAY_SOF	271
#define CMDQ_EVENT_VPP0_VPP1_IN_MMSYS_TOP_RELAY_SOF_PRE	272

#define CMDQ_EVENT_VPP0_MDP_RDMA_FRAME_DONE	288
#define CMDQ_EVENT_VPP0_MDP_FG_TILE_DONE	289
#define CMDQ_EVENT_VPP0_STITCH_FRAME_DONE	290
#define CMDQ_EVENT_VPP0_MDP_HDR_FRAME_DONE	291
#define CMDQ_EVENT_VPP0_MDP_AAL_FRAME_DONE	292
#define CMDQ_EVENT_VPP0_MDP_RSZ_FRAME_DONE	293
#define CMDQ_EVENT_VPP0_MDP_TDSHP_FRAME_DONE	294
#define CMDQ_EVENT_VPP0_DISP_COLOR_FRAME_DONE	295
#define CMDQ_EVENT_VPP0_DISP_OVL_NOAFBC_FRAME_DONE	296
#define CMDQ_EVENT_VPP0_VPP_PADDING_IN_PADDING_FRAME_DONE	297
#define CMDQ_EVENT_VPP0_MDP_TCC_TCC_FRAME_DONE	298
#define CMDQ_EVENT_VPP0_MDP_WROT_VIDO_WDONE	299

#define CMDQ_EVENT_VPP0_STREAM_DONE_0	320
#define CMDQ_EVENT_VPP0_STREAM_DONE_1	321
#define CMDQ_EVENT_VPP0_STREAM_DONE_2	322
#define CMDQ_EVENT_VPP0_STREAM_DONE_3	323
#define CMDQ_EVENT_VPP0_STREAM_DONE_4	324
#define CMDQ_EVENT_VPP0_STREAM_DONE_5	325
#define CMDQ_EVENT_VPP0_STREAM_DONE_6	326
#define CMDQ_EVENT_VPP0_STREAM_DONE_7	327
#define CMDQ_EVENT_VPP0_STREAM_DONE_8	328
#define CMDQ_EVENT_VPP0_STREAM_DONE_9	329
#define CMDQ_EVENT_VPP0_STREAM_DONE_10	330
#define CMDQ_EVENT_VPP0_STREAM_DONE_11	331
#define CMDQ_EVENT_VPP0_STREAM_DONE_12	332
#define CMDQ_EVENT_VPP0_STREAM_DONE_13	333
#define CMDQ_EVENT_VPP0_STREAM_DONE_14	334
#define CMDQ_EVENT_VPP0_STREAM_DONE_15	335
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_0	336
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_1	337
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_2	338
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_3	339
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_4	340
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_5	341
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_6	342
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_7	343
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_8	344
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_9	345
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_10	346
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_11	347
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_12	348
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_13	349
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_14	350
#define CMDQ_EVENT_VPP0_BUF_UNDERRUN_15	351
#define CMDQ_EVENT_VPP0_MDP_RDMA_SW_RST_DONE	352
#define CMDQ_EVENT_VPP0_MDP_RDMA_PM_VALID	353
#define CMDQ_EVENT_VPP0_DISP_OVL_NOAFBC_FRAME_RESET_DONE_PULSE	354
#define CMDQ_EVENT_VPP0_MDP_WROT_SW_RST_DONE	355

#define CMDQ_EVENT_VPP1_HDMI_META_SOF		384
#define CMDQ_EVENT_VPP1_DGI_SOF			385
#define CMDQ_EVENT_VPP1_VPP_SPLIT_SOF		386
#define CMDQ_EVENT_VPP1_SVPP1_MDP_TCC_SOF	387
#define CMDQ_EVENT_VPP1_SVPP1_MDP_RDMA_SOF	388
#define CMDQ_EVENT_VPP1_SVPP2_MDP_RDMA_SOF	389
#define CMDQ_EVENT_VPP1_SVPP3_MDP_RDMA_SOF	390
#define CMDQ_EVENT_VPP1_SVPP1_MDP_FG_SOF	391
#define CMDQ_EVENT_VPP1_SVPP2_MDP_FG_SOF	392
#define CMDQ_EVENT_VPP1_SVPP3_MDP_FG_SOF	393
#define CMDQ_EVENT_VPP1_SVPP1_MDP_HDR_SOF	394
#define CMDQ_EVENT_VPP1_SVPP2_MDP_HDR_SOF	395
#define CMDQ_EVENT_VPP1_SVPP3_MDP_HDR_SOF	396
#define CMDQ_EVENT_VPP1_SVPP1_MDP_AAL_SOF	397
#define CMDQ_EVENT_VPP1_SVPP2_MDP_AAL_SOF	398
#define CMDQ_EVENT_VPP1_SVPP3_MDP_AAL_SOF	399
#define CMDQ_EVENT_VPP1_SVPP1_MDP_RSZ_SOF	400
#define CMDQ_EVENT_VPP1_SVPP2_MDP_RSZ_SOF	401
#define CMDQ_EVENT_VPP1_SVPP3_MDP_RSZ_SOF	402
#define CMDQ_EVENT_VPP1_SVPP1_TDSHP_SOF		403
#define CMDQ_EVENT_VPP1_SVPP2_TDSHP_SOF		404
#define CMDQ_EVENT_VPP1_SVPP3_TDSHP_SOF		405
#define CMDQ_EVENT_VPP1_SVPP2_VPP_MERGE_SOF	406
#define CMDQ_EVENT_VPP1_SVPP3_VPP_MERGE_SOF	407
#define CMDQ_EVENT_VPP1_SVPP1_MDP_COLOR_SOF	408
#define CMDQ_EVENT_VPP1_SVPP2_MDP_COLOR_SOF	409
#define CMDQ_EVENT_VPP1_SVPP3_MDP_COLOR_SOF	410
#define CMDQ_EVENT_VPP1_SVPP1_MDP_OVL_SOF	411
#define CMDQ_EVENT_VPP1_SVPP1_VPP_PAD_SOF	412
#define CMDQ_EVENT_VPP1_SVPP2_VPP_PAD_SOF	413
#define CMDQ_EVENT_VPP1_SVPP3_VPP_PAD_SOF	414
#define CMDQ_EVENT_VPP1_SVPP1_MDP_WROT_SOF	415
#define CMDQ_EVENT_VPP1_SVPP2_MDP_WROT_SOF	416
#define CMDQ_EVENT_VPP1_SVPP3_MDP_WROT_SOF	417
#define CMDQ_EVENT_VPP1_VPP0_DL_IRLY_SOF	418
#define CMDQ_EVENT_VPP1_VPP0_DL_ORLY_SOF	419
#define CMDQ_EVENT_VPP1_VDO0_DL_ORLY_0_SOF	420
#define CMDQ_EVENT_VPP1_VDO0_DL_ORLY_1_SOF	421
#define CMDQ_EVENT_VPP1_VDO1_DL_ORLY_0_SOF	422
#define CMDQ_EVENT_VPP1_VDO1_DL_ORLY_1_SOF	423
#define CMDQ_EVENT_VPP1_SVPP1_MDP_RDMA_FRAME_DONE	424
#define CMDQ_EVENT_VPP1_SVPP2_MDP_RDMA_FRAME_DONE	425
#define CMDQ_EVENT_VPP1_SVPP3_MDP_RDMA_FRAME_DONE	426
#define CMDQ_EVENT_VPP1_SVPP1_MDP_WROT_FRAME_DONE	427
#define CMDQ_EVENT_VPP1_SVPP2_MDP_WROT_FRAME_DONE	428
#define CMDQ_EVENT_VPP1_SVPP3_MDP_WROT_FRAME_DONE	429
#define CMDQ_EVENT_VPP1_SVPP1_MDP_OVL_FRAME_DONE	430
#define CMDQ_EVENT_VPP1_SVPP1_MDP_RSZ_FRAME_DONE	431
#define CMDQ_EVENT_VPP1_SVPP2_MDP_RSZ_FRAME_DONE	432
#define CMDQ_EVENT_VPP1_SVPP3_MDP_RSZ_FRAME_DONE	433
#define CMDQ_EVENT_VPP1_FRAME_DONE_10	434
#define CMDQ_EVENT_VPP1_FRAME_DONE_11	435
#define CMDQ_EVENT_VPP1_FRAME_DONE_12	436
#define CMDQ_EVENT_VPP1_FRAME_DONE_13	437
#define CMDQ_EVENT_VPP1_FRAME_DONE_14	438
#define CMDQ_EVENT_VPP1_STREAM_DONE_0	439
#define CMDQ_EVENT_VPP1_STREAM_DONE_1	440
#define CMDQ_EVENT_VPP1_STREAM_DONE_2	441
#define CMDQ_EVENT_VPP1_STREAM_DONE_3	442
#define CMDQ_EVENT_VPP1_STREAM_DONE_4	443
#define CMDQ_EVENT_VPP1_STREAM_DONE_5	444
#define CMDQ_EVENT_VPP1_STREAM_DONE_6	445
#define CMDQ_EVENT_VPP1_STREAM_DONE_7	446
#define CMDQ_EVENT_VPP1_STREAM_DONE_8	447
#define CMDQ_EVENT_VPP1_STREAM_DONE_9	448
#define CMDQ_EVENT_VPP1_STREAM_DONE_10	449
#define CMDQ_EVENT_VPP1_STREAM_DONE_11	450
#define CMDQ_EVENT_VPP1_STREAM_DONE_12	451
#define CMDQ_EVENT_VPP1_STREAM_DONE_13	452
#define CMDQ_EVENT_VPP1_STREAM_DONE_14	453
#define CMDQ_EVENT_VPP1_STREAM_DONE_15	454
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_0	455
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_1	456
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_2	457
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_3	458
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_4	459
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_5	460
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_6	461
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_7	462
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_8	463
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_9	464
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_10	465
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_11	466
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_12	467
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_13	468
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_14	469
#define CMDQ_EVENT_VPP1_MDP_BUF_UNDERRUN_15	470
#define CMDQ_EVENT_VPP1_DGI_0	471
#define CMDQ_EVENT_VPP1_DGI_1	472
#define CMDQ_EVENT_VPP1_DGI_2	473
#define CMDQ_EVENT_VPP1_DGI_3	474
#define CMDQ_EVENT_VPP1_DGI_4	475
#define CMDQ_EVENT_VPP1_DGI_5	476
#define CMDQ_EVENT_VPP1_DGI_6	477
#define CMDQ_EVENT_VPP1_DGI_7	478
#define CMDQ_EVENT_VPP1_DGI_8	479
#define CMDQ_EVENT_VPP1_DGI_9	480
#define CMDQ_EVENT_VPP1_DGI_10	481
#define CMDQ_EVENT_VPP1_DGI_11	482
#define CMDQ_EVENT_VPP1_DGI_12	483
#define CMDQ_EVENT_VPP1_DGI_13	484
#define CMDQ_EVENT_VPP1_SVPP3_VPP_MERGE	485
#define CMDQ_EVENT_VPP1_SVPP2_VPP_MERGE	486
#define CMDQ_EVENT_VPP1_MDP_OVL_FRAME_RESET_DONE_PULSE	487
#define CMDQ_EVENT_VPP1_VPP_SPLIT_DGI	488
#define CMDQ_EVENT_VPP1_VPP_SPLIT_HDMI	489
#define CMDQ_EVENT_VPP1_SVPP3_MDP_WROT_SW_RST_DONE	490
#define CMDQ_EVENT_VPP1_SVPP2_MDP_WROT_SW_RST_DONE	491
#define CMDQ_EVENT_VPP1_SVPP1_MDP_WROT_SW_RST_DONE	492
#define CMDQ_EVENT_VPP1_SVPP3_MDP_FG_TILE_DONE	493
#define CMDQ_EVENT_VPP1_SVPP2_MDP_FG_TILE_DONE	494
#define CMDQ_EVENT_VPP1_SVPP1_MDP_FG_TILE_DONE	495

#define CMDQ_EVENT_VDO0_DISP_OVL0_SOF	512
#define CMDQ_EVENT_VDO0_DISP_WDMA0_SOF	513
#define CMDQ_EVENT_VDO0_DISP_RDMA0_SOF	514
#define CMDQ_EVENT_VDO0_DISP_COLOR0_SOF	515
#define CMDQ_EVENT_VDO0_DISP_CCORR0_SOF	516
#define CMDQ_EVENT_VDO0_DISP_AAL0_SOF	517
#define CMDQ_EVENT_VDO0_DISP_GAMMA0_SOF	518
#define CMDQ_EVENT_VDO0_DISP_DITHER0_SOF	519
#define CMDQ_EVENT_VDO0_DSI0_SOF	520
#define CMDQ_EVENT_VDO0_DSC_WRAP0C0_SOF	521
#define CMDQ_EVENT_VDO0_DISP_OVL1_SOF	522
#define CMDQ_EVENT_VDO0_DISP_WDMA1_SOF	523
#define CMDQ_EVENT_VDO0_DISP_RDMA1_SOF	524
#define CMDQ_EVENT_VDO0_DISP_COLOR1_SOF	525
#define CMDQ_EVENT_VDO0_DISP_CCORR1_SOF	526
#define CMDQ_EVENT_VDO0_DISP_AAL1_SOF	527
#define CMDQ_EVENT_VDO0_DISP_GAMMA1_SOF	528
#define CMDQ_EVENT_VDO0_DISP_DITHER1_SOF	529
#define CMDQ_EVENT_VDO0_DSI1_SOF	530
#define CMDQ_EVENT_VDO0_DSC_WRAP0C1_SOF	531
#define CMDQ_EVENT_VDO0_VPP_MERGE0_SOF	532
#define CMDQ_EVENT_VDO0_DP_INTF0_SOF	533
#define CMDQ_EVENT_VDO0_VPP1_DL_RELAY0_SOF	534
#define CMDQ_EVENT_VDO0_VPP1_DL_RELAY1_SOF	535
#define CMDQ_EVENT_VDO0_VDO1_DL_RELAY2_SOF	536
#define CMDQ_EVENT_VDO0_VDO0_DL_RELAY3_SOF	537
#define CMDQ_EVENT_VDO0_VDO0_DL_RELAY4_SOF	538
#define CMDQ_EVENT_VDO0_DISP_PWM0_SOF	539
#define CMDQ_EVENT_VDO0_DISP_PWM1_SOF	540

#define CMDQ_EVENT_VDO0_DISP_OVL0_FRAME_DONE	544
#define CMDQ_EVENT_VDO0_DISP_WDMA0_FRAME_DONE	545
#define CMDQ_EVENT_VDO0_DISP_RDMA0_FRAME_DONE	546
#define CMDQ_EVENT_VDO0_DISP_COLOR0_FRAME_DONE	547
#define CMDQ_EVENT_VDO0_DISP_CCORR0_FRAME_DONE	548
#define CMDQ_EVENT_VDO0_DISP_AAL0_FRAME_DONE	549
#define CMDQ_EVENT_VDO0_DISP_GAMMA0_FRAME_DONE	550
#define CMDQ_EVENT_VDO0_DISP_DITHER0_FRAME_DONE	551
#define CMDQ_EVENT_VDO0_DSI0_FRAME_DONE	552
#define CMDQ_EVENT_VDO0_DSC_WRAP0C0_FRAME_DONE	553
#define CMDQ_EVENT_VDO0_DISP_OVL1_FRAME_DONE	554
#define CMDQ_EVENT_VDO0_DISP_WDMA1_FRAME_DONE	555
#define CMDQ_EVENT_VDO0_DISP_RDMA1_FRAME_DONE	556
#define CMDQ_EVENT_VDO0_DISP_COLOR1_FRAME_DONE	557
#define CMDQ_EVENT_VDO0_DISP_CCORR1_FRAME_DONE	558
#define CMDQ_EVENT_VDO0_DISP_AAL1_FRAME_DONE	559
#define CMDQ_EVENT_VDO0_DISP_GAMMA1_FRAME_DONE	560
#define CMDQ_EVENT_VDO0_DISP_DITHER1_FRAME_DONE	561
#define CMDQ_EVENT_VDO0_DSI1_FRAME_DONE	562
#define CMDQ_EVENT_VDO0_DSC_WRAP0C1_FRAME_DONE	563

#define CMDQ_EVENT_VDO0_DP_INTF0_FRAME_DONE	565

#define CMDQ_EVENT_VDO0_DISP_SMIASSERT_ENG	576
#define CMDQ_EVENT_VDO0_DSI0_IRQ_ENG_EVENT_MM	577
#define CMDQ_EVENT_VDO0_DSI0_TE_ENG_EVENT_MM	578
#define CMDQ_EVENT_VDO0_DSI0_DONE_ENG_EVENT_MM	579
#define CMDQ_EVENT_VDO0_DSI0_SOF_ENG_EVENT_MM	580
#define CMDQ_EVENT_VDO0_DSI0_VACTL_ENG_EVENT_MM	581
#define CMDQ_EVENT_VDO0_DSI1_IRQ_ENG_EVENT_MM	582
#define CMDQ_EVENT_VDO0_DSI1_TE_ENG_EVENT_MM	583
#define CMDQ_EVENT_VDO0_DSI1_DONE_ENG_EVENT_MM	584
#define CMDQ_EVENT_VDO0_DSI1_SOF_ENG_EVENT_MM	585
#define CMDQ_EVENT_VDO0_DSI1_VACTL_ENG_EVENT_MM	586
#define CMDQ_EVENT_VDO0_DISP_WDMA0_SW_RST_DONE_ENG	587
#define CMDQ_EVENT_VDO0_DISP_WDMA1_SW_RST_DONE_ENG	588
#define CMDQ_EVENT_VDO0_DISP_OVL0_RST_DONE_ENG	589
#define CMDQ_EVENT_VDO0_DISP_OVL1_RST_DONE_ENG	590
#define CMDQ_EVENT_VDO0_DP_INTF0_VSYNC_START_ENG_EVENT_MM	591
#define CMDQ_EVENT_VDO0_DP_INTF0_VSYNC_END_ENG_EVENT_MM	592
#define CMDQ_EVENT_VDO0_DP_INTF0_VDE_START_ENG_EVENT_MM	593
#define CMDQ_EVENT_VDO0_DP_INTF0_VDE_END_ENG_EVENT_MM	594
#define CMDQ_EVENT_VDO0_DP_INTF0_TARGET_LINE_ENG_EVENT_MM	595
#define CMDQ_EVENT_VDO0_VPP_MERGE0_ENG	596
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_0	597
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_1	598
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_2	599
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_3	600
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_4	601
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_5	602
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_6	603
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_7	604
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_8	605
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_9	606
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_10	607
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_11	608
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_12	609
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_13	610
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_14	611
#define CMDQ_EVENT_VDO0_DISP_STREAM_DONE_15	612
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_0	613
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_1	614
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_2	615
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_3	616
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_4	617
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_5	618
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_6	619
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_7	620
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_8	621
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_9	622
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_10	623
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_11	624
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_12	625
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_13	626
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_14	627
#define CMDQ_EVENT_VDO0_DISP_BUF_UNDERRUN_15	628

#define CMDQ_EVENT_VDO1_MDP_RDMA0_SOF	640
#define CMDQ_EVENT_VDO1_MDP_RDMA1_SOF	641
#define CMDQ_EVENT_VDO1_MDP_RDMA2_SOF	642
#define CMDQ_EVENT_VDO1_MDP_RDMA3_SOF	643
#define CMDQ_EVENT_VDO1_MDP_RDMA4_SOF	644
#define CMDQ_EVENT_VDO1_MDP_RDMA5_SOF	645
#define CMDQ_EVENT_VDO1_MDP_RDMA6_SOF	646
#define CMDQ_EVENT_VDO1_MDP_RDMA7_SOF	647
#define CMDQ_EVENT_VDO1_VPP_MERGE0_SOF	648
#define CMDQ_EVENT_VDO1_VPP_MERGE1_SOF	649
#define CMDQ_EVENT_VDO1_VPP_MERGE2_SOF	650
#define CMDQ_EVENT_VDO1_VPP_MERGE3_SOF	651
#define CMDQ_EVENT_VDO1_VPP_MERGE4_SOF	652
#define CMDQ_EVENT_VDO1_VPP2_DL_RELAY_SOF	653
#define CMDQ_EVENT_VDO1_VPP3_DL_RELAY_SOF	654
#define CMDQ_EVENT_VDO1_VDO0_DSC_DL_ASYNC_SOF	655
#define CMDQ_EVENT_VDO1_VDO0_MERGE_DL_ASYNC_SOF	656
#define CMDQ_EVENT_VDO1_OUT_DL_RELAY_SOF	657
#define CMDQ_EVENT_VDO1_DISP_MIXER_SOF	658
#define CMDQ_EVENT_VDO1_HDR_VDO_FE0_SOF	659
#define CMDQ_EVENT_VDO1_HDR_VDO_FE1_SOF	660
#define CMDQ_EVENT_VDO1_HDR_GFX_FE0_SOF	661
#define CMDQ_EVENT_VDO1_HDR_GFX_FE1_SOF	662
#define CMDQ_EVENT_VDO1_HDR_VDO_BE0_SOF	663
#define CMDQ_EVENT_VDO1_HDR_MLOAD_SOF	664

#define CMDQ_EVENT_VDO1_MDP_RDMA0_FRAME_DONE	672
#define CMDQ_EVENT_VDO1_MDP_RDMA1_FRAME_DONE	673
#define CMDQ_EVENT_VDO1_MDP_RDMA2_FRAME_DONE	674
#define CMDQ_EVENT_VDO1_MDP_RDMA3_FRAME_DONE	675
#define CMDQ_EVENT_VDO1_MDP_RDMA4_FRAME_DONE	676
#define CMDQ_EVENT_VDO1_MDP_RDMA5_FRAME_DONE	677
#define CMDQ_EVENT_VDO1_MDP_RDMA6_FRAME_DONE	678
#define CMDQ_EVENT_VDO1_MDP_RDMA7_FRAME_DONE	679
#define CMDQ_EVENT_VDO1_VPP_MERGE0_FRAME_DONE	680
#define CMDQ_EVENT_VDO1_VPP_MERGE1_FRAME_DONE	681
#define CMDQ_EVENT_VDO1_VPP_MERGE2_FRAME_DONE	682
#define CMDQ_EVENT_VDO1_VPP_MERGE3_FRAME_DONE	683
#define CMDQ_EVENT_VDO1_VPP_MERGE4_FRAME_DONE	684
#define CMDQ_EVENT_VDO1_DPI0_FRAME_DONE	685
#define CMDQ_EVENT_VDO1_DPI1_FRAME_DONE	686
#define CMDQ_EVENT_VDO1_DP_INTF0_FRAME_DONE	687
#define CMDQ_EVENT_VDO1_DISP_MIXER_FRAME_DONE_MM	688

#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_0	704
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_1	705
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_2	706
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_3	707
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_4	708
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_5	709
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_6	710
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_7	711
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_8	712
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_9	713
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_10	714
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_11	715
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_12	716
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_13	717
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_14	718
#define CMDQ_EVENT_VDO1_STREAM_DONE_ENG_15	719
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_0	720
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_1	721
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_2	722
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_3	723
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_4	724
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_5	725
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_6	726
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_7	727
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_8	728
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_9	729
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_10	730
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_11	731
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_12	732
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_13	733
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_14	734
#define CMDQ_EVENT_VDO1_BUF_UNDERRUN_ENG_15	735
#define CMDQ_EVENT_VDO1_MDP_RDMA0_SW_RST_DONE	736
#define CMDQ_EVENT_VDO1_MDP_RDMA1_SW_RST_DONE	737
#define CMDQ_EVENT_VDO1_MDP_RDMA2_SW_RST_DONE	738
#define CMDQ_EVENT_VDO1_MDP_RDMA3_SW_RST_DONE	739
#define CMDQ_EVENT_VDO1_MDP_RDMA4_SW_RST_DONE	740
#define CMDQ_EVENT_VDO1_MDP_RDMA5_SW_RST_DONE	741
#define CMDQ_EVENT_VDO1_MDP_RDMA6_SW_RST_DONE	742
#define CMDQ_EVENT_VDO1_MDP_RDMA7_SW_RST_DONE	743

#define CMDQ_EVENT_VDO1_DP0_VDE_END_ENG_EVENT_MM	745
#define CMDQ_EVENT_VDO1_DP0_VDE_START_ENG_EVENT_MM	746
#define CMDQ_EVENT_VDO1_DP0_VSYNC_END_ENG_EVENT_MM	747
#define CMDQ_EVENT_VDO1_DP0_VSYNC_START_ENG_EVENT_MM	748
#define CMDQ_EVENT_VDO1_DP0_TARGET_LINE_ENG_EVENT_MM	749
#define CMDQ_EVENT_VDO1_VPP_MERGE0	750
#define CMDQ_EVENT_VDO1_VPP_MERGE1	751
#define CMDQ_EVENT_VDO1_VPP_MERGE2	752
#define CMDQ_EVENT_VDO1_VPP_MERGE3	753
#define CMDQ_EVENT_VDO1_VPP_MERGE4	754
#define CMDQ_EVENT_VDO1_HDMITX	755
#define CMDQ_EVENT_VDO1_HDR_VDO_BE0_ADL_TRIG_EVENT_MM	756
#define CMDQ_EVENT_VDO1_HDR_GFX_FE1_THDR_ADL_TRIG_EVENT_MM	757
#define CMDQ_EVENT_VDO1_HDR_GFX_FE1_DM_ADL_TRIG_EVENT_MM	758
#define CMDQ_EVENT_VDO1_HDR_GFX_FE0_THDR_ADL_TRIG_EVENT_MM	759
#define CMDQ_EVENT_VDO1_HDR_GFX_FE0_DM_ADL_TRIG_EVENT_MM	760
#define CMDQ_EVENT_VDO1_HDR_VDO_FE1_ADL_TRIG_EVENT_MM	761
#define CMDQ_EVENT_VDO1_HDR_VDO_FE1_AD0_TRIG_EVENT_MM	762

#define CMDQ_EVENT_CAM_A_PASS1_DONE	769
#define CMDQ_EVENT_CAM_B_PASS1_DONE	770
#define CMDQ_EVENT_GCAMSV_A_PASS1_DONE	771
#define CMDQ_EVENT_GCAMSV_B_PASS1_DONE	772
#define CMDQ_EVENT_MRAW_0_PASS1_DONE	773
#define CMDQ_EVENT_MRAW_1_PASS1_DONE	774
#define CMDQ_EVENT_MRAW_2_PASS1_DONE	775
#define CMDQ_EVENT_MRAW_3_PASS1_DONE	776
#define CMDQ_EVENT_SENINF_CAM0_FIFO_FULL_X	777
#define CMDQ_EVENT_SENINF_CAM1_FIFO_FULL_X	778
#define CMDQ_EVENT_SENINF_CAM2_FIFO_FULL	779
#define CMDQ_EVENT_SENINF_CAM3_FIFO_FULL	780
#define CMDQ_EVENT_SENINF_CAM4_FIFO_FULL	781
#define CMDQ_EVENT_SENINF_CAM5_FIFO_FULL	782
#define CMDQ_EVENT_SENINF_CAM6_FIFO_FULL	783
#define CMDQ_EVENT_SENINF_CAM7_FIFO_FULL	784
#define CMDQ_EVENT_SENINF_CAM8_FIFO_FULL	785
#define CMDQ_EVENT_SENINF_CAM9_FIFO_FULL	786
#define CMDQ_EVENT_SENINF_CAM10_FIFO_FULL_X	787
#define CMDQ_EVENT_SENINF_CAM11_FIFO_FULL_X	788
#define CMDQ_EVENT_SENINF_CAM12_FIFO_FULL_X	789
#define CMDQ_EVENT_SENINF_CAM13_FIFO_FULL_X	790
#define CMDQ_EVENT_TG_OVRUN_MRAW0_INT_X0	791
#define CMDQ_EVENT_TG_OVRUN_MRAW1_INT_X0	792
#define CMDQ_EVENT_TG_OVRUN_MRAW2_INT	793
#define CMDQ_EVENT_TG_OVRUN_MRAW3_INT	794
#define CMDQ_EVENT_DMA_R1_ERROR_MRAW0_INT	795
#define CMDQ_EVENT_DMA_R1_ERROR_MRAW1_INT	796
#define CMDQ_EVENT_DMA_R1_ERROR_MRAW2_INT	797
#define CMDQ_EVENT_DMA_R1_ERROR_MRAW3_INT	798
#define CMDQ_EVENT_U_CAMSYS_PDA_IRQO_EVENT_DONE_D1	799
#define CMDQ_EVENT_SUBB_TG_INT4	800
#define CMDQ_EVENT_SUBB_TG_INT3	801
#define CMDQ_EVENT_SUBB_TG_INT2	802
#define CMDQ_EVENT_SUBB_TG_INT1	803
#define CMDQ_EVENT_SUBA_TG_INT4	804
#define CMDQ_EVENT_SUBA_TG_INT3	805
#define CMDQ_EVENT_SUBA_TG_INT2	806
#define CMDQ_EVENT_SUBA_TG_INT1	807
#define CMDQ_EVENT_SUBB_DRZS4NO_R1_LOW_LATENCY_LINE_CNT_INT	808
#define CMDQ_EVENT_SUBB_YUVO_R3_LOW_LATENCY_LINE_CNT_INT	809
#define CMDQ_EVENT_SUBB_YUVO_R1_LOW_LATENCY_LINE_CNT_INT	810
#define CMDQ_EVENT_SUBB_IMGO_R1_LOW_LATENCY_LINE_CNT_INT	811
#define CMDQ_EVENT_SUBA_DRZS4NO_R1_LOW_LATENCY_LINE_CNT_INT	812
#define CMDQ_EVENT_SUBA_YUVO_R3_LOW_LATENCY_LINE_CNT_INT	813
#define CMDQ_EVENT_SUBA_YUVO_R1_LOW_LATENCY_LINE_CNT_INT	814
#define CMDQ_EVENT_SUBA_IMGO_R1_LOW_LATENCY_LINE_CNT_INT	815
#define CMDQ_EVENT_GCE1_SOF_0	816
#define CMDQ_EVENT_GCE1_SOF_1	817
#define CMDQ_EVENT_GCE1_SOF_2	818
#define CMDQ_EVENT_GCE1_SOF_3	819
#define CMDQ_EVENT_GCE1_SOF_4	820
#define CMDQ_EVENT_GCE1_SOF_5	821
#define CMDQ_EVENT_GCE1_SOF_6	822
#define CMDQ_EVENT_GCE1_SOF_7	823
#define CMDQ_EVENT_GCE1_SOF_8	824
#define CMDQ_EVENT_GCE1_SOF_9	825
#define CMDQ_EVENT_GCE1_SOF_10	826
#define CMDQ_EVENT_GCE1_SOF_11	827
#define CMDQ_EVENT_GCE1_SOF_12	828
#define CMDQ_EVENT_GCE1_SOF_13	829
#define CMDQ_EVENT_GCE1_SOF_14	830
#define CMDQ_EVENT_GCE1_SOF_15	831

#define CMDQ_EVENT_VDEC_LAT_LINE_COUNT_THRESHOLD_INTERRUPT	832
#define CMDQ_EVENT_VDEC_LAT_VDEC_INT	833
#define CMDQ_EVENT_VDEC_LAT_VDEC_PAUSE	834
#define CMDQ_EVENT_VDEC_LAT_VDEC_DEC_ERROR	835
#define CMDQ_EVENT_VDEC_LAT_MC_BUSY_OVERFLOW_MDEC_TIMEOUT	836
#define CMDQ_EVENT_VDEC_LAT_VDEC_FRAME_DONE	837
#define CMDQ_EVENT_VDEC_LAT_INI_FETCH_RDY	838
#define CMDQ_EVENT_VDEC_LAT_PROCESS_FLAG	839
#define CMDQ_EVENT_VDEC_LAT_SEARCH_START_CODE_DONE	840
#define CMDQ_EVENT_VDEC_LAT_REF_REORDER_DONE	841
#define CMDQ_EVENT_VDEC_LAT_WP_TBLE_DONE	842
#define CMDQ_EVENT_VDEC_LAT_COUNT_SRAM_CLR_DONE_AND_CTX_SRAM_CLR_DONE	843
#define CMDQ_EVENT_VDEC_LAT_GCE_CNT_OP_THRESHOLD	847

#define CMDQ_EVENT_VDEC_LAT1_LINE_COUNT_THRESHOLD_INTERRUPT	848
#define CMDQ_EVENT_VDEC_LAT1_VDEC_INT	849
#define CMDQ_EVENT_VDEC_LAT1_VDEC_PAUSE	850
#define CMDQ_EVENT_VDEC_LAT1_VDEC_DEC_ERROR	851
#define CMDQ_EVENT_VDEC_LAT1_MC_BUSY_OVERFLOW_MDEC_TIMEOUT	852
#define CMDQ_EVENT_VDEC_LAT1_VDEC_FRAME_DONE	853
#define CMDQ_EVENT_VDEC_LAT1_INI_FETCH_RDY	854
#define CMDQ_EVENT_VDEC_LAT1_PROCESS_FLAG	855
#define CMDQ_EVENT_VDEC_LAT1_SEARCH_START_CODE_DONE	856
#define CMDQ_EVENT_VDEC_LAT1_REF_REORDER_DONE	857
#define CMDQ_EVENT_VDEC_LAT1_WP_TBLE_DONE	858
#define CMDQ_EVENT_VDEC_LAT1_COUNT_SRAM_CLR_DONE_AND_CTX_SRAM_CLR_DONE	859
#define CMDQ_EVENT_VDEC_LAT1_GCE_CNT_OP_THRESHOLD	863

#define CMDQ_EVENT_VDEC_SOC_GLOBAL_CON_250_0	864
#define CMDQ_EVENT_VDEC_SOC_GLOBAL_CON_250_1	865

#define CMDQ_EVENT_VDEC_SOC_GLOBAL_CON_250_8	872
#define CMDQ_EVENT_VDEC_SOC_GLOBAL_CON_250_9	873

#define CMDQ_EVENT_VDEC_CORE_LINE_COUNT_THRESHOLD_INTERRUPT	896
#define CMDQ_EVENT_VDEC_CORE_VDEC_INT	897
#define CMDQ_EVENT_VDEC_CORE_VDEC_PAUSE	898
#define CMDQ_EVENT_VDEC_CORE_VDEC_DEC_ERROR	899
#define CMDQ_EVENT_VDEC_CORE_MC_BUSY_OVERFLOW_MDEC_TIMEOUT	900
#define CMDQ_EVENT_VDEC_CORE_VDEC_FRAME_DONE	901
#define CMDQ_EVENT_VDEC_CORE_INI_FETCH_RDY	902
#define CMDQ_EVENT_VDEC_CORE_PROCESS_FLAG	903
#define CMDQ_EVENT_VDEC_CORE_SEARCH_START_CODE_DONE	904
#define CMDQ_EVENT_VDEC_CORE_REF_REORDER_DONE	905
#define CMDQ_EVENT_VDEC_CORE_WP_TBLE_DONE	906
#define CMDQ_EVENT_VDEC_CORE_COUNT_SRAM_CLR_DONE_AND_CTX_SRAM_CLR_DONE	907
#define CMDQ_EVENT_VDEC_CORE_GCE_CNT_OP_THRESHOLD	911

#define CMDQ_EVENT_VDEC_CORE1_LINE_COUNT_THRESHOLD_INTERRUPT	912
#define CMDQ_EVENT_VDEC_CORE1_VDEC_INT	913
#define CMDQ_EVENT_VDEC_CORE1_VDEC_PAUSE	914
#define CMDQ_EVENT_VDEC_CORE1_VDEC_DEC_ERROR	915
#define CMDQ_EVENT_VDEC_CORE1_MC_BUSY_OVERFLOW_MDEC_TIMEOUT	916
#define CMDQ_EVENT_VDEC_CORE1_VDEC_FRAME_DONE	917
#define CMDQ_EVENT_VDEC_CORE1_INI_FETCH_RDY	918
#define CMDQ_EVENT_VDEC_CORE1_PROCESS_FLAG	919
#define CMDQ_EVENT_VDEC_CORE1_SEARCH_START_CODE_DONE	920
#define CMDQ_EVENT_VDEC_CORE1_REF_REORDER_DONE	921
#define CMDQ_EVENT_VDEC_CORE1_WP_TBLE_DONE	922
#define CMDQ_EVENT_VDEC_CORE1_COUNT_SRAM_CLR_DONE_AND_CTX_SRAM_CLR_DONE	923
#define CMDQ_EVENT_VDEC_CORE1_CNT_OP_THRESHOLD	927

#define CMDQ_EVENT_VENC_TOP_FRAME_DONE	929
#define CMDQ_EVENT_VENC_TOP_PAUSE_DONE	930
#define CMDQ_EVENT_VENC_TOP_JPGENC_DONE	931
#define CMDQ_EVENT_VENC_TOP_MB_DONE	932
#define CMDQ_EVENT_VENC_TOP_128BYTE_DONE	933
#define CMDQ_EVENT_VENC_TOP_JPGDEC_DONE	934
#define CMDQ_EVENT_VENC_TOP_JPGDEC_C1_DONE	935
#define CMDQ_EVENT_VENC_TOP_JPGDEC_INSUFF_DONE	936
#define CMDQ_EVENT_VENC_TOP_JPGDEC_C1_INSUFF_DONE	937
#define CMDQ_EVENT_VENC_TOP_WP_2ND_STAGE_DONE	938
#define CMDQ_EVENT_VENC_TOP_WP_3RD_STAGE_DONE	939
#define CMDQ_EVENT_VENC_TOP_PPS_HEADER_DONE	940
#define CMDQ_EVENT_VENC_TOP_SPS_HEADER_DONE	941
#define CMDQ_EVENT_VENC_TOP_VPS_HEADER_DONE	942

#define CMDQ_EVENT_VENC_CORE1_TOP_FRAME_DONE	945
#define CMDQ_EVENT_VENC_CORE1_TOP_PAUSE_DONE	946
#define CMDQ_EVENT_VENC_CORE1_TOP_JPGENC_DONE	947
#define CMDQ_EVENT_VENC_CORE1_TOP_MB_DONE	948
#define CMDQ_EVENT_VENC_CORE1_TOP_128BYTE_DONE	949
#define CMDQ_EVENT_VENC_CORE1_TOP_JPGDEC_DONE	950
#define CMDQ_EVENT_VENC_CORE1_TOP_JPGDEC_C1_DONE	951
#define CMDQ_EVENT_VENC_CORE1_TOP_JPGDEC_INSUFF_DONE	952
#define CMDQ_EVENT_VENC_CORE1_TOP_JPGDEC_C1_INSUFF_DONE	953
#define CMDQ_EVENT_VENC_CORE1_TOP_WP_2ND_STAGE_DONE	954
#define CMDQ_EVENT_VENC_CORE1_TOP_WP_3RD_STAGE_DONE	955
#define CMDQ_EVENT_VENC_CORE1_TOP_PPS_HEADER_DONE	956
#define CMDQ_EVENT_VENC_CORE1_TOP_SPS_HEADER_DONE	957
#define CMDQ_EVENT_VENC_CORE1_TOP_VPS_HEADER_DONE	958

#define CMDQ_EVENT_WPE_VPP0_WPE_GCE_FRAME_DONE	962
#define CMDQ_EVENT_WPE_VPP0_WPE_DONE_SYNC_OUT	963

#define CMDQ_EVENT_WPE_VPP1_WPE_GCE_FRAME_DONE	969
#define CMDQ_EVENT_WPE_VPP1_WPE_DONE_SYNC_OUT	970

#define CMDQ_EVENT_DP_TX_VBLANK_FALLING	994 /* exception */
#define CMDQ_EVENT_DP_TX_VSC_FINISH	995 /* exception */

#define CMDQ_EVENT_OUTPIN_0	1018 /* exception */
#define CMDQ_EVENT_OUTPIN_1	1019 /* exception */

/* end of hw event */
#define CMDQ_MAX_HW_EVENT				1019

/* sw token should use the unused event id from 0 to 1023 */
#define CMDQ_SYNC_TOKEN_INVALID				(-1)

/* Event for imgsys flow control */
#define CMDQ_SYNC_TOKEN_IMGSYS_WPE_EIS			33
#define CMDQ_SYNC_TOKEN_IMGSYS_WPE_TNR			34
#define CMDQ_SYNC_TOKEN_IMGSYS_TRAW			35
#define CMDQ_SYNC_TOKEN_IMGSYS_LTRAW			36
#define CMDQ_SYNC_TOKEN_IMGSYS_DIP			37
#define CMDQ_SYNC_TOKEN_IMGSYS_PQDIP_A			38
#define CMDQ_SYNC_TOKEN_IMGSYS_PQDIP_B			39
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_1			41
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_2			42
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_3			43
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_4			44
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_5			45
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_6			46
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_7			47
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_8			48
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_9			49
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_10			50
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_11			51
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_12			52
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_13			53
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_14			54
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_15			55
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_16			56
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_17			57
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_18			58
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_19			59
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_20			60
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_21			157
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_22			158
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_23			159
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_24			160
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_25			161
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_26			162
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_27			163
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_28			164
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_29			165
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_30			166
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_31			167
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_32			168
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_33			169
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_34			170
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_35			171
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_36			172
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_37			173
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_38			174
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_39			175
#define CMDQ_SYNC_TOKEN_IMGSYS_POOL_40			176
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_1			177
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_2			178
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_3			179
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_4			180
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_5			181
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_6			182
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_7			183
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_8			184
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_9			185
#define CMDQ_SYNC_TOKEN_CAMSYS_POOL_10			186

/* Config thread notify trigger thread */
#define CMDQ_SYNC_TOKEN_CONFIG_DIRTY			971
/* Trigger thread notify config thread */
#define CMDQ_SYNC_TOKEN_STREAM_EOF			972
/* Block Trigger thread until the ESD check finishes. */
#define CMDQ_SYNC_TOKEN_ESD_EOF				973
#define CMDQ_SYNC_TOKEN_STREAM_BLOCK			974
/* check CABC setup finish */
#define CMDQ_SYNC_TOKEN_CABC_EOF			975
/* Pass-2 notifies VENC frame is ready to be encoded */
#define CMDQ_SYNC_TOKEN_VENC_INPUT_READY		976
/* VENC notifies Pass-2 encode done so next frame may start */
#define CMDQ_SYNC_TOKEN_VENC_EOF			977

/* Notify normal CMDQ there are some secure task done
 * MUST NOT CHANGE, this token sync with secure world
 */
#define CMDQ_SYNC_SECURE_THR_EOF			980

/* CMDQ use sw token */
#define CMDQ_SYNC_TOKEN_USER_0				981
#define CMDQ_SYNC_TOKEN_USER_1				982
#define CMDQ_SYNC_TOKEN_POLL_MONITOR			983
#define CMDQ_SYNC_TOKEN_TPR_LOCK			984

/* ISP sw token */
#define CMDQ_SYNC_TOKEN_MSS				985
#define CMDQ_SYNC_TOKEN_MSF				986

/* GPR access tokens (for register backup)
 * There are 15 32-bit GPR, 3 GPR form a set
 * (64-bit for address, 32-bit for value)
 * MUST NOT CHANGE, these tokens sync with MDP
 */
#define CMDQ_SYNC_TOKEN_GPR_SET_0			987
#define CMDQ_SYNC_TOKEN_GPR_SET_1			988
#define CMDQ_SYNC_TOKEN_GPR_SET_2			989
#define CMDQ_SYNC_TOKEN_GPR_SET_3			990
#define CMDQ_SYNC_TOKEN_GPR_SET_4			991

/* Resource lock event to control resource in GCE thread */
#define CMDQ_SYNC_RESOURCE_WROT0			992
#define CMDQ_SYNC_RESOURCE_WROT1			993

/* Event for gpr timer, used in sleep and poll with timeout */
#define CMDQ_TOKEN_GPR_TIMER_R0				996
#define CMDQ_TOKEN_GPR_TIMER_R1				997
#define CMDQ_TOKEN_GPR_TIMER_R2				998
#define CMDQ_TOKEN_GPR_TIMER_R3				999
#define CMDQ_TOKEN_GPR_TIMER_R4				1000
#define CMDQ_TOKEN_GPR_TIMER_R5				1001
#define CMDQ_TOKEN_GPR_TIMER_R6				1002
#define CMDQ_TOKEN_GPR_TIMER_R7				1003
#define CMDQ_TOKEN_GPR_TIMER_R8				1004
#define CMDQ_TOKEN_GPR_TIMER_R9				1005
#define CMDQ_TOKEN_GPR_TIMER_R10			1006
#define CMDQ_TOKEN_GPR_TIMER_R11			1007
#define CMDQ_TOKEN_GPR_TIMER_R12			1008
#define CMDQ_TOKEN_GPR_TIMER_R13			1009
#define CMDQ_TOKEN_GPR_TIMER_R14			1010
#define CMDQ_TOKEN_GPR_TIMER_R15			1011

/* defined in mtk-cmdq-mailbox.h */
/* #define CMDQ_EVENT_MAX					0x3ff */

#endif
