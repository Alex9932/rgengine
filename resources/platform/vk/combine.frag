#version 430 core

layout(location = 0) in vec2 o_position;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler smplr;
layout(set = 1, binding = 0) uniform texture2D t_unit0;
layout(set = 2, binding = 0) uniform texture2D t_unit1;

vec3 toneMapACES(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 toneMapUchimura(vec3 x, float P, float a, float m, 
                     float l, float c, float b) {
    // P - максимум яркости
    // a - наклон плеча
    // m - наклон линейной части
    // l - переходная точка
    // c - черная точка
    // b - яркость черной точки
    
    float l0 = ((P - m) * l) / a;
    float S0 = m + m * a / (P - m);
    float S1 = m + (1.0 - m) * a / (P - m);
    float C2 = (a * P) / (P - m);
    float CP = -C2 / P;
    
    vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    vec3 w2 = vec3(step(m + l0, x));
    vec3 w1 = vec3(1.0 - w0 - w2);
    
    vec3 T = m * pow(x / m, vec3(c)) + vec3(b);
    vec3 S = P - (P - S1) * exp(CP * (x - m - l0));
    vec3 L = m + a * (x - m);
    
    return T * w0 + L * w1 + S * w2;
}

// Упрощенная версия Uchimura
vec3 toneMapUchimuraSimple(vec3 x) {
    const float P = 1.0;  // пиковая яркость
    const float a = 1.0;  // наклон плеча
    const float m = 0.22; // линейный наклон
    const float l = 0.4;  // начало плеча
    const float c = 1.33; // контраст черной точки
    const float b = 0.0;  // черная точка
    
    return toneMapUchimura(x, P, a, m, l, c, b);
}

vec3 toneMapAgX(vec3 x) {
    // Насыщение
    const mat3 saturationMat = mat3(
        0.842479, 0.078433, 0.079223,
        0.042328, 0.878469, 0.079166,
        0.042375, 0.078433, 0.879422
    );
    
    x = saturationMat * x;
    
    // Sigmoid
    const float cutoff = 0.027; // черная точка
    x = max(vec3(0.0), x - cutoff);
    x = pow(x, vec3(0.9)); // небольшая гамма-коррекция
    
    return x / (x + 0.59); // sigmoid
}

vec3 toneMapGT(vec3 x) {
    float A = 0.22; // плечо
    float B = 0.30; // линейная часть
    float C = 0.10; // черная точка
    float D = 0.20; // насыщенность
    float E = 0.01; // белая точка
    float F = 0.30; // яркость белого
    
    return ((x * (A * x + C * B) + D * E) / 
            (x * (A * x + B) + D * F)) - E / F;
}

vec3 toneMapReinhardJodie(vec3 x) {
    float luma = dot(x, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = x / (1.0 + x);
    return mix(x / (1.0 + luma), tc, tc);
}

vec3 linearToACEScg(vec3 linear) {
    mat3 ACEScgInputMat = mat3(
        0.613097, 0.339523, 0.047379,
        0.070194, 0.916354, 0.013452,
        0.020616, 0.109570, 0.869814
    );
    return ACEScgInputMat * linear;
}

vec3 saturateColor(vec3 color, float saturation) {
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luma), color, saturation);
}

void main() {

	vec2 uv = o_position * 0.5 + 0.5;
	uv.y = 1.0 - uv.y; // Flip final image
	
	vec4 u0 = texture(sampler2D(t_unit0, smplr), uv);
	vec4 u1 = texture(sampler2D(t_unit1, smplr), uv); // Blur

	vec3 lColor;
	//lColor = pow(u0.rgb, vec3(1.0 / 2.2)); // And apply gamma correction
	lColor = u0.rgb;
	lColor += u1.rgb; // Add blurred image on top

	color.rgb = saturateColor(toneMapACES(lColor), 1.05);
	color.a = 1;
}