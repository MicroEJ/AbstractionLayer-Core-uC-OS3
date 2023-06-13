![SDK](https://shields.microej.com/endpoint?url=https://repository.microej.com/packages/badges/sdk_5.7.json)
![ARCH](https://shields.microej.com/endpoint?url=https://repository.microej.com/packages/badges/arch_7.18.json)

# Overview

MicroEJ Core Engine Abstraction Layer implementation for uC-OS3.

This module implements the `LLMJVM` Low Level API for MicroEJ VEE Ports connected to a Board Support Package based on [uC-OS3](https://github.com/weston-embedded/uC-OS3/).

See the MicroEJ documentation for a description of the `LLMJVM` functions:
- [LLMJVM: MicroEJ Core Engine](https://docs.microej.com/en/latest/PlatformDeveloperGuide/appendix/llapi.html#llmjvm-microej-core-engine)
- [MicroEJ Core Engine: Implementation](https://docs.microej.com/en/latest/PlatformDeveloperGuide/coreEngine.html#implementation)

# Usage

1. Install ``src`` and ``inc`` directories in your Board Support Package. They can be automatically downloaded using the following command lines:
   ```sh
    svn export --force https://github.com/MicroEJ/AbstractionLayer-Core-uC-OS3/trunk/src/main/c/inc [path_to_bsp_directory]    
    svn export --force https://github.com/MicroEJ/AbstractionLayer-Core-uC-OS3/trunk/src/main/c/src [path_to_bsp_directory]
   ```

2. Implement the MicroEJ time functions, as described in [microej_time.h](./src/main/c/inc/microej_time.h).

3. The `LLMJVM_IMPL_scheduleRequest` function in [LLMJVM_uCOS3.c](./src/main/c/src/LLMJVM_uCOS3.c) uses a software timer. In order to correctly schedule MicroEJ threads, check the following elements in the uC-OS3 configuration file:

   - Timers are enabled with `#define OS_CFG_TMR_EN 1`
   - `OS_CFG_TMR_TASK_PRIO` is smaller than MicroEJ task priority
   - `OS_CFG_TMR_TASK_RATE_HZ`: can depend on the application, if it needs a 1 ms precision then the tick rate would be 1000 Hz, the recommended value is between 100 Hz and 1000 Hz

# Requirements

None.

# Validation

This Abstraction Layer implementation can be validated in the target Board Support Package using the [MicroEJ Core Validation](https://github.com/MicroEJ/VEEPortQualificationTools/tree/master/tests/core/java-testsuite-runner-core) VEE LLMJVM_UCOS3_CONFIGURATION Qualification Tools project.

Here is a non exhaustive list of tested environments:
- Hardware:
  - STMicroelectronics STM32F746-DISCO
- Compilers / Integrated Development Environments:
  - IAR Embedded Workbench 9.30.1
- uC-OS3 version:
  - uC-OS3 3.08.02

# MISRA Compliance

This Abstraction Layer implementation is MISRA-compliant (MISRA C 2012) with the following observed deviations:
| Deviation | Category |                                                 Justification                                                 |
|:---------:|:--------:|:-------------------------------------------------------------------------------------------------------------:|
| Rule 8.4  | Required | ``LLMJVM_IMPL`` functions are declared in ``LLMJVM_impl.h``. 														   | 
| Rule 21.6 | Required | ``printf`` from ``<stdio.h>`` is used by ``LLMJVM_ASSERT_TRACE_OUTPUT``. The output function can be configured in [LLMJVM_uCOS3_configuration.h](./src/main/c/inc/LLMJVM_uCOS3_configuration.h).	| 
| Rule 8.9  | Advisory | ``LLMJVM_UCOS3_next_wake_up_time`` is needed as a global variable.              |
| Rule 2.7  | Advisory | The uC-OS3 timer callback function need to comply with the ``OS_TMR_CALLBACK_PTR`` definition.              |
| Rule 8.7  | Advisory | ``LLMJVM_IMPL_getCurrentTime`` needs to be accessed by the Core Engine.            |  

# Dependencies

- MicroEJ Architecture ``7.x`` or higher.
- FreeRTOS ``3.08.02`` or higher.

# Source

N/A.

# Restrictions

None.

---

_Copyright 2023 MicroEJ Corp. All rights reserved._
_Use of this source code is governed by a BSD-style license that can be found with this software._
 
