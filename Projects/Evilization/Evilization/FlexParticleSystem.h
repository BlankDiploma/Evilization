#include <d3d9.h>
#include <math.h>
#include <d3dx9.h>
#include <d3dx9math.h>
#include <assert.h>
#include "StructParse.h"
#include "TextureLibrary.h"
#include "earray.h"
#include "DefLibrary.h"
#include <DxErr.h>
#include "FlexRenderer.h"

struct FlexParticle
{
	D3DXVECTOR3 pos;
	const TCHAR* text;
	GameTexture* pFontTex;
	D3DXCOLOR color;
	DWORD lifetime;
	DWORD timeLeft;
	D3DXVECTOR3 vel;
};

class FlexParticleSystem
{
	FlexParticle** eaParticles;
public:
	void UpdateParticles(DWORD timeElapsed);
	//void AddParticle(FlexParticle newParticle);
	void AddParticle(GameTexture* pFontTex, const TCHAR* text, D3DXVECTOR3 pos, D3DXCOLOR color, DWORD lifetime, D3DXVECTOR3 vel);
	void RenderParticles();
};

extern FlexParticleSystem g_ParticleSystem;