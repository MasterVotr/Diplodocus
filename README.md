# Acceleration Data Structures for Ray Tracing Algorithms on the GPU

<!--toc:start-->
- [Overview](#overview)
- [Project goals](#project-goals)
- [Tech stack](#tech-stack)
- [Project structure](#project-structure)
- [Build & run](#build-run)
  - [Requirements:](#requirements)
  - [Configuration](#configuration)
- [Project state](#project-state)
- [Potential extensions](#potential-extensions)
<!--toc:end-->

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

Licence can be found in [licence.txt](licence.txt)

---

## Project goals

* Implement **GPU-based BVH construction using PLOC**
* Extend PLOC with:
  * Extended Morton Codes (EMC) - two versions
  * SOBB
  * Combined EMC + SOBB approach *(novel combination)*
* Evaluate and compare all variants:
  * `PLOC`
  * `PLOC + EMC v1`
  * `PLOC + EMC v2`
  * `PLOC + SOBB`
  * `PLOC + EMC v1 + SOBB`
  * `PLOC + EMC v2 + SOBB`
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
* Scene/model format: **OBJ** ([rapidobj](https://github.com/guybrush77/rapidobj))
* Output image format: **PPM**
* Metrics export: **Console** & **JSON** ([nlohmann](https://github.com/nlohmann/json))
* Benchmark orchestation: **Python**

---

## Project structure

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
experiments/        # Benchmark orchestration, benchmark results
report/             # Master's thesis report and report source files

b.sh                # Build script
r.sh                # Run script (with sample_config.json)
sample_config.json  # Sample configuration
```

---

## Build & run


1. Using scripts:

    ```bash
    ./b.sh <preset> # choose debug/release preset
    ./r.sh          # sample_config.json is used
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
* You can look up the default values in `config/` and the precise json formulation in `io/config/`)

---

## Project state

* **CLI framework**

  * Config loading (JSON)
  * Scene loading (OBJ)
  * PPM image export
  * Collected metrics export (Console or JSON)

* **CPU reference renderer**

  * Whitted-style ray tracer
  * Metrics collection

* **CPU reference acceleration data structure**

  * SAH-based BVH inspired by an [article](https://ieeexplore.ieee.org/abstract/document/4342588) from Wald et al.
  * Metrics collection

* **GPU infrastructure**

  * Scene representation and memory transfer
  * Ray and configuration structures for the GPU

* **GPU ray tracing**

  * Whitted-style non-stack ray tracer
  * Metrics collection

* **GPU BVH construction** 

  * PLOC
  * PLOC + EMC v1
  * PLOC + EMC v2
  * PLOC + SOBB
  * PLOC + EMC v1 + SOBB
  * PLOC + EMC v2 + SOBB
  * Metrics collection

* **Scenes**

  * Cornell Box
  * Cornell Box (sphere variant)
  * Stanford Bunny (in a Cornell Box)

* **Extended benchmarking**

  * Metrics collection structures
  * Console export
  * JSON export
  * Orechstration using Python with data collection into CSV

---

## Potential extensions

* GUI application
* Automatic documentation

---

