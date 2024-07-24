#ifndef __VFCINSTRUMENTONLINE_VECTOR_TYPES_H__
#define __VFCINSTRUMENTONLINE_VECTOR_TYPES_H__

#ifdef __GCC__
#warning "GCC"
typedef float float2 __attribute__((ext_vector_type(8)));
typedef float float4 __attribute__((ext_vector_type(16)));
typedef float float8 __attribute__((ext_vector_type(32)));
typedef float float16 __attribute__((ext_vector_type(64)));
typedef float float32 __attribute__((ext_vector_type(128)));
typedef float float64 __attribute__((ext_vector_type(256)));
typedef double double2 __attribute__((ext_vector_type(16)));
typedef double double4 __attribute__((ext_vector_type(32)));
typedef double double8 __attribute__((ext_vector_type(64)));
typedef double double16 __attribute__((ext_vector_type(128)));
typedef double double32 __attribute__((ext_vector_type(256)));
typedef double double64 __attribute__((ext_vector_type(512)));
typedef int32_t int2 __attribute__((ext_vector_type(8)));
typedef int32_t int4 __attribute__((ext_vector_type(16)));
typedef int32_t int8 __attribute__((ext_vector_type(32)));
typedef int32_t int16 __attribute__((ext_vector_type(64)));
typedef int32_t int32 __attribute__((ext_vector_type(128)));
typedef int32_t int64 __attribute__((ext_vector_type(256)));
typedef int64_t long2 __attribute__((ext_vector_type(16)));
typedef int64_t long4 __attribute__((ext_vector_type(32)));
typedef int64_t long8 __attribute__((ext_vector_type(64)));
typedef int64_t long16 __attribute__((ext_vector_type(128)));
typedef int64_t long32 __attribute__((ext_vector_type(256)));
typedef int64_t long64 __attribute__((ext_vector_type(512)));
typedef uint32_t uint2 __attribute__((ext_vector_type(8)));
typedef uint32_t uint4 __attribute__((ext_vector_type(16)));
typedef uint32_t uint8 __attribute__((ext_vector_type(32)));
typedef uint32_t uint16 __attribute__((ext_vector_type(64)));
typedef uint32_t uint32 __attribute__((ext_vector_type(128)));
typedef uint32_t uint64 __attribute__((ext_vector_type(256)));
typedef uint64_t ulong2 __attribute__((ext_vector_type(16)));
typedef uint64_t ulong4 __attribute__((ext_vector_type(32)));
typedef uint64_t ulong8 __attribute__((ext_vector_type(64)));
typedef uint64_t ulong16 __attribute__((ext_vector_type(128)));
typedef uint64_t ulong32 __attribute__((ext_vector_type(256)));
typedef uint64_t ulong64 __attribute__((ext_vector_type(512)));
#elif defined(__clang__)
#warning "clang"
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef float float32 __attribute__((ext_vector_type(32)));
typedef float float64 __attribute__((ext_vector_type(64)));
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));
typedef double double32 __attribute__((ext_vector_type(32)));
typedef double double64 __attribute__((ext_vector_type(64)));
typedef int32_t int2 __attribute__((ext_vector_type(2)));
typedef int32_t int4 __attribute__((ext_vector_type(4)));
typedef int32_t int8 __attribute__((ext_vector_type(8)));
typedef int32_t int16 __attribute__((ext_vector_type(16)));
typedef int32_t int32 __attribute__((ext_vector_type(32)));
typedef int32_t int64 __attribute__((ext_vector_type(64)));
typedef int64_t long2 __attribute__((ext_vector_type(2)));
typedef int64_t long4 __attribute__((ext_vector_type(4)));
typedef int64_t long8 __attribute__((ext_vector_type(8)));
typedef int64_t long16 __attribute__((ext_vector_type(16)));
typedef int64_t long32 __attribute__((ext_vector_type(32)));
typedef int64_t long64 __attribute__((ext_vector_type(64)));
typedef uint32_t uint2 __attribute__((ext_vector_type(2)));
typedef uint32_t uint4 __attribute__((ext_vector_type(4)));
typedef uint32_t uint8 __attribute__((ext_vector_type(8)));
typedef uint32_t uint16 __attribute__((ext_vector_type(16)));
typedef uint32_t uint32 __attribute__((ext_vector_type(32)));
typedef uint32_t uint64 __attribute__((ext_vector_type(64)));
typedef uint64_t ulong2 __attribute__((ext_vector_type(2)));
typedef uint64_t ulong4 __attribute__((ext_vector_type(4)));
typedef uint64_t ulong8 __attribute__((ext_vector_type(8)));
typedef uint64_t ulong16 __attribute__((ext_vector_type(16)));
typedef uint64_t ulong32 __attribute__((ext_vector_type(32)));
typedef uint64_t ulong64 __attribute__((ext_vector_type(64)));
#else
#error "Unsupported compiler"
#endif

#endif
