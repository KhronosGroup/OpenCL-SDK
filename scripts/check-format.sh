#!/usr/bin/env bash

# Arg used to specify non-'origin/main' comparison branch
ORIGIN_BRANCH=${1:-"origin/main"}

# Run git-clang-format to check for violations
if [ "$TRAVIS" == "true" ]; then
    EXTRA_OPTS="--binary `which clang-format-9`"
fi
CLANG_FORMAT_OUTPUT=$(git-clang-format --diff $ORIGIN_BRANCH --extensions c,cpp,h,hpp $EXTRA_OPTS)

# Check for no-ops
grep '^no modified files to format$' <<<"$CLANG_FORMAT_OUTPUT" && exit 0
grep '^clang-format did not modify any files$' <<<"$CLANG_FORMAT_OUTPUT" && exit 0

# Dump formatting diff and signal failure
echo -e "\n==== FORMATTING VIOLATIONS DETECTED ====\n"
echo "$CLANG_FORMAT_OUTPUT"
exit 1
