#!/usr/bin/bash

# ensure shader output directories exists
mkdir -p ./output/shader

make && \
    glslc ./src/shaders/shader.vert -o ./output/shader/vert.spv && \
    glslc ./src/shaders/shader.frag -o ./output/shader/frag.spv

