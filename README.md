# rgengine

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg?style=for-the-badge)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D7.svg?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green.svg?style=for-the-badge)

**rgengine** is a 3D game engine written in **C++**, featuring a modular architecture, **JavaScript scripting**, and a modern **DirectX 11** rendering pipeline.

---

## 🚀 About

rgengine is an experimental 3D engine built from the ground up as a personal project focused on learning, performance, and creative freedom.  
It’s designed to be both a **sandbox for engine development** and a **foundation for real projects**.

---

## 🧠 Tech Stack

- **Language:** C++
- **Scripting:** JavaScript
- **Rendering:** DirectX 11 (HLSL)
- **Shaders:** HLSL
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

---

## 🧩 Planned Features

- Physics simulation
- Networking / multiplayer
- Extended editor tools
- Improved PBR pipeline
- Vulkan backend with ray tracing support

---

## ⚙️ Build

Currently, **rgengine** is built using **Microsoft Visual Studio**.  
A **Premake5**-based build system is planned to enable easy project generation for multiple platforms and compilers (MSVC, GCC, Clang, etc).

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

*(c) 2018 - 2025 rgengine project*
