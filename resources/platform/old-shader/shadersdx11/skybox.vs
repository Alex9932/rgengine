struct VSINPUT {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : VPOS;
};

struct PSINPUT {
	float4 position : SV_POSITION;
	float3 vpos : VPOS;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 viewproj;
	row_major float4x4 model;
};

PSINPUT vmain(VSINPUT vin) {
	float4 pos4   = float4(vin.position, 1);
	PSINPUT vout;
	vout.vpos     = mul(pos4, model).xyz;
	vout.position = mul(float4(vout.vpos, 1), viewproj);
	return vout;
}