#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include "phong.h"

using namespace optix;

rtDeclareVariable(float3,       Ka, , );
rtDeclareVariable(float3,       Kd, , );
rtDeclareVariable(float3,       Ks, , );
rtDeclareVariable(float3,       Kr, , );
rtDeclareVariable(float,        phong_exp, , );

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 


RT_PROGRAM void any_hit_shadow()
{
  phongShadowed();
}


RT_PROGRAM void closest_hit_radiance()
{
  float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
  float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );

  float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );
  phongShade( Kd, Ka, Ks, Kr, phong_exp, ffnormal );
}


rtTextureSampler<float4, 2> Kd_map;
rtDeclareVariable(float3, texcoord, attribute texcoord, ); 

RT_PROGRAM void closest_hit_radiance_textured()
{
  float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
  float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
  
  float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

  const float3 Kd_val = make_float3( tex2D( Kd_map, texcoord.x, texcoord.y ) );
  phongShade( Kd_val, Ka, Ks, Kr, phong_exp, ffnormal );
}
