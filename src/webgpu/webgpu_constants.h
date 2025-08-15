/**
 * @file webgpu/webgpu_constants.h
 * @brief WebGPU constants and descriptors needed for our implementation.
 */

#ifndef WEBGPU_CONSTANTS_H
#define WEBGPU_CONSTANTS_H

#ifdef WEBGPU_BACKEND_DAWN

/* Buffer usage flags */
#define WGPUBufferUsage_Vertex (1 << 0)
#define WGPUBufferUsage_Index (1 << 1) 
#define WGPUBufferUsage_Uniform (1 << 2)
#define WGPUBufferUsage_CopyDst (1 << 3)
#define WGPUBufferUsage_CopySrc (1 << 4)

/* Texture usage flags */
#define WGPUTextureUsage_TextureBinding (1 << 0)
#define WGPUTextureUsage_CopyDst (1 << 1)
#define WGPUTextureUsage_RenderAttachment (1 << 2)

/* Shader stage flags */
#define WGPUShaderStage_Vertex (1 << 0)
#define WGPUShaderStage_Fragment (1 << 1)

/* Color write mask */
#define WGPUColorWriteMask_All 0xF

/* Request adapter status */
typedef enum {
    WGPURequestAdapterStatus_Success = 0,
    WGPURequestAdapterStatus_Unavailable = 1,
    WGPURequestAdapterStatus_Error = 2
} WGPURequestAdapterStatus;

/* Request device status */
typedef enum {
    WGPURequestDeviceStatus_Success = 0,
    WGPURequestDeviceStatus_Error = 1
} WGPURequestDeviceStatus;

/* Error types */
typedef enum {
    WGPUErrorType_NoError = 0,
    WGPUErrorType_Validation = 1,
    WGPUErrorType_OutOfMemory = 2,
    WGPUErrorType_Internal = 3,
    WGPUErrorType_Unknown = 4,
    WGPUErrorType_DeviceLost = 5
} WGPUErrorType;

/* Power preference */
typedef enum {
    WGPUPowerPreference_Undefined = 0,
    WGPUPowerPreference_LowPower = 1,
    WGPUPowerPreference_HighPerformance = 2
} WGPUPowerPreference;

/* Texture formats */
typedef enum {
    WGPUTextureFormat_BGRA8Unorm = 0x80000000 + 4,
    WGPUTextureFormat_Depth24Plus = 0x80000000 + 13
} WGPUTextureFormat;

/* Index formats */
typedef enum {
    WGPUIndexFormat_Undefined = 0,
    WGPUIndexFormat_Uint16 = 1,
    WGPUIndexFormat_Uint32 = 2
} WGPUIndexFormat;

/* Vertex formats */
typedef enum {
    WGPUVertexFormat_Float32x3 = 0x80000000 + 3,
    WGPUVertexFormat_Float32x4 = 0x80000000 + 4,
    WGPUVertexFormat_Float32x2 = 0x80000000 + 2
} WGPUVertexFormat;

/* Vertex step modes */
typedef enum {
    WGPUVertexStepMode_Vertex = 0,
    WGPUVertexStepMode_Instance = 1
} WGPUVertexStepMode;

/* Primitive topology */
typedef enum {
    WGPUPrimitiveTopology_TriangleList = 0x80000000 + 4
} WGPUPrimitiveTopology;

/* Front face */
typedef enum {
    WGPUFrontFace_CCW = 0,
    WGPUFrontFace_CW = 1
} WGPUFrontFace;

/* Cull mode */
typedef enum {
    WGPUCullMode_None = 0,
    WGPUCullMode_Front = 1,
    WGPUCullMode_Back = 2
} WGPUCullMode;

/* Compare function */
typedef enum {
    WGPUCompareFunction_Less = 3
} WGPUCompareFunction;

/* Load/Store operations */
typedef enum {
    WGPULoadOp_Clear = 0,
    WGPULoadOp_Load = 1
} WGPULoadOp;

typedef enum {
    WGPUStoreOp_Store = 0,
    WGPUStoreOp_Discard = 1
} WGPUStoreOp;

/* Buffer binding types */
typedef enum {
    WGPUBufferBindingType_Uniform = 0
} WGPUBufferBindingType;

/* Blend operations */
typedef enum {
    WGPUBlendOperation_Add = 0
} WGPUBlendOperation;

/* Blend factors */
typedef enum {
    WGPUBlendFactor_SrcAlpha = 4,
    WGPUBlendFactor_OneMinusSrcAlpha = 5,
    WGPUBlendFactor_One = 1
} WGPUBlendFactor;

/* Texture dimension */
typedef enum {
    WGPUTextureDimension_2D = 1
} WGPUTextureDimension;

/* SType for chained structures */
typedef enum {
    WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector = 0x80000000 + 4,
    WGPUSType_ShaderModuleWGSLDescriptor = 0x80000000 + 6
} WGPUSType;

/* Callback types */
typedef void (*WGPURequestAdapterCallback)(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata);
typedef void (*WGPURequestDeviceCallback)(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata);
typedef void (*WGPUErrorCallback)(WGPUErrorType type, const char* message, void* userdata);

/* Descriptor structures */
typedef struct {
    const void* nextInChain;
} WGPUInstanceDescriptor;

typedef struct {
    const void* nextInChain;
    WGPUSurface compatibleSurface;
    WGPUPowerPreference powerPreference;
    bool forceFallbackAdapter;
} WGPURequestAdapterOptions;

typedef struct {
    const char* label;
    uint64_t size;
    WGPUBufferUsageFlags usage;
    bool mappedAtCreation;
} WGPUBufferDescriptor;

typedef struct {
    const void* nextInChain;
    const char* label;
} WGPUDeviceDescriptor;

typedef struct {
    const void* nextInChain;
    WGPUSType sType;
} WGPUChainedStruct;

/* Add more descriptors as needed... */

#endif /* WEBGPU_BACKEND_DAWN */

#endif /* WEBGPU_CONSTANTS_H */