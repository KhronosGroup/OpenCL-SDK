/*******************************************************************************
// Copyright (c) 2023 Ben Ashbaugh
//
// SPDX-License-Identifier: MIT or Apache-2.0
*/

// clang-format off

#ifndef LOADER_INFO_PROTOTYPES_H_
#define LOADER_INFO_PROTOTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(cl_loader_info)

extern CL_API_ENTRY cl_int CL_API_CALL
clGetICDLoaderInfoOCLICD(cl_icdl_info param_name,
                         size_t       param_value_size,
                         void *       param_value,
                         size_t *     param_value_size_ret);

#endif // defined(cl_loader_info)

#ifdef __cplusplus
}
#endif

#endif // LOADER_INFO_PROTOTYPES_H_
