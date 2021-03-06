/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2015, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "C4FoW.h"

#include <float.h>


C4FoW::C4FoW()
	: pLights(NULL)
{
}

C4Shader *C4FoW::GetFramebufShader()
{
#ifndef USE_CONSOLE
	// Not created yet?
	if (!FramebufShader.Initialised())
	{

		// Create the frame buffer shader. The details are in C4FoWRegion, but
		// this is about how to utilise old frame buffer data in the lights texture.
		// Or put in other words: This shader is responsible for fading lights out.
		FramebufShader.AddVertexDefaults();
		FramebufShader.AddTexCoord("texCoord");
		FramebufShader.AddFragmentSlice(-1, "uniform sampler2D tex;");
		FramebufShader.AddFragmentSlice(0,
			"gl_FragColor = texture2D(tex, texCoord.st);");
		const char *szUniforms[] = { "tex", NULL };
		if (!FramebufShader.Init("framebuf", szUniforms)) {
			FramebufShader.ClearSlices();
			return NULL;
		}

	}
	return &FramebufShader;
#else
	return NULL;
#endif
}

void C4FoW::Add(C4Object *pObj)
{
#ifndef USE_CONSOLE
	// No view range? Probably want to remove instead
	if(!pObj->lightRange && !pObj->lightFadeoutRange)
	{
		Remove(pObj);
		return;
	}

	// Look for matching light
	C4FoWLight *pLight;
	for (pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;

	if (pLight)
	{

		// Update reach and light color
		pLight->SetReach(pObj->lightRange, pObj->lightFadeoutRange);
		pLight->SetColor(pObj->lightColor);
	}
	else
	{
		// Create new light otherwise
		pLight = new C4FoWLight(pObj);
		pLight->pNext = pLights;
		pLights = pLight;
	}
#endif
}

void C4FoW::Remove(C4Object *pObj)
{
#ifndef USE_CONSOLE
	// Look for matching light
	C4FoWLight *pPrev = NULL, *pLight;
	for (pLight = pLights; pLight; pPrev = pLight, pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;
	if (!pLight)
		return;

	// Remove
	(pPrev ? pPrev->pNext : pLights) = pLight->getNext();
	delete pLight;
#endif
}

void C4FoW::Invalidate(C4Rect r)
{
#ifndef USE_CONSOLE
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Invalidate(r);
#endif
}

void C4FoW::Update(C4Rect r, C4Player *pPlr)
{
#ifndef USE_CONSOLE
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->IsVisibleForPlayer(pPlr))
			pLight->Update(r);
#endif
}

void C4FoW::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen, C4Player *pPlr)
{
#ifndef USE_CONSOLE
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->IsVisibleForPlayer(pPlr))
			pLight->Render(pRegion, pOnScreen);
#endif
}
