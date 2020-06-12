#!/usr/bin/bash

# ensure shader output directories exists
mkdir -p ./output/shader

# Build and compile shaders
make && \
    glslc ./src/shaders/shader.vert -o ./output/shader/vert.spv && \
    glslc ./src/shaders/shader.frag -o ./output/shader/frag.spv

# Ensure textures directory exists and copy textures
mkdir -p ./output/texture && \
    rm -rf ./output/texture/* && \
    cp ./src/textures/* ./output/texture


