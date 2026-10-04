#!/bin/bash

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a72 \
    -m 512M \
    -nographic \
    -monitor none \
    -serial stdio \
    -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive if=pflash,format=raw,file=/opt/homebrew/share/qemu/edk2-arm-vars.fd \
    -drive format=raw,file=fat:rw:esp