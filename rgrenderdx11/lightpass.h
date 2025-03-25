#ifndef _LIGHTPASS_H
#define _LIGHTPASS_H

#include <rgvector.h>
#include <rgmatrix.h>
//#include "dx11.h"

#include <rendertypes.h>

#include "queue.h"

typedef struct GlobalLight {
	vec3    direction;
	vec3    color;
	Float32 ambient;
	Float32 intensity;
	Float32 turbidity;
	Float32 time;
} GlobalLight;

void CreateLightpass(ivec2* size);
void DestroyLightpass();
void ResizeLightpass(ivec2* size);

void ReloadShadersLightpass();

void DoLightpass(RQueue* lights);

void SetGlobalLight(GlobalLight* light);

struct ID3D11ShaderResourceView* GetLightpassShaderResource();

void  SetLightDescription(R3D_GlobalLightDescrition* desc);
mat4* GetLightMatrix();

vec3* GetSunPosition();
Float32 GetAtmosphereTurbidity();
Float32 GetSunTime();

#endif