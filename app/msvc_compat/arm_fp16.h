/*
 * SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * arm_fp16.h — minimal MSVC ARM64 compatibility stub
 *
 * MSVC ARM64 supports __fp16 as a built-in half-precision type but does not
 * ship arm_fp16.h (an ARM ACLE header). XNNPACK's fp16arith scalar kernels
 * include this header for the float16_t typedef. This stub satisfies that
 * dependency without disabling any XNNPACK optimisation paths.
 */
#pragma once
#ifndef ARM_FP16_H_
#define ARM_FP16_H_

typedef __fp16 float16_t;

#endif /* ARM_FP16_H_ */
