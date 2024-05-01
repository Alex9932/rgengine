struct INPUT {
	float4 position : SV_POSITION;
    float4 texcoord : TEXCOORD0;
};

struct OUTPUT {
    float4 position : SV_POSITION;
    float4 texcoord : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 viewproj;
};

[maxvertexcount(6)]
void gmain(point INPUT vinput[1], inout TriangleStream<OUTPUT> output) {

    float2 halfSize    = float2(0.5f, 0.5f);
    float2 vertices[4] = { float2(-halfSize.x, -halfSize.y), float2(-halfSize.x,  halfSize.y),
                           float2( halfSize.x,  halfSize.y), float2(halfSize.x, -halfSize.y) };

    INPUT vin = vinput[0];

    if (vin.texcoord.z < 0) {
        return;
    }

    OUTPUT gout;
    gout.position = float4(0, 0, 0, 0);
    gout.texcoord = float4(0, 0, 0, 0);

    gout.texcoord.z = vin.texcoord.z;

    gout.texcoord.xy = vertices[0];
    gout.position = mul(float4(vin.position.xy + vertices[0], vin.position.z, 1.0f), viewproj);
    output.Append(gout);
    gout.texcoord.xy = vertices[1];
    gout.position = mul(float4(vin.position.xy + vertices[1], vin.position.z, 1.0f), viewproj);
    output.Append(gout);
    gout.texcoord.xy = vertices[2];
    gout.position = mul(float4(vin.position.xy + vertices[2], vin.position.z, 1.0f), viewproj);
    output.Append(gout);

    gout.texcoord.xy = vertices[3];
    gout.position = mul(float4(vin.position.xy + vertices[3], vin.position.z, 1.0f), viewproj);
    output.Append(gout);
    gout.texcoord.xy = vertices[2];
    gout.position = mul(float4(vin.position.xy + vertices[2], vin.position.z, 1.0f), viewproj);
    output.Append(gout);
    gout.texcoord.xy = vertices[0];
    gout.position = mul(float4(vin.position.xy + vertices[0], vin.position.z, 1.0f), viewproj);
    output.Append(gout);
}