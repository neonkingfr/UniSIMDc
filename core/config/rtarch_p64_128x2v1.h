/******************************************************************************/
/* Copyright (c) 2013-2023 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_P64_128X2V1_H
#define RT_RTARCH_P64_128X2V1_H

#include "rtarch_p32_128x2v1.h"
#include "rtarch_pHB_128x2v1.h"

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_p64_128x2v1.h: Implementation of POWER fp64 VSX1/2 instruction pairs.
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

#if (RT_128X2 == 1 || RT_128X2 == 16) && (RT_SIMD_COMPAT_XMM > 0)

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/* elm (D = S), store first SIMD element with natural alignment
 * allows to decouple scalar subset from SIMD where appropriate */

#define elmdx_st(XS, MD, DD) /* 1st elem as in mem with SIMD load/store */  \
        elmjx_st(W(XS), W(MD), W(DD))

/***************   packed double-precision generic move/logic   ***************/

/* mov (D = S) */

#define movdx_rr(XD, XS)                                                    \
        EMITW(0xF0000497 | MXM(REG(XD), REG(XS), REG(XS)))                  \
        EMITW(0xF0000497 | MXM(RYG(XD), RYG(XS), RYG(XS)))

#define movdx_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(REG(XD), T0xx,    TPxx))                     \
        EMITW(0x7C000699 | MXM(RYG(XD), T1xx,    TPxx))

#define movdx_st(XS, MD, DD)                                                \
        AUW(SIB(MD),  EMPTY,  EMPTY,    MOD(MD), VAL(DD), C2(DD), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MD), VAL(DD), B2(DD), P2(DD)))  \
        EMITW(0x7C000799 | MXM(REG(XS), T0xx,    TPxx))                     \
        EMITW(0x7C000799 | MXM(RYG(XS), T1xx,    TPxx))

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#define mmvdx_rr(XG, XS)                                                    \
        EMITW(0xF000003F | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0xF000043F | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define mmvdx_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000003F | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000043F | MXM(RYG(XG), RYG(XG), TmmM))

#define mmvdx_st(XS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C2(DG), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MG), VAL(DG), B2(DG), P2(DG)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000003F | MXM(TmmM,    TmmM,    REG(XS)))                  \
        EMITW(0x7C000799 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000043F | MXM(TmmM,    TmmM,    RYG(XS)))                  \
        EMITW(0x7C000799 | MXM(TmmM,    T1xx,    TPxx))

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define anddx_rr(XG, XS)                                                    \
        anddx3rr(W(XG), W(XG), W(XS))

#define anddx_ld(XG, MS, DS)                                                \
        anddx3ld(W(XG), W(XG), W(MS), W(DS))

#define anddx3rr(XD, XS, XT)                                                \
        EMITW(0xF0000417 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000417 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define anddx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000417 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000417 | MXM(RYG(XD), RYG(XS), TmmM))

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define anndx_rr(XG, XS)                                                    \
        anndx3rr(W(XG), W(XG), W(XS))

#define anndx_ld(XG, MS, DS)                                                \
        anndx3ld(W(XG), W(XG), W(MS), W(DS))

#define anndx3rr(XD, XS, XT)                                                \
        EMITW(0xF0000457 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0xF0000457 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define anndx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000457 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000457 | MXM(RYG(XD), TmmM,    RYG(XS)))

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrdx_rr(XG, XS)                                                    \
        orrdx3rr(W(XG), W(XG), W(XS))

#define orrdx_ld(XG, MS, DS)                                                \
        orrdx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrdx3rr(XD, XS, XT)                                                \
        EMITW(0xF0000497 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000497 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define orrdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000497 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000497 | MXM(RYG(XD), RYG(XS), TmmM))

/* orn (G = ~G | S), (D = ~S | T) if (#D != #T) */

#if (RT_SIMD_COMPAT_PW8 == 0)

#define orndx_rr(XG, XS)                                                    \
        notdx_rx(W(XG))                                                     \
        orrdx_rr(W(XG), W(XS))

#define orndx_ld(XG, MS, DS)                                                \
        notdx_rx(W(XG))                                                     \
        orrdx_ld(W(XG), W(MS), W(DS))

#define orndx3rr(XD, XS, XT)                                                \
        notdx_rr(W(XD), W(XS))                                              \
        orrdx_rr(W(XD), W(XT))

#define orndx3ld(XD, XS, MT, DT)                                            \
        notdx_rr(W(XD), W(XS))                                              \
        orrdx_ld(W(XD), W(MT), W(DT))

#else /* RT_SIMD_COMPAT_PW8 == 1 */

#define orndx_rr(XG, XS)                                                    \
        orndx3rr(W(XG), W(XG), W(XS))

#define orndx_ld(XG, MS, DS)                                                \
        orndx3ld(W(XG), W(XG), W(MS), W(DS))

#define orndx3rr(XD, XS, XT)                                                \
        EMITW(0xF0000557 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0xF0000557 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define orndx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000557 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000557 | MXM(RYG(XD), TmmM,    RYG(XS)))

#endif /* RT_SIMD_COMPAT_PW8 == 1 */

/* xor (G = G ^ S), (D = S ^ T) if (#D != #T) */

#define xordx_rr(XG, XS)                                                    \
        xordx3rr(W(XG), W(XG), W(XS))

#define xordx_ld(XG, MS, DS)                                                \
        xordx3ld(W(XG), W(XG), W(MS), W(DS))

#define xordx3rr(XD, XS, XT)                                                \
        EMITW(0xF00004D7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF00004D7 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define xordx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00004D7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00004D7 | MXM(RYG(XD), RYG(XS), TmmM))

/* not (G = ~G), (D = ~S) */

#define notdx_rx(XG)                                                        \
        notdx_rr(W(XG), W(XG))

#define notdx_rr(XD, XS)                                                    \
        EMITW(0xF0000517 | MXM(REG(XD), REG(XS), REG(XS)))                  \
        EMITW(0xF0000517 | MXM(RYG(XD), RYG(XS), RYG(XS)))

/************   packed double-precision floating-point arithmetic   ***********/

/* neg (G = -G), (D = -S) */

#define negds_rx(XG)                                                        \
        negds_rr(W(XG), W(XG))

#define negds_rr(XD, XS)                                                    \
        EMITW(0xF00007E7 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00007E7 | MXM(RYG(XD), 0x00,    RYG(XS)))

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addds_rr(XG, XS)                                                    \
        addds3rr(W(XG), W(XG), W(XS))

#define addds_ld(XG, MS, DS)                                                \
        addds3ld(W(XG), W(XG), W(MS), W(DS))

#define addds3rr(XD, XS, XT)                                                \
        EMITW(0xF0000307 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000307 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define addds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000307 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000307 | MXM(RYG(XD), RYG(XS), TmmM))

        /* adp, adh are defined in rtbase.h (first 15-regs only)
         * under "COMMON SIMD INSTRUCTIONS" section */

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subds_rr(XG, XS)                                                    \
        subds3rr(W(XG), W(XG), W(XS))

#define subds_ld(XG, MS, DS)                                                \
        subds3ld(W(XG), W(XG), W(MS), W(DS))

#define subds3rr(XD, XS, XT)                                                \
        EMITW(0xF0000347 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000347 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define subds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000347 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000347 | MXM(RYG(XD), RYG(XS), TmmM))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulds_rr(XG, XS)                                                    \
        mulds3rr(W(XG), W(XG), W(XS))

#define mulds_ld(XG, MS, DS)                                                \
        mulds3ld(W(XG), W(XG), W(MS), W(DS))

#define mulds3rr(XD, XS, XT)                                                \
        EMITW(0xF0000387 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000387 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define mulds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000387 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000387 | MXM(RYG(XD), RYG(XS), TmmM))

        /* mlp, mlh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* div (G = G / S), (D = S / T) if (#D != #T) and on ARMv7 if (#D != #S) */

#define divds_rr(XG, XS)                                                    \
        divds3rr(W(XG), W(XG), W(XS))

#define divds_ld(XG, MS, DS)                                                \
        divds3ld(W(XG), W(XG), W(MS), W(DS))

#define divds3rr(XD, XS, XT)                                                \
        EMITW(0xF00003C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF00003C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define divds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00003C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00003C7 | MXM(RYG(XD), RYG(XS), TmmM))

/* sqr (D = sqrt S) */

#define sqrds_rr(XD, XS)                                                    \
        EMITW(0xF000032F | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF000032F | MXM(RYG(XD), 0x00,    RYG(XS)))

#define sqrds_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000032F | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000032F | MXM(RYG(XD), 0x00,    TmmM))

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rceds_rr(XD, XS)                                                    \
        movdx_st(W(XS), Mebp, inf_SCR02(0))                                 \
        movdx_ld(W(XD), Mebp, inf_GPC01_64)                                 \
        divds_ld(W(XD), Mebp, inf_SCR02(0))

#define rcsds_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RCP */

        /* rce, rcs, rcp are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rseds_rr(XD, XS)                                                    \
        sqrds_rr(W(XD), W(XS))                                              \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        movdx_ld(W(XD), Mebp, inf_GPC01_64)                                 \
        divds_ld(W(XD), Mebp, inf_SCR02(0))

#define rssds_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rse, rss, rsq are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T) if (#G != #S && #G != #T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmads_rr(XG, XS, XT)                                                \
        EMITW(0xF000030F | MXM(REG(XG), REG(XS), REG(XT)))                  \
        EMITW(0xF000030F | MXM(RYG(XG), RYG(XS), RYG(XT)))

#define fmads_ld(XG, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000030F | MXM(REG(XG), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000030F | MXM(RYG(XG), RYG(XS), TmmM))

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T) if (#G != #S && #G != #T)
 * NOTE: due to final negation being outside of rounding on all POWER systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsds_rr(XG, XS, XT)                                                \
        EMITW(0xF000078F | MXM(REG(XG), REG(XS), REG(XT)))                  \
        EMITW(0xF000078F | MXM(RYG(XG), RYG(XS), RYG(XT)))

#define fmsds_ld(XG, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000078F | MXM(REG(XG), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000078F | MXM(RYG(XG), RYG(XS), TmmM))

#endif /* RT_SIMD_COMPAT_FMS */

/*************   packed double-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T) */

#define minds_rr(XG, XS)                                                    \
        minds3rr(W(XG), W(XG), W(XS))

#define minds_ld(XG, MS, DS)                                                \
        minds3ld(W(XG), W(XG), W(MS), W(DS))

#define minds3rr(XD, XS, XT)                                                \
        EMITW(0xF0000747 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000747 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define minds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000747 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000747 | MXM(RYG(XD), RYG(XS), TmmM))

        /* mnp, mnh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T) */

#define maxds_rr(XG, XS)                                                    \
        maxds3rr(W(XG), W(XG), W(XS))

#define maxds_ld(XG, MS, DS)                                                \
        maxds3ld(W(XG), W(XG), W(MS), W(DS))

#define maxds3rr(XD, XS, XT)                                                \
        EMITW(0xF0000707 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000707 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define maxds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000707 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000707 | MXM(RYG(XD), RYG(XS), TmmM))

        /* mxp, mxh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqds_rr(XG, XS)                                                    \
        ceqds3rr(W(XG), W(XG), W(XS))

#define ceqds_ld(XG, MS, DS)                                                \
        ceqds3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqds3rr(XD, XS, XT)                                                \
        EMITW(0xF000031F | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF000031F | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define ceqds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000031F | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000031F | MXM(RYG(XD), RYG(XS), TmmM))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cneds_rr(XG, XS)                                                    \
        cneds3rr(W(XG), W(XG), W(XS))

#define cneds_ld(XG, MS, DS)                                                \
        cneds3ld(W(XG), W(XG), W(MS), W(DS))

#define cneds3rr(XD, XS, XT)                                                \
        EMITW(0xF000031F | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF0000517 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0xF000031F | MXM(RYG(XD), RYG(XS), RYG(XT)))                  \
        EMITW(0xF0000517 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cneds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000031F | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0xF0000517 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000031F | MXM(RYG(XD), RYG(XS), TmmM))                     \
        EMITW(0xF0000517 | MXM(RYG(XD), RYG(XD), RYG(XD)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T) */

#define cltds_rr(XG, XS)                                                    \
        cltds3rr(W(XG), W(XG), W(XS))

#define cltds_ld(XG, MS, DS)                                                \
        cltds3ld(W(XG), W(XG), W(MS), W(DS))

#define cltds3rr(XD, XS, XT)                                                \
        EMITW(0xF000035F | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0xF000035F | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define cltds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000035F | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000035F | MXM(RYG(XD), TmmM,    RYG(XS)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T) */

#define cleds_rr(XG, XS)                                                    \
        cleds3rr(W(XG), W(XG), W(XS))

#define cleds_ld(XG, MS, DS)                                                \
        cleds3ld(W(XG), W(XG), W(MS), W(DS))

#define cleds3rr(XD, XS, XT)                                                \
        EMITW(0xF000039F | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0xF000039F | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define cleds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000039F | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000039F | MXM(RYG(XD), TmmM,    RYG(XS)))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T) */

#define cgtds_rr(XG, XS)                                                    \
        cgtds3rr(W(XG), W(XG), W(XS))

#define cgtds_ld(XG, MS, DS)                                                \
        cgtds3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtds3rr(XD, XS, XT)                                                \
        EMITW(0xF000035F | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF000035F | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgtds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000035F | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000035F | MXM(RYG(XD), RYG(XS), TmmM))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T) */

#define cgeds_rr(XG, XS)                                                    \
        cgeds3rr(W(XG), W(XG), W(XS))

#define cgeds_ld(XG, MS, DS)                                                \
        cgeds3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeds3rr(XD, XS, XT)                                                \
        EMITW(0xF000039F | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0xF000039F | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgeds3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF000039F | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF000039F | MXM(RYG(XD), RYG(XS), TmmM))

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE64_256  MN64_256   /* none satisfy the condition */
#define RT_SIMD_MASK_FULL64_256  MF64_256   /*  all satisfy the condition */

/* #define S0(mask)    S1(mask)            (defined in 32_128-bit header) */
/* #define S1(mask)    S##mask             (defined in 32_128-bit header) */

#define SMN64_256(xs, lb) /* not portable, do not use outside */            \
        EMITW(0xF0000497 | MXM(TmmM,    xs,      xs+16))                    \
        EMITW(0x10000486 | MXM(TmmM,    TmmM,    TmmQ))                     \
        ASM_BEG ASM_OP2(beq, cr6, lb) ASM_END

#define SMF64_256(xs, lb) /* not portable, do not use outside */            \
        EMITW(0xF0000417 | MXM(TmmM,    xs,      xs+16))                    \
        EMITW(0x10000486 | MXM(TmmM,    TmmM,    TmmQ))                     \
        ASM_BEG ASM_OP2(blt, cr6, lb) ASM_END

#define mkjdx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        EMITW(0x1000038C | MXM(TmmQ,    0x1F,    0x00))                     \
        AUW(EMPTY, EMPTY, EMPTY, REG(XS), lb,                               \
        S0(RT_SIMD_MASK_##mask##64_256), EMPTY2)

/*************   packed double-precision floating-point convert   *************/

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnzds_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0xF0000367 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF0000367 | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnzds_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000367 | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000367 | MXM(RYG(XD), 0x00,    TmmM))

#define cvzds_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0xF0000763 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF0000763 | MXM(RYG(XD), 0x00,    RYG(XS)))

#define cvzds_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF0000763 | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF0000763 | MXM(RYG(XD), 0x00,    TmmM))

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnpds_rr(XD, XS)     /* round towards +inf */                       \
        EMITW(0xF00003A7 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00003A7 | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnpds_ld(XD, MS, DS) /* round towards +inf */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00003A7 | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00003A7 | MXM(RYG(XD), 0x00,    TmmM))

#define cvpds_rr(XD, XS)     /* round towards +inf */                       \
        rnpds_rr(W(XD), W(XS))                                              \
        cvzds_rr(W(XD), W(XD))

#define cvpds_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpds_ld(W(XD), W(MS), W(DS))                                       \
        cvzds_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnmds_rr(XD, XS)     /* round towards -inf */                       \
        EMITW(0xF00003E7 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00003E7 | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnmds_ld(XD, MS, DS) /* round towards -inf */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00003E7 | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00003E7 | MXM(RYG(XD), 0x00,    TmmM))

#define cvmds_rr(XD, XS)     /* round towards -inf */                       \
        rnmds_rr(W(XD), W(XS))                                              \
        cvzds_rr(W(XD), W(XD))

#define cvmds_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmds_ld(W(XD), W(MS), W(DS))                                       \
        cvzds_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnnds_rr(XD, XS)     /* round towards near */                       \
        EMITW(0xF00003AF | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00003AF | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnnds_ld(XD, MS, DS) /* round towards near */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00003AF | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00003AF | MXM(RYG(XD), 0x00,    TmmM))

#define cvnds_rr(XD, XS)     /* round towards near */                       \
        rnnds_rr(W(XD), W(XS))                                              \
        cvzds_rr(W(XD), W(XD))

#define cvnds_ld(XD, MS, DS) /* round towards near */                       \
        rnnds_ld(W(XD), W(MS), W(DS))                                       \
        cvzds_rr(W(XD), W(XD))

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvndn_rr(XD, XS)     /* round towards near */                       \
        cvtdn_rr(W(XD), W(XS))

#define cvndn_ld(XD, MS, DS) /* round towards near */                       \
        cvtdn_ld(W(XD), W(MS), W(DS))

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX POWER systems, use cvz
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rndds_rr(XD, XS)                                                    \
        EMITW(0xF00003AF | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00003AF | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rndds_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00003AF | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00003AF | MXM(RYG(XD), 0x00,    TmmM))

#define cvtds_rr(XD, XS)                                                    \
        rndds_rr(W(XD), W(XS))                                              \
        cvzds_rr(W(XD), W(XD))

#define cvtds_ld(XD, MS, DS)                                                \
        rndds_ld(W(XD), W(MS), W(DS))                                       \
        cvzds_rr(W(XD), W(XD))

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX POWER systems */

#define cvtdn_rr(XD, XS)                                                    \
        EMITW(0xF00007E3 | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0xF00007E3 | MXM(RYG(XD), 0x00,    RYG(XS)))

#define cvtdn_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0xF00007E3 | MXM(REG(XD), 0x00,    TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0xF00007E3 | MXM(RYG(XD), 0x00,    TmmM))

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnrds_rr(XD, XS, mode)                                              \
        FCTRL_ENTER(mode)                                                   \
        rndds_rr(W(XD), W(XS))                                              \
        FCTRL_LEAVE(mode)

#define cvrds_rr(XD, XS, mode)                                              \
        rnrds_rr(W(XD), W(XS), mode)                                        \
        cvzds_rr(W(XD), W(XD))

/************   packed double-precision integer arithmetic/shifts   ***********/

#if (RT_SIMD_COMPAT_PW8 == 0)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define adddx_rr(XG, XS)                                                    \
        adddx3rr(W(XG), W(XG), W(XS))

#define adddx_ld(XG, MS, DS)                                                \
        adddx3ld(W(XG), W(XG), W(MS), W(DS))

#define adddx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x00))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x08))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x10))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x18))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define adddx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x00))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x08))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x10))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x18))                              \
        addzx_st(Reax,  Mebp, inf_SCR01(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subdx_rr(XG, XS)                                                    \
        subdx3rr(W(XG), W(XG), W(XS))

#define subdx_ld(XG, MS, DS)                                                \
        subdx3ld(W(XG), W(XG), W(MS), W(DS))

#define subdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x00))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x08))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x10))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x18))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define subdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x00))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x08))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x10))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR02(0x18))                              \
        subzx_st(Reax,  Mebp, inf_SCR01(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define muldx_rr(XG, XS)                                                    \
        muldx3rr(W(XG), W(XG), W(XS))

#define muldx_ld(XG, MS, DS)                                                \
        muldx3ld(W(XG), W(XG), W(MS), W(DS))

#define muldx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
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
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define muldx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
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
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shldx_ri(XG, IS)                                                    \
        shldx3ri(W(XG), W(XG), W(IS))

#define shldx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shldx3ld(W(XG), W(XG), W(MS), W(DS))

#define shldx3ri(XD, XS, IT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shlzx_mi(Mebp,  inf_SCR01(0x00), W(IT))                             \
        shlzx_mi(Mebp,  inf_SCR01(0x08), W(IT))                             \
        shlzx_mi(Mebp,  inf_SCR01(0x10), W(IT))                             \
        shlzx_mi(Mebp,  inf_SCR01(0x18), W(IT))                             \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define shldx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  W(MT), W(DT))                                       \
        shlzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        shlzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        shlzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        shlzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrdx_ri(XG, IS)                                                    \
        shrdx3ri(W(XG), W(XG), W(IS))

#define shrdx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrdx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrdx3ri(XD, XS, IT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shrzx_mi(Mebp,  inf_SCR01(0x00), W(IT))                             \
        shrzx_mi(Mebp,  inf_SCR01(0x08), W(IT))                             \
        shrzx_mi(Mebp,  inf_SCR01(0x10), W(IT))                             \
        shrzx_mi(Mebp,  inf_SCR01(0x18), W(IT))                             \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  W(MT), W(DT))                                       \
        shrzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        shrzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        shrzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        shrzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrdn_ri(XG, IS)                                                    \
        shrdn3ri(W(XG), W(XG), W(IS))

#define shrdn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrdn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrdn3ri(XD, XS, IT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shrzn_mi(Mebp,  inf_SCR01(0x00), W(IT))                             \
        shrzn_mi(Mebp,  inf_SCR01(0x08), W(IT))                             \
        shrzn_mi(Mebp,  inf_SCR01(0x10), W(IT))                             \
        shrzn_mi(Mebp,  inf_SCR01(0x18), W(IT))                             \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrdn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  W(MT), W(DT))                                       \
        shrzn_mx(Mebp,  inf_SCR01(0x00))                                    \
        shrzn_mx(Mebp,  inf_SCR01(0x08))                                    \
        shrzn_mx(Mebp,  inf_SCR01(0x10))                                    \
        shrzn_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svldx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svldx3rr(W(XG), W(XG), W(XS))

#define svldx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svldx3ld(W(XG), W(XG), W(MS), W(DS))

#define svldx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define svldx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shlzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrdx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrdx3rr(W(XG), W(XG), W(XS))

#define svrdx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrdx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define svrdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrzx_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrdn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrdn3rr(W(XG), W(XG), W(XS))

#define svrdn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrdn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrdn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define svrdn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x10))                                    \
        movzx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrzn_mx(Mebp,  inf_SCR01(0x18))                                    \
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_SIMD_COMPAT_PW8 == 1 */

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define adddx_rr(XG, XS)                                                    \
        adddx3rr(W(XG), W(XG), W(XS))

#define adddx_ld(XG, MS, DS)                                                \
        adddx3ld(W(XG), W(XG), W(MS), W(DS))

#define adddx3rr(XD, XS, XT)                                                \
        EMITW(0x100000C0 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100000C0 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define adddx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100000C0 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100000C0 | MXM(RYG(XD), RYG(XS), TmmM))

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subdx_rr(XG, XS)                                                    \
        subdx3rr(W(XG), W(XG), W(XS))

#define subdx_ld(XG, MS, DS)                                                \
        subdx3ld(W(XG), W(XG), W(MS), W(DS))

#define subdx3rr(XD, XS, XT)                                                \
        EMITW(0x100004C0 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100004C0 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define subdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100004C0 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100004C0 | MXM(RYG(XD), RYG(XS), TmmM))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define muldx_rr(XG, XS)                                                    \
        muldx3rr(W(XG), W(XG), W(XS))

#define muldx_ld(XG, MS, DS)                                                \
        muldx3ld(W(XG), W(XG), W(MS), W(DS))

#define muldx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
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
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

#define muldx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
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
        stack_ld(Recx)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR01(0))

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shldx_ri(XG, IS)                                                    \
        shldx3ri(W(XG), W(XG), W(IS))

#define shldx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shldx3ld(W(XG), W(XG), W(MS), W(DS))

#define shldx3ri(XD, XS, IT)                                                \
        movzx_mi(Mebp, inf_SCR01(0), W(IT))                                 \
        shldx3ld(W(XD), W(XS), Mebp, inf_SCR01(0))

#define shldx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000299 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100005C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x100005C4 | MXM(RYG(XD), RYG(XS), TmmM))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrdx_ri(XG, IS)                                                    \
        shrdx3ri(W(XG), W(XG), W(IS))

#define shrdx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrdx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrdx3ri(XD, XS, IT)                                                \
        movzx_mi(Mebp, inf_SCR01(0), W(IT))                                 \
        shrdx3ld(W(XD), W(XS), Mebp, inf_SCR01(0))

#define shrdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000299 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100006C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x100006C4 | MXM(RYG(XD), RYG(XS), TmmM))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrdn_ri(XG, IS)                                                    \
        shrdn3ri(W(XG), W(XG), W(IS))

#define shrdn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrdn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrdn3ri(XD, XS, IT)                                                \
        movzx_mi(Mebp, inf_SCR01(0), W(IT))                                 \
        shrdn3ld(W(XD), W(XS), Mebp, inf_SCR01(0))

#define shrdn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000299 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x100003C4 | MXM(RYG(XD), RYG(XS), TmmM))

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svldx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svldx3rr(W(XG), W(XG), W(XS))

#define svldx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svldx3ld(W(XG), W(XG), W(MS), W(DS))

#define svldx3rr(XD, XS, XT)                                                \
        EMITW(0x100005C4 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100005C4 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define svldx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100005C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100005C4 | MXM(RYG(XD), RYG(XS), TmmM))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrdx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrdx3rr(W(XG), W(XG), W(XS))

#define svrdx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrdx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrdx3rr(XD, XS, XT)                                                \
        EMITW(0x100006C4 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100006C4 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define svrdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100006C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100006C4 | MXM(RYG(XD), RYG(XS), TmmM))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrdn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrdn3rr(W(XG), W(XG), W(XS))

#define svrdn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrdn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrdn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C4 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100003C4 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define svrdn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C4 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C4 | MXM(RYG(XD), RYG(XS), TmmM))

#endif /* RT_SIMD_COMPAT_PW8 == 1 */

/****************   packed double-precision integer compare   *****************/

#if (RT_SIMD_COMPAT_PW8 == 0)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define mindx_rr(XG, XS)                                                    \
        mindx3rr(W(XG), W(XG), W(XS))

#define mindx_ld(XG, MS, DS)                                                \
        mindx3ld(W(XG), W(XG), W(MS), W(DS))

#define mindx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        mindx_rx(W(XD))

#define mindx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        mindx_rx(W(XD))

#define mindx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define mindn_rr(XG, XS)                                                    \
        mindn3rr(W(XG), W(XG), W(XS))

#define mindn_ld(XG, MS, DS)                                                \
        mindn3ld(W(XG), W(XG), W(MS), W(DS))

#define mindn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        mindn_rx(W(XD))

#define mindn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        mindn_rx(W(XD))

#define mindn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxdx_rr(XG, XS)                                                    \
        maxdx3rr(W(XG), W(XG), W(XS))

#define maxdx_ld(XG, MS, DS)                                                \
        maxdx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxdx_rx(W(XD))

#define maxdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxdx_rx(W(XD))

#define maxdx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxdn_rr(XG, XS)                                                    \
        maxdn3rr(W(XG), W(XG), W(XS))

#define maxdn_ld(XG, MS, DS)                                                \
        maxdn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxdn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxdn_rx(W(XD))

#define maxdn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxdn_rx(W(XD))

#define maxdn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        movzx_st(Reax,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqdx_rr(XG, XS)                                                    \
        ceqdx3rr(W(XG), W(XG), W(XS))

#define ceqdx_ld(XG, MS, DS)                                                \
        ceqdx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        ceqdx_rx(W(XD))

#define ceqdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        ceqdx_rx(W(XD))

#define ceqdx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnedx_rr(XG, XS)                                                    \
        cnedx3rr(W(XG), W(XG), W(XS))

#define cnedx_ld(XG, MS, DS)                                                \
        cnedx3ld(W(XG), W(XG), W(MS), W(DS))

#define cnedx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cnedx_rx(W(XD))

#define cnedx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cnedx_rx(W(XD))

#define cnedx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40820008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltdx_rr(XG, XS)                                                    \
        cltdx3rr(W(XG), W(XG), W(XS))

#define cltdx_ld(XG, MS, DS)                                                \
        cltdx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cltdx_rx(W(XD))

#define cltdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cltdx_rx(W(XD))

#define cltdx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltdn_rr(XG, XS)                                                    \
        cltdn3rr(W(XG), W(XG), W(XS))

#define cltdn_ld(XG, MS, DS)                                                \
        cltdn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltdn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cltdn_rx(W(XD))

#define cltdn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cltdn_rx(W(XD))

#define cltdn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define cledx_rr(XG, XS)                                                    \
        cledx3rr(W(XG), W(XG), W(XS))

#define cledx_ld(XG, MS, DS)                                                \
        cledx3ld(W(XG), W(XG), W(MS), W(DS))

#define cledx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cledx_rx(W(XD))

#define cledx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cledx_rx(W(XD))

#define cledx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define cledn_rr(XG, XS)                                                    \
        cledn3rr(W(XG), W(XG), W(XS))

#define cledn_ld(XG, MS, DS)                                                \
        cledn3ld(W(XG), W(XG), W(MS), W(DS))

#define cledn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cledn_rx(W(XD))

#define cledn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cledn_rx(W(XD))

#define cledn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtdx_rr(XG, XS)                                                    \
        cgtdx3rr(W(XG), W(XG), W(XS))

#define cgtdx_ld(XG, MS, DS)                                                \
        cgtdx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtdx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgtdx_rx(W(XD))

#define cgtdx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgtdx_rx(W(XD))

#define cgtdx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtdn_rr(XG, XS)                                                    \
        cgtdn3rr(W(XG), W(XG), W(XS))

#define cgtdn_ld(XG, MS, DS)                                                \
        cgtdn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtdn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgtdn_rx(W(XD))

#define cgtdn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgtdn_rx(W(XD))

#define cgtdn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x41810008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgedx_rr(XG, XS)                                                    \
        cgedx3rr(W(XG), W(XG), W(XS))

#define cgedx_ld(XG, MS, DS)                                                \
        cgedx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgedx3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgedx_rx(W(XD))

#define cgedx3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgedx_rx(W(XD))

#define cgedx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpld, %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgedn_rr(XG, XS)                                                    \
        cgedn3rr(W(XG), W(XG), W(XS))

#define cgedn_ld(XG, MS, DS)                                                \
        cgedn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgedn3rr(XD, XS, XT)                                                \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgedn_rx(W(XD))

#define cgedn3ld(XD, XS, MT, DT)                                            \
        movdx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movdx_ld(W(XD), W(MT), W(DT))                                       \
        movdx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgedn_rx(W(XD))

#define cgedn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Reax)                                                      \
        stack_st(Recx)                                                      \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x00))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x00))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x00))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x08))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x08))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x08))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x10))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x10))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x10))                              \
        movzx_ld(Recx,  Mebp, inf_GPC07)                                    \
        movzx_ld(Reax,  Mebp, inf_SCR01(0x18))                              \
        cmpzx_rm(Reax,  Mebp, inf_SCR02(0x18))                              \
        ASM_BEG ASM_OP2(cmpd,  %%r24, %%r25) ASM_END                        \
        EMITW(0x40800008)                                                   \
        xorzx_rr(Recx,  Recx)                                               \
        movzx_st(Recx,  Mebp, inf_SCR02(0x18))                              \
        stack_ld(Recx)                                                      \
        stack_ld(Reax)                                                      \
        movdx_ld(W(XD), Mebp, inf_SCR02(0))

#else /* RT_SIMD_COMPAT_PW8 == 1 */

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define mindx_rr(XG, XS)                                                    \
        mindx3rr(W(XG), W(XG), W(XS))

#define mindx_ld(XG, MS, DS)                                                \
        mindx3ld(W(XG), W(XG), W(MS), W(DS))

#define mindx3rr(XD, XS, XT)                                                \
        EMITW(0x100002C2 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100002C2 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define mindx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100002C2 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100002C2 | MXM(RYG(XD), RYG(XS), TmmM))

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define mindn_rr(XG, XS)                                                    \
        mindn3rr(W(XG), W(XG), W(XS))

#define mindn_ld(XG, MS, DS)                                                \
        mindn3ld(W(XG), W(XG), W(MS), W(DS))

#define mindn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C2 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100003C2 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define mindn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C2 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C2 | MXM(RYG(XD), RYG(XS), TmmM))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxdx_rr(XG, XS)                                                    \
        maxdx3rr(W(XG), W(XG), W(XS))

#define maxdx_ld(XG, MS, DS)                                                \
        maxdx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxdx3rr(XD, XS, XT)                                                \
        EMITW(0x100000C2 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100000C2 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define maxdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100000C2 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100000C2 | MXM(RYG(XD), RYG(XS), TmmM))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxdn_rr(XG, XS)                                                    \
        maxdn3rr(W(XG), W(XG), W(XS))

#define maxdn_ld(XG, MS, DS)                                                \
        maxdn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxdn3rr(XD, XS, XT)                                                \
        EMITW(0x100001C2 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100001C2 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define maxdn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100001C2 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100001C2 | MXM(RYG(XD), RYG(XS), TmmM))

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqdx_rr(XG, XS)                                                    \
        ceqdx3rr(W(XG), W(XG), W(XS))

#define ceqdx_ld(XG, MS, DS)                                                \
        ceqdx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqdx3rr(XD, XS, XT)                                                \
        EMITW(0x100000C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100000C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define ceqdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100000C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100000C7 | MXM(RYG(XD), RYG(XS), TmmM))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnedx_rr(XG, XS)                                                    \
        cnedx3rr(W(XG), W(XG), W(XS))

#define cnedx_ld(XG, MS, DS)                                                \
        cnedx3ld(W(XG), W(XG), W(MS), W(DS))

#define cnedx3rr(XD, XS, XT)                                                \
        EMITW(0x100000C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100000C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cnedx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100000C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100000C7 | MXM(RYG(XD), RYG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltdx_rr(XG, XS)                                                    \
        cltdx3rr(W(XG), W(XG), W(XS))

#define cltdx_ld(XG, MS, DS)                                                \
        cltdx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltdx3rr(XD, XS, XT)                                                \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define cltdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(RYG(XD), TmmM,    RYG(XS)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltdn_rr(XG, XS)                                                    \
        cltdn3rr(W(XG), W(XG), W(XS))

#define cltdn_ld(XG, MS, DS)                                                \
        cltdn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltdn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define cltdn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(RYG(XD), TmmM,    RYG(XS)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define cledx_rr(XG, XS)                                                    \
        cledx3rr(W(XG), W(XG), W(XS))

#define cledx_ld(XG, MS, DS)                                                \
        cledx3ld(W(XG), W(XG), W(MS), W(DS))

#define cledx3rr(XD, XS, XT)                                                \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cledx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define cledn_rr(XG, XS)                                                    \
        cledn3rr(W(XG), W(XG), W(XS))

#define cledn_ld(XG, MS, DS)                                                \
        cledn3ld(W(XG), W(XG), W(MS), W(DS))

#define cledn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cledn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XS), TmmM))                     \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtdx_rr(XG, XS)                                                    \
        cgtdx3rr(W(XG), W(XG), W(XS))

#define cgtdx_ld(XG, MS, DS)                                                \
        cgtdx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtdx3rr(XD, XS, XT)                                                \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgtdx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XS), TmmM))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtdn_rr(XG, XS)                                                    \
        cgtdn3rr(W(XG), W(XG), W(XS))

#define cgtdn_ld(XG, MS, DS)                                                \
        cgtdn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtdn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgtdn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XS), TmmM))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgedx_rr(XG, XS)                                                    \
        cgedx3rr(W(XG), W(XG), W(XS))

#define cgedx_ld(XG, MS, DS)                                                \
        cgedx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgedx3rr(XD, XS, XT)                                                \
        EMITW(0x100002C7 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100002C7 | MXM(RYG(XD), RYG(XT), RYG(XS)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cgedx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100002C7 | MXM(RYG(XD), TmmM,    RYG(XS)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgedn_rr(XG, XS)                                                    \
        cgedn3rr(W(XG), W(XG), W(XS))

#define cgedn_ld(XG, MS, DS)                                                \
        cgedn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgedn3rr(XD, XS, XT)                                                \
        EMITW(0x100003C7 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100003C7 | MXM(RYG(XD), RYG(XT), RYG(XS)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cgedn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    MOD(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C000699 | MXM(TmmM,    T0xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x7C000699 | MXM(TmmM,    T1xx,    TPxx))                     \
        EMITW(0x100003C7 | MXM(RYG(XD), TmmM,    RYG(XS)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#endif /* RT_SIMD_COMPAT_PW8 == 1 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_128X2 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_P64_128X2V1_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
