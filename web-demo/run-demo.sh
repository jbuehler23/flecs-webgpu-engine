#!/bin/bash

echo "🎮 Flecs WebGPU Engine - Web Demo"
echo "=================================="
echo ""
echo "🚀 Starting local web server..."
echo "📂 Demo files: $(pwd)"
echo "🌐 URL: http://localhost:8080"
echo ""
echo "⚠️  Requirements:"
echo "   • Chrome/Edge 94+ with WebGPU enabled"
echo "   • Go to chrome://flags and enable 'Unsafe WebGPU'"
echo ""
echo "🎯 Available Demos:"
echo "   • index.html - JavaScript simulation demo"
echo "   • flecs-native.html - Real Flecs WebGPU engine (WebAssembly)"
echo ""
echo "🎯 Native Demo Features:"
echo "   • Actual Flecs ECS with full component system"
echo "   • Native WebGPU rendering compiled from C"
echo "   • Real 3D graphics with WebGPU shaders"
echo "   • Component-based entity management"
echo ""
echo "🛑 Press Ctrl+C to stop the server"
echo ""

# Start the Python server
python3 serve.py