# Release Instructions

## Creating a Release

Releases are automated via GitHub Actions. To create a release:

1. **Update version in CMakeLists.txt:**
   ```bash
   # Edit CMakeLists.txt and update PROJECT_VERSION
   # e.g., VERSION 0.0.4 -> VERSION 0.0.5
   ```

2. **Commit version change:**
   ```bash
   git add CMakeLists.txt
   git commit -m "chore: bump version to 0.0.5"
   git push origin main
   ```

3. **Create Git tag:**
   ```bash
   git tag -a v0.0.5 -m "Release version 0.0.5"
   git push origin v0.0.5
   ```

4. **Automated Build:**
   - GitHub Actions automatically triggers on tag push
   - Builds all packages in parallel: AppImage, deb, rpm, Flatpak, Snap
   - Creates GitHub Release with all artifacts attached
   - Each artifact is named with version: `winafi_0.0.5_amd64.deb`, etc.

## Package Formats

### AppImage
- Single executable file, works on most Linux distributions
- Downloaded from: https://github.com/AlphaGlider25/Winafi/releases
- Usage: `./Winafi-0.0.4-Beta-x86_64.AppImage`

### Debian/Ubuntu
- Install: `sudo apt install ./winafi_0.0.4_amd64.deb`
- Uninstall: `sudo apt remove winafi`

### RPM (Fedora/RHEL)
- Install: `sudo dnf install ./winafi-0.0.4-1.fc38.x86_64.rpm`
- Uninstall: `sudo dnf remove winafi`

### Flatpak
- Install from Flathub: `flatpak install flathub com.github.alphaglider25.winafi`
- Or from GitHub Release: `flatpak install --bundle Winafi.flatpak`
- Run: `flatpak run com.github.alphaglider25.winafi`

**For building Flatpak locally:**
```bash
# Install Flatpak Builder
flatpak install flathub org.flatpak.Builder

# Build the Flatpak
flatpak run --privileged org.flatpak.Builder --repo=repo --force-clean build-dir flatpak/com.github.alphaglider25.winafi.yml

# Create bundle
flatpak build-bundle repo Winafi.flatpak com.github.alphaglider25.winafi
```

### Snap
- Install: `sudo snap install winafi`
- Run: `winafi` or `winafi-gui`

## Manual Steps

### Publishing to Flathub
1. Fork https://github.com/flathub/flathub
2. Create branch: `git checkout -b com.github.alphaglider25.winafi`
3. Copy manifest: `cp flatpak/com.github.alphaglider25.winafi.yml flathub/com/github/alphaglider25/winafi.yml`
4. Update manifest with release artifacts
5. Create pull request to Flathub main

### Publishing to AUR
Handled manually via AUR PKGBUILD maintenance.

## Troubleshooting

### AppImage fails to run
```bash
# Check dependencies
./Winafi-*.AppImage --version

# Run with debug output
./Winafi-*.AppImage --debug
```

### Deb installation fails
```bash
# Check dependencies
dpkg -I winafi_*.deb | grep Depends

# Install missing dependencies
sudo apt install <dependency>
```

### Snap confinement issues
Check snap permissions:
```bash
sudo snap connect winafi:hardware-observe
sudo snap connect winafi:block-devices
```
