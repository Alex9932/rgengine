struct INPUT {
	float4 position : SV_POSITION;
    float4 texcoord : TEXCOORD0;
};

struct OUTPUT {
    float4 position : SV_POSITION;
    float4 texcoord : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0) {
	row_major float4x4 proj;
	row_major float4x4 view;
};

[maxvertexcount(6)]
void gmain(point INPUT vinput[1], inout TriangleStream<OUTPUT> output) {
    INPUT vin = vinput[0];
    
    // Skip "dead" particles (vin.texcoord.z - lifetime)
    if (vin.texcoord.z < 0) {
        return;
    }

    // Quad vertices
    float2 fullSize    = float2(1, 1);
    float2 halfSize    = fullSize * 0.5;
    float2 vertices[4] = { float2( halfSize.x, -halfSize.y),
                           float2(-halfSize.x, -halfSize.y),
                           float2( halfSize.x,  halfSize.y),
                           float2(-halfSize.x,  halfSize.y) };
                           
    float2 tcoords[4]  = { float2( fullSize.x, fullSize.y),
                           float2( 0,          fullSize.y),
                           float2( fullSize.x, 0),
                           float2( 0,          0) };
    
    // Make model matrix
    float4x4 model_matrix = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    model_matrix[3][0] = vin.position.x;
    model_matrix[3][1] = vin.position.y;
    model_matrix[3][2] = vin.position.z;


    // Modelview matrix
    float4x4 modelview = mul(model_matrix, view);
    modelview[0][0] = 1;
    modelview[0][1] = 0;
    modelview[0][2] = 0;
    modelview[1][0] = 0;
    modelview[1][1] = 1;
    modelview[1][2] = 0;
    modelview[2][0] = 0;
    modelview[2][1] = 0;
    modelview[2][2] = 1;

    // Build output quad
    OUTPUT gout;
    gout.position = float4(0, 0, 0, 0);
    gout.texcoord = float4(0, 0, 0, 0);

    gout.texcoord.z = vin.texcoord.z;

    float scale = 1.0;
    float4 pos_4 = float4(0, 0, 0, 1);

    for(int i = 0; i < 4; i++) {
        gout.texcoord.xy = tcoords[i];
        pos_4.xy = vertices[i] * scale;
        gout.position = mul(mul(pos_4, modelview), proj);
        output.Append(gout);
    }
}