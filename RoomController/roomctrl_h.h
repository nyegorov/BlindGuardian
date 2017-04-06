

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0620 */
/* at Tue Jan 19 05:14:07 2038
 */
/* Compiler settings for C:\Users\nick\AppData\Local\Temp\roomctrl.idl-775b66b2:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=ARM 8.01.0620 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __roomctrl_h_h__
#define __roomctrl_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__cplusplus)
#if defined(__MIDL_USE_C_ENUM)
#define MIDL_ENUM enum
#else
#define MIDL_ENUM enum class
#endif
#endif


/* Forward Declarations */ 

/* header files for imported files */
#include "inspectable.h"
#include "Windows.Foundation.h"
#include "Windows.ApplicationModel.background.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_roomctrl_0000_0000 */
/* [local] */ 

#ifdef __cplusplus
namespace ABI {
namespace roomctrl {
class StartupTask;
} /*roomctrl*/
}
#endif

#ifndef RUNTIMECLASS_roomctrl_StartupTask_DEFINED
#define RUNTIMECLASS_roomctrl_StartupTask_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_roomctrl_StartupTask[] = L"roomctrl.StartupTask";
#endif


/* interface __MIDL_itf_roomctrl_0000_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_roomctrl_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_roomctrl_0000_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


