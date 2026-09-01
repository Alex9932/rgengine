struct VSINPUT {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : VPOS;
};

struct PSINPUT {
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 viewproj;
	row_major float4x4 model;
};

PSINPUT vmain(VSINPUT vin) {
	float3 pos    = vin.position;
	float4 pos4   = {pos.x, pos.y, pos.z, 1};
	float4 norm4  = {vin.normal.x, vin.normal.y, vin.normal.z, 0};

	PSINPUT vout;
	vout.position = mul(mul(pos4, model), viewproj);
	vout.normal   = normalize(mul(norm4, model).xyz);
	vout.uv       = vin.uv;
	return vout;
}