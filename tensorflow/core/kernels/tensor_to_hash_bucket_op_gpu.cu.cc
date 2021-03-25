/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if GOOGLE_CUDA

#define EIGEN_USE_GPU
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/kernels/tensor_to_hash_bucket_op.h"
#include "third_party/farmhash_gpu/src/farmhash_gpu.h"
#include "tensorflow/core/util/gpu_kernel_helper.h"

namespace tensorflow {

namespace {

constexpr int kSharedMemBufferSizePerThread = 64;

template<typename T>
__device__ __forceinline__ void FillDigits(T val, int num_digits, int* i,
                                           char *buf) {
  eigen_assert(num_digits <= kSharedMemBufferSizePerThread - (*i));

  int factor = (val < 0 ? -1: 1);

  int num_digits_a = num_digits;
  do {
    int digit = static_cast<int>((val % 10) * factor);
    buf[(*i) + num_digits - 1] = digit + '0';
    val /= 10;
    num_digits--;
  } while(val != 0);

  (*i) += num_digits_a;
}

template<typename T>
__device__ __forceinline__ int IntegerToString(T val, char *buf) {
  int num_digits = 0;
  T val_a = val;
  do {
    val_a = val_a / 10;
    num_digits++;
  } while(val_a != 0);

  int i = 0;
  if (val < 0) {
    buf[i++] = '-';
  }

  FillDigits(val, num_digits, &i, buf);
  
  return i;
}

template<typename T>
__global__ void ComputeHashes(const T* __restrict__ vals, int vals_size,
                              int64 num_buckets, int64* __restrict__ hashes) {
  extern __shared__ char s[];

  GPU_1D_KERNEL_LOOP(tid, vals_size) {
    int size = IntegerToString(
        vals[tid], s + threadIdx.x * kSharedMemBufferSizePerThread);
    uint64_t a_hash = ::util_gpu::Fingerprint64(
        s + threadIdx.x * kSharedMemBufferSizePerThread, size);
    int64 a_bucket = static_cast<int64>(a_hash % num_buckets);
    hashes[tid] = a_bucket;
  }
}

} // end namespace

namespace functor {

template <typename T>
void LaunchTensorToHashBucket<Eigen::GpuDevice, T>::operator()(
         OpKernelContext* c, const int64 num_buckets, const T* input,
         const int num_elems, int64* output) {
  const Eigen::GpuDevice& d = c->eigen_gpu_device();
  if (num_elems > 0) {
    constexpr int32 kThreadsInBlock = 128;
    auto shared_memory_bytes =
        kThreadsInBlock * kSharedMemBufferSizePerThread * sizeof(char);
    GpuLaunchConfig config = GetGpuLaunchConfigFixedBlockSize(
        num_elems, d, ComputeHashes<T>, shared_memory_bytes, kThreadsInBlock);
    OP_REQUIRES_OK(c, GpuLaunchKernel(
        ComputeHashes<T>, config.block_count, config.thread_per_block,
        shared_memory_bytes, d.stream(), input, num_elems, num_buckets,
        output));
  }
}

} // namespace functor

#define REGISTER_FUNCTORS(type)                           \
    template struct functor::LaunchTensorToHashBucket<    \
        Eigen::GpuDevice, type>;

TF_CALL_INTEGRAL_TYPES(REGISTER_FUNCTORS);

#undef REGISTER_FUNCTORS

} // namespace tensorflow
#endif // GOOGLE_CUDA

