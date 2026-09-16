![](https://raw.githubusercontent.com/Alex9932/rgengine/master/resources/platform/rgengine%20logo.png)
# rgengine

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-0078D7.svg)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D7.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**rgengine** is a 3D game engine written in **C++**, featuring a modular architecture, **JavaScript scripting**, and a modern **Vulkan 1.3** and **DirectX 11** rendering pipelines.

---

## 🚀 About

rgengine is an experimental 3D engine built from the ground up as a personal project focused on learning, performance, and creative freedom.  
It’s designed to be both a **sandbox for engine development** and a **foundation for real projects**.

---

## 🧠 Tech Stack

- **Language:** C++
- **Scripting:** JavaScript
- **Rendering:** Vulkan / DirectX 11
- **Shaders:** GLSL
- **UI:** ImGui
- **Platform:** Windows | Linux (Console tools only)

---

## ✨ Features

- **Rendering**
  - Physically-based shading
  - **Screen-Space Reflections (SSR)**
  - **Skeletal animation** support
  - **ImGui** integration for in-engine debugging and tools
  - **Custom material system** ~~with real-time parameter editing~~

- **Audio**
  - Integrated 3D audio playback system

- **Scripting**
  - **JavaScript** scripting for game logic and entitys behavior

- **Asset System**
  - Custom model format (`.pm2`) with support for meshes, materials, tangents, and normals
  - Automatic import/export to `gamedata/models` and `gamedata/textures`

- **Utilities**
  - Built-in debug tools
  - Model tools
  - Scene editing
  - HLSL generator based on SPIRV-Cross

---

## 🧩 Planned Features

- Physics simulation
- Networking / multiplayer
- Extended editor tools
- Improved PBR pipeline
- Vulkan backend with ray tracing support

---

## ⚙️ Build

This project uses [Premake5](https://premake.github.io/) to generate build files.

### 1. Prepare

Install Premake5

- **Windows**: download `premake5.exe` and add it to your `PATH`.
- **Linux / macOS**: use your package manager or download the binary.

Install dependencies

On Windows

```bash
vcpkg install
```

On Linux

Use your package manager to install dev libraries:
- sdl3
- mujs
- cjson
- iconv
- assimp
- freetype
- zlib
- nativefiledialog-extended
- openal-soft
- bullet3


### 2. Generate project files

From the project root:

```bash
premake5 gmake2      # Makefile (Linux/macOS)
premake5 vs2026      # Visual Studio 2026 (Windows)
premake5 xcode4      # Xcode (macOS)
```

### 3. Build

- **Makefile**: run `make`
- **Visual Studio**: open the generated `.sln` and build.
- **Xcode**: open the project and build.

---

## 📜 License

MIT License — free for personal and commercial use.

---

## 📸 Screenshots

![](https://raw.githubusercontent.com/Alex9932/rgengine/master/resources/platform/screenshot2.png)
Sponza Palace rendered by rgengine

![](https://raw.githubusercontent.com/Alex9932/rgengine/master/resources/platform/screenshot.png)
Screenshot from older version of rgengine

---

*(c) 2018 - 2026 rgengine project*
