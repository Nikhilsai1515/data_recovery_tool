# Data Recovery Tool

This tool was developed as part of my **Realtime Research Project** at university to recover lost and deleted files from **disk images** and **external drives**.

It features a user-friendly **Graphical User Interface (GUI)** built using **Dear ImGui**, making data recovery accessible and efficient. The tool supports:

- Automatic detection of connected physical drives
- Manual selection of disk image files for analysis
- A file-tree style view of the file system
- Visual indicators for deleted files, including:
  - Color-coded status
  - Overwrite percentage estimation

The backend uses **The Sleuth Kit (TSK)** for forensic disk analysis and **Dear ImGui** for the GUI frontend.

---

## 🏗️ Structure and Build Process

The project is written in **C/C++**.

- The **core backend**, written in C, handles analysis and recovery using **TSK**. This code resides in the `src/` directory.
- The **GUI components** and related assets (such as fonts) are located in the `gui/` directory and built using **Dear ImGui**.

The project is built using **Visual Studio 2019**, and a complete solution file is included in the repository.  
You may use your own versions of TSK and Dear ImGui, but static libraries are already provided for convenience.This project currently supports only Windows x64.

---

## 📦 Third-Party Libraries

This project includes the following third-party libraries:

- **Dear ImGui** – MIT License – See [`gui/imgui/LICENSE.txt`](gui/imgui/LICENSE.txt)
- **The Sleuth Kit (TSK)** – IBM Public License and others – See [`tsk/licenses/`](tsk/licenses/)
- **JetBrains Mono** – Apache 2.0 – See [`gui/fonts/jetbrains_mono/LICENSE.txt`](gui/fonts/jetbrains_mono/LICENSE.txt)
- **Font Awesome Free** – SIL Open Font License 1.1 (Fonts), MIT (Code) – See [`gui/fonts/font_awesome/LICENSE.txt`](gui/fonts/font_awesome/LICENSE.txt)

All third-party licenses are included both in their respective `vendor/` subdirectories and once collectively under `Third_Party_Licenses/` for convenience.

---

## 📝 License

This project is licensed under the **Apache License 2.0**.  
See the [`LICENSE`](LICENSE) file for full details.
