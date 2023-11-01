struct VSINPUT {
	float2 position : POSITION;
};

struct PSINPUT {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSINPUT vmain(VSINPUT vin) {
	float4 pos4   = {vin.position.x, -vin.position.y, 0, 1};
	PSINPUT vout;
	vout.position = pos4;
	vout.uv       = vin.position * 0.5 + 0.5;
	return vout;
}