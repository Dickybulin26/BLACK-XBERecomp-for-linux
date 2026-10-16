# Black - XBE Recomp
An unofficial native PC port of the original Xbox version of Black, with extended graphics options, keyboard and mouse support, and other enhancements.

This port was created using the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool.

> [!NOTE]
> This project does not include any game assets. You must provide the files from your own legally acquired copy of the game to install or build.

## Installation (Windows)
- Put your game files in the `assets` folder. This can be an ISO, or the extracted data files, of the USA or Europe release.
- Run `BLACK.exe`.
- Recomp settings can be adjusted in the in-game options menu while the game is running, or through the Settings overlay, which is opened with F10 by default.
- Linux users: skip this and follow [Linux (Proton)](#linux-proton) below.
<img width="1920" height="1080" alt="{5852D359-24A0-48C8-9319-07C3A619E355}" src="https://github.com/user-attachments/assets/e0156eb8-c1b5-49d9-ab93-3abb46bf5f47" />

## Linux (Proton)

This port ships as a Windows binary (`Direct3D 12` + `XAudio2`). On Linux we run that same binary through **Proton**, which translates D3D12 to Vulkan via `vkd3d-proton` so your GPU still does the real work. No game code is changed. The `linux/` folder contains the launcher.

Think of it as: `BLACK.exe` → `run-proton.sh` → Proton → your GPU.

### 1. What you need

- 64-bit Linux (tested on Arch/Hyprland, should work on Ubuntu/Fedora).
- A Vulkan-capable GPU with proprietary or Mesa drivers. Tested on NVIDIA RTX 3050 Laptop. Check yours:
  ```bash
  nvidia-smi -L        # NVIDIA
  vulkaninfo --summary # any vendor, install `vulkan-tools` if missing
  ```
- Steam installed and logged in.
- **Proton Hotfix** installed:
  1. Open Steam → `Library`.
  2. Enable the `Tools` filter (dropdown above the game list).
  3. Search `Proton Hotfix` → `Install`.
  4. Expected path afterwards: `~/.local/share/Steam/steamapps/common/Proton Hotfix/proton`.
- Your own legally acquired copy of Black (Xbox USA or Europe). This repo contains no game data.

### 2. Prepare the game folder

You need a folder that looks like this. It can live anywhere, e.g. `~/Games/BLACK/`:

```
~/Games/BLACK/
├── BLACK.exe            # from Releases, see below
├── d3dcompiler_47.dll   # ships next to BLACK.exe in the Release zip
├── black.toml           # optional, copy from linux/black.toml.default
└── assets/              # your game dump
    ├── default.xbe
    ├── GlobData.bin
    ├── chars/ data/ export/ language/ levels/ sound/ videos/
    └── (or a single .iso of the game disc)
```

Steps:

1. Clone this launcher repo:
   ```bash
   git clone https://github.com/Dickybulin26/BLACK-XBERecomp-for-linux.git
   cd BLACK-XBERecomp-for-linux
   ```
2. Download prebuilt `BLACK.exe` (+ `d3dcompiler_47.dll`) from [Upstream Releases](https://github.com/TGP482/BLACK-XBERecomp/releases). Upstream only ships Windows binaries.
3. Create your game folder and copy those files in:
   ```bash
   mkdir -p ~/Games/BLACK/assets
   cp BLACK.exe d3dcompiler_47.dll ~/Games/BLACK/
   ```
4. Copy your Xbox dump into `~/Games/BLACK/assets/`. Either the full extracted files (`default.xbe` + `GlobData.bin` + folders above) or the disc `.iso`.
5. Optional but recommended — copy default settings:
   ```bash
   cp linux/black.toml.default ~/Games/BLACK/black.toml
   ```

> [!NOTE]
> `GAME_DIR` in the commands below always means the folder that contains `BLACK.exe`, e.g. `~/Games/BLACK`. It is not the repo folder unless you put `BLACK.exe` there.

### 3. Run the game

```bash
GAME_DIR=~/Games/BLACK ./linux/run-proton.sh
```

What happens on first run:
- A Wine prefix is created in `~/Games/BLACK/compatdata/`. This is normal and takes ~30s.
- The game window opens. Press `F10` in-game to open Recomp Settings (resolution, fullscreen/borderless, mouse sensitivity, FPS cap).
- If `assets/` is empty the script warns you but still tries to boot so you can verify Proton works.

Make scripts executable if needed:
```bash
chmod +x linux/run-proton.sh linux/debug-proton.sh
```

### 4. Optional: app menu / rofi shortcut

Install a launcher entry with the BLACK cover art as its icon:

```bash
GAME_DIR=~/Games/BLACK ./linux/install-menu-entry.sh
```

- `GAME_DIR` tells it where `BLACK.exe` + `assets/` live (defaults to `~/Games/BLACK`).
- It copies `linux/black.png` to `~/.local/share/icons/black-xberecomp/` and writes a `.desktop` file to `~/.local/share/applications/`.
- Remove everything it installed with `./linux/install-menu-entry.sh --remove`.
- Black now appears in your app launcher / rofi.

### Troubleshooting

| Symptom | Fix |
|---|---|
| `ERROR: Proton not found` | Install Proton Hotfix (see step 1) or set `PROTON_DIR=/path/to/proton GAME_DIR=~/Games/BLACK ./linux/run-proton.sh`. |
| `ERROR: BLACK.exe not found` | You pointed `GAME_DIR` to the wrong folder. It must contain `BLACK.exe`. Example: `GAME_DIR=~/Games/BLACK`. |
| No window / crash on boot | Run `GAME_DIR=~/Games/BLACK ./linux/debug-proton.sh`, then open the newest `proton-*.log` in `GAME_DIR`. Look for `vkd3d`, `d3d12`, `err`. |
| Black screen but audio plays | GPU not picked up. Run `nvidia-smi dmon` while the game runs — you should see `BLACK.exe`. On laptops, force discrete GPU / disable iGPU offload. |
| Window misbehaves on Wayland/Hyprland | Uncomment `PROTON_ENABLE_WAYLAND=1` in `linux/run-proton.sh` and retry. |
| Save shows as **damaged** | Expected. Saves are signed with the original Xbox EEPROM key. Start a new save; do not reuse downloads from other consoles. See `hdd/E/UDATA/<title-id>/` under `GAME_DIR`. |

Still stuck? Include in your issue: distro + GPU + driver version, Proton version, and the `vkd3d/d3d12/err` lines from the debug log.

### Notes
- **Saves:** `$GAME_DIR/hdd/E/UDATA/<title-id>/`.
- **Settings:** `black.toml` next to `BLACK.exe`. See `linux/black.toml.default`.
- **Updating:** replace `BLACK.exe` with the newer Release, keep `assets/`, `black.toml`, `compatdata/`.
- **Uninstall:** `./linux/install-menu-entry.sh --remove`, then delete `GAME_DIR`. Nothing is installed to your system.

> [!IMPORTANT]
> This Linux launcher is a community add-on. It does not modify the game port itself. All game code, the recompiler, and the renderer come from the original project below — please support the upstream author.

## Features
### Display
- **Monitor** - Picks the monitor the game shows on, for Fullscreen and Borderless (and where Windowed opens). Lists every connected monitor by its Windows number and desktop resolution.
- **Display Mode** - Switch between Fullscreen, Borderless and Windowed.
- **Resolution** - Lists every mode your monitor reports and defaults to your desktop resolution, replacing the original 640x480 resolution.
- **Render Resolution** - Renders the game at 20% to 200% of the output resolution, allowing supersampling or downscaling independent of the window size.
- **Field of View** - Sets the horizontal field of view during gameplay from 45 to 140 degrees, with 70 being the game's default.
- **Viewmodel Field of View** - Adjusts the first-person weapon and arms field of view.
- **Max Frame Rate** - Caps the frame rate to your monitor's refresh rate or a custom value, or removes the cap entirely, replacing the Xbox's 30 FPS limiter.
- **V-Sync** - Reduces screen tearing.
- **HDR Output** - Enables HDR10 output on displays with Windows HDR turned on, detected automatically. Highlights extend up to the display's peak brightness.
- **FPS Counter** - Displays the frame rate on screen.
- **FMV Scale** - Choose between Fill, which zooms the 4:3 FMVs to fill widescreen displays, and 4:3, which keeps them at their original aspect ratio.

### Graphics
- **Multisample Antialiasing (MSAA)** - Enables 2x, 4x or 8x MSAA to smooth jagged edges while preserving a sharp image.
- **Subpixel Morphological Antialiasing (SMAA)** - Enables SMAA to smooth jagged edges across the whole frame with a softer image. SMAA also smooths alpha-tested edges such as fences and foliage, and can be combined with MSAA.
- **Anisotropic Filtering** - Forces the selected anisotropic filtering level on all textures.
- **Draw Distance** - Increases the level of detail and draw distances past the game's values: 2x, 4x or 8x.
- **Ambient Occlusion** - Adds screen space ambient occlusion, creating soft contact shadows where surfaces meet. Choose between SSAO and GTAO.
- **Reload Blur** - Toggles the blur effect while reloading.

### Input
- **Mouse and Keyboard** - Full keyboard and mouse support with raw input, adjustable sensitivity, inverted look and rebindable controls.
- **Controller** - Controller with SDL3 support. 

### General
- **Skip Intro** - Skips the intro logos.
- **Pause On Focus Loss** - Pauses the game and its sound while the window is in the background or minimized.

### Miscellaneous
- **Custom Menu Cursor** - Place a `cursor.png` (or `cursor.dds`) beside the exe to replace the Windows arrow in the game's menus.

## System Requirements
### Minimum
- **OS**
  - Windows 10 64-bit
- **CPU** with 2 cores and support for SSE4.2 and POPCNT
  - Intel Nehalem (Intel Core 1st Generation)
  - AMD Bulldozer (AMD FX series)
- **GPU** with support for Direct3D 12
  - NVIDIA GeForce GT 630 (Kepler)
  - AMD Radeon HD 7750
  - Intel HD Graphics 510 (Skylake)
- **Memory**
  - 4 GB
- **Storage**
  - 2 GB for the game files and saves

## Building
### 1. Clone the Repository
Clone this repository using [Git](https://git-scm.com/).
```
git clone https://github.com/TGP482/BLACK-XBERecomp.git
```

### 2. Add the Required Game Files
Place your extracted game data files in the `assets` folder. `default.xbe` from the USA release must be there, with the rest of the game's files beside it.


### 3. Install Dependencies
Install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) and select the following **Workloads** and **Individual components**:
- Desktop development with C++
- C++ Clang tools for Windows
- C++ CMake tools for Windows

> [!TIP]
> `build.bat` offers to install the Clang and CMake components for you if they're missing.

### 4. Build the Project
- Run `build.bat`. The first build recompiles the entire game and can take 30 to 90 minutes on weaker systems.
- When it finishes, run `out\build\release\BLACK.exe`.

## Credits
- **[TGP482](https://github.com/TGP482)** — original author of [BLACK - XBE Recomp](https://github.com/TGP482/BLACK-XBERecomp) and the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool. All game code, the recompiler, the Direct3D 12 renderer and every feature listed above are their work.
- This repo is an unofficial fork that adds only a **Linux launcher** (`linux/`) wrapping the upstream Windows build in Proton. No game code was changed.
- If you enjoy this port, please star and support the [upstream project](https://github.com/TGP482/BLACK-XBERecomp).
