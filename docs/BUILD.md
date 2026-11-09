# DangoPlayer Switch Build Instructions

## Prerequisites

### 1. Install devkitPro

**Windows:**
1. Download the graphical installer from: https://github.com/devkitPro/installer/releases
2. Run the installer and check **"Switch Development"** during component selection
3. The installer includes MSYS2 — use it for all build commands

**Linux / macOS:**
```bash
# Follow devkitPro's official setup for your OS, then:
sudo dkp-pacman -S switch-dev
```

### 2. Install Required Libraries (portlibs)

Open the MSYS2 terminal (Windows) or your regular terminal (Linux/macOS) and run:

```bash
# Graphics & UI
dkp-pacman -S switch-sdl2 switch-sdl2_ttf switch-sdl2_image switch-sdl2_gfx switch-sdl2_mixer

# Graphics drivers (required by SDL2)
dkp-pacman -S switch-mesa switch-glad

# Font rendering
dkp-pacman -S switch-freetype

# Image format support
dkp-pacman -S switch-libpng switch-libjpeg-turbo switch-libwebp

# Compression
dkp-pacman -S switch-zlib switch-bzip2

# Media & Video Playback (MPV, FFmpeg, Subtitles)
dkp-pacman -S switch-mpv switch-ffmpeg switch-libass switch-libfribidi switch-dav1d

# Networking & TLS (IPTV, Web Streams, curl)
dkp-pacman -S switch-curl switch-mbedtls
```

### 3. Bundled Assets (Included)

The project already includes:
- **`icon.jpg`**: 256×256 app icon featuring the Dango theme for the Homebrew Menu.
- **`romfs/fonts/font.ttf`**: Google Noto Sans TTF font embedded directly into the `.nro` via RomFS.

---

## Building

### Method 1: Local devkitPro (Standard)

```bash
# In MSYS2 terminal (Windows) or bash (Linux/macOS):
make
```

This compiles all sources, packages RomFS, embeds `icon.jpg`, and outputs `DangoPlayerSwitch.nro` and `DangoPlayerSwitch.nacp`.

### Method 2: Docker (No local devkitPro installation needed)

If you have Docker installed on Windows, Linux, or macOS:

```bash
# On Windows:
build_docker.bat

# On Linux / macOS:
chmod +x build_docker.sh
./build_docker.sh
```

### Method 3: GitHub Actions (Cloud Build)

If you push this project to a GitHub repository, the included workflow `.github/workflows/build.yml` will automatically build `DangoPlayerSwitch.nro` on every push and make it downloadable under the **Actions** tab as an artifact!

---

### Clean build artifacts

```bash
make clean
```

---

## Deploying to Nintendo Switch

### Option A: SD Card (Manual)

1. Copy `DangoPlayerSwitch.nro` to your SD card at:
   ```
   sdmc:/switch/DangoPlayerSwitch/DangoPlayerSwitch.nro
   ```
2. Launch Homebrew Menu and select DangoPlayer Switch

### Option B: nxlink (WiFi — recommended for development)

1. On the Switch, open Homebrew Menu and note the IP address shown
2. On your PC:
   ```bash
   nxlink -a <switch_ip> DangoPlayerSwitch.nro
   ```
3. The app launches on the Switch, and stdout/stderr are redirected to your PC terminal for debugging

---

## Important: Title Override Mode

> ⚠️ **For media playback, always launch in Title Override mode!**

Homebrew launched from the Album (Applet Mode) gets limited RAM (~400MB). For video decoding, you need full system RAM (~3.2GB).

**How to launch in Title Override mode:**
1. Hold the **R** button
2. While holding R, launch any installed game from the Home Menu
3. The Homebrew Menu opens in full-RAM mode
4. Select DangoPlayer Switch

You can configure this in `sdmc:/atmosphere/config/override_config.ini`.

---

## Custom Icon

Place a 256×256 JPEG image as `icon.jpg` in the project root. It will be automatically included in the NRO and shown in the Homebrew Menu.
