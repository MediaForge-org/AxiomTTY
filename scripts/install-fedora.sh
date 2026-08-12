#!/usr/bin/env bash
set -euo pipefail

sudo dnf install -y \
  gcc-c++ \
  cmake \
  ninja-build \
  qt6-qtbase-devel \
  qt6-qtdeclarative-devel
