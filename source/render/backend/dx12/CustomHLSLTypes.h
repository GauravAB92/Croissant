#ifndef CUSTOM_HLSL_TYPEDEF_H
#define CUSTOM_HLSL_TYPEDEF_H
#include <glm.hpp>
#include <gtc/quaternion.hpp>

// Custom HLSL type definitions for use in C++ with glm


typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;
typedef glm::ivec2 int2;
typedef glm::ivec3 int3;
typedef glm::ivec4 int4;
typedef glm::mat2 float2x2;
typedef glm::mat3 float3x3;
typedef glm::mat4 float4x4;
typedef glm::mat2x3 float2x3;   
typedef glm::mat2x4 float2x4;
typedef glm::mat3x2 float3x2;
typedef glm::mat3x4 float3x4;
typedef glm::mat4x2 float4x2;
typedef glm::mat4x3 float4x3;
typedef glm::quat float4_quat; // Quaternion as a 4D vector
typedef glm::dvec2 double2;
typedef glm::dvec3 double3;
typedef glm::dvec4 double4;
typedef glm::dmat2 double2x2;
typedef glm::dmat3 double3x3;
typedef glm::dmat4 double4x4;
typedef glm::dmat2x3 double2x3;
typedef glm::dmat2x4 double2x4;
typedef glm::dmat3x2 double3x2;
typedef glm::dmat3x4 double3x4;
typedef glm::dmat4x2 double4x2;
typedef glm::dmat4x3 double4x3;
typedef glm::dquat double4_quat; // Quaternion as a 4D vector
typedef uint32_t   uint;
typedef glm::uvec2 uint2;   
typedef glm::uvec3 uint3;
typedef glm::uvec4 uint4;   


#endif // CUSTOM_HLSL_TYPEDEF_H