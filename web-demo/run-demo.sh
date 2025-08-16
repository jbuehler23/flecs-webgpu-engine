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
echo "🎯 Demo Features:"
echo "   • Add 3D boxes and rectangles"
echo "   • Real-time camera controls"
echo "   • Performance monitoring"
echo "   • ECS-based entity management"
echo ""
echo "🛑 Press Ctrl+C to stop the server"
echo ""

# Start the Python server
python3 serve.py