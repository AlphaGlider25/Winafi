#!/usr/bin/env bash
#
# fetch-uefi-ntfs.sh
#
# Downloads the Microsoft-signed UEFI:NTFS x64 bootloader from pbatard/uefi-ntfs,
# verifies it against src/assets/uefi-ntfs/SHA256SUMS, and installs it to
# src/assets/uefi-ntfs/bootx64.efi.
#
# This signed loader is required for Winafi's Windows media to boot under
# UEFI with Secure Boot enabled. The asset is signed by the
# "Microsoft Corporation UEFI CA 2011" chain (the Secure Boot trust anchor).
#
# URL HISTORY:
#   - The originally-specified URL
#       https://github.com/pbatard/uefi-ntfs/releases/download/v3.2/bootx64.efi
#     returns HTTP 404 (no such release/asset at time of writing).
#   - The working source is the current latest release, v2.7. The *signed* x64
#     asset is named "bootx64_signed.efi" (the plain "bootx64.efi" asset in that
#     release is the UNSIGNED build and must NOT be used for Secure Boot).
#   - WORKING URL (used to produce the pinned hash in SHA256SUMS):
#       https://github.com/pbatard/uefi-ntfs/releases/download/v2.7/bootx64_signed.efi
#
set -euo pipefail

# Pinned, signed x64 UEFI:NTFS loader.
UEFI_NTFS_URL="https://github.com/pbatard/uefi-ntfs/releases/download/v2.7/bootx64_signed.efi"

# Resolve repo paths relative to this script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ASSET_DIR="${REPO_ROOT}/src/assets/uefi-ntfs"
DEST="${ASSET_DIR}/bootx64.efi"
SUMS="${ASSET_DIR}/SHA256SUMS"

if [[ ! -f "${SUMS}" ]]; then
    echo "ERROR: checksum file not found: ${SUMS}" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

echo "Downloading signed UEFI:NTFS loader from:"
echo "  ${UEFI_NTFS_URL}"
if ! curl -fsSL -o "${TMP_DIR}/bootx64.efi" "${UEFI_NTFS_URL}"; then
    echo "ERROR: download failed from ${UEFI_NTFS_URL}" >&2
    echo "Locate the current signed x64 asset at https://github.com/pbatard/uefi-ntfs/releases" >&2
    exit 1
fi

# Verify the download against the pinned checksum. sha256sum -c needs the named
# file present in the working directory, so verify inside the temp dir using a
# checksum line that references the temp filename.
EXPECTED_HASH="$(awk '{print $1}' "${SUMS}")"
echo "${EXPECTED_HASH}  bootx64.efi" > "${TMP_DIR}/SHA256SUMS"
( cd "${TMP_DIR}" && sha256sum -c SHA256SUMS )

# Sanity check: must be a PE/COFF EFI application, not a placeholder.
if ! file "${TMP_DIR}/bootx64.efi" | grep -q 'PE32+'; then
    echo "ERROR: downloaded file is not a PE32+ EFI application" >&2
    exit 1
fi

install -m 0644 "${TMP_DIR}/bootx64.efi" "${DEST}"
echo "Installed signed UEFI:NTFS loader to ${DEST}"
