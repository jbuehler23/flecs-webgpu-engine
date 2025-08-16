#!/usr/bin/env python3
"""
Simple HTTP server for Flecs WebGPU demo
Serves files with proper CORS headers for WebGPU
"""

import http.server
import socketserver
import os
import sys
from pathlib import Path

class CORSRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Enable CORS for all origins (needed for WebGPU)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        
        # Enable SharedArrayBuffer (needed for threading)
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        
        super().end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

def main():
    PORT = 8080
    
    # Change to the web-demo directory
    demo_dir = Path(__file__).parent
    os.chdir(demo_dir)
    
    print(f"🚀 Starting Flecs WebGPU demo server...")
    print(f"📂 Serving files from: {demo_dir}")
    print(f"🌐 Demo URL: http://localhost:{PORT}")
    print(f"🛑 Press Ctrl+C to stop")
    print()
    
    try:
        with socketserver.TCPServer(("", PORT), CORSRequestHandler) as httpd:
            print(f"✅ Server running on port {PORT}")
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Server stopped")
    except OSError as e:
        if e.errno == 48:  # Address already in use
            print(f"❌ Error: Port {PORT} is already in use")
            print(f"   Try: lsof -ti:{PORT} | xargs kill")
        else:
            print(f"❌ Error starting server: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()