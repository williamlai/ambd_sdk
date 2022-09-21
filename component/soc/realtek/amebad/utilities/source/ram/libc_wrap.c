/**************************************************************************//**
* @file     libc_wrap.c
* @brief    The wraper functions of ROM code to replace some of utility
*           functions in Compiler's Library.
* @version  V1.00
* @date     2018-08-15
*
* @note
*
******************************************************************************
*
* Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the License); you may
* not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an AS IS BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************************/
#include "basic_types.h"

/* for GNU C++ */
#if defined(__GNUC__)
void* __dso_handle = 0;
#endif

/**************************************************
* FreeRTOS memory management functions's wrapper to replace 
* malloc/free/realloc of GCC Lib.
**************************************************/
//#include "FreeRTOS.h"
// pvPortReAlloc currently not defined in portalbe.h
extern void* pvPortReAlloc( void *pv,  size_t xWantedSize );
extern void *pvPortMalloc( size_t xWantedSize );
extern void vPortFree( void *pv );

void* __wrap_malloc( size_t size )
{
    return pvPortMalloc(size);
}

void* __wrap_realloc( void *p, size_t size )
{
    return (void*)pvPortReAlloc(p, size);
}

void* __wrap_calloc( size_t n, size_t size )
{
    void* tmp = pvPortMalloc(n*size);
    if(tmp) rt_memset(tmp, 0, n*size);
    return tmp;
}

void __wrap_free( void *p )
{
    vPortFree(p);
}

/**************************************************
* string and memory api wrap for compiler
*
**************************************************/

#if defined(__GNUC__)
#include <errno.h>

static int gnu_errno;
volatile int * __aeabi_errno_addr (void)
{
    return &gnu_errno;
}
#endif
