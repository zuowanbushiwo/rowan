/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <mmio.h>
#include <mt8167_def.h>
#include <platform.h>
#include <spm.h>
#include <spm_hotplug.h>
#include <spm_mcdi.h>

/*
 * System Power Manager (SPM) is a hardware module, which controls cpu or
 * system power for different power scenarios using different firmware.
 * This driver controls the cpu power in cpu hotplug flow.
 */

#define PCM_HOTPLUG_VALID_MASK	0x00ff0000
#define PCM_HOTPLUG_VALID_SHIFT	0x8

/**********************************************************
 * PCM sequence for CPU hotplug
 **********************************************************/
static const unsigned int hotplug_binary[] = {
	0x1900001f, 0x1020020c, 0x1950001f, 0x1020020c, 0xa9400005, 0x00000001,
	0xe1000005, 0x1910001f, 0x10006720, 0x814c9001, 0xd82000e5, 0x17c07c1f,
	0x1900001f, 0x10001220, 0x1950001f, 0x10001220, 0xa15f0405, 0xe1000005,
	0x1900001f, 0x10001228, 0x1950001f, 0x10001228, 0x810f1401, 0xd8200244,
	0x17c07c1f, 0xe2e0006d, 0xe2e0002d, 0x1a00001f, 0x100062b8, 0x1910001f,
	0x100062b8, 0xa9000004, 0x00000001, 0xe2000004, 0x1910001f, 0x100062b8,
	0x81142804, 0xd8200444, 0x17c07c1f, 0xe2e0002c, 0xe2e0003c, 0xe2e0003e,
	0xe2e0003a, 0xe2e00032, 0x1910001f, 0x1000660c, 0x81079001, 0x1950001f,
	0x10006610, 0x81479401, 0xa1001404, 0xd8000584, 0x17c07c1f, 0x1900001f,
	0x10006404, 0x1950001f, 0x10006404, 0xa1568405, 0xe1000005, 0xf0000000,
	0x17c07c1f, 0x1900001f, 0x10006404, 0x1950001f, 0x10006404, 0x89400005,
	0x0000dfff, 0xe1000005, 0xe2e00036, 0xe2e0003e, 0x1910001f, 0x1000660c,
	0x81079001, 0x1950001f, 0x10006610, 0x81479401, 0x81001404, 0xd82008c4,
	0x17c07c1f, 0xe2e0002e, 0x1a00001f, 0x100062b8, 0x1910001f, 0x100062b8,
	0x89000004, 0x0000fffe, 0xe2000004, 0x1910001f, 0x100062b8, 0x81142804,
	0xd8000ae4, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e, 0xe2e0004c, 0xe2e0004d,
	0x1900001f, 0x10001220, 0x1950001f, 0x10001220, 0x89400005, 0xbfffffff,
	0xe1000005, 0x1900001f, 0x10001228, 0x1950001f, 0x10001228, 0x810f1401,
	0xd8000ce4, 0x17c07c1f, 0x1900001f, 0x1020020c, 0x1950001f, 0x1020020c,
	0x89400005, 0xfffffffe, 0xe1000005, 0xf0000000, 0x17c07c1f, 0x1212841f,
	0xe2e00036, 0xe2e0003e, 0x1380201f, 0xe2e0003c, 0xe2a00000, 0x1b80001f,
	0x20000080, 0xe2e0007c, 0x1b80001f, 0x20000003, 0xe2e0005c, 0xe2e0004c,
	0xe2e0004d, 0xf0000000, 0x17c07c1f, 0xe2e0004f, 0xe2e0006f, 0xe2e0002f,
	0xe2a00001, 0x1b80001f, 0x20000080, 0xe2e0002e, 0xe2e0003e, 0xe2e00032,
	0xf0000000, 0x17c07c1f, 0x1212841f, 0xe2e00026, 0xe2e0002e, 0x1380201f,
	0x1a00001f, 0x100062b4, 0x1910001f, 0x100062b4, 0x81322804, 0xe2000004,
	0x81202804, 0xe2000004, 0x1b80001f, 0x20000034, 0x1910001f, 0x100062b4,
	0x81142804, 0xd8001404, 0x17c07c1f, 0xe2e0000e, 0xe2e0000c, 0xe2e0000d,
	0xf0000000, 0x17c07c1f, 0xe2e0002d, 0x1a00001f, 0x100062b4, 0x1910001f,
	0x100062b4, 0xa1002804, 0xe2000004, 0xa1122804, 0xe2000004, 0x1b80001f,
	0x20000080, 0x1910001f, 0x100062b4, 0x81142804, 0xd82016a4, 0x17c07c1f,
	0xe2e0002f, 0xe2e0002b, 0xe2e00023, 0x1380201f, 0xe2e00022, 0xf0000000,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0x1840001f, 0x00000001,
	0x1840001f, 0x00000001, 0xa1d48407, 0x1b00001f, 0x2f7be75f, 0xe8208000,
	0x10006354, 0xfffe7b47, 0xa1d10407, 0x1b80001f, 0x20000020, 0x17c07c1f,
	0x1910001f, 0x10006b00, 0x81461001, 0xb14690a1, 0xd82044e5, 0x17c07c1f,
	0x1910001f, 0x10006610, 0x81079001, 0xd80044e4, 0x17c07c1f, 0x1990001f,
	0x10006b00, 0x81421801, 0x82429801, 0x81402405, 0xd80044e5, 0x17c07c1f,
	0x1a40001f, 0x100062b0, 0x1280041f, 0xc24007a0, 0x17c07c1f, 0x1910001f,
	0x10006b00, 0x81449001, 0xd8204be5, 0x17c07c1f, 0x1910001f, 0x10006b00,
	0x81009001, 0xd8204984, 0x17c07c1f, 0x1910001f, 0x10006610, 0x81051001,
	0xd8204be4, 0x17c07c1f, 0x1910001f, 0x10006720, 0x81489001, 0xd82046c5,
	0x17c07c1f, 0x1a40001f, 0x10006218, 0x1a80001f, 0x10006264, 0xc24010e0,
	0x17c07c1f, 0x1910001f, 0x1000660c, 0x81051001, 0x1950001f, 0x10006610,
	0x81451401, 0xa1001404, 0xd8004824, 0x17c07c1f, 0xd0004b00, 0x17c07c1f,
	0x17c07c1f, 0x1910001f, 0x10006610, 0x81051001, 0xd8004be4, 0x17c07c1f,
	0x1a40001f, 0x10006218, 0x1a80001f, 0x10006264, 0xc2400ee0, 0x17c07c1f,
	0x1910001f, 0x10006b00, 0x89000004, 0xfffffdff, 0x1940001f, 0x10006b00,
	0xe1400004, 0x17c07c1f, 0x1910001f, 0x10006b00, 0x81451001, 0xd8205305,
	0x17c07c1f, 0x1910001f, 0x10006b00, 0x81011001, 0xd82050a4, 0x17c07c1f,
	0x1910001f, 0x10006610, 0x81059001, 0xd8205304, 0x17c07c1f, 0x1910001f,
	0x10006720, 0x81491001, 0xd8204de5, 0x17c07c1f, 0x1a40001f, 0x1000621c,
	0x1a80001f, 0x1000626c, 0xc24010e0, 0x17c07c1f, 0x1910001f, 0x1000660c,
	0x81059001, 0x1950001f, 0x10006610, 0x81459401, 0xa1001404, 0xd8004f44,
	0x17c07c1f, 0xd0005220, 0x17c07c1f, 0x17c07c1f, 0x1910001f, 0x10006610,
	0x81059001, 0xd8005304, 0x17c07c1f, 0x1a40001f, 0x1000621c, 0x1a80001f,
	0x1000626c, 0xc2400ee0, 0x17c07c1f, 0x1910001f, 0x10006b00, 0x89000004,
	0xfffffbff, 0x1940001f, 0x10006b00, 0xe1400004, 0x17c07c1f, 0x1910001f,
	0x10006b00, 0x81459001, 0xd8205a25, 0x17c07c1f, 0x1910001f, 0x10006b00,
	0x81019001, 0xd82057c4, 0x17c07c1f, 0x1910001f, 0x10006610, 0x81061001,
	0xd8205a24, 0x17c07c1f, 0x1910001f, 0x10006720, 0x81499001, 0xd8205505,
	0x17c07c1f, 0x1a40001f, 0x10006220, 0x1a80001f, 0x10006274, 0xc24010e0,
	0x17c07c1f, 0x1910001f, 0x1000660c, 0x81061001, 0x1950001f, 0x10006610,
	0x81461401, 0xa1001404, 0xd8005664, 0x17c07c1f, 0xd0005940, 0x17c07c1f,
	0x17c07c1f, 0x1910001f, 0x10006610, 0x81061001, 0xd8005a24, 0x17c07c1f,
	0x1a40001f, 0x10006220, 0x1a80001f, 0x10006274, 0xc2400ee0, 0x17c07c1f,
	0x1910001f, 0x10006b00, 0x89000004, 0xfffff7ff, 0x1940001f, 0x10006b00,
	0xe1400004, 0x17c07c1f, 0x1910001f, 0x10006b00, 0x81461001, 0xd8206185,
	0x17c07c1f, 0x1910001f, 0x10006b00, 0x81021001, 0xd8205ec4, 0x17c07c1f,
	0x1910001f, 0x10006610, 0x81081001, 0xd8206184, 0x17c07c1f, 0x1910001f,
	0x10006720, 0x814a1001, 0xd8205c25, 0x17c07c1f, 0x1a40001f, 0x100062a0,
	0x1280041f, 0xc2401540, 0x17c07c1f, 0x1910001f, 0x1000660c, 0x81081001,
	0x1950001f, 0x10006610, 0x81481401, 0xa1001404, 0xd8005d64, 0x17c07c1f,
	0xd00060a0, 0x17c07c1f, 0x17c07c1f, 0x1910001f, 0x10006610, 0x81479001,
	0x81881001, 0x69a00006, 0x00000000, 0x81401805, 0xd8206185, 0x17c07c1f,
	0x1a40001f, 0x100062a0, 0x1280041f, 0xc2401240, 0x17c07c1f, 0x1910001f,
	0x10006b00, 0x89000004, 0xffffefff, 0x1940001f, 0x10006b00, 0xe1400004,
	0x17c07c1f, 0x1910001f, 0x10006b00, 0x81469001, 0xd82068e5, 0x17c07c1f,
	0x1910001f, 0x10006b00, 0x81029001, 0xd8206624, 0x17c07c1f, 0x1910001f,
	0x10006610, 0x81089001, 0xd82068e4, 0x17c07c1f, 0x1910001f, 0x10006720,
	0x814a9001, 0xd8206385, 0x17c07c1f, 0x1a40001f, 0x100062a4, 0x1290841f,
	0xc2401540, 0x17c07c1f, 0x1910001f, 0x1000660c, 0x81089001, 0x1950001f,
	0x10006610, 0x81489401, 0xa1001404, 0xd80064c4, 0x17c07c1f, 0xd0006800,
	0x17c07c1f, 0x17c07c1f, 0x1910001f, 0x10006610, 0x81479001, 0x81889001,
	0x69a00006, 0x00000000, 0x81401805, 0xd82068e5, 0x17c07c1f, 0x1a40001f,
	0x100062a4, 0x1290841f, 0xc2401240, 0x17c07c1f, 0x1910001f, 0x10006b00,
	0x89000004, 0xffffdfff, 0x1940001f, 0x10006b00, 0xe1400004, 0x1910001f,
	0x10006610, 0x81479001, 0x81881001, 0x69600005, 0x00000000, 0xa1401805,
	0x81889001, 0xa1401805, 0xd8006bc5, 0x17c07c1f, 0x1910001f, 0x10006b00,
	0x81421001, 0x82429001, 0x82802405, 0xd8206bca, 0x17c07c1f, 0x1a40001f,
	0x100062b0, 0x1280041f, 0xc2400000, 0x17c07c1f, 0x1990001f, 0x10006b00,
	0x89800006, 0x00003f00, 0x69200006, 0x00000000, 0xd82041e4, 0x17c07c1f,
	0x1990001f, 0x10006320, 0x69200006, 0xbeefbeef, 0xd8006dc4, 0x17c07c1f,
	0xd00041e0, 0x17c07c1f, 0x1910001f, 0x10006358, 0x810b1001, 0xd8006dc4,
	0x17c07c1f, 0x1980001f, 0xdeaddead, 0x19c0001f, 0x01411820, 0xf0000000
};
static const struct pcm_desc hotplug_pcm = {
	.version	= "pcm_power_down_mt8167_V37",
	.base		= hotplug_binary,
	.size		= 888,
	.sess		= 2,
	.replace	= 0,
};

static struct pwr_ctrl hotplug_ctrl = {
	.wake_src = 0,
	.wake_src_md32 = 0,
	.wfi_op = WFI_OP_OR,
	.mcusys_idle_mask = 1,
	.ca7top_idle_mask = 1,
	.ca15top_idle_mask = 1,
	.disp_req_mask = 1,
	.mfg_req_mask = 1,
	.md32_req_mask = 1,
	.syspwreq_mask = 1,
	.pcm_flags = 0,
};

static const struct spm_lp_scen spm_hotplug = {
	.pcmdesc = &hotplug_pcm,
	.pwrctrl = &hotplug_ctrl,
};

void spm_go_to_hotplug(void)
{
	const struct pcm_desc *pcmdesc = spm_hotplug.pcmdesc;
	struct pwr_ctrl *pwrctrl = spm_hotplug.pwrctrl;

	set_pwrctrl_pcm_flags(pwrctrl, 0);
	spm_reset_and_init_pcm();
	spm_kick_im_to_fetch(pcmdesc);
	spm_set_power_control(pwrctrl);
	spm_set_wakeup_event(pwrctrl);
	spm_kick_pcm_to_run(pwrctrl);
}

void spm_clear_hotplug(void)
{
	/* Inform SPM that CPU wants to program CPU_WAKEUP_EVENT and
	 * DISABLE_CPU_DROM */

	mmio_write_32(SPM_PCM_REG_DATA_INI, PCM_HANDSHAKE_SEND1);
	mmio_write_32(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R6);
	mmio_write_32(SPM_PCM_PWR_IO_EN, 0);

	/* Wait SPM's response, can't use sleep api */
	while ((mmio_read_32(SPM_PCM_FSM_STA) & PCM_END_FSM_STA_MASK)
		!= PCM_END_FSM_STA_DEF)
		;

	/* no hotplug pcm running */
	clear_all_ready();
}

void spm_hotplug_on(unsigned long mpidr)
{
	unsigned long linear_id;

	linear_id = platform_get_core_pos(mpidr);
	spm_lock_get();
	if (is_hotplug_ready() == 0) {
		spm_mcdi_wakeup_all_cores();
		mmio_clrbits_32(SPM_PCM_RESERVE, PCM_HOTPLUG_VALID_MASK);
		spm_go_to_hotplug();
		set_hotplug_ready();
	}
	/* turn on CPUx */
	mmio_clrsetbits_32(SPM_PCM_RESERVE,
		PCM_HOTPLUG_VALID_MASK | (1 << linear_id),
		1 << (linear_id + PCM_HOTPLUG_VALID_SHIFT));
	spm_lock_release();
}

void spm_hotplug_off(unsigned long mpidr)
{
	unsigned long linear_id;

	linear_id = platform_get_core_pos(mpidr);
	spm_lock_get();
	if (is_hotplug_ready() == 0) {
		spm_mcdi_wakeup_all_cores();
		mmio_clrbits_32(SPM_PCM_RESERVE, PCM_HOTPLUG_VALID_MASK);
		spm_go_to_hotplug();
		set_hotplug_ready();
	}
	mmio_clrsetbits_32(SPM_PCM_RESERVE, PCM_HOTPLUG_VALID_MASK,
		(1 << linear_id) |
		(1 << (linear_id + PCM_HOTPLUG_VALID_SHIFT)));
	spm_lock_release();
}