struct OUTPUT {
	float4 position : SV_POSITION;
    float4 texcoord : TEXCOORD0;
};

struct Particle {
	float4 pos_lifetime;
	float4 vel_mul;
};

StructuredBuffer<Particle> particles : register(t0);

OUTPUT vmain(uint id : SV_InstanceID) {
	OUTPUT vout;
	vout.position = float4(particles[id].pos_lifetime.xyz, 1);
    vout.texcoord = float4(0, 0, particles[id].pos_lifetime.w, 0);
	return vout;
}