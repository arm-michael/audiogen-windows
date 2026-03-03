/*
 * SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * arm_fp16.h — minimal MSVC ARM64 compatibility stub
 *
 * MSVC ARM64 does not ship arm_fp16.h (an ARM ACLE header). XNNPACK's
 * fp16arith scalar kernels include this header for the float16_t typedef.
 * This stub satisfies that dependency without disabling any XNNPACK
 * optimisation paths.
 *
 * __fp16 is a built-in in MSVC C++ mode but is NOT valid in C mode.
 * XNNPACK compiles its .c files as C, so we must use _Float16, which is
 * the C11 standard half-precision type and is supported by MSVC ARM64
 * in both C and C++ mode (VS 2022 17.x+).
 */
#pragma once
#ifndef ARM_FP16_H_
#define ARM_FP16_H_

typedef _Float16 float16_t;

#endif /* ARM_FP16_H_ */
