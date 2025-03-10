/******************************************************************************/
/* Copyright (c) 2013-2023 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_X64_128X1V2_H
#define RT_RTARCH_X64_128X1V2_H

#include "rtarch_x32_128x1v2.h"
#include "rtarch_xHB_128x1v2.h"
#include "rtarch_xHF_128x1v2.h"

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_x64_128x1v2.h: Implementation of x86_64 fp64 AVX512VL/DQ instructions.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmdp*_ri - applies [cmd] to [p]acked: [r]egister from [i]mmediate
 * cmdp*_rr - applies [cmd] to [p]acked: [r]egister from [r]egister
 *
 * cmdp*_rm - applies [cmd] to [p]acked: [r]egister from [m]emory
 * cmdp*_ld - applies [cmd] to [p]acked: as above
 *
 * cmdi*_** - applies [cmd] to 32-bit elements SIMD args, packed-128-bit
 * cmdj*_** - applies [cmd] to 64-bit elements SIMD args, packed-128-bit
 * cmdl*_** - applies [cmd] to L-size elements SIMD args, packed-128-bit
 *
 * cmdc*_** - applies [cmd] to 32-bit elements SIMD args, packed-256-bit
 * cmdd*_** - applies [cmd] to 64-bit elements SIMD args, packed-256-bit
 * cmdf*_** - applies [cmd] to L-size elements SIMD args, packed-256-bit
 *
 * cmdo*_** - applies [cmd] to 32-bit elements SIMD args, packed-var-len
 * cmdp*_** - applies [cmd] to L-size elements SIMD args, packed-var-len
 * cmdq*_** - applies [cmd] to 64-bit elements SIMD args, packed-var-len
 *
 * cmdr*_** - applies [cmd] to 32-bit elements ELEM args, scalar-fp-only
 * cmds*_** - applies [cmd] to L-size elements ELEM args, scalar-fp-only
 * cmdt*_** - applies [cmd] to 64-bit elements ELEM args, scalar-fp-only
 *
 * cmd*x_** - applies [cmd] to SIMD/BASE unsigned integer args, [x] - default
 * cmd*n_** - applies [cmd] to SIMD/BASE   signed integer args, [n] - negatable
 * cmd*s_** - applies [cmd] to SIMD/ELEM floating point   args, [s] - scalable
 *
 * The cmdp*_** (rtconf.h) instructions are intended for SPMD programming model
 * and can be configured to work with 32/64-bit data elements (fp+int).
 * In this model data paths are fixed-width, BASE and SIMD data elements are
 * width-compatible, code path divergence is handled via mkj**_** pseudo-ops.
 * Matching element-sized BASE subset cmdy*_** is defined in rtconf.h as well.
 *
 * Note, when using fixed-data-size 128/256-bit SIMD subsets simultaneously
 * upper 128-bit halves of full 256-bit SIMD registers may end up undefined.
 * On RISC targets they remain unchanged, while on x86-AVX they are zeroed.
 * This happens when registers written in 128-bit subset are then used/read
 * from within 256-bit subset. The same rule applies to mixing with 512-bit
 * and wider vectors. Use of scalars may leave respective vector registers
 * undefined, as seen from the perspective of any particular vector subset.
 *
 * 256-bit vectors used with wider subsets may not be compatible with regards
 * to memory loads/stores when mixed in the code. It means that data loaded
 * with wider vector and stored within 256-bit subset at the same address may
 * result in changing the initial representation in memory. The same can be
 * said about mixing vector and scalar subsets. Scalars can be completely
 * detached on some architectures. Use elm*x_st to store 1st vector element.
 * 128-bit vectors should be memory-compatible with any wider vector subset.
 *
 * Handling of NaNs in the floating point pipeline may not be consistent
 * across different architectures. Avoid NaNs entering the data flow by using
 * masking or control flow instructions. Apply special care when dealing with
 * floating point compare and min/max input/output. The result of floating point
 * compare instructions can be considered a -QNaN, though it is also interpreted
 * as integer -1 and is often treated as a mask. Most arithmetic instructions
 * should propagate QNaNs unchanged, however this behavior hasn't been tested.
 *
 * Note, that instruction subsets operating on vectors of different length
 * may support different number of SIMD registers, therefore mixing them
 * in the same code needs to be done with register awareness in mind.
 * For example, AVX-512 supports 32 SIMD registers, while AVX2 only has 16,
 * as does 256-bit paired subset on ARMv8, while 128-bit and SVE have 32.
 * These numbers should be consistent across architectures if properly
 * mapped to SIMD target mask presented in rtzero.h (compatibility layer).
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * XD - SIMD register serving as destination only, if present
 * XG - SIMD register serving as destination and first source
 * XS - SIMD register serving as second source (first if any)
 * XT - SIMD register serving as third source (second if any)
 *
 * RD - BASE register serving as destination only, if present
 * RG - BASE register serving as destination and first source
 * RS - BASE register serving as second source (first if any)
 * RT - BASE register serving as third source (second if any)
 *
 * MD - BASE addressing mode (Oeax, M***, I***) (memory-dest)
 * MG - BASE addressing mode (Oeax, M***, I***) (memory-dsrc)
 * MS - BASE addressing mode (Oeax, M***, I***) (memory-src2)
 * MT - BASE addressing mode (Oeax, M***, I***) (memory-src3)
 *
 * DD - displacement value (DP, DF, DG, DH, DV) (memory-dest)
 * DG - displacement value (DP, DF, DG, DH, DV) (memory-dsrc)
 * DS - displacement value (DP, DF, DG, DH, DV) (memory-src2)
 * DT - displacement value (DP, DF, DG, DH, DV) (memory-src3)
 *
 * IS - immediate value (is used as a second or first source)
 * IT - immediate value (is used as a third or second source)
 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#if (defined RT_SIMD_CODE)

#if (RT_128X1 == 2)

#define ck1jx_rm(XS, MT, DT) /* not portable, do not use outside */         \
        EVW(0,       RXB(XS),    0x00, 0, 2, 2) EMITB(0x39)                 \
        MRM(0x01,    MOD(XS), REG(XS))

#define mz1jx_ld(XD, MS, DS) /* not portable, do not use outside */         \
        EVW(RXB(XD),       0,    0x00, 0, 2, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x03,    0x01)

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/* elm (D = S), store first SIMD element with natural alignment
 * allows to decouple scalar subset from SIMD where appropriate */

#define elmjx_st(XS, MD, DD) /* 1st elem as in mem with SIMD load/store */  \
        movts_st(W(XS), W(MD), W(DD))

/***************   packed double-precision generic move/logic   ***************/

/* mov (D = S) */

#define movjx_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movjx_ld(XD, MS, DS)                                                \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define movjx_st(XS, MD, DD)                                                \
    ADR EVW(RXB(XS), RXB(MD),    0x00, 0, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#define mmvjx_rr(XG, XS)                                                    \
        ck1jx_rm(Xmm0, Mebp, inf_GPC07)                                     \
        EKW(RXB(XG), RXB(XS),    0x00, 0, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mmvjx_ld(XG, MS, DS)                                                \
        ck1jx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(RXB(XG), RXB(MS),    0x00, 0, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mmvjx_st(XS, MG, DG)                                                \
        ck1jx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(RXB(XS), RXB(MG),    0x00, 0, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MG), REG(MG))                                      \
        AUX(SIB(MG), CMD(DG), EMPTY)

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define andjx_rr(XG, XS)                                                    \
        andjx3rr(W(XG), W(XG), W(XS))

#define andjx_ld(XG, MS, DS)                                                \
        andjx3ld(W(XG), W(XG), W(MS), W(DS))

#define andjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define andjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define annjx_rr(XG, XS)                                                    \
        annjx3rr(W(XG), W(XG), W(XS))

#define annjx_ld(XG, MS, DS)                                                \
        annjx3ld(W(XG), W(XG), W(MS), W(DS))

#define annjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define annjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrjx_rr(XG, XS)                                                    \
        orrjx3rr(W(XG), W(XG), W(XS))

#define orrjx_ld(XG, MS, DS)                                                \
        orrjx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define orrjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* orn (G = ~G | S), (D = ~S | T) if (#D != #T) */

#define ornjx_rr(XG, XS)                                                    \
        notjx_rx(W(XG))                                                     \
        orrjx_rr(W(XG), W(XS))

#define ornjx_ld(XG, MS, DS)                                                \
        notjx_rx(W(XG))                                                     \
        orrjx_ld(W(XG), W(MS), W(DS))

#define ornjx3rr(XD, XS, XT)                                                \
        notjx_rr(W(XD), W(XS))                                              \
        orrjx_rr(W(XD), W(XT))

#define ornjx3ld(XD, XS, MT, DT)                                            \
        notjx_rr(W(XD), W(XS))                                              \
        orrjx_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (#D != #T) */

#define xorjx_rr(XG, XS)                                                    \
        xorjx3rr(W(XG), W(XG), W(XS))

#define xorjx_ld(XG, MS, DS)                                                \
        xorjx3ld(W(XG), W(XG), W(MS), W(DS))

#define xorjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define xorjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* not (G = ~G), (D = ~S) */

#define notjx_rx(XG)                                                        \
        notjx_rr(W(XG), W(XG))

#define notjx_rr(XD, XS)                                                    \
        annjx3ld(W(XD), W(XS), Mebp, inf_GPC07)

/************   packed double-precision floating-point arithmetic   ***********/

/* neg (G = -G), (D = -S) */

#define negjs_rx(XG)                                                        \
        negjs_rr(W(XG), W(XG))

#define negjs_rr(XD, XS)                                                    \
        xorjx3ld(W(XD), W(XS), Mebp, inf_GPC06_64)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addjs_rr(XG, XS)                                                    \
        addjs3rr(W(XG), W(XG), W(XS))

#define addjs_ld(XG, MS, DS)                                                \
        addjs3ld(W(XG), W(XG), W(MS), W(DS))

#define addjs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

        /* adp, adh are defined in rtbase.h (first 15-regs only)
         * under "COMMON SIMD INSTRUCTIONS" section */

#undef  adpjs3rr
#define adpjs3rr(XD, XS, XT)                                                \
        VEX(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x7C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#undef  adpjs3ld
#define adpjs3ld(XD, XS, MT, DT)                                            \
    ADR VEX(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x7C)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subjs_rr(XG, XS)                                                    \
        subjs3rr(W(XG), W(XG), W(XS))

#define subjs_ld(XG, MS, DS)                                                \
        subjs3ld(W(XG), W(XG), W(MS), W(DS))

#define subjs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define subjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define muljs_rr(XG, XS)                                                    \
        muljs3rr(W(XG), W(XG), W(XS))

#define muljs_ld(XG, MS, DS)                                                \
        muljs3ld(W(XG), W(XG), W(MS), W(DS))

#define muljs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define muljs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

        /* mlp, mlh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* div (G = G / S), (D = S / T) if (#D != #T) and on ARMv7 if (#D != #S) */

#define divjs_rr(XG, XS)                                                    \
        divjs3rr(W(XG), W(XG), W(XS))

#define divjs_ld(XG, MS, DS)                                                \
        divjs3ld(W(XG), W(XG), W(MS), W(DS))

#define divjs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define divjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* sqr (D = sqrt S) */

#define sqrjs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define sqrjs_ld(XD, MS, DS)                                                \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if   RT_SIMD_COMPAT_RCP == 0

#define rcejs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 2) EMITB(0xCA)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rcsjs_rr(XG, XS) /* destroys XS */

#elif RT_SIMD_COMPAT_RCP == 2

#define rcejs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 2) EMITB(0x4C)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rcsjs_rr(XG, XS) /* destroys XS */                                  \
        muljs_rr(W(XS), W(XG))                                              \
        muljs_rr(W(XS), W(XG))                                              \
        addjs_rr(W(XG), W(XG))                                              \
        subjs_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RCP */

        /* rce, rcs, rcp are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if   RT_SIMD_COMPAT_RSQ == 0

#define rsejs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 2) EMITB(0xCC)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rssjs_rr(XG, XS) /* destroys XS */

#elif RT_SIMD_COMPAT_RSQ == 2

#define rsejs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 2) EMITB(0x4E)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rssjs_rr(XG, XS) /* destroys XS */                                  \
        muljs_rr(W(XS), W(XG))                                              \
        muljs_rr(W(XS), W(XG))                                              \
        subjs_ld(W(XS), Mebp, inf_GPC03_64)                                 \
        muljs_ld(W(XS), Mebp, inf_GPC02_64)                                 \
        muljs_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rse, rss, rsq are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T) if (#G != #S && #G != #T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmajs_rr(XG, XS, XT)                                                \
    ADR EVW(RXB(XG), RXB(XT), REN(XS), 0, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmajs_ld(XG, XS, MT, DT)                                            \
    ADR EVW(RXB(XG), RXB(MT), REN(XS), 0, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T) if (#G != #S && #G != #T)
 * NOTE: due to final negation being outside of rounding on all POWER systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsjs_rr(XG, XS, XT)                                                \
    ADR EVW(RXB(XG), RXB(XT), REN(XS), 0, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmsjs_ld(XG, XS, MT, DT)                                            \
    ADR EVW(RXB(XG), RXB(MT), REN(XS), 0, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMS */

/*************   packed double-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T) */

#define minjs_rr(XG, XS)                                                    \
        minjs3rr(W(XG), W(XG), W(XS))

#define minjs_ld(XG, MS, DS)                                                \
        minjs3ld(W(XG), W(XG), W(MS), W(DS))

#define minjs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

        /* mnp, mnh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T) */

#define maxjs_rr(XG, XS)                                                    \
        maxjs3rr(W(XG), W(XG), W(XS))

#define maxjs_ld(XG, MS, DS)                                                \
        maxjs3ld(W(XG), W(XG), W(MS), W(DS))

#define maxjs3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

        /* mxp, mxh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqjs_rr(XG, XS)                                                    \
        ceqjs3rr(W(XG), W(XG), W(XS))

#define ceqjs_ld(XG, MS, DS)                                                \
        ceqjs3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqjs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define ceqjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x00))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnejs_rr(XG, XS)                                                    \
        cnejs3rr(W(XG), W(XG), W(XS))

#define cnejs_ld(XG, MS, DS)                                                \
        cnejs3ld(W(XG), W(XG), W(MS), W(DS))

#define cnejs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cnejs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x04))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T) */

#define cltjs_rr(XG, XS)                                                    \
        cltjs3rr(W(XG), W(XG), W(XS))

#define cltjs_ld(XG, MS, DS)                                                \
        cltjs3ld(W(XG), W(XG), W(MS), W(DS))

#define cltjs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cltjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T) */

#define clejs_rr(XG, XS)                                                    \
        clejs3rr(W(XG), W(XG), W(XS))

#define clejs_ld(XG, MS, DS)                                                \
        clejs3ld(W(XG), W(XG), W(MS), W(DS))

#define clejs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define clejs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T) */

#define cgtjs_rr(XG, XS)                                                    \
        cgtjs3rr(W(XG), W(XG), W(XS))

#define cgtjs_ld(XG, MS, DS)                                                \
        cgtjs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtjs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgtjs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T) */

#define cgejs_rr(XG, XS)                                                    \
        cgejs3rr(W(XG), W(XG), W(XS))

#define cgejs_ld(XG, MS, DS)                                                \
        cgejs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgejs3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgejs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE64_128    0x00     /* none satisfy the condition */
#define RT_SIMD_MASK_FULL64_128    0x0F     /*  all satisfy the condition */

/* #define mk1wx_rx(RD)                    (defined in 32_128-bit header) */
/* #define ck1ix_rm(XS, MT, DT)            (defined in 32_128-bit header) */

#define mkjjx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        ck1ix_rm(W(XS), Mebp, inf_GPC07)                                    \
        mk1wx_rx(Reax)                                                      \
        cmpwx_ri(Reax, IB(RT_SIMD_MASK_##mask##64_128))                     \
        jeqxx_lb(lb)

/*************   packed double-precision floating-point convert   *************/

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnzjs_rr(XD, XS)     /* round towards zero */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))

#define rnzjs_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x03))

#define cvzjs_rr(XD, XS)     /* round towards zero */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvzjs_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnpjs_rr(XD, XS)     /* round towards +inf */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))

#define rnpjs_ld(XD, MS, DS) /* round towards +inf */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x02))

#define cvpjs_rr(XD, XS)     /* round towards +inf */                       \
        ERW(RXB(XD), RXB(XS),    0x00, 2, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvpjs_ld(XD, MS, DS) /* round towards +inf */                       \
        movjs_ld(W(XD), W(MS), W(DS))                                       \
        cvpjs_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnmjs_rr(XD, XS)     /* round towards -inf */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))

#define rnmjs_ld(XD, MS, DS) /* round towards -inf */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x01))

#define cvmjs_rr(XD, XS)     /* round towards -inf */                       \
        ERW(RXB(XD), RXB(XS),    0x00, 1, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvmjs_ld(XD, MS, DS) /* round towards -inf */                       \
        movjs_ld(W(XD), W(MS), W(DS))                                       \
        cvmjs_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnnjs_rr(XD, XS)     /* round towards near */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define rnnjs_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))

#define cvnjs_rr(XD, XS)     /* round towards near */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvnjs_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvnjn_rr(XD, XS)     /* round towards near */                       \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvnjn_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX POWER systems, use cvz
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rndjs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))

#define rndjs_ld(XD, MS, DS)                                                \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x04))

#define cvtjs_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtjs_ld(XD, MS, DS)                                                \
        EVW(RXB(XD), RXB(MS),    0x00, 0, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX POWER systems */

#define cvtjn_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtjn_ld(XD, MS, DS)                                                \
        EVW(RXB(XD), RXB(MS),    0x00, 0, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnrjs_rr(XD, XS, mode)                                              \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))

#define cvrjs_rr(XD, XS, mode)                                              \
        ERW(RXB(XD), RXB(XS), 0x00, RT_SIMD_MODE_##mode&3, 1, 1) EMITB(0x7B)\
        MRM(REG(XD), MOD(XS), REG(XS))

/************   packed double-precision integer arithmetic/shifts   ***********/

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addjx_rr(XG, XS)                                                    \
        addjx3rr(W(XG), W(XG), W(XS))

#define addjx_ld(XG, MS, DS)                                                \
        addjx3ld(W(XG), W(XG), W(MS), W(DS))

#define addjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subjx_rr(XG, XS)                                                    \
        subjx3rr(W(XG), W(XG), W(XS))

#define subjx_ld(XG, MS, DS)                                                \
        subjx3ld(W(XG), W(XG), W(MS), W(DS))

#define subjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define subjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define muljx_rr(XG, XS)                                                    \
        muljx3rr(W(XG), W(XG), W(XS))

#define muljx_ld(XG, MS, DS)                                                \
        muljx3ld(W(XG), W(XG), W(MS), W(DS))

#define muljx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define muljx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shljx_ri(XG, IS)                                                    \
        shljx3ri(W(XG), W(XG), W(IS))

#define shljx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shljx3ld(W(XG), W(XG), W(MS), W(DS))

#define shljx3ri(XD, XS, IT)                                                \
        EVW(0,       RXB(XS), REN(XD), 0, 1, 1) EMITB(0x73)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shljx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0xF3)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrjx_ri(XG, IS)                                                    \
        shrjx3ri(W(XG), W(XG), W(IS))

#define shrjx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrjx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrjx3ri(XD, XS, IT)                                                \
        EVW(0,       RXB(XS), REN(XD), 0, 1, 1) EMITB(0x73)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0xD3)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrjn_ri(XG, IS)                                                    \
        shrjn3ri(W(XG), W(XG), W(IS))

#define shrjn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrjn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrjn3ri(XD, XS, IT)                                                \
        EVW(0,       RXB(XS), REN(XD), 0, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svljx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svljx3rr(W(XG), W(XG), W(XS))

#define svljx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svljx3ld(W(XG), W(XG), W(MS), W(DS))

#define svljx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svljx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrjx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrjx3rr(W(XG), W(XG), W(XS))

#define svrjx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrjx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrjn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrjn3rr(W(XG), W(XG), W(XS))

#define svrjn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrjn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrjn3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/****************   packed double-precision integer compare   *****************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minjx_rr(XG, XS)                                                    \
        minjx3rr(W(XG), W(XG), W(XS))

#define minjx_ld(XG, MS, DS)                                                \
        minjx3ld(W(XG), W(XG), W(MS), W(DS))

#define minjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minjn_rr(XG, XS)                                                    \
        minjn3rr(W(XG), W(XG), W(XS))

#define minjn_ld(XG, MS, DS)                                                \
        minjn3ld(W(XG), W(XG), W(MS), W(DS))

#define minjn3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxjx_rr(XG, XS)                                                    \
        maxjx3rr(W(XG), W(XG), W(XS))

#define maxjx_ld(XG, MS, DS)                                                \
        maxjx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxjx3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxjn_rr(XG, XS)                                                    \
        maxjn3rr(W(XG), W(XG), W(XS))

#define maxjn_ld(XG, MS, DS)                                                \
        maxjn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxjn3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqjx_rr(XG, XS)                                                    \
        ceqjx3rr(W(XG), W(XG), W(XS))

#define ceqjx_ld(XG, MS, DS)                                                \
        ceqjx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqjx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define ceqjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x00))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnejx_rr(XG, XS)                                                    \
        cnejx3rr(W(XG), W(XG), W(XS))

#define cnejx_ld(XG, MS, DS)                                                \
        cnejx3ld(W(XG), W(XG), W(MS), W(DS))

#define cnejx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cnejx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x04))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltjx_rr(XG, XS)                                                    \
        cltjx3rr(W(XG), W(XG), W(XS))

#define cltjx_ld(XG, MS, DS)                                                \
        cltjx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltjx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cltjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltjn_rr(XG, XS)                                                    \
        cltjn3rr(W(XG), W(XG), W(XS))

#define cltjn_ld(XG, MS, DS)                                                \
        cltjn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltjn3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cltjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x01))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define clejx_rr(XG, XS)                                                    \
        clejx3rr(W(XG), W(XG), W(XS))

#define clejx_ld(XG, MS, DS)                                                \
        clejx3ld(W(XG), W(XG), W(MS), W(DS))

#define clejx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define clejx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define clejn_rr(XG, XS)                                                    \
        clejn3rr(W(XG), W(XG), W(XS))

#define clejn_ld(XG, MS, DS)                                                \
        clejn3ld(W(XG), W(XG), W(MS), W(DS))

#define clejn3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define clejn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x02))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtjx_rr(XG, XS)                                                    \
        cgtjx3rr(W(XG), W(XG), W(XS))

#define cgtjx_ld(XG, MS, DS)                                                \
        cgtjx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtjx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgtjx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtjn_rr(XG, XS)                                                    \
        cgtjn3rr(W(XG), W(XG), W(XS))

#define cgtjn_ld(XG, MS, DS)                                                \
        cgtjn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtjn3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgtjn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x06))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgejx_rr(XG, XS)                                                    \
        cgejx3rr(W(XG), W(XG), W(XS))

#define cgejx_ld(XG, MS, DS)                                                \
        cgejx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgejx3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgejx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgejn_rr(XG, XS)                                                    \
        cgejn3rr(W(XG), W(XG), W(XS))

#define cgejn_ld(XG, MS, DS)                                                \
        cgejn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgejn3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

#define cgejn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x05))                                  \
        mz1jx_ld(W(XD), Mebp, inf_GPC07)

/******************************************************************************/
/**********************************   ELEM   **********************************/
/******************************************************************************/

/*********   scalar double-precision floating-point move/arithmetic   *********/

/* mov (D = S) */

#define movts_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS), REN(XD), 0, 3, 1) EMITB(0x10)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movts_ld(XD, MS, DS)                                                \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 3, 1) EMITB(0x10)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define movts_st(XS, MD, DD)                                                \
    ADR EVW(RXB(XS), RXB(MD),    0x00, 0, 3, 1) EMITB(0x11)                 \
        MRM(REG(XS), MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addts_rr(XG, XS)                                                    \
        addts3rr(W(XG), W(XG), W(XS))

#define addts_ld(XG, MS, DS)                                                \
        addts3ld(W(XG), W(XG), W(MS), W(DS))

#define addts3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subts_rr(XG, XS)                                                    \
        subts3rr(W(XG), W(XG), W(XS))

#define subts_ld(XG, MS, DS)                                                \
        subts3ld(W(XG), W(XG), W(MS), W(DS))

#define subts3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define subts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mults_rr(XG, XS)                                                    \
        mults3rr(W(XG), W(XG), W(XS))

#define mults_ld(XG, MS, DS)                                                \
        mults3ld(W(XG), W(XG), W(MS), W(DS))

#define mults3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define mults3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* div (G = G / S), (D = S / T) if (#D != #T) and on ARMv7 if (#D != #S) */

#define divts_rr(XG, XS)                                                    \
        divts3rr(W(XG), W(XG), W(XS))

#define divts_ld(XG, MS, DS)                                                \
        divts3ld(W(XG), W(XG), W(MS), W(DS))

#define divts3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define divts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* sqr (D = sqrt S) */

#define sqrts_rr(XD, XS)                                                    \
        EVW(RXB(XD), RXB(XS),    0x00, 0, 3, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define sqrts_ld(XD, MS, DS)                                                \
    ADR EVW(RXB(XD), RXB(MS),    0x00, 0, 3, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rcets_rr(XD, XS)                                                    \
        movts_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movts_ld(W(XD), Mebp, inf_GPC01_64)                                 \
        divts_ld(W(XD), Mebp, inf_SCR01(0))

#define rcsts_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RCP */

        /* rce, rcs, rcp are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rsets_rr(XD, XS)                                                    \
        sqrts_rr(W(XD), W(XS))                                              \
        movts_st(W(XD), Mebp, inf_SCR01(0))                                 \
        movts_ld(W(XD), Mebp, inf_GPC01_64)                                 \
        divts_ld(W(XD), Mebp, inf_SCR01(0))

#define rssts_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rse, rss, rsq are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T) if (#G != #S && #G != #T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmats_rr(XG, XS, XT)                                                \
    ADR EVW(RXB(XG), RXB(XT), REN(XS), 0, 1, 2) EMITB(0xB9)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmats_ld(XG, XS, MT, DT)                                            \
    ADR EVW(RXB(XG), RXB(MT), REN(XS), 0, 1, 2) EMITB(0xB9)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T) if (#G != #S && #G != #T)
 * NOTE: due to final negation being outside of rounding on all POWER systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsts_rr(XG, XS, XT)                                                \
    ADR EVW(RXB(XG), RXB(XT), REN(XS), 0, 1, 2) EMITB(0xBD)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmsts_ld(XG, XS, MT, DT)                                            \
    ADR EVW(RXB(XG), RXB(MT), REN(XS), 0, 1, 2) EMITB(0xBD)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMS */

/*************   scalar double-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T) */

#define mints_rr(XG, XS)                                                    \
        mints3rr(W(XG), W(XG), W(XS))

#define mints_ld(XG, MS, DS)                                                \
        mints3ld(W(XG), W(XG), W(MS), W(DS))

#define mints3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define mints3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T) */

#define maxts_rr(XG, XS)                                                    \
        maxts3rr(W(XG), W(XG), W(XS))

#define maxts_ld(XG, MS, DS)                                                \
        maxts3ld(W(XG), W(XG), W(MS), W(DS))

#define maxts3rr(XD, XS, XT)                                                \
        EVW(RXB(XD), RXB(XT), REN(XS), 0, 3, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(RXB(XD), RXB(MT), REN(XS), 0, 3, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqts_rr(XG, XS)                                                    \
        ceqts3rr(W(XG), W(XG), W(XS))

#define ceqts_ld(XG, MS, DS)                                                \
        ceqts3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqts3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define ceqts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x00))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnets_rr(XG, XS)                                                    \
        cnets3rr(W(XG), W(XG), W(XS))

#define cnets_ld(XG, MS, DS)                                                \
        cnets3ld(W(XG), W(XG), W(MS), W(DS))

#define cnets3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define cnets3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x04))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T) */

#define cltts_rr(XG, XS)                                                    \
        cltts3rr(W(XG), W(XG), W(XS))

#define cltts_ld(XG, MS, DS)                                                \
        cltts3ld(W(XG), W(XG), W(MS), W(DS))

#define cltts3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define cltts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x01))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T) */

#define clets_rr(XG, XS)                                                    \
        clets3rr(W(XG), W(XG), W(XS))

#define clets_ld(XG, MS, DS)                                                \
        clets3ld(W(XG), W(XG), W(MS), W(DS))

#define clets3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define clets3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x02))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T) */

#define cgtts_rr(XG, XS)                                                    \
        cgtts3rr(W(XG), W(XG), W(XS))

#define cgtts_ld(XG, MS, DS)                                                \
        cgtts3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtts3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define cgtts3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x06))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T) */

#define cgets_rr(XG, XS)                                                    \
        cgets3rr(W(XG), W(XG), W(XS))

#define cgets_ld(XG, MS, DS)                                                \
        cgets3ld(W(XG), W(XG), W(MS), W(DS))

#define cgets3rr(XD, XS, XT)                                                \
        EVW(0,       RXB(XT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)

#define cgets3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REN(XS), 0, 3, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMITB(0x05))                                  \
        mz1tx_ld(W(XD), Mebp, inf_GPC07)


#define mz1tx_ld(XG, MS, DS) /* not portable, do not use outside */         \
    ADR EZW(RXB(XG), RXB(MS),    0x00, 0, 3, 1) EMITB(0x10)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_128X1 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_X64_128X1V2_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
