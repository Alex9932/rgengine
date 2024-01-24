struct VSINPUT {
	float3 position : POSITION;
};

struct PSINPUT {
	float4 position : SV_POSITION;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 viewproj;
	row_major float4x4 model;
};

PSINPUT vmain(VSINPUT vin) {
	float4 pos4 = float4(vin.position, 1);
	float4 vpos = mul(pos4, model);

	PSINPUT vout;
	vout.position = mul(vpos, viewproj);
	return vout;
}