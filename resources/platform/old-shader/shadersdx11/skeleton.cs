struct R3D_Vertex {
    float3 pos;
    float3 norm;
    float3 tang;
    float2 uv;
};

struct R3D_Weight {
    float4 weight;
    int4   idx;
};

struct Matrix {
    row_major float4x4 m;
};

// Input
StructuredBuffer<Matrix>     matrices : register(t0); // Bone matrices
StructuredBuffer<R3D_Vertex> vertices : register(t1); // Vertices
StructuredBuffer<R3D_Weight> weights  : register(t2); // Weights

// Output
RWStructuredBuffer<R3D_Vertex> output : register(u0); // Vertices

#define MAX_BONES 1024

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {

    int idx = DTid.x;

    R3D_Vertex in_vtx = vertices[idx];
    R3D_Weight in_wgt = weights[idx];

    float4 total_position = { 0, 0, 0, 0 };
    float3 total_normal   = { 0, 0, 0 };
    float3 total_tangent  = { 0, 0, 0 };

    for(int i = 0; i < 4; i++) {
        float w  = in_wgt.weight[i];
        int   id = in_wgt.idx[i];

        float4 pos4 = float4(in_vtx.pos, 1);

        if (id == -1) { continue; }
        if (id >= MAX_BONES) {
            total_position = pos4;
            total_normal   = in_vtx.norm;
            total_tangent  = in_vtx.tang;
            break;
        }

        float4x4 mat  = matrices[id].m;
        float3x3 mat3 = (float3x3)mat;

        float4 local_position = mul(pos4, mat);
        float3 local_normal   = mul(in_vtx.norm, mat3);
        float3 local_tangent  = mul(in_vtx.tang, mat3);

        total_position += local_position * w;
        total_normal   += local_normal   * w;
        total_tangent  += local_tangent  * w;
    }

    output[idx].pos  = total_position.xyz;
    output[idx].norm = total_normal;
    output[idx].tang = total_tangent;
    output[idx].uv   = vertices[idx].uv;

}