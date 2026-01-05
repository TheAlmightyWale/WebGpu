## Overview

This is a 2D Renderer built using WebGPU, currently marked as a work-in-progress (WIP). The project implements a modern graphics rendering pipeline for 2D sprite-based animations and terrain rendering.

## Core Technologies

### Graphics API

WebGPU - The next-generation web graphics API, providing modern GPU features
WGSL (WebGPU Shading Language) - For shader programming

### Build System

CMake (minimum version 3.25) - Cross-platform build system
CMake Presets - For standardized build configurations
Programming Language
C++20 - Modern C++ with strict standards compliance
Compiler warnings treated as errors for code quality

## Development Setup

The project uses CMake with development mode enabled by default, which:

Loads resources from the source tree for live editing
Enables hot reload for MSVC compilers
Provides detailed error reporting
In release mode, resources are loaded relative to the executable for portability.

## Features

### Current Implementation

✅ WebGPU-based rendering pipeline
✅ Sprite animation system with multiple concurrent animations
✅ Instanced rendering for efficient sprite batching
✅ Texture array support for animation frames
✅ 2D camera system
✅ Depth testing for sprite layering
✅ Alpha blending for transparency
✅ Gamma-correct rendering

### Planned Features (Roadmap)

🔲 Loading and displaying multiple animations simultaneously
🔲 Debug text rendering
🔲 Sprite/texture atlasing for improved performance
