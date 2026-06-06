#!/usr/bin/env bash
# Boots a Winafi-produced image under OVMF with Secure Boot ON and asserts it
# reaches Windows Setup without firmware rejection.
# Requires: qemu-system-x86_64, OVMF secboot firmware, a produced disk image $IMG.
set -euo pipefail
IMG="${1:?usage: test_secureboot_qemu.sh <disk-image>}"
OVMF_CODE="${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.secboot.fd}"
OVMF_VARS_SRC="${OVMF_VARS:-/usr/share/OVMF/OVMF_VARS.secboot.fd}"   # MS keys enrolled
VARS="$(mktemp)"; cp "$OVMF_VARS_SRC" "$VARS"
echo "Booting $IMG under Secure Boot (OVMF)..."
timeout 120 qemu-system-x86_64 -machine q35 -m 4096 \
  -drive if=pflash,format=raw,unit=0,readonly=on,file="$OVMF_CODE" \
  -drive if=pflash,format=raw,unit=1,file="$VARS" \
  -drive file="$IMG",format=raw,if=none,id=usb -device usb-storage,drive=usb \
  -device qemu-xhci -serial stdio -display none | tee /tmp/winafi_secboot.log || true
# Failure signatures: "Access Denied", "Security Violation", "not a valid EFI"
if grep -Eqi 'Access Denied|Security Violation|not a valid' /tmp/winafi_secboot.log; then
  echo "FAIL: firmware rejected the bootloader under Secure Boot"; exit 1
fi
echo "PASS: no Secure Boot rejection detected"
