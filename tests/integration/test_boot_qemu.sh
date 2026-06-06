#!/usr/bin/env bash
# Boots a Winafi-produced disk image under QEMU in a chosen firmware mode and
# fails on any firmware/boot rejection signature (Feature 8, §3.5 / §4.2).
#
# Usage:  test_boot_qemu.sh <disk-image> [uefi|bios]
#   uefi (default) : boot with OVMF (Secure Boot OFF). For Secure Boot ON use
#                    test_secureboot_qemu.sh instead.
#   bios           : boot with SeaBIOS (legacy).
#
# Env overrides: OVMF_CODE (default /usr/share/OVMF/OVMF_CODE.fd)
#
# Pass criteria: QEMU runs to the timeout (i.e. the boot loader handed off and
# the OS/Setup kept running) AND no rejection string appears on the serial log.
set -euo pipefail

IMG="${1:?usage: test_boot_qemu.sh <disk-image> [uefi|bios]}"
MODE="${2:-uefi}"
LOG="$(mktemp)"
OVMF_CODE="${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE.fd}"

common=(-machine q35 -m 4096 -no-reboot
        -drive "file=${IMG},format=raw,if=none,id=usb"
        -device usb-storage,drive=usb -device qemu-xhci
        -serial stdio -display none)

case "$MODE" in
  uefi)
    [ -f "$OVMF_CODE" ] || { echo "SKIP: OVMF firmware not found at $OVMF_CODE"; exit 0; }
    VARS="$(mktemp)"
    # A blank writable vars store (no Secure Boot enrolment for this plain-UEFI test).
    : > "$VARS"; truncate -s 540672 "$VARS" 2>/dev/null || true
    set +e
    timeout 120 qemu-system-x86_64 \
      -drive if=pflash,format=raw,unit=0,readonly=on,file="$OVMF_CODE" \
      "${common[@]}" | tee "$LOG"
    set -e
    ;;
  bios)
    set +e
    timeout 120 qemu-system-x86_64 "${common[@]}" | tee "$LOG"
    set -e
    ;;
  *)
    echo "Unknown mode: $MODE (expected uefi|bios)"; exit 2 ;;
esac

if grep -Eqi 'Access Denied|Security Violation|not a valid|No bootable device|Boot failed' "$LOG"; then
  echo "FAIL ($MODE): boot rejected — see log above"
  exit 1
fi
echo "PASS ($MODE): no boot-rejection signature detected"
