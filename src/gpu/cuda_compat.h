#pragma once

#ifdef __CUDACC__
#define G __global__
#define H __host__
#define D __device__
#define I __forceinline__
#define C __constant__
#define HD __host__ __device__
#define HDI __host__ __device__ __forceinline__
#define D __device__
#define DI __device__ __forceinline__
#else
#define G
#define H
#define D
#define I inline
#define C constexpr
#define HD
#define HDI inline
#define D
#define DI inline
#endif
