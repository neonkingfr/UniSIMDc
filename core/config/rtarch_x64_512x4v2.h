/******************************************************************************/
/* Copyright (c) 2013-2023 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_X64_512X4V2_H
#define RT_RTARCH_X64_512X4V2_H

#include "rtarch_x32_512x4v2.h"
#include "rtarch_xHB_512x4v2.h"
#include "rtarch_xHF_512x4v2.h"

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_x64_512x4v2.h: Implementation of x86_64 fp64 AVX512F/DQ quaded ops.
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

#if (RT_512X4 >= 1 && RT_512X4 <= 2)

#if (RT_512X4 == 1)

#define ck1qx_rm(XS, MT, DT) /* not portable, do not use outside */         \
    ADR EVW(0,       RXB(MT), REN(XS), K, 1, 2) EMITB(0x29)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

#define mz1qx_ld(XD, MS, DS) /* not portable, do not use outside */         \
    ADR EZW(RXB(XD), RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#else  /* (RT_512X4 == 2) */

#define ck1qx_rm(XS, MT, DT) /* not portable, do not use outside */         \
        EVW(0,       RXB(XS),    0x00, K, 2, 2) EMITB(0x39)                 \
        MRM(0x01,    MOD(XS), REG(XS))

#define mz1qx_ld(XD, MS, DS) /* not portable, do not use outside */         \
        EVW(RXB(XD),       0,    0x00, K, 2, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x03,    0x01)

#endif /* (RT_512X4 == 2) */

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/* elm (D = S), store first SIMD element with natural alignment
 * allows to decouple scalar subset from SIMD where appropriate */

#define elmqx_st(XS, MD, DD) /* 1st elem as in mem with SIMD load/store */  \
        elmjx_st(W(XS), W(MD), W(DD))

/***************   packed double-precision generic move/logic   ***************/

/* mov (D = S) */

#define movqx_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movqx_ld(XD, MS, DS)                                                \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#define movqx_st(XS, MD, DD)                                                \
    ADR EVW(0,       RXB(MD),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VAL(DD)), EMPTY)                                 \
    ADR EVW(1,       RXB(MD),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VZL(DD)), EMPTY)                                 \
    ADR EVW(2,       RXB(MD),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VSL(DD)), EMPTY)                                 \
    ADR EVW(3,       RXB(MD),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VTL(DD)), EMPTY)

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#define mmvqx_rr(XG, XS)                                                    \
        ck1qx_rm(Xmm0, Mebp, inf_GPC07)                                     \
        EKW(0,             0,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1qx_rm(Xmm8, Mebp, inf_GPC07)                                     \
        EKW(1,             1,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1qx_rm(XmmG, Mebp, inf_GPC07)                                     \
        EKW(2,             2,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1qx_rm(XmmO, Mebp, inf_GPC07)                                     \
        EKW(3,             3,    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mmvqx_ld(XG, MS, DS)                                                \
        ck1qx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
        ck1qx_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
        ck1qx_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
        ck1qx_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x28)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#define mmvqx_st(XS, MG, DG)                                                \
        ck1qx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(0,       RXB(MG),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VAL(DG)), EMPTY)                                 \
        ck1qx_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKW(1,       RXB(MG),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VZL(DG)), EMPTY)                                 \
        ck1qx_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKW(2,       RXB(MG),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VSL(DG)), EMPTY)                                 \
        ck1qx_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKW(3,       RXB(MG),    0x00, K, 1, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VTL(DG)), EMPTY)

#if (RT_512X4 < 2)

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define andqx_rr(XG, XS)                                                    \
        andqx3rr(W(XG), W(XG), W(XS))

#define andqx_ld(XG, MS, DS)                                                \
        andqx3ld(W(XG), W(XG), W(MS), W(DS))

#define andqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define andqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define annqx_rr(XG, XS)                                                    \
        annqx3rr(W(XG), W(XG), W(XS))

#define annqx_ld(XG, MS, DS)                                                \
        annqx3ld(W(XG), W(XG), W(MS), W(DS))

#define annqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define annqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrqx_rr(XG, XS)                                                    \
        orrqx3rr(W(XG), W(XG), W(XS))

#define orrqx_ld(XG, MS, DS)                                                \
        orrqx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define orrqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orn (G = ~G | S), (D = ~S | T) if (#D != #T) */

#define ornqx_rr(XG, XS)                                                    \
        notqx_rx(W(XG))                                                     \
        orrqx_rr(W(XG), W(XS))

#define ornqx_ld(XG, MS, DS)                                                \
        notqx_rx(W(XG))                                                     \
        orrqx_ld(W(XG), W(MS), W(DS))

#define ornqx3rr(XD, XS, XT)                                                \
        notqx_rr(W(XD), W(XS))                                              \
        orrqx_rr(W(XD), W(XT))

#define ornqx3ld(XD, XS, MT, DT)                                            \
        notqx_rr(W(XD), W(XS))                                              \
        orrqx_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (#D != #T) */

#define xorqx_rr(XG, XS)                                                    \
        xorqx3rr(W(XG), W(XG), W(XS))

#define xorqx_ld(XG, MS, DS)                                                \
        xorqx3ld(W(XG), W(XG), W(MS), W(DS))

#define xorqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define xorqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#else /* RT_512X4 >= 2 */

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define andqx_rr(XG, XS)                                                    \
        andqx3rr(W(XG), W(XG), W(XS))

#define andqx_ld(XG, MS, DS)                                                \
        andqx3ld(W(XG), W(XG), W(MS), W(DS))

#define andqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define andqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x54)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define annqx_rr(XG, XS)                                                    \
        annqx3rr(W(XG), W(XG), W(XS))

#define annqx_ld(XG, MS, DS)                                                \
        annqx3ld(W(XG), W(XG), W(MS), W(DS))

#define annqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define annqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x55)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrqx_rr(XG, XS)                                                    \
        orrqx3rr(W(XG), W(XG), W(XS))

#define orrqx_ld(XG, MS, DS)                                                \
        orrqx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define orrqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x56)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orn (G = ~G | S), (D = ~S | T) if (#D != #T) */

#define ornqx_rr(XG, XS)                                                    \
        notqx_rx(W(XG))                                                     \
        orrqx_rr(W(XG), W(XS))

#define ornqx_ld(XG, MS, DS)                                                \
        notqx_rx(W(XG))                                                     \
        orrqx_ld(W(XG), W(MS), W(DS))

#define ornqx3rr(XD, XS, XT)                                                \
        notqx_rr(W(XD), W(XS))                                              \
        orrqx_rr(W(XD), W(XT))

#define ornqx3ld(XD, XS, MT, DT)                                            \
        notqx_rr(W(XD), W(XS))                                              \
        orrqx_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (#D != #T) */

#define xorqx_rr(XG, XS)                                                    \
        xorqx3rr(W(XG), W(XG), W(XS))

#define xorqx_ld(XG, MS, DS)                                                \
        xorqx3ld(W(XG), W(XG), W(MS), W(DS))

#define xorqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define xorqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x57)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* not (G = ~G), (D = ~S) */

#define notqx_rx(XG)                                                        \
        notqx_rr(W(XG), W(XG))

#define notqx_rr(XD, XS)                                                    \
        annqx3ld(W(XD), W(XS), Mebp, inf_GPC07)

/************   packed double-precision floating-point arithmetic   ***********/

/* neg (G = -G), (D = -S) */

#define negqs_rx(XG)                                                        \
        negqs_rr(W(XG), W(XG))

#define negqs_rr(XD, XS)                                                    \
        xorqx3ld(W(XD), W(XS), Mebp, inf_GPC06_64)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addqs_rr(XG, XS)                                                    \
        addqs3rr(W(XG), W(XG), W(XS))

#define addqs_ld(XG, MS, DS)                                                \
        addqs3ld(W(XG), W(XG), W(MS), W(DS))

#define addqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x58)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

        /* adp, adh are defined in rtbase.h (first 15-regs only)
         * under "COMMON SIMD INSTRUCTIONS" section */

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subqs_rr(XG, XS)                                                    \
        subqs3rr(W(XG), W(XG), W(XS))

#define subqs_ld(XG, MS, DS)                                                \
        subqs3ld(W(XG), W(XG), W(MS), W(DS))

#define subqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define subqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x5C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulqs_rr(XG, XS)                                                    \
        mulqs3rr(W(XG), W(XG), W(XS))

#define mulqs_ld(XG, MS, DS)                                                \
        mulqs3ld(W(XG), W(XG), W(MS), W(DS))

#define mulqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define mulqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x59)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

        /* mlp, mlh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* div (G = G / S), (D = S / T) if (#D != #T) and on ARMv7 if (#D != #S) */

#define divqs_rr(XG, XS)                                                    \
        divqs3rr(W(XG), W(XG), W(XS))

#define divqs_ld(XG, MS, DS)                                                \
        divqs3ld(W(XG), W(XG), W(MS), W(DS))

#define divqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define divqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x5E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sqr (D = sqrt S) */

#define sqrqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define sqrqs_ld(XD, MS, DS)                                                \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x51)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if   RT_SIMD_COMPAT_RCP == 0

#define rceqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 2) EMITB(0xCA)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 2) EMITB(0xCA)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 2) EMITB(0xCA)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 2) EMITB(0xCA)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rcsqs_rr(XG, XS) /* destroys XS */

#elif RT_SIMD_COMPAT_RCP == 2

#define rceqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 2) EMITB(0x4C)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 2) EMITB(0x4C)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 2) EMITB(0x4C)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 2) EMITB(0x4C)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rcsqs_rr(XG, XS) /* destroys XS */                                  \
        mulqs_rr(W(XS), W(XG))                                              \
        mulqs_rr(W(XS), W(XG))                                              \
        addqs_rr(W(XG), W(XG))                                              \
        subqs_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RCP */

        /* rce, rcs, rcp are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if   RT_SIMD_COMPAT_RSQ == 0

#define rseqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 2) EMITB(0xCC)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 2) EMITB(0xCC)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 2) EMITB(0xCC)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 2) EMITB(0xCC)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rssqs_rr(XG, XS) /* destroys XS */

#elif RT_SIMD_COMPAT_RSQ == 2

#define rseqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 2) EMITB(0x4E)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 2) EMITB(0x4E)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 2) EMITB(0x4E)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 2) EMITB(0x4E)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rssqs_rr(XG, XS) /* destroys XS */                                  \
        mulqs_rr(W(XS), W(XG))                                              \
        mulqs_rr(W(XS), W(XG))                                              \
        subqs_ld(W(XS), Mebp, inf_GPC03_64)                                 \
        mulqs_ld(W(XS), Mebp, inf_GPC02_64)                                 \
        mulqs_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rse, rss, rsq are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T) if (#G != #S && #G != #T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmaqs_rr(XG, XS, XT)                                                \
    ADR EVW(0,             0, REG(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(1,             1, REH(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(2,             2, REI(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmaqs_ld(XG, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T) if (#G != #S && #G != #T)
 * NOTE: due to final negation being outside of rounding on all POWER systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsqs_rr(XG, XS, XT)                                                \
    ADR EVW(0,             0, REG(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(1,             1, REH(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(2,             2, REI(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))                                      \
    ADR EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmsqs_ld(XG, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_SIMD_COMPAT_FMS */

/*************   packed double-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T) */

#define minqs_rr(XG, XS)                                                    \
        minqs3rr(W(XG), W(XG), W(XS))

#define minqs_ld(XG, MS, DS)                                                \
        minqs3ld(W(XG), W(XG), W(MS), W(DS))

#define minqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x5D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

        /* mnp, mnh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T) */

#define maxqs_rr(XG, XS)                                                    \
        maxqs3rr(W(XG), W(XG), W(XS))

#define maxqs_ld(XG, MS, DS)                                                \
        maxqs3ld(W(XG), W(XG), W(MS), W(DS))

#define maxqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0x5F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

        /* mxp, mxh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqqs_rr(XG, XS)                                                    \
        ceqqs3rr(W(XG), W(XG), W(XS))

#define ceqqs_ld(XG, MS, DS)                                                \
        ceqqs3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define ceqqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x00))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x00))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x00))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x00))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cneqs_rr(XG, XS)                                                    \
        cneqs3rr(W(XG), W(XG), W(XS))

#define cneqs_ld(XG, MS, DS)                                                \
        cneqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cneqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cneqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x04))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x04))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x04))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x04))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T) */

#define cltqs_rr(XG, XS)                                                    \
        cltqs3rr(W(XG), W(XG), W(XS))

#define cltqs_ld(XG, MS, DS)                                                \
        cltqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cltqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T) */

#define cleqs_rr(XG, XS)                                                    \
        cleqs3rr(W(XG), W(XG), W(XS))

#define cleqs_ld(XG, MS, DS)                                                \
        cleqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cleqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T) */

#define cgtqs_rr(XG, XS)                                                    \
        cgtqs3rr(W(XG), W(XG), W(XS))

#define cgtqs_ld(XG, MS, DS)                                                \
        cgtqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgtqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T) */

#define cgeqs_rr(XG, XS)                                                    \
        cgeqs3rr(W(XG), W(XG), W(XS))

#define cgeqs_ld(XG, MS, DS)                                                \
        cgeqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqs3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgeqs3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xC2)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE64_2K8    0x0000   /* none satisfy the condition */
#define RT_SIMD_MASK_FULL64_2K8    0xFFFF   /*  all satisfy the condition */

/* #define mk1wx_rx(RD)                    (defined in 32_2K8-bit header) */
/* #define ck1ox_rm(XS, MT, DT)            (defined in 32_2K8-bit header) */

#define mkjqx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        ck1ox_rm(W(XS), Mebp, inf_GPC07)                                    \
        mk1wx_rx(Reax)                                                      \
        REX(1,             0) EMITB(0x8B)                                   \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1ox_rm(V(XS), Mebp, inf_GPC07)                                    \
        mk1wx_rx(Reax)                                                      \
        REX(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##64_2K8 >> 15) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1ox_rm(X(XS), Mebp, inf_GPC07)                                    \
        mk1wx_rx(Reax)                                                      \
        REX(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##64_2K8 >> 15) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1ox_rm(Z(XS), Mebp, inf_GPC07)                                    \
        mk1wx_rx(Reax)                                                      \
        REX(0,             1)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##64_2K8 >> 15) << 1)))  \
        MRM(0x00,       0x03, 0x07)                                         \
        cmpwx_ri(Reax, IH(RT_SIMD_MASK_##mask##64_2K8))                     \
        jeqxx_lb(lb)

/*************   packed double-precision floating-point convert   *************/

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnzqs_rr(XD, XS)     /* round towards zero */                       \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))                                  \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))                                  \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))                                  \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))

#define rnzqs_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMITB(0x03))                           \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMITB(0x03))                           \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMITB(0x03))                           \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMITB(0x03))

#if (RT_512X4 < 2)

#define cvzqs_rr(XD, XS)     /* round towards zero */                       \
        movqx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        fpuzs_ld(Mebp,  inf_SCR01(0x00))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x00))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x08))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x08))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x10))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x10))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x18))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x18))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x20))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x20))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x28))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x28))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x30))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x30))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x38))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x38))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x40))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x40))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x48))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x48))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x50))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x50))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x58))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x58))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x60))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x60))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x68))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x68))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x70))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x70))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x78))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x78))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x80))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x80))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x88))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x88))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x90))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x90))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x98))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0x98))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xF8))                                    \
        fpuzt_st(Mebp,  inf_SCR01(0xF8))                                    \
        movqx_ld(W(XD), Mebp, inf_SCR01(0))

#define cvzqs_ld(XD, MS, DS) /* round towards zero */                       \
        movqx_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvzqs_rr(XD, XS)     /* round towards zero */                       \
        EVW(0,             0,    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvzqs_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7A)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnpqs_rr(XD, XS)     /* round towards +inf */                       \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))

#define rnpqs_ld(XD, MS, DS) /* round towards +inf */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMITB(0x02))                           \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMITB(0x02))                           \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMITB(0x02))                           \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMITB(0x02))

#if (RT_512X4 < 2)

#define cvpqs_rr(XD, XS)     /* round towards +inf */                       \
        rnpqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvpqs_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvpqs_rr(XD, XS)     /* round towards +inf */                       \
        ERW(0,             0,    0x00, 2, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(1,             1,    0x00, 2, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(2,             2,    0x00, 2, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(3,             3,    0x00, 2, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvpqs_ld(XD, MS, DS) /* round towards +inf */                       \
        movqs_ld(W(XD), W(MS), W(DS))                                       \
        cvpqs_rr(W(XD), W(XD))

#endif /* RT_512X4 >= 2 */

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnmqs_rr(XD, XS)     /* round towards -inf */                       \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))

#define rnmqs_ld(XD, MS, DS) /* round towards -inf */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMITB(0x01))                           \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMITB(0x01))                           \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMITB(0x01))                           \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMITB(0x01))

#if (RT_512X4 < 2)

#define cvmqs_rr(XD, XS)     /* round towards -inf */                       \
        rnmqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvmqs_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvmqs_rr(XD, XS)     /* round towards -inf */                       \
        ERW(0,             0,    0x00, 1, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(1,             1,    0x00, 1, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(2,             2,    0x00, 1, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(3,             3,    0x00, 1, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvmqs_ld(XD, MS, DS) /* round towards -inf */                       \
        movqs_ld(W(XD), W(MS), W(DS))                                       \
        cvmqs_rr(W(XD), W(XD))

#endif /* RT_512X4 >= 2 */

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnnqs_rr(XD, XS)     /* round towards near */                       \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define rnnqs_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMITB(0x00))                           \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMITB(0x00))                           \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMITB(0x00))                           \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMITB(0x00))

#if (RT_512X4 < 2)

#define cvnqs_rr(XD, XS)     /* round towards near */                       \
        movqx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        fpuzs_ld(Mebp,  inf_SCR01(0x00))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x00))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x08))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x08))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x10))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x10))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x18))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x18))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x20))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x20))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x28))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x28))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x30))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x30))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x38))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x38))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x40))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x40))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x48))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x48))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x50))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x50))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x58))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x58))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x60))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x60))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x68))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x68))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x70))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x70))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x78))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x78))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x80))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x80))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x88))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x88))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x90))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x90))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0x98))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0x98))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzs_ld(Mebp,  inf_SCR01(0xF8))                                    \
        fpuzn_st(Mebp,  inf_SCR01(0xF8))                                    \
        movqx_ld(W(XD), Mebp, inf_SCR01(0))

#define cvnqs_ld(XD, MS, DS) /* round towards near */                       \
        movqx_ld(W(XD), W(MS), W(DS))                                       \
        cvnqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvnqs_rr(XD, XS)     /* round towards near */                       \
        EVW(0,             0,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvnqs_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#if (RT_512X4 < 2)

#define cvnqn_rr(XD, XS)     /* round towards near */                       \
        movqx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        fpuzn_ld(Mebp,  inf_SCR01(0x00))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x00))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x08))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x08))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x10))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x10))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x18))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x18))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x20))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x20))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x28))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x28))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x30))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x30))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x38))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x38))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x40))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x40))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x48))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x48))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x50))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x50))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x58))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x58))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x60))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x60))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x68))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x68))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x70))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x70))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x78))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x78))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x80))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x80))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x88))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x88))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x90))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x90))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0x98))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0x98))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xA0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xA8))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xB0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xB8))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xC0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xC8))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xD0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xD8))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xE0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xE8))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xF0))                                    \
        fpuzn_ld(Mebp,  inf_SCR01(0xF8))                                    \
        fpuzs_st(Mebp,  inf_SCR01(0xF8))                                    \
        movqx_ld(W(XD), Mebp, inf_SCR01(0))

#define cvnqn_ld(XD, MS, DS) /* round towards near */                       \
        movqx_ld(W(XD), W(MS), W(DS))                                       \
        cvnqn_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvnqn_rr(XD, XS)     /* round towards near */                       \
        EVW(0,             0,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvnqn_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVW(0,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVW(1,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVW(2,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVW(3,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX POWER systems, use cvz
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rndqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))

#define rndqs_ld(XD, MS, DS)                                                \
    ADR EVW(0,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMITB(0x04))                           \
    ADR EVW(1,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMITB(0x04))                           \
    ADR EVW(2,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMITB(0x04))                           \
    ADR EVW(3,       RXB(MS),    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMITB(0x04))

#if (RT_512X4 < 2)

#define cvtqs_rr(XD, XS)                                                    \
        rndqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvtqs_ld(XD, MS, DS)                                                \
        rndqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvtqs_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtqs_ld(XD, MS, DS)                                                \
        EVW(0,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
        EVW(1,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
        EVW(2,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
        EVW(3,       RXB(MS),    0x00, K, 1, 1) EMITB(0x7B)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX POWER systems */

#if (RT_512X4 < 2)

#define cvtqn_rr(XD, XS)                                                    \
        fpucw_st(Mebp,  inf_SCR02(4))                                       \
        mxcsr_st(Mebp,  inf_SCR02(0))                                       \
        shrwx_mi(Mebp,  inf_SCR02(0), IB(3))                                \
        andwx_mi(Mebp,  inf_SCR02(0), IH(0x0C00))                           \
        orrwx_mi(Mebp,  inf_SCR02(0), IB(0x7F))                             \
        fpucw_ld(Mebp,  inf_SCR02(0))                                       \
        cvnqn_rr(W(XD), W(XS))                                              \
        fpucw_ld(Mebp,  inf_SCR02(4))

#define cvtqn_ld(XD, MS, DS)                                                \
        movqx_ld(W(XD), W(MS), W(DS))                                       \
        cvtqn_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvtqn_rr(XD, XS)                                                    \
        EVW(0,             0,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(1,             1,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(2,             2,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVW(3,             3,    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtqn_ld(XD, MS, DS)                                                \
        EVW(0,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
        EVW(1,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
        EVW(2,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
        EVW(3,       RXB(MS),    0x00, K, 2, 1) EMITB(0xE6)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnrqs_rr(XD, XS, mode)                                              \
        EVW(0,             0,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))                 \
        EVW(1,             1,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))                 \
        EVW(2,             2,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))                 \
        EVW(3,             3,    0x00, K, 1, 3) EMITB(0x09)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))

#if (RT_512X4 < 2)

#define cvrqs_rr(XD, XS, mode)                                              \
        rnrqs_rr(W(XD), W(XS), mode)                                        \
        cvzqs_rr(W(XD), W(XD))

#else /* RT_512X4 >= 2 */

#define cvrqs_rr(XD, XS, mode)                                              \
        ERW(0,             0, 0x00, RT_SIMD_MODE_##mode&3, 1, 1) EMITB(0x7B)\
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(1,             1, 0x00, RT_SIMD_MODE_##mode&3, 1, 1) EMITB(0x7B)\
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(2,             2, 0x00, RT_SIMD_MODE_##mode&3, 1, 1) EMITB(0x7B)\
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        ERW(3,             3, 0x00, RT_SIMD_MODE_##mode&3, 1, 1) EMITB(0x7B)\
        MRM(REG(XD), MOD(XS), REG(XS))

#endif /* RT_512X4 >= 2 */

/************   packed double-precision integer arithmetic/shifts   ***********/

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addqx_rr(XG, XS)                                                    \
        addqx3rr(W(XG), W(XG), W(XS))

#define addqx_ld(XG, MS, DS)                                                \
        addqx3ld(W(XG), W(XG), W(MS), W(DS))

#define addqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD4)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subqx_rr(XG, XS)                                                    \
        subqx3rr(W(XG), W(XG), W(XS))

#define subqx_ld(XG, MS, DS)                                                \
        subqx3ld(W(XG), W(XG), W(MS), W(DS))

#define subqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define subqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xFB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#if (RT_512X4 < 2)

#define mulqx_rr(XG, XS)                                                    \
        mulqx3rr(W(XG), W(XG), W(XS))

#define mulqx_ld(XG, MS, DS)                                                \
        mulqx3ld(W(XG), W(XG), W(MS), W(DS))

#define mulqx3rr(XD, XS, XT)                                                \
        movqx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movqx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x00))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x08))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x10))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x18))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x18))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x20))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x20))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x28))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x28))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x30))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x30))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x38))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x38))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x40))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x40))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x48))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x48))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x50))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x50))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x58))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x58))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x60))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x60))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x68))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x68))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x70))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x70))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x78))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x78))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x80))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x80))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x88))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x88))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x90))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x90))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x98))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x98))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xA0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xA0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xA8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xA8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xB0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xB0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xB8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xB8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xC0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xC0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xC8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xC8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xD0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xD0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xD8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xD8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xE0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xE0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xE8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xE8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xF0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xF0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xF8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xF8))                              \
        stack_ld(Recx)                                                      \
        movqx_ld(W(XD), Mebp, inf_SCR01(0))

#define mulqx3ld(XD, XS, MT, DT)                                            \
        movqx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movqx_ld(W(XD), W(MT), W(DT))                                       \
        movqx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x00))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x08))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x10))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x18))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x18))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x20))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x20))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x28))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x28))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x30))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x30))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x38))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x38))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x40))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x40))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x48))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x48))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x50))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x50))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x58))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x58))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x60))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x60))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x68))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x68))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x70))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x70))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x78))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x78))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x80))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x80))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x88))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x88))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x90))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x90))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0x98))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0x98))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xA0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xA0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xA8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xA8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xB0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xB0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xB8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xB8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xC0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xC0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xC8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xC8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xD0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xD0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xD8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xD8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xE0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xE0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xE8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xE8))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xF0))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xF0))                              \
        movzx_ld(Recx,  Mebp, inf_SCR01(0xF8))                              \
        mulzx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        movzx_st(Recx,  Mebp, inf_SCR01(0xF8))                              \
        stack_ld(Recx)                                                      \
        movqx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_512X4 >= 2 */

#define mulqx_rr(XG, XS)                                                    \
        mulqx3rr(W(XG), W(XG), W(XS))

#define mulqx_ld(XG, MS, DS)                                                \
        mulqx3ld(W(XG), W(XG), W(MS), W(DS))

#define mulqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define mulqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x40)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlqx_ri(XG, IS)                                                    \
        shlqx3ri(W(XG), W(XG), W(IS))

#define shlqx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shlqx3ld(W(XG), W(XG), W(MS), W(DS))

#define shlqx3ri(XD, XS, IT)                                                \
        EVW(0,             0, REG(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             1, REH(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             2, REI(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             3, REJ(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shlqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xF3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xF3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xF3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xF3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrqx_ri(XG, IS)                                                    \
        shrqx3ri(W(XG), W(XG), W(IS))

#define shrqx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrqx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrqx3ri(XD, XS, IT)                                                \
        EVW(0,             0, REG(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             1, REH(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             2, REI(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             3, REJ(XD), K, 1, 1) EMITB(0x73)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD3)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrqn_ri(XG, IS)                                                    \
        shrqn3ri(W(XG), W(XG), W(IS))

#define shrqn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrqn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrqn3ri(XD, XS, IT)                                                \
        EVW(0,             0, REG(XD), K, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             1, REH(XD), K, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             2, REI(XD), K, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVW(0,             3, REJ(XD), K, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlqx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svlqx3rr(W(XG), W(XG), W(XS))

#define svlqx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svlqx3ld(W(XG), W(XG), W(MS), W(DS))

#define svlqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svlqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrqx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrqx3rr(W(XG), W(XG), W(XS))

#define svrqx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrqx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrqn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrqn3rr(W(XG), W(XG), W(XS))

#define svrqn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrqn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/****************   packed double-precision integer compare   *****************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minqx_rr(XG, XS)                                                    \
        minqx3rr(W(XG), W(XG), W(XS))

#define minqx_ld(XG, MS, DS)                                                \
        minqx3ld(W(XG), W(XG), W(MS), W(DS))

#define minqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3B)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minqn_rr(XG, XS)                                                    \
        minqn3rr(W(XG), W(XG), W(XS))

#define minqn_ld(XG, MS, DS)                                                \
        minqn3ld(W(XG), W(XG), W(MS), W(DS))

#define minqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x39)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxqx_rr(XG, XS)                                                    \
        maxqx3rr(W(XG), W(XG), W(XS))

#define maxqx_ld(XG, MS, DS)                                                \
        maxqx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3F)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxqn_rr(XG, XS)                                                    \
        maxqn3rr(W(XG), W(XG), W(XS))

#define maxqn_ld(XG, MS, DS)                                                \
        maxqn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3D)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqqx_rr(XG, XS)                                                    \
        ceqqx3rr(W(XG), W(XG), W(XS))

#define ceqqx_ld(XG, MS, DS)                                                \
        ceqqx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define ceqqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x00))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x00))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x00))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x00))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cneqx_rr(XG, XS)                                                    \
        cneqx3rr(W(XG), W(XG), W(XS))

#define cneqx_ld(XG, MS, DS)                                                \
        cneqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cneqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cneqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x04))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x04))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x04))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x04))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltqx_rr(XG, XS)                                                    \
        cltqx3rr(W(XG), W(XG), W(XS))

#define cltqx_ld(XG, MS, DS)                                                \
        cltqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cltqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltqn_rr(XG, XS)                                                    \
        cltqn3rr(W(XG), W(XG), W(XS))

#define cltqn_ld(XG, MS, DS)                                                \
        cltqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cltqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define cleqx_rr(XG, XS)                                                    \
        cleqx3rr(W(XG), W(XG), W(XS))

#define cleqx_ld(XG, MS, DS)                                                \
        cleqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cleqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define cleqn_rr(XG, XS)                                                    \
        cleqn3rr(W(XG), W(XG), W(XS))

#define cleqn_ld(XG, MS, DS)                                                \
        cleqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cleqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtqx_rr(XG, XS)                                                    \
        cgtqx3rr(W(XG), W(XG), W(XS))

#define cgtqx_ld(XG, MS, DS)                                                \
        cgtqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgtqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtqn_rr(XG, XS)                                                    \
        cgtqn3rr(W(XG), W(XG), W(XS))

#define cgtqn_ld(XG, MS, DS)                                                \
        cgtqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgtqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgeqx_rr(XG, XS)                                                    \
        cgeqx3rr(W(XG), W(XG), W(XS))

#define cgeqx_ld(XG, MS, DS)                                                \
        cgeqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgeqx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgeqn_rr(XG, XS)                                                    \
        cgeqn3rr(W(XG), W(XG), W(XS))

#define cgeqn_ld(XG, MS, DS)                                                \
        cgeqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

#define cgeqn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1qx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1qx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1qx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x1F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1qx_ld(Z(XD), Mebp, inf_GPC07)

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_512X4 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_X64_512X4V2_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
