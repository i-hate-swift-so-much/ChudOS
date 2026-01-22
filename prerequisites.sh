#!/bin/bash
echo "IMPORTANT!!! This must use the brew version of these packages, other builds will result in errors"
brew update
brew install x86_64-elf-binutils
brew install x86_64-elf-gcc #this is installed separately from x86_64-elf-binutils because it isn't included in that package.
brew install nasm
brew install binutils