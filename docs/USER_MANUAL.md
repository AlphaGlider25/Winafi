# Winafi User Manual

Winafi creates bootable USB drives from Windows and Linux ISO images on Linux. This manual covers the
graphical app; for the command line run `winafi --help`.

> **Warning:** Writing to a USB drive **erases everything on it**. Double-check the target device.

## Installing

Use your distribution's package (`.deb`, `.rpm`, AUR, AppImage, Flatpak, or Snap) — see the project
README. Or run the portable tarball: download `winafi-<version>-portable-<arch>.tar.gz`, extract, and
run `./winafi`.

## The window at a glance

Winafi uses a two-pane layout:

- **Left navigation** — four sections: **Source**, **Target**, **Windows**, **Advanced**.
- **Right pane** — the controls for the selected section.
- **Bottom footer (always visible)** — a one-line summary of what will be written, a progress bar with
  estimated time remaining, and the **START** button.
- **Top bar** — a status indicator (Ready / Writing… / Done / Error) and a **Light/Dark** theme toggle
  (your choice is remembered).

## Step by step

### 1. Source
1. Click **Browse** and pick your `.iso` file.
2. Winafi shows the detected operating system (e.g. "Windows 11").
3. Optional: **Verify** re-checks the image; **Compute hash** opens the SHA-256 dialog so you can
   compare against a checksum published by the ISO's source.

### 2. Target
1. Plug in your USB drive.
2. Pick it from the device list (capacity is shown). Click **Refresh** if it isn't listed.
3. Only removable drives appear by default. **Show hard drives** reveals fixed disks — use with care.

### 3. Windows (Windows ISOs only)
This section is greyed out for non-Windows images. For Windows 11 you can:

- **Compatibility bypasses** — remove individual Windows 11 setup requirements: **TPM**,
  **Secure Boot**, **RAM**, **CPU**, **Storage**, or tick **Apply all compatibility bypasses**. Use
  these to install Windows 11 on unsupported hardware. Each has a tooltip explaining what it does.
- **Create local account automatically** — set a **Username** (defaults to your Linux username) and,
  optionally, make it an **Administrator account**.
- **Skip Microsoft account requirement** / **Allow offline installation** — finish Windows setup
  without signing in to a Microsoft account or a network.

### 4. Advanced
Partition scheme (GPT/MBR), target system (UEFI/BIOS), file system (NTFS/FAT32/exFAT), cluster size,
volume label, and quick format. Defaults are correct for most modern Windows installs.

### 5. Write
1. Check the footer summary — it lists the ISO, target device, file system, and any active bypasses.
2. Click **START** and confirm the erase warning.
3. Watch progress and the estimated time remaining. The status indicator turns green (**Done**) when
   finished, or red (**Error**) with a message if something goes wrong.

## After writing

- **UEFI / Secure Boot:** Winafi installs a Microsoft-signed boot loader, so the drive boots on
  Secure Boot machines without disabling it. Just boot from the USB.
- If your PC boots its own OS instead, select the USB drive in the firmware boot menu (often F12 /
  F10 / Esc at power-on).

## Tips & troubleshooting

- **The Windows section is greyed out** — the selected ISO isn't a Windows image.
- **My drive isn't listed** — click **Refresh**; if it's an internal disk, enable **Show hard drives**.
- **Boots on one PC but not another (Secure Boot)** — see `docs/SECURE_BOOT_ROOT_CAUSE.md`.
- **Keyboard only** — Tab moves between controls; the nav buttons and **START** are reachable without
  a mouse. The interface supports high-DPI displays and light/dark themes.

## Privacy & safety notes

The Windows compatibility bypasses and account options write a standard `autounattend.xml` to the
media using Microsoft-documented settings. Winafi does not modify Windows binaries or signatures.
