struct VSINPUT {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : VPOS;
};

struct PSINPUT {
	float4 position : SV_POSITION;
	float3 vpos : VPOS;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 viewproj;
	row_major float4x4 model;
};

PSINPUT vmain(VSINPUT vin) {
	float4 pos4   = float4(vin.position, 1);
	float4 norm4  = float4(vin.normal, 0);
	float4 tang4  = float4(vin.tangent, 0);

	float3 N = mul(norm4, model).xyz;
	float3 T = mul(tang4, model).xyz;
	float3 B = normalize(cross(N, T));


	PSINPUT vout;
	vout.vpos     = mul(pos4, model).xyz;
	vout.position = mul(float4(vout.vpos, 1), viewproj);
	//vout.normal   = normalize(mul(norm4, model).xyz);

	vout.normal   = N;
	vout.tangent  = T;
	vout.binormal = B;

	vout.uv       = vin.uv;
	return vout;
}