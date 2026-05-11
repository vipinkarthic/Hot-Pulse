#include <cuda_runtime.h>
#include <cstdint>

#define TILE 128
#define CLAMP_LO -3.0f
#define CLAMP_HI 3.0f

struct Params {
    float scale;
    float bias;
    int mode;
};

__device__ __forceinline__ float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

__device__ __forceinline__ float warp_sum(float v) {
    for (int offset = 16; offset > 0; offset >>= 1) {
        v += __shfl_down_sync(0xffffffff, v, offset);
    }
    return v;
}

template <typename T>
__device__ __forceinline__ T blend(T a, T b, int mode) {
    switch (mode) {
        case 0: return a + b;
        case 1: return a - b;
        case 2: return a * b;
        default: return a;
    }
}

__device__ float nonlinear(float x) {
    float y = __sinf(x) + __cosf(x * 0.5f);
    return clampf(y, CLAMP_LO, CLAMP_HI);
}

extern "C" __global__ void fused_transform(
    float* __restrict__ out,
    const float* __restrict__ in0,
    const float* __restrict__ in1,
    const Params* __restrict__ params,
    int n,
    float* __restrict__ block_sums) {

    __shared__ float shared[TILE];

    int tid = threadIdx.x;
    int gid = blockIdx.x * blockDim.x + tid;
    int stride = blockDim.x * gridDim.x;

    float local = 0.0f;
    float scale = params->scale;
    float bias = params->bias;
    int mode = params->mode;

    for (int i = gid; i < n; i += stride) {
        float a = in0[i];
        float b = in1[i];
        float v = blend(a, b, mode);
        v = fmaf(v, scale, bias);
        v = nonlinear(v);
        out[i] = v;
        local += v;
    }

    shared[tid] = local;
    __syncthreads();

    for (int offset = blockDim.x / 2; offset >= 32; offset >>= 1) {
        if (tid < offset) {
            shared[tid] += shared[tid + offset];
        }
        __syncthreads();
    }

    if (tid < 32) {
        float v = shared[tid];
        v = warp_sum(v);
        if (tid == 0) {
            block_sums[blockIdx.x] = v;
        }
    }
}

extern "C" __global__ void initialize_float_array(
    float* __restrict__ out,
    int n) {

    constexpr float values[] = {
        0.0f, 1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f, 9.0f
    };

    int gid = blockIdx.x * blockDim.x + threadIdx.x;
    if (gid >= n) {
        return;
    }

    out[gid] = values[gid % (sizeof(values) / sizeof(values[0]))];
}

extern "C" __global__ void prefix_mask(
    const float* __restrict__ in,
    unsigned int* __restrict__ out,
    int n,
    float threshold) {

    int gid = blockIdx.x * blockDim.x + threadIdx.x;
    if (gid >= n) {
        return;
    }

    float x = in[gid];
    unsigned int mask = (x > threshold) ? 1u : 0u;
    out[gid] = mask;

    if ((gid & 31) == 0) {
        atomicAdd(out, mask);
    }
}

extern "C" __global__ void gather_even_indices(
    const int* __restrict__ in,
    int* __restrict__ out,
    int n,
    int* __restrict__ counter) {

    int gid = blockIdx.x * blockDim.x + threadIdx.x;
    if (gid >= n) {
        return;
    }

    int v = in[gid];
    if ((v & 1) == 0) {
        int idx = atomicAdd(counter, 1);
        out[idx] = v;
    }
}