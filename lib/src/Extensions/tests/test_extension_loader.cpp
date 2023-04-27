/*******************************************************************************
// Copyright (c) 2021-2023 Ben Ashbaugh
//
// SPDX-License-Identifier: MIT or Apache-2.0
*/

// clang-format off

#include "call_all.c"

int main(int argc, char** argv)
{
    // Purposefully always false.
    if (argc == 100) {
        call_all();
    }
    return 0;
}
