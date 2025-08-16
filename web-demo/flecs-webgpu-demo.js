/**
 * Flecs WebGPU Engine Demo
 * Demonstrates web-native 3D rendering with ECS architecture
 */

class FlecsWebGPUDemo {
    constructor() {
        this.canvas = document.getElementById('canvas');
        this.world = null;
        this.entities = [];
        this.isRunning = false;
        this.lastFrameTime = 0;
        this.frameCount = 0;
        this.fps = 0;
        
        // Performance tracking
        this.perfStartTime = performance.now();
        
        // Camera state
        this.cameraRotation = 0;
        this.cameraDistance = 5;
        
        this.init();
    }
    
    async init() {
        try {
            await this.checkWebGPUSupport();
            await this.initializeEngine();
            this.setupEventHandlers();
            this.startRenderLoop();
            this.updateStatus('ready', '✅ Engine ready! Click buttons to add 3D objects.');
        } catch (error) {
            console.error('Demo initialization failed:', error);
            this.updateStatus('error', `❌ Failed to initialize: ${error.message}`);
        }
    }
    
    async checkWebGPUSupport() {
        const webgpuInfo = document.getElementById('webgpu-info');
        
        if (!navigator.gpu) {
            throw new Error('WebGPU not supported in this browser');
        }
        
        const adapter = await navigator.gpu.requestAdapter();
        if (!adapter) {
            throw new Error('No WebGPU adapter found');
        }
        
        const device = await adapter.requestDevice();
        const canvas = this.canvas;
        const context = canvas.getContext('webgpu');
        
        if (!context) {
            throw new Error('Failed to get WebGPU context');
        }
        
        // Configure canvas
        const canvasFormat = navigator.gpu.getPreferredCanvasFormat();
        context.configure({
            device: device,
            format: canvasFormat,
        });
        
        webgpuInfo.textContent = `✅ Adapter: ${adapter.info?.vendor || 'Available'}, Format: ${canvasFormat}`;
        
        // Store WebGPU objects for engine
        this.adapter = adapter;
        this.device = device;
        this.context = context;
        this.canvasFormat = canvasFormat;
    }
    
    async initializeEngine() {
        // For this demo, we'll simulate the Flecs WebGPU engine behavior
        // In a real implementation, this would load the WebAssembly module
        
        // Create a mock Flecs world
        this.world = {
            entities: [],
            components: new Map(),
            systems: []
        };
        
        // Initialize basic rendering pipeline
        await this.createRenderPipeline();
        
        // Add some initial entities
        this.addInitialScene();
    }
    
    async createRenderPipeline() {
        // Basic vertex shader for 3D cubes
        const vertexShaderSource = `
            struct VertexInput {
                @location(0) position: vec3<f32>,
                @location(1) color: vec3<f32>,
                @location(2) instancePos: vec3<f32>,
            }
            
            struct VertexOutput {
                @builtin(position) position: vec4<f32>,
                @location(0) color: vec3<f32>,
            }
            
            struct Uniforms {
                mvpMatrix: mat4x4<f32>,
            }
            @group(0) @binding(0) var<uniform> uniforms: Uniforms;
            
            @vertex
            fn vs_main(input: VertexInput) -> VertexOutput {
                var output: VertexOutput;
                let worldPos = input.position + input.instancePos;
                output.position = uniforms.mvpMatrix * vec4<f32>(worldPos, 1.0);
                output.color = input.color;
                return output;
            }
        `;
        
        const fragmentShaderSource = `
            @fragment
            fn fs_main(@location(0) color: vec3<f32>) -> @location(0) vec4<f32> {
                return vec4<f32>(color, 1.0);
            }
        `;
        
        // Create shaders
        const vertexShader = this.device.createShaderModule({
            label: 'Vertex Shader',
            code: vertexShaderSource
        });
        
        const fragmentShader = this.device.createShaderModule({
            label: 'Fragment Shader', 
            code: fragmentShaderSource
        });
        
        // Create uniform buffer
        this.uniformBuffer = this.device.createBuffer({
            size: 64, // 4x4 matrix
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        });
        
        // Create bind group layout
        const bindGroupLayout = this.device.createBindGroupLayout({
            entries: [{
                binding: 0,
                visibility: GPUShaderStage.VERTEX,
                buffer: { type: 'uniform' }
            }]
        });
        
        // Create bind group
        this.bindGroup = this.device.createBindGroup({
            layout: bindGroupLayout,
            entries: [{
                binding: 0,
                resource: { buffer: this.uniformBuffer }
            }]
        });
        
        // Create render pipeline
        this.renderPipeline = this.device.createRenderPipeline({
            label: 'Flecs WebGPU Demo Pipeline',
            layout: this.device.createPipelineLayout({
                bindGroupLayouts: [bindGroupLayout]
            }),
            vertex: {
                module: vertexShader,
                entryPoint: 'vs_main',
                buffers: [{
                    arrayStride: 24, // position(12) + color(12)
                    attributes: [
                        { format: 'float32x3', offset: 0, shaderLocation: 0 }, // position
                        { format: 'float32x3', offset: 12, shaderLocation: 1 }, // color
                    ]
                }, {
                    arrayStride: 12, // instancePos(12)
                    stepMode: 'instance',
                    attributes: [
                        { format: 'float32x3', offset: 0, shaderLocation: 2 }, // instancePos
                    ]
                }]
            },
            fragment: {
                module: fragmentShader,
                entryPoint: 'fs_main',
                targets: [{ format: this.canvasFormat }]
            },
            primitive: {
                topology: 'triangle-list',
                cullMode: 'back'
            },
            depthStencil: {
                depthWriteEnabled: true,
                depthCompare: 'less',
                format: 'depth24plus'
            }
        });
        
        // Create depth texture
        this.depthTexture = this.device.createTexture({
            size: [this.canvas.width, this.canvas.height],
            format: 'depth24plus',
            usage: GPUTextureUsage.RENDER_ATTACHMENT,
        });
        
        // Create cube geometry
        this.createCubeGeometry();
    }
    
    createCubeGeometry() {
        // Cube vertices (position + color)
        const vertices = new Float32Array([
            // Front face (red)
            -1, -1,  1,  1, 0, 0,
             1, -1,  1,  1, 0, 0,
             1,  1,  1,  1, 0, 0,
            -1,  1,  1,  1, 0, 0,
            // Back face (green)
            -1, -1, -1,  0, 1, 0,
            -1,  1, -1,  0, 1, 0,
             1,  1, -1,  0, 1, 0,
             1, -1, -1,  0, 1, 0,
            // Top face (blue)
            -1,  1, -1,  0, 0, 1,
            -1,  1,  1,  0, 0, 1,
             1,  1,  1,  0, 0, 1,
             1,  1, -1,  0, 0, 1,
            // Bottom face (yellow)
            -1, -1, -1,  1, 1, 0,
             1, -1, -1,  1, 1, 0,
             1, -1,  1,  1, 1, 0,
            -1, -1,  1,  1, 1, 0,
            // Right face (magenta)
             1, -1, -1,  1, 0, 1,
             1,  1, -1,  1, 0, 1,
             1,  1,  1,  1, 0, 1,
             1, -1,  1,  1, 0, 1,
            // Left face (cyan)
            -1, -1, -1,  0, 1, 1,
            -1, -1,  1,  0, 1, 1,
            -1,  1,  1,  0, 1, 1,
            -1,  1, -1,  0, 1, 1,
        ]);
        
        const indices = new Uint16Array([
            0, 1, 2,   0, 2, 3,    // front
            4, 5, 6,   4, 6, 7,    // back
            8, 9, 10,  8, 10, 11,  // top
            12, 13, 14, 12, 14, 15, // bottom
            16, 17, 18, 16, 18, 19, // right
            20, 21, 22, 20, 22, 23  // left
        ]);
        
        this.vertexBuffer = this.device.createBuffer({
            size: vertices.byteLength,
            usage: GPUBufferUsage.VERTEX,
            mappedAtCreation: true,
        });
        new Float32Array(this.vertexBuffer.getMappedRange()).set(vertices);
        this.vertexBuffer.unmap();
        
        this.indexBuffer = this.device.createBuffer({
            size: indices.byteLength,
            usage: GPUBufferUsage.INDEX,
            mappedAtCreation: true,
        });
        new Uint16Array(this.indexBuffer.getMappedRange()).set(indices);
        this.indexBuffer.unmap();
        
        this.indexCount = indices.length;
    }
    
    addInitialScene() {
        // Add a few initial cubes to showcase the system
        this.addEntity('box', [0, 0, 0]);
        this.addEntity('box', [3, 0, 0]);
        this.addEntity('box', [-3, 0, 0]);
    }
    
    addEntity(type, position) {
        const entity = {
            id: this.entities.length,
            type: type,
            position: position,
            color: [Math.random(), Math.random(), Math.random()]
        };
        
        this.entities.push(entity);
        this.updateInstanceBuffer();
        this.updateEntityCount();
        
        console.log(`Added ${type} entity at position:`, position);
        return entity;
    }
    
    updateInstanceBuffer() {
        if (this.entities.length === 0) return;
        
        // Create instance data (positions)
        const instanceData = new Float32Array(this.entities.length * 3);
        this.entities.forEach((entity, i) => {
            instanceData[i * 3 + 0] = entity.position[0];
            instanceData[i * 3 + 1] = entity.position[1];
            instanceData[i * 3 + 2] = entity.position[2];
        });
        
        // Update or create instance buffer
        if (this.instanceBuffer) {
            this.instanceBuffer.destroy();
        }
        
        this.instanceBuffer = this.device.createBuffer({
            size: instanceData.byteLength,
            usage: GPUBufferUsage.VERTEX,
            mappedAtCreation: true,
        });
        new Float32Array(this.instanceBuffer.getMappedRange()).set(instanceData);
        this.instanceBuffer.unmap();
    }
    
    setupEventHandlers() {
        // Scene controls
        document.getElementById('addBox').addEventListener('click', () => {
            const x = (Math.random() - 0.5) * 10;
            const z = (Math.random() - 0.5) * 10;
            this.addEntity('box', [x, 0, z]);
        });
        
        document.getElementById('addRectangle').addEventListener('click', () => {
            const x = (Math.random() - 0.5) * 10;
            const z = (Math.random() - 0.5) * 10;
            this.addEntity('rectangle', [x, 0.5, z]);
        });
        
        document.getElementById('clearScene').addEventListener('click', () => {
            this.entities = [];
            this.updateInstanceBuffer();
            this.updateEntityCount();
        });
        
        document.getElementById('randomColors').addEventListener('click', () => {
            this.entities.forEach(entity => {
                entity.color = [Math.random(), Math.random(), Math.random()];
            });
        });
        
        // View controls
        document.getElementById('rotateLeft').addEventListener('click', () => {
            this.cameraRotation -= 0.3;
        });
        
        document.getElementById('rotateRight').addEventListener('click', () => {
            this.cameraRotation += 0.3;
        });
        
        document.getElementById('zoomIn').addEventListener('click', () => {
            this.cameraDistance = Math.max(2, this.cameraDistance - 1);
        });
        
        document.getElementById('zoomOut').addEventListener('click', () => {
            this.cameraDistance = Math.min(20, this.cameraDistance + 1);
        });
        
        // Enable all buttons
        document.querySelectorAll('button').forEach(btn => btn.disabled = false);
    }
    
    updateStatus(type, message) {
        const status = document.getElementById('status');
        status.className = `status ${type}`;
        status.textContent = message;
    }
    
    updateEntityCount() {
        document.getElementById('entities').textContent = this.entities.length;
        document.getElementById('triangles').textContent = this.entities.length * 12; // 12 triangles per cube
    }
    
    startRenderLoop() {
        this.isRunning = true;
        this.lastFrameTime = performance.now();
        this.render();
    }
    
    render() {
        if (!this.isRunning) return;
        
        const now = performance.now();
        const deltaTime = now - this.lastFrameTime;
        this.lastFrameTime = now;
        
        // Update FPS
        this.frameCount++;
        if (now - this.perfStartTime >= 1000) {
            this.fps = Math.round((this.frameCount * 1000) / (now - this.perfStartTime));
            document.getElementById('fps').textContent = this.fps;
            this.frameCount = 0;
            this.perfStartTime = now;
        }
        
        // Update camera matrix
        this.updateCamera();
        
        // Render frame
        this.renderFrame();
        
        requestAnimationFrame(() => this.render());
    }
    
    updateCamera() {
        // Create view-projection matrix
        const time = performance.now() * 0.001;
        
        // View matrix (camera looking at origin)
        const cameraX = Math.sin(this.cameraRotation) * this.cameraDistance;
        const cameraZ = Math.cos(this.cameraRotation) * this.cameraDistance;
        const cameraY = 3;
        
        const view = this.createLookAtMatrix(
            [cameraX, cameraY, cameraZ], // eye
            [0, 0, 0], // target
            [0, 1, 0]  // up
        );
        
        // Projection matrix
        const aspect = this.canvas.width / this.canvas.height;
        const projection = this.createPerspectiveMatrix(Math.PI / 4, aspect, 0.1, 100);
        
        // Combined MVP matrix
        const mvp = this.multiplyMatrices(projection, view);
        
        // Update uniform buffer
        this.device.queue.writeBuffer(this.uniformBuffer, 0, mvp);
    }
    
    renderFrame() {
        if (this.entities.length === 0) return;
        
        const commandEncoder = this.device.createCommandEncoder();
        const textureView = this.context.getCurrentTexture().createView();
        
        const renderPassDescriptor = {
            colorAttachments: [{
                view: textureView,
                clearValue: { r: 0.1, g: 0.1, b: 0.2, a: 1.0 },
                loadOp: 'clear',
                storeOp: 'store',
            }],
            depthStencilAttachment: {
                view: this.depthTexture.createView(),
                depthClearValue: 1.0,
                depthLoadOp: 'clear',
                depthStoreOp: 'store',
            }
        };
        
        const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
        passEncoder.setPipeline(this.renderPipeline);
        passEncoder.setBindGroup(0, this.bindGroup);
        passEncoder.setVertexBuffer(0, this.vertexBuffer);
        passEncoder.setVertexBuffer(1, this.instanceBuffer);
        passEncoder.setIndexBuffer(this.indexBuffer, 'uint16');
        
        // Draw all instances
        passEncoder.drawIndexed(this.indexCount, this.entities.length);
        
        passEncoder.end();
        this.device.queue.submit([commandEncoder.finish()]);
    }
    
    // Math utility functions
    createLookAtMatrix(eye, target, up) {
        const zAxis = this.normalize(this.subtract(eye, target));
        const xAxis = this.normalize(this.cross(up, zAxis));
        const yAxis = this.cross(zAxis, xAxis);
        
        return new Float32Array([
            xAxis[0], yAxis[0], zAxis[0], 0,
            xAxis[1], yAxis[1], zAxis[1], 0,
            xAxis[2], yAxis[2], zAxis[2], 0,
            -this.dot(xAxis, eye), -this.dot(yAxis, eye), -this.dot(zAxis, eye), 1
        ]);
    }
    
    createPerspectiveMatrix(fovy, aspect, near, far) {
        const f = 1 / Math.tan(fovy / 2);
        const rangeInv = 1 / (near - far);
        
        return new Float32Array([
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (near + far) * rangeInv, -1,
            0, 0, near * far * rangeInv * 2, 0
        ]);
    }
    
    multiplyMatrices(a, b) {
        const result = new Float32Array(16);
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                result[i * 4 + j] = 
                    a[i * 4 + 0] * b[0 * 4 + j] +
                    a[i * 4 + 1] * b[1 * 4 + j] +
                    a[i * 4 + 2] * b[2 * 4 + j] +
                    a[i * 4 + 3] * b[3 * 4 + j];
            }
        }
        return result;
    }
    
    subtract(a, b) { return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]; }
    normalize(v) {
        const len = Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        return [v[0] / len, v[1] / len, v[2] / len];
    }
    cross(a, b) {
        return [
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        ];
    }
    dot(a, b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
}

// Initialize demo when page loads
window.addEventListener('load', () => {
    new FlecsWebGPUDemo();
});