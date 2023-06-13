/*
 * C
 *
 * Copyright 2023 MicroEJ Corp. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be found with this software.
 */
#ifndef LLMJVM_UCOS3_CONFIGURATION
#define LLMJVM_UCOS3_CONFIGURATION

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Compatibility sanity check value.
 * This define value is checked in the implementation to validate that the version of this configuration
 * is compatible with the implementation.
 *
 * This value must not be changed by the user of the CCO.
 * This value must be incremented by the implementor of the CCO when a configuration define is added, deleted or modified.
 */
#define LLMJVM_UCOS3_CONFIGURATION_VERSION (1)

#define LLMJVM_TRACE_ENABLED

#ifndef LLMJVM_TRACE_ENABLED
#define LLMJVM_ASSERT_TRACE_OUTPUT(...) ((void)0U)
#else
// cppcheck-suppress [misra-c2012-21.6] printf is used by default by LLMJVM_ASSERT_TRACE_OUTPUT, it can be changed below.
#include <stdio.h>
#define LLMJVM_ASSERT_TRACE_OUTPUT                \
  (void) printf("[LLMJVM ASSERT FAILED] ");       \
  (void) printf
#endif

#ifdef __cplusplus
    }
#endif

#endif /* LLMJVM_UCOS3_CONFIGURATION_H */