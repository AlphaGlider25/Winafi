#!/bin/bash
# Publish Winafi Beta-V0.0.4 Release

set -e

REPO="AlphaGlider25/Winafi"
TAG="Beta-V0.0.4"
APPIMAGE="Winafi-4.0.0-x86_64.AppImage"

if [ ! -f "$APPIMAGE" ]; then
    echo "Error: $APPIMAGE not found in current directory"
    exit 1
fi

echo "Publishing release for tag $TAG..."
echo ""
echo "This script requires a GitHub personal access token with repo permissions."
echo "You can create one at: https://github.com/settings/tokens"
echo ""
echo "Method 1: Using gh CLI (recommended)"
echo "  gh auth login -h github.com"
echo "  gh release create $TAG -t 'Winafi Beta-V0.0.4 - Feature Complete' --notes '...' $APPIMAGE"
echo ""
echo "Method 2: Using curl (requires GITHUB_TOKEN env variable)"
echo "  export GITHUB_TOKEN='your_token_here'"
echo "  curl -X POST -H 'Authorization: token \$GITHUB_TOKEN' -H 'Content-Type: application/octet-stream' \\"
echo "    --data-binary @$APPIMAGE \\"
echo "    'https://uploads.github.com/repos/$REPO/releases/$(git rev-list --count $TAG..HEAD)/assets?name=$APPIMAGE'"
echo ""
echo "Method 3: Manual upload via GitHub web interface"
echo "  1. Go to: https://github.com/$REPO/releases/tag/$TAG"
echo "  2. Click 'Edit release'"
echo "  3. Upload $APPIMAGE file"
echo ""
echo "To authenticate with gh CLI now, run:"
echo "  gh auth login -h github.com"
