# Data Recovery Tool
## Rise project
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



## 📝 License

This project is licensed under the **Apache License 2.0**.  
See the [`LICENSE`](LICENSE) file for full details.
