extern "C" __global__ void add_one(int* out, const int* in, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i] + 1;
    }
}