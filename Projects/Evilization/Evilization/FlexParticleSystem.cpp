#include "stdafx.h"
#include "FlexParticleSystem.h"

void FlexParticleSystem::UpdateParticles(DWORD timeElapsed)
{
	for (int i = eaSize(&eaParticles)-1; i >= 0; i--)
	{
		if (eaParticles[i]->timeLeft > timeElapsed)
			eaParticles[i]->timeLeft -= timeElapsed;
		else
			eaParticles[i]->timeLeft = 0;
		if (eaParticles[i]->timeLeft <= 0)
		{
			eaRemove(&eaParticles, i);
		}
		else
		{
			for (int j = 0; j < 3; j++)
			{
				eaParticles[i]->pos[j] += eaParticles[i]->vel[j] * timeElapsed;
				eaParticles[i]->color.a *= 1.0f - ((1.0f/eaParticles[i]->lifetime) * timeElapsed);
			}
		}
	}
}

void FlexParticleSystem::AddParticle(GameTexture* pFontTex, const TCHAR* text, D3DXVECTOR3 pos, D3DXCOLOR color, DWORD lifetime, D3DXVECTOR3 vel)
{
	FlexParticle* newParticle = new FlexParticle;
	newParticle->pos = pos;
	newParticle->text = _wcsdup(text);
	newParticle->pFontTex = pFontTex;
	newParticle->color = color;
	newParticle->lifetime = lifetime;
	newParticle->timeLeft = lifetime;
	for (int i = 0; i < 3; i++)
	{
		newParticle->vel[i] = vel[i];
	}
	eaPush(&eaParticles, newParticle);
}

void FlexParticleSystem::RenderParticles()
{
	POINT screenPt;
	for (int i = eaSize(&eaParticles)-1; i >= 0; i--)
	{
		g_Renderer.WorldSpaceToScreen(&(eaParticles[i]->pos), &screenPt);
		g_Renderer.AddStringToRenderList(eaParticles[i]->pFontTex, eaParticles[i]->text, (float)screenPt.x, (float)screenPt.y, eaParticles[i]->color, false, 0, true);
	}
}

FlexParticleSystem g_ParticleSystem;