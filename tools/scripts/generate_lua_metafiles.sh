#!/usr/bin/env bash

# Regenerate Lua metafiles from rts/Lua/*.cpp
#
# Assumes lua-doc-extractor is installed:
# https://github.com/rhys-vdw/lua-doc-extractor
#
# TODO: Add to GitHub action and delete this file.

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "Provide project root as argument"

project_root="$1"

lua-doc-extractor \
	--src $project_root/rts/Lua/*.cpp \
	--dest $project_root/rts/Lua/library/generated