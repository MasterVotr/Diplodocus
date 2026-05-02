#!/usr/bin/env bash

set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "This script expects an apt-based Linux/WSL system." >&2
    exit 1
fi

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    clang \
    git \
    pkg-config \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    xorg-dev

if command -v nvcc >/dev/null 2>&1; then
    echo "CUDA toolkit found: $(nvcc --version | tail -n 1)"
else
    echo "CUDA toolkit not found. Install CUDA 12.x (or load the cluster CUDA module) before building." >&2
fi

echo "Installed build dependencies."
echo "The repository vendors rapidobj, nlohmann/json, GLFW, GLAD, ImGui, and tomlplusplus."
