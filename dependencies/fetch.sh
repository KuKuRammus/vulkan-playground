#!/usr/bin/bash

# Clone glfw
echo "Fetching - GLFW"
git clone -b 3.3-stable git@github.com:glfw/glfw.git glfw

# Clone cglm
echo "Fetching - cglm"
git clone -b master git@github.com:recp/cglm.git cglm

