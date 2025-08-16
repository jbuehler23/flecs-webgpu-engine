#!/bin/bash

echo "Flecs WebGPU Engine - Web Demo"
echo "=================================="
echo ""
echo "Starting local web server..."
echo "Demo files: $(pwd)"
echo "URL: http://localhost:8080"
echo ""
echo "Requirements:" 
echo "  - Chrome/Edge 94+ with WebGPU enabled"
echo "  - Enable 'Unsafe WebGPU' at chrome://flags"
echo ""
echo "Available demos:"
echo "  - index.html    (JavaScript simulation)"
echo "  - flecs-native.html (WebAssembly build)"
echo ""
echo "Native demo features:"
echo "  - Flecs ECS with component system"
echo "  - WebGPU rendering compiled from C"
echo "  - 3D rendering with WGSL shaders"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Start the Python server
python3 serve.py