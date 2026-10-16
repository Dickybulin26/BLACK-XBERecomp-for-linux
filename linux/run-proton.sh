#!/bin/bash
# ============================================================================
# Black - XBE Recomp :: Linux launcher (Proton)
# Runs the Windows-only BLACK.exe through Proton.
# D3D12 -> vkd3d-proton -> Vulkan -> your GPU.
#
# Usage:
#   GAME_DIR=~/Games/BLACK ./linux/run-proton.sh
#   GAME_DIR=/path/to/game PROTON_DIR=/path/to/proton ./linux/run-proton.sh
#   ./linux/run-proton.sh --help
#
# GAME_DIR must be the folder that contains BLACK.exe (not the repo folder,
# unless you put BLACK.exe there).
# ============================================================================
set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  GAME_DIR=~/Games/BLACK ./linux/run-proton.sh

Env vars:
  GAME_DIR    Folder containing BLACK.exe (default: repo root)
  PROTON_DIR  Path to Proton 'proton' script
              (default: $HOME/.local/share/Steam/steamapps/common/Proton Hotfix/proton)
  STEAM_DIR   Path to Steam install (default: $HOME/.local/share/Steam)

Examples:
  GAME_DIR=~/Games/BLACK ./linux/run-proton.sh
  GAME_DIR=~/Games/BLACK PROTON_DIR=~/.steam/steam/steamapps/common/Proton\ Hotfix/proton ./linux/run-proton.sh
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
GAME_DIR="${GAME_DIR:-$(dirname -- "$SCRIPT_DIR")}"
EXE="$GAME_DIR/BLACK.exe"
PROTON_DIR="${PROTON_DIR:-$HOME/.local/share/Steam/steamapps/common/Proton Hotfix/proton}"
STEAM_DIR="${STEAM_DIR:-$HOME/.local/share/Steam}"

if [ ! -x "$PROTON_DIR" ]; then
    echo "ERROR: Proton not found at:"
    echo "  $PROTON_DIR"
    echo
    echo "Fix:"
    echo "  1. Open Steam -> Library -> enable the 'Tools' filter."
    echo "  2. Search 'Proton Hotfix' -> Install."
    echo "  3. Retry this script. Or point to another Proton:"
    echo "     PROTON_DIR=/path/to/proton GAME_DIR=\"$GAME_DIR\" ./linux/run-proton.sh"
    exit 1
fi
if [ ! -f "$EXE" ]; then
    echo "ERROR: BLACK.exe not found at:"
    echo "  $EXE"
    echo
    echo "GAME_DIR must be the folder that contains BLACK.exe. Example layout:"
    echo "  ~/Games/BLACK/BLACK.exe"
    echo "  ~/Games/BLACK/d3dcompiler_47.dll"
    echo "  ~/Games/BLACK/assets/default.xbe"
    echo
    echo "Retry with:"
    echo "  GAME_DIR=~/Games/BLACK ./linux/run-proton.sh"
    echo
    echo "Current GAME_DIR contents ($GAME_DIR):"
    ls -la "$GAME_DIR" 2>/dev/null || echo "  (folder does not exist)"
    exit 1
fi

# --- Proton environment
export STEAM_COMPAT_DATA_PATH="$GAME_DIR/compatdata"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM_DIR"
mkdir -p "$STEAM_COMPAT_DATA_PATH" "$GAME_DIR/assets"

# --- D3D12 via vkd3d-proton
export PROTON_ENABLE_NVAPI=1
export VKD3D_FEATURE_LEVEL=12_0
export PROTON_HIDE_NVIDIA_GPU=0
export PROTON_ENABLE_NGX_UPDATER=1

# --- Wayland / Hyprland (uncomment if the window misbehaves)
# export PROTON_ENABLE_WAYLAND=1
# export PROTON_DXVK_FULLSCREEN_HACK=1

if [ ! -f "$GAME_DIR/d3dcompiler_47.dll" ]; then
    echo "WARNING: d3dcompiler_47.dll not found next to BLACK.exe."
    echo "  Copy it from the upstream Release zip. The game may still boot without it."
    echo
fi

if [ -z "$(ls -A "$GAME_DIR/assets" 2>/dev/null)" ]; then
    echo "INFO: $GAME_DIR/assets/ is empty."
    echo "  Put your own legally acquired dump there:"
    echo "  - the game ISO (USA or Europe), or"
    echo "  - default.xbe + GlobData.bin + chars/ data/ export/ language/ levels/ sound/ videos/"
    echo
    echo "  Continuing anyway so you can verify Proton boots..."
    echo
fi

echo "Game folder : $GAME_DIR"
echo "Proton      : $PROTON_DIR"
echo "Prefix      : $STEAM_COMPAT_DATA_PATH"
echo "GPU         : $(nvidia-smi -L 2>/dev/null || echo 'nvidia-smi not found (OK on AMD/Intel, check vulkaninfo)') "
echo "Starting BLACK.exe ..."
exec "$PROTON_DIR" run "$EXE"
