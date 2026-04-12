#pragma once

#ifdef __CUDACC__
#define HD __host__ __device__
#define HDI __host__ __device__ __forceinline__
#define D __device__
#define DI __device__ __forceinline__
#else
#define HD
#define HDI
#define D
#define DI
#endif
