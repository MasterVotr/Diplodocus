# Acceleration Data Structures for Ray Tracing Algorithms on the GPU

## Overview

**Author**: Jakub Votrubec
**Supervisor**: Ivan Šimeček
**University**: Czech Technical University, Faculty of Informatics

This project is part of my master's thesis focused on **GPU-based acceleration data structures for ray tracing**.
The goal is to **research, implement, extend, and evaluate BVH construction algorithms** based on *Parallel Locally-Ordered Clustering (PLOC)*.

Specifically, the project explores:

* [**PLOC**](https://ieeexplore.ieee.org/abstract/document/7857089) (Meister et al. 2017 PLOC)
* [**Extended Morton Codes (EMC)**](https://dl.acm.org/doi/abs/10.1145/3105762.3105782) (Vinkler et al. 2017 Extended Morton Codes)
* [**SOBB (Skewed Oriented Bounding Boxes)**](https://onlinelibrary.wiley.com/doi/full/10.1111/cgf.70062) (Káčerik et al. 2025 SOBB)

## This project is a work-in-progress

* Functioning snapshot is on branch **main**
* The work-in-progress version is on branch **dev**

---

## Project Goals

* Implement **GPU-based BVH construction using PLOC**
* Extend PLOC with:
  * Extended Morton Codes (EMC)
  * SOBB
  * Combined EMC + SOBB approach *(novel combination)*
* Evaluate and compare all variants:
  * `PLOC`
  * `PLOC + EMC`
  * `PLOC + SOBB`
  * `PLOC + EMC + SOBB`
* The main evaluation metrics are:
  * BVH build time
  * Tracing performance
  * BVH cost

---

## Tech stack

* CPU code: **C++20** (Clang)
* GPU code: **CUDA 12** (NVCC)
* Build system: **CMake** 
* Configuration: **JSON** ([nlohmann](https://github.com/nlohmann/json))
* Scene/model format: **OBJ** ([tinyobjloader](https://github.com/tinyobjloader/tinyobjloader))
* Output image format: **PPM**

Planned (not yet implemented):

* GUI and debug visualisation: **OpenGL + ImGui** (GUI & debug visualization)
* Results export: **CSV**

---

## Project Structure

```bash
src/
  acceleration/     # CPU acceleration
  app/              # Driver code
  config/           # Configuration definition
  framebuffer/      # Framebuffer struct and logic
  gpu/              # CUDA kernels and GPU-specific code
  io/               # Config/scene loaders, framebuffer/results exporters
  renderer/         # Rendering logic and GPU frontend
  scene/            # Scene and primitive representation
  stats/            # Statistics definition
  util/             # Utilities

third-party/        # External dependencies
res/                # Scenes and metadata
out/                # Output renders

b.sh                # Build script
r.sh                # Run script (with sample_config.json)
sample_config.json  # Sample configuration
```

---

## Build & Run


1. Using scripts:

    ```bash
    ./b.sh
    ./r.sh # sample_config.json is used
    ```

2. Directly running the executable:

    ```
    ./build/diplodocus <json config path>
    ```

### Requirements:

* Clang 14+
* CUDA Toolkit 12 (NVCC)
* CMake (CMake 3.28+)
* git

### Configuration

* To change the configuration you can either use and modify `sample_config.json`, or create your own config file
* Unspecified values fall back to hardcoded deafults
* Yru can look up the default values in `config/` and the precise json formulation in `io/config/`)

---

## Current State

### Implemented

* CLI framework:

  * Config loading (JSON)
  * Scene setup
  * PPM image export

* **CPU Reference Renderer**

  * Whitted-style ray tracer
  * Metrics collection

* **CPU Reference Acceleration Data Structure**

  * SAH-based BVH inspired by an [article](https://ieeexplore.ieee.org/abstract/document/4342588) from Wald et al.
  * Metrics collection

* **GPU Infrastructure**

  * Scene representation and memory transfer
  * Ray and configuration structures for the GPU

* **GPU Ray Tracing (experimental)**

  * Stack-based Whitted ray tracer *(planned to be replaced)*
  * Basic path tracing prototype

* **Scenes**

  * Cornell Box
  * Cornell Box (sphere variant)
  * Stanford Bunny (in a Cornell Box)


### Work In Progress / Planned

* GPU BVH construction:

  * PLOC
  * PLOC + EMC
  * PLOC + SOBB
  * PLOC + EMC + SOBB

* GPU-side metrics:

  * Build statistics
  * Traversal statistics
  * Ray tracing performance

* Extended benchmarking:

  * More scenes
  * CSV export (optional)

* GUI application (optional stretch goal)

---

