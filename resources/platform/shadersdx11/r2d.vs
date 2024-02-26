struct VSINPUT {
	float2 position : POSITION;
	float2 uv : VPOS;
	float4 color : COLOR;
};

struct PSINPUT {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 model;
};

PSINPUT vmain(VSINPUT vin) {
	float4 pos4   = float4(vin.position, 0, 1);

	PSINPUT vout;
	vout.position = mul(float4(pos4), model);
    vout.color    = vin.color;
	vout.uv       = vin.uv;
	return vout;
}