/******************************************************************************/
/* Copyright (c) 2013-2023 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_A64_SVEX2V1_H
#define RT_RTARCH_A64_SVEX2V1_H

#include "rtarch_a32_SVEx2v1.h"
#include "rtarch_aHB_SVEx2v1.h"
#include "rtarch_aHF_SVEx2v1.h"

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_a64_SVEx2v1.h: Implementation of AArch64 fp64 SVE instruction pairs.
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

#if (RT_SVEX2 != 0)

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/* elm (D = S), store first SIMD element with natural alignment
 * allows to decouple scalar subset from SIMD where appropriate */

#define elmqx_st(XS, MD, DD) /* 1st elem as in mem with SIMD load/store */  \
        movts_st(W(XS), W(MD), W(DD))

/***************   packed double-precision generic move/logic   ***************/

/* mov (D = S) */

#define movqx_rr(XD, XS)                                                    \
        EMITW(0x04603000 | MXM(REG(XD), REG(XS), REG(XS)))                  \
        EMITW(0x04603000 | MXM(RYG(XD), RYG(XS), RYG(XS)))

#define movqx_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(REG(XD), MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x85804000 | MPM(RYG(XD), MOD(MS), VZL(DS), B3(DS), K1(DS)))

#define movqx_st(XS, MD, DD)                                                \
        AUW(SIB(MD),  EMPTY,  EMPTY,    MOD(MD), VAL(DD), A1(DD), EMPTY2)   \
        EMITW(0xE5804000 | MPM(REG(XS), MOD(MD), VAL(DD), B3(DD), K1(DD)))  \
        EMITW(0xE5804000 | MPM(RYG(XS), MOD(MD), VZL(DD), B3(DD), K1(DD)))

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#define mmvqx_rr(XG, XS)                                                    \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0,    TmmQ))                     \
        EMITW(0x05E0C400 | MXM(REG(XG), REG(XS), REG(XG)))                  \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0+16, TmmQ))                     \
        EMITW(0x05E0C400 | MXM(RYG(XG), RYG(XS), RYG(XG)))

#define mmvqx_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0,    TmmQ))                     \
        EMITW(0x05E0C400 | MXM(REG(XG), TmmM,    REG(XG)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0+16, TmmQ))                     \
        EMITW(0x05E0C400 | MXM(RYG(XG), TmmM,    RYG(XG)))

#define mmvqx_st(XS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), A1(DG), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MG), VAL(DG), B3(DG), K1(DG)))  \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0,    TmmQ))                     \
        EMITW(0x05E0C400 | MXM(TmmM,    REG(XS), TmmM))                     \
        EMITW(0xE5804000 | MPM(TmmM,    MOD(MG), VAL(DG), B3(DG), K1(DG)))  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MG), VZL(DG), B3(DG), K1(DG)))  \
        EMITW(0x24C0A000 | MXM(0x01,    Tmm0+16, TmmQ))                     \
        EMITW(0x05E0C400 | MXM(TmmM,    RYG(XS), TmmM))                     \
        EMITW(0xE5804000 | MPM(TmmM,    MOD(MG), VZL(DG), B3(DG), K1(DG)))

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define andqx_rr(XG, XS)                                                    \
        andqx3rr(W(XG), W(XG), W(XS))

#define andqx_ld(XG, MS, DS)                                                \
        andqx3ld(W(XG), W(XG), W(MS), W(DS))

#define andqx3rr(XD, XS, XT)                                                \
        EMITW(0x04203000 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x04203000 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define andqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04203000 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04203000 | MXM(RYG(XD), RYG(XS), TmmM))

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define annqx_rr(XG, XS)                                                    \
        annqx3rr(W(XG), W(XG), W(XS))

#define annqx_ld(XG, MS, DS)                                                \
        annqx3ld(W(XG), W(XG), W(MS), W(DS))

#define annqx3rr(XD, XS, XT)                                                \
        EMITW(0x04E03000 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x04E03000 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define annqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E03000 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E03000 | MXM(RYG(XD), TmmM,    RYG(XS)))

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrqx_rr(XG, XS)                                                    \
        orrqx3rr(W(XG), W(XG), W(XS))

#define orrqx_ld(XG, MS, DS)                                                \
        orrqx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrqx3rr(XD, XS, XT)                                                \
        EMITW(0x04603000 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x04603000 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define orrqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04603000 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04603000 | MXM(RYG(XD), RYG(XS), TmmM))

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
        EMITW(0x04A03000 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define xorqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XS), TmmM))

/* not (G = ~G), (D = ~S) */

#define notqx_rx(XG)                                                        \
        notqx_rr(W(XG), W(XG))

#define notqx_rr(XD, XS)                                                    \
        EMITW(0x04DEA000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x04DEA000 | MXM(RYG(XD), RYG(XS), 0x00))

/************   packed double-precision floating-point arithmetic   ***********/

/* neg (G = -G), (D = -S) */

#define negqs_rx(XG)                                                        \
        negqs_rr(W(XG), W(XG))

#define negqs_rr(XD, XS)                                                    \
        EMITW(0x04DDA000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x04DDA000 | MXM(RYG(XD), RYG(XS), 0x00))

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addqs_rr(XG, XS)                                                    \
        addqs3rr(W(XG), W(XG), W(XS))

#define addqs_ld(XG, MS, DS)                                                \
        addqs3ld(W(XG), W(XG), W(MS), W(DS))

#define addqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C00000 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x65C00000 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define addqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00000 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00000 | MXM(RYG(XD), RYG(XS), TmmM))

        /* adp, adh are defined in rtbase.h (first 15-regs only)
         * under "COMMON SIMD INSTRUCTIONS" section */

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subqs_rr(XG, XS)                                                    \
        subqs3rr(W(XG), W(XG), W(XS))

#define subqs_ld(XG, MS, DS)                                                \
        subqs3ld(W(XG), W(XG), W(MS), W(DS))

#define subqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C00400 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x65C00400 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define subqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00400 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00400 | MXM(RYG(XD), RYG(XS), TmmM))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulqs_rr(XG, XS)                                                    \
        mulqs3rr(W(XG), W(XG), W(XS))

#define mulqs_ld(XG, MS, DS)                                                \
        mulqs3ld(W(XG), W(XG), W(MS), W(DS))

#define mulqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C00800 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x65C00800 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define mulqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00800 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C00800 | MXM(RYG(XD), RYG(XS), TmmM))

        /* mlp, mlh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* div (G = G / S), (D = S / T) if (#D != #T) and on ARMv7 if (#D != #S) */

#define divqs_rr(XG, XS)                                                    \
        EMITW(0x65CD8000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x65CD8000 | MXM(RYG(XG), RYG(XS), 0x00))

#define divqs_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65CD8000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65CD8000 | MXM(RYG(XG), TmmM,    0x00))

#define divqs3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        divqs_rr(W(XD), W(XT))

#define divqs3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        divqs_ld(W(XD), W(MT), W(DT))

/* sqr (D = sqrt S) */

#define sqrqs_rr(XD, XS)                                                    \
        EMITW(0x65CDA000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65CDA000 | MXM(RYG(XD), RYG(XS), 0x00))

#define sqrqs_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65CDA000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65CDA000 | MXM(RYG(XD), TmmM,    0x00))

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rceqs_rr(XD, XS)                                                    \
        EMITW(0x65CE3000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65CE3000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rcsqs_rr(XG, XS) /* destroys XS */                                  \
        EMITW(0x65C01800 | MXM(REG(XS), REG(XS), REG(XG)))                  \
        EMITW(0x65C00800 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x65C01800 | MXM(RYG(XS), RYG(XS), RYG(XG)))                  \
        EMITW(0x65C00800 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#endif /* RT_SIMD_COMPAT_RCP */

        /* rce, rcs, rcp are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rseqs_rr(XD, XS)                                                    \
        EMITW(0x65CF3000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65CF3000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rssqs_rr(XG, XS) /* destroys XS */                                  \
        EMITW(0x65C00800 | MXM(REG(XS), REG(XS), REG(XG)))                  \
        EMITW(0x65C01C00 | MXM(REG(XS), REG(XS), REG(XG)))                  \
        EMITW(0x65C00800 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x65C00800 | MXM(RYG(XS), RYG(XS), RYG(XG)))                  \
        EMITW(0x65C01C00 | MXM(RYG(XS), RYG(XS), RYG(XG)))                  \
        EMITW(0x65C00800 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rse, rss, rsq are defined in rtconf.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T) if (#G != #S && #G != #T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmaqs_rr(XG, XS, XT)                                                \
        EMITW(0x65E00000 | MXM(REG(XG), REG(XS), REG(XT)))                  \
        EMITW(0x65E00000 | MXM(RYG(XG), RYG(XS), RYG(XT)))

#define fmaqs_ld(XG, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65E00000 | MXM(REG(XG), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65E00000 | MXM(RYG(XG), RYG(XS), TmmM))

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T) if (#G != #S && #G != #T)
 * NOTE: due to final negation being outside of rounding on all POWER systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsqs_rr(XG, XS, XT)                                                \
        EMITW(0x65E02000 | MXM(REG(XG), REG(XS), REG(XT)))                  \
        EMITW(0x65E02000 | MXM(RYG(XG), RYG(XS), RYG(XT)))

#define fmsqs_ld(XG, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65E02000 | MXM(REG(XG), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65E02000 | MXM(RYG(XG), RYG(XS), TmmM))

#endif /* RT_SIMD_COMPAT_FMS */

/*************   packed double-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T) */

#define minqs_rr(XG, XS)                                                    \
        EMITW(0x65C78000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x65C78000 | MXM(RYG(XG), RYG(XS), 0x00))

#define minqs_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C78000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C78000 | MXM(RYG(XG), TmmM,    0x00))

#define minqs3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        minqs_rr(W(XD), W(XT))

#define minqs3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        minqs_ld(W(XD), W(MT), W(DT))

        /* mnp, mnh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T) */

#define maxqs_rr(XG, XS)                                                    \
        EMITW(0x65C68000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x65C68000 | MXM(RYG(XG), RYG(XS), 0x00))

#define maxqs_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C68000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C68000 | MXM(RYG(XG), TmmM,    0x00))

#define maxqs3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        maxqs_rr(W(XD), W(XT))

#define maxqs3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        maxqs_ld(W(XD), W(MT), W(DT))

        /* mxp, mxh are defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqqs_rr(XG, XS)                                                    \
        ceqqs3rr(W(XG), W(XG), W(XS))

#define ceqqs_ld(XG, MS, DS)                                                \
        ceqqs3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C06000 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C06000 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define ceqqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C06000 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C06000 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cneqs_rr(XG, XS)                                                    \
        cneqs3rr(W(XG), W(XG), W(XS))

#define cneqs_ld(XG, MS, DS)                                                \
        cneqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cneqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C06010 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C06010 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cneqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C06010 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C06010 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T) */

#define cltqs_rr(XG, XS)                                                    \
        cltqs3rr(W(XG), W(XG), W(XS))

#define cltqs_ld(XG, MS, DS)                                                \
        cltqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C04010 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C04010 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cltqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04010 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04010 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T) */

#define cleqs_rr(XG, XS)                                                    \
        cleqs3rr(W(XG), W(XG), W(XS))

#define cleqs_ld(XG, MS, DS)                                                \
        cleqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C04000 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C04000 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cleqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04000 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04000 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T) */

#define cgtqs_rr(XG, XS)                                                    \
        cgtqs3rr(W(XG), W(XG), W(XS))

#define cgtqs_ld(XG, MS, DS)                                                \
        cgtqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C04010 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C04010 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgtqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04010 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04010 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T) */

#define cgeqs_rr(XG, XS)                                                    \
        cgeqs3rr(W(XG), W(XG), W(XS))

#define cgeqs_ld(XG, MS, DS)                                                \
        cgeqs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqs3rr(XD, XS, XT)                                                \
        EMITW(0x65C04000 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x65C04000 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgeqs3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04000 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x65C04000 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE64_SVE     0x00    /* none satisfy the condition */
#define RT_SIMD_MASK_FULL64_SVE     0x01    /*  all satisfy the condition */

#define mkjqx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        EMITW(0x04203000 | MXM(TmmM,    REG(XS), RYG(XS)) |                 \
                     (1 - RT_SIMD_MASK_##mask##64_SVE) << 22)               \
        EMITW(0x04982000 | MXM(TmmM,    TmmM,    0x00) |                    \
                          RT_SIMD_MASK_##mask##64_SVE << 17)                \
        EMITW(0x0E043C00 | MXM(Teax,    TmmM,    0x00))                     \
        addwxZri(Reax, IB(RT_SIMD_MASK_##mask##64_SVE))                     \
        jezxx_lb(lb)

/*************   packed double-precision floating-point convert   *************/

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnzqs_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0x65C3A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65C3A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rnzqs_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C3A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C3A000 | MXM(RYG(XD), TmmM,    0x00))

#define cvzqs_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0x65DEA000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65DEA000 | MXM(RYG(XD), RYG(XS), 0x00))

#define cvzqs_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65DEA000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65DEA000 | MXM(RYG(XD), TmmM,    0x00))

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnpqs_rr(XD, XS)     /* round towards +inf */                       \
        EMITW(0x65C1A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65C1A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rnpqs_ld(XD, MS, DS) /* round towards +inf */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C1A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C1A000 | MXM(RYG(XD), TmmM,    0x00))

#define cvpqs_rr(XD, XS)     /* round towards +inf */                       \
        rnpqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvpqs_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnmqs_rr(XD, XS)     /* round towards -inf */                       \
        EMITW(0x65C2A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65C2A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rnmqs_ld(XD, MS, DS) /* round towards -inf */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C2A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C2A000 | MXM(RYG(XD), TmmM,    0x00))

#define cvmqs_rr(XD, XS)     /* round towards -inf */                       \
        rnmqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvmqs_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnnqs_rr(XD, XS)     /* round towards near */                       \
        EMITW(0x65C0A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65C0A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rnnqs_ld(XD, MS, DS) /* round towards near */                       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C0A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C0A000 | MXM(RYG(XD), TmmM,    0x00))

#define cvnqs_rr(XD, XS)     /* round towards near */                       \
        rnnqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvnqs_ld(XD, MS, DS) /* round towards near */                       \
        rnnqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvnqn_rr(XD, XS)     /* round towards near */                       \
        cvtqn_rr(W(XD), W(XS))

#define cvnqn_ld(XD, MS, DS) /* round towards near */                       \
        cvtqn_ld(W(XD), W(MS), W(DS))

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX POWER systems, use cvz
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rndqs_rr(XD, XS)                                                    \
        EMITW(0x65C7A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65C7A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define rndqs_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C7A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65C7A000 | MXM(RYG(XD), TmmM,    0x00))

#define cvtqs_rr(XD, XS)                                                    \
        rndqs_rr(W(XD), W(XS))                                              \
        cvzqs_rr(W(XD), W(XD))

#define cvtqs_ld(XD, MS, DS)                                                \
        rndqs_ld(W(XD), W(MS), W(DS))                                       \
        cvzqs_rr(W(XD), W(XD))

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX POWER systems */

#define cvtqn_rr(XD, XS)                                                    \
        EMITW(0x65D6A000 | MXM(REG(XD), REG(XS), 0x00))                     \
        EMITW(0x65D6A000 | MXM(RYG(XD), RYG(XS), 0x00))

#define cvtqn_ld(XD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65D6A000 | MXM(REG(XD), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x65D6A000 | MXM(RYG(XD), TmmM,    0x00))

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, fp64 SIMD fp-to-int
 * round instructions are only accurate within 64-bit signed int range */

#define rnrqs_rr(XD, XS, mode)                                              \
        EMITW(0x65C0A000 | MXM(REG(XD), REG(XS), 0x00) |                    \
                                        RT_SIMD_MODE_##mode << 16)          \
        EMITW(0x65C0A000 | MXM(RYG(XD), RYG(XS), 0x00) |                    \
                                        RT_SIMD_MODE_##mode << 16)

#define cvrqs_rr(XD, XS, mode)                                              \
        rnrqs_rr(W(XD), W(XS), mode)                                        \
        cvzqs_rr(W(XD), W(XD))

/************   packed double-precision integer arithmetic/shifts   ***********/

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addqx_rr(XG, XS)                                                    \
        addqx3rr(W(XG), W(XG), W(XS))

#define addqx_ld(XG, MS, DS)                                                \
        addqx3ld(W(XG), W(XG), W(MS), W(DS))

#define addqx3rr(XD, XS, XT)                                                \
        EMITW(0x04E00000 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x04E00000 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define addqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E00000 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E00000 | MXM(RYG(XD), RYG(XS), TmmM))

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define subqx_rr(XG, XS)                                                    \
        subqx3rr(W(XG), W(XG), W(XS))

#define subqx_ld(XG, MS, DS)                                                \
        subqx3ld(W(XG), W(XG), W(MS), W(DS))

#define subqx3rr(XD, XS, XT)                                                \
        EMITW(0x04E00400 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x04E00400 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define subqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E00400 | MXM(REG(XD), REG(XS), TmmM))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x04E00400 | MXM(RYG(XD), RYG(XS), TmmM))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulqx_rr(XG, XS)                                                    \
        EMITW(0x04D00000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04D00000 | MXM(RYG(XG), RYG(XS), 0x00))

#define mulqx_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D00000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D00000 | MXM(RYG(XG), TmmM,    0x00))

#define mulqx3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        mulqx_rr(W(XD), W(XT))

#define mulqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        mulqx_ld(W(XD), W(MT), W(DT))

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlqx_ri(XG, IS)     /* emits shift-right with out-of-range args */ \
        shlqx3ri(W(XG), W(XG), W(IS))

#define shlqx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xF8400000 | MDM(TMxx,    MOD(MS), VXL(DS), B1(DS), P1(DS)))  \
        EMITW(0x05E03800 | MXM(TmmM,    TMxx,    0x00))                     \
        EMITW(0x04D38000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x04D38000 | MXM(RYG(XG), TmmM,    0x00))

#define shlqx3ri(XD, XS, IT)                                                \
        EMITW(0x04A09400 | MXM(REG(XD), REG(XS), 0x00) |                    \
        (M(VAL(IT) < 64) & 0x00000800) | (M(VAL(IT) > 63) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 & VAL(IT)) << 17 |                        \
                            (0x1F & VAL(IT)) << 16)))                       \
        EMITW(0x04A09400 | MXM(RYG(XD), RYG(XS), 0x00) |                    \
        (M(VAL(IT) < 64) & 0x00000800) | (M(VAL(IT) > 63) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 & VAL(IT)) << 17 |                        \
                            (0x1F & VAL(IT)) << 16)))

#define shlqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        shlqx_ld(W(XD), W(MT), W(DT))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrqx_ri(XG, IS)     /* emits shift-left for immediate-zero args */ \
        shrqx3ri(W(XG), W(XG), W(IS))

#define shrqx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xF8400000 | MDM(TMxx,    MOD(MS), VXL(DS), B1(DS), P1(DS)))  \
        EMITW(0x05E03800 | MXM(TmmM,    TMxx,    0x00))                     \
        EMITW(0x04D18000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x04D18000 | MXM(RYG(XG), TmmM,    0x00))

#define shrqx3ri(XD, XS, IT)                                                \
        EMITW(0x04A09400 | MXM(REG(XD), REG(XS), 0x00) |                    \
        (M(VAL(IT) == 0) & 0x00000800) | (M(VAL(IT) != 0) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 &-VAL(IT)) << 17 |                        \
                            (0x1F &-VAL(IT)) << 16)))                       \
        EMITW(0x04A09400 | MXM(RYG(XD), RYG(XS), 0x00) |                    \
        (M(VAL(IT) == 0) & 0x00000800) | (M(VAL(IT) != 0) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 &-VAL(IT)) << 17 |                        \
                            (0x1F &-VAL(IT)) << 16)))

#define shrqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        shrqx_ld(W(XD), W(MT), W(DT))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrqn_ri(XG, IS)     /* emits shift-left for immediate-zero args */ \
        shrqn3ri(W(XG), W(XG), W(IS))

#define shrqn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xF8400000 | MDM(TMxx,    MOD(MS), VXL(DS), B1(DS), P1(DS)))  \
        EMITW(0x05E03800 | MXM(TmmM,    TMxx,    0x00))                     \
        EMITW(0x04D08000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x04D08000 | MXM(RYG(XG), TmmM,    0x00))

#define shrqn3ri(XD, XS, IT)                                                \
        EMITW(0x04A09000 | MXM(REG(XD), REG(XS), 0x00) |                    \
        (M(VAL(IT) == 0) & 0x00000C00) | (M(VAL(IT) != 0) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 &-VAL(IT)) << 17 |                        \
                            (0x1F &-VAL(IT)) << 16)))                       \
        EMITW(0x04A09000 | MXM(RYG(XD), RYG(XS), 0x00) |                    \
        (M(VAL(IT) == 0) & 0x00000C00) | (M(VAL(IT) != 0) & 0x00000000) |   \
        (M(VAL(IT) < 64) & ((0x20 &-VAL(IT)) << 17 |                        \
                            (0x1F &-VAL(IT)) << 16)))

#define shrqn3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        shrqn_ld(W(XD), W(MT), W(DT))

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlqx_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x04D38000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04D38000 | MXM(RYG(XG), RYG(XS), 0x00))

#define svlqx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D38000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D38000 | MXM(RYG(XG), TmmM,    0x00))

#define svlqx3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        svlqx_rr(W(XD), W(XT))

#define svlqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        svlqx_ld(W(XD), W(MT), W(DT))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrqx_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x04D18000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04D18000 | MXM(RYG(XG), RYG(XS), 0x00))

#define svrqx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D18000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D18000 | MXM(RYG(XG), TmmM,    0x00))

#define svrqx3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        svrqx_rr(W(XD), W(XT))

#define svrqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        svrqx_ld(W(XD), W(MT), W(DT))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrqn_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x04D08000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04D08000 | MXM(RYG(XG), RYG(XS), 0x00))

#define svrqn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D08000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04D08000 | MXM(RYG(XG), TmmM,    0x00))

#define svrqn3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        svrqn_rr(W(XD), W(XT))

#define svrqn3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        svrqn_ld(W(XD), W(MT), W(DT))

/****************   packed double-precision integer compare   *****************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minqx_rr(XG, XS)                                                    \
        EMITW(0x04CB0000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04CB0000 | MXM(RYG(XG), RYG(XS), 0x00))

#define minqx_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04CB0000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04CB0000 | MXM(RYG(XG), TmmM,    0x00))

#define minqx3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        minqx_rr(W(XD), W(XT))

#define minqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        minqx_ld(W(XD), W(MT), W(DT))

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minqn_rr(XG, XS)                                                    \
        EMITW(0x04CA0000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04CA0000 | MXM(RYG(XG), RYG(XS), 0x00))

#define minqn_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04CA0000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04CA0000 | MXM(RYG(XG), TmmM,    0x00))

#define minqn3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        minqn_rr(W(XD), W(XT))

#define minqn3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        minqn_ld(W(XD), W(MT), W(DT))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxqx_rr(XG, XS)                                                    \
        EMITW(0x04C90000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04C90000 | MXM(RYG(XG), RYG(XS), 0x00))

#define maxqx_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04C90000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04C90000 | MXM(RYG(XG), TmmM,    0x00))

#define maxqx3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        maxqx_rr(W(XD), W(XT))

#define maxqx3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        maxqx_ld(W(XD), W(MT), W(DT))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxqn_rr(XG, XS)                                                    \
        EMITW(0x04C80000 | MXM(REG(XG), REG(XS), 0x00))                     \
        EMITW(0x04C80000 | MXM(RYG(XG), RYG(XS), 0x00))

#define maxqn_ld(XG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), A1(DS), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VAL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04C80000 | MXM(REG(XG), TmmM,    0x00))                     \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MS), VZL(DS), B3(DS), K1(DS)))  \
        EMITW(0x04C80000 | MXM(RYG(XG), TmmM,    0x00))

#define maxqn3rr(XD, XS, XT)                                                \
        movqx_rr(W(XD), W(XS))                                              \
        maxqn_rr(W(XD), W(XT))

#define maxqn3ld(XD, XS, MT, DT)                                            \
        movqx_rr(W(XD), W(XS))                                              \
        maxqn_ld(W(XD), W(MT), W(DT))

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqqx_rr(XG, XS)                                                    \
        ceqqx3rr(W(XG), W(XG), W(XS))

#define ceqqx_ld(XG, MS, DS)                                                \
        ceqqx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C0A000 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C0A000 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define ceqqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C0A000 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C0A000 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cneqx_rr(XG, XS)                                                    \
        cneqx3rr(W(XG), W(XG), W(XS))

#define cneqx_ld(XG, MS, DS)                                                \
        cneqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cneqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C0A010 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C0A010 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cneqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C0A010 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C0A010 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltqx_rr(XG, XS)                                                    \
        cltqx3rr(W(XG), W(XG), W(XS))

#define cltqx_ld(XG, MS, DS)                                                \
        cltqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C00010 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C00010 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cltqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00010 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00010 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltqn_rr(XG, XS)                                                    \
        cltqn3rr(W(XG), W(XG), W(XS))

#define cltqn_ld(XG, MS, DS)                                                \
        cltqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltqn3rr(XD, XS, XT)                                                \
        EMITW(0x24C08010 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C08010 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cltqn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08010 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08010 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define cleqx_rr(XG, XS)                                                    \
        cleqx3rr(W(XG), W(XG), W(XS))

#define cleqx_ld(XG, MS, DS)                                                \
        cleqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C00000 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C00000 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cleqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00000 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00000 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define cleqn_rr(XG, XS)                                                    \
        cleqn3rr(W(XG), W(XG), W(XS))

#define cleqn_ld(XG, MS, DS)                                                \
        cleqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cleqn3rr(XD, XS, XT)                                                \
        EMITW(0x24C08000 | MXM(0x01,    REG(XT), REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C08000 | MXM(0x01,    RYG(XT), RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cleqn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08000 | MXM(0x01,    TmmM,    REG(XS)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08000 | MXM(0x01,    TmmM,    RYG(XS)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtqx_rr(XG, XS)                                                    \
        cgtqx3rr(W(XG), W(XG), W(XS))

#define cgtqx_ld(XG, MS, DS)                                                \
        cgtqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C00010 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C00010 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgtqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00010 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00010 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtqn_rr(XG, XS)                                                    \
        cgtqn3rr(W(XG), W(XG), W(XS))

#define cgtqn_ld(XG, MS, DS)                                                \
        cgtqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtqn3rr(XD, XS, XT)                                                \
        EMITW(0x24C08010 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C08010 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgtqn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08010 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08010 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgeqx_rr(XG, XS)                                                    \
        cgeqx3rr(W(XG), W(XG), W(XS))

#define cgeqx_ld(XG, MS, DS)                                                \
        cgeqx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqx3rr(XD, XS, XT)                                                \
        EMITW(0x24C00000 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C00000 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgeqx3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00000 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C00000 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgeqn_rr(XG, XS)                                                    \
        cgeqn3rr(W(XG), W(XG), W(XS))

#define cgeqn_ld(XG, MS, DS)                                                \
        cgeqn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgeqn3rr(XD, XS, XT)                                                \
        EMITW(0x24C08000 | MXM(0x01,    REG(XS), REG(XT)))                  \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x24C08000 | MXM(0x01,    RYG(XS), RYG(XT)))                  \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

#define cgeqn3ld(XD, XS, MT, DT)                                            \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), A1(DT), EMPTY2)   \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VAL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08000 | MXM(0x01,    REG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x05E0C400 | MXM(REG(XD), TmmQ,    REG(XD)))                  \
        EMITW(0x85804000 | MPM(TmmM,    MOD(MT), VZL(DT), B3(DT), K1(DT)))  \
        EMITW(0x24C08000 | MXM(0x01,    RYG(XS), TmmM))                     \
        EMITW(0x04A03000 | MXM(RYG(XD), RYG(XD), RYG(XD)))                  \
        EMITW(0x05E0C400 | MXM(RYG(XD), TmmQ,    RYG(XD)))

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_SVEX2 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_A64_SVEX2V1_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
