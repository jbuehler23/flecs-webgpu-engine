/**
 * @file webgpu/dawn_impl.c
 * @brief Minimal Dawn WebGPU implementation for native builds.
 */

#include "../private_api.h"

#ifdef WEBGPU_BACKEND_DAWN

/* For now, we'll create a minimal stub implementation that allows compilation
   but prints warnings about missing functionality. This allows us to test
   the build system and focus on the web implementation first. */

#include <stdio.h>

/* Minimal Dawn stub implementations */
WGPUInstance wgpuCreateInstance(const WGPUInstanceDescriptor* descriptor) {
    (void)descriptor;
    printf("Dawn: wgpuCreateInstance called (stub)\n");
    return (WGPUInstance){.id = 1};
}

void wgpuInstanceRequestAdapter(WGPUInstance instance,
                               const WGPURequestAdapterOptions* options,
                               WGPURequestAdapterCallback callback,
                               void* userdata) {
    (void)instance;
    (void)options;
    printf("Dawn: wgpuInstanceRequestAdapter called (stub)\n");
    if (callback) {
        callback(WGPURequestAdapterStatus_Success, (WGPUAdapter){.id = 1}, NULL, userdata);
    }
}

WGPUDevice wgpuAdapterRequestDevice(WGPUAdapter adapter,
                                   const WGPUDeviceDescriptor* descriptor,
                                   WGPURequestDeviceCallback callback,
                                   void* userdata) {
    (void)adapter;
    (void)descriptor;
    printf("Dawn: wgpuAdapterRequestDevice called (stub)\n");
    if (callback) {
        callback(WGPURequestDeviceStatus_Success, (WGPUDevice){.id = 1}, NULL, userdata);
    }
    return (WGPUDevice){.id = 1};
}

WGPUQueue wgpuDeviceGetQueue(WGPUDevice device) {
    (void)device;
    return (WGPUQueue){.id = 1};
}

WGPUBuffer wgpuDeviceCreateBuffer(WGPUDevice device, const WGPUBufferDescriptor* descriptor) {
    (void)device;
    (void)descriptor;
    printf("Dawn: wgpuDeviceCreateBuffer called (stub)\n");
    return (WGPUBuffer){.id = 1};
}

void wgpuDeviceSetUncapturedErrorCallback(WGPUDevice device,
                                         WGPUErrorCallback callback,
                                         void* userdata) {
    (void)device;
    (void)callback;
    (void)userdata;
    printf("Dawn: wgpuDeviceSetUncapturedErrorCallback called (stub)\n");
}

/* Add more stub implementations as needed... */
void wgpuInstanceRelease(WGPUInstance instance) { (void)instance; }
void wgpuAdapterRelease(WGPUAdapter adapter) { (void)adapter; }
void wgpuDeviceRelease(WGPUDevice device) { (void)device; }
void wgpuQueueRelease(WGPUQueue queue) { (void)queue; }
void wgpuBufferRelease(WGPUBuffer buffer) { (void)buffer; }

#endif /* WEBGPU_BACKEND_DAWN */