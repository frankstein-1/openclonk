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

#ifndef USE_CONSOLE

#include "C4FoWDrawStrategy.h"
#include "C4FoWLight.h"
#include "C4FoWRegion.h"
#include "C4DrawGL.h"

enum C4DrawPass
{
	C4DP_First = 0,
	C4DP_Second = 1,
	C4DP_Color = 2,
	C4DP_Last
};

void C4FoWDrawLightTextureStrategy::Begin(int32_t passPar)
{
	pass = passPar;

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	float width = region->getSurface()->Wdt;
	float height = region->getSurface()->Hgt / 2.0;

	// Set up blend equation, see C4FoWDrawLightTextureStrategy::DrawVertex
	// for details.
	switch (pass)
	{
		case C4DP_First:
			glBlendFunc(GL_ONE, GL_ONE);
			glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
			glScissor(0, height, width, height);
			break;
		case C4DP_Second:
			glBlendFunc(GL_ONE, GL_ONE);
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			glScissor(0, height, width, height);
			break;
		case C4DP_Color:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
			glScissor(0, 0, width, height);
			break;
		default:
			assert(!"Unexpected value for light drawing pass");
			break;
	}

	glEnable(GL_SCISSOR_TEST);
}

void C4FoWDrawLightTextureStrategy::End(int32_t pass)
{
	glBlendEquation( GL_FUNC_ADD );
	glDisable(GL_SCISSOR_TEST);
}

void C4FoWDrawLightTextureStrategy::DrawVertex(float x, float y, bool shadow)
{

	// Here's the master plan for updating the lights texture. We want to
	//
	//  1. sum up the normals, weighted by intensity
	//  2. take over intensity maximum as new intensity
	//
	// where intensity is in the A channel and normals are in the GB channels.
	// Normals are obviously meant to be though of as signed, though,
	// so the equation we want would be something like
	//
	//  A_new = max(A_old, A);
	//  G_new = BoundBy(G_old + G - 0.5, 0.0, 1.0);
	//  B_new = BoundBy(B_old + B - 0.5, 0.0, 1.0);
	//
	// It seems we can't get that directly though - glBlendFunc only talks
	// about two operands. Even if we make two passes, we have to take
	// care that that we don't over- or underflow in the intermediate pass.
	//
	// Therefore, we store G/1.5 instead of G, losing a bit of accuracy,
	// but allowing us to formulate the following approximation without
	// overflows:
	//
	//  G_new = BoundBy(BoundBy(G_old + G / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)
	//  B_new = BoundBy(BoundBy(B_old + B / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)

	float y_offset = 0.0f;

	switch (pass)
	{
		case C4DP_First:
			if (shadow)
			{
				float dx = x - light->getX();
				float dy = y - light->getY();
				float dist = sqrt(dx*dx+dy*dy);
				float bright = light->getBrightness();
				float mult = std::min(0.5f / light->getNormalSize(), 0.5f / dist);
				float normX = Clamp(0.5f + dx * mult, 0.0f, 1.0f) / 1.5f;
				float normY = Clamp(0.5f + dy * mult, 0.0f, 1.0f) / 1.5f;
				glColor4f(0.0f, normX,     normY,     bright);
			}
			else
			{
				glColor4f(0.0f, 0.5f / 1.5f, 0.5f / 1.5f, 0.0f);
			}
			break;
		case C4DP_Second:
			glColor4f(0.0f, 0.5f / 1.5f, 0.5f / 1.5f, 0.0f);
			break;
		case C4DP_Color: // has a block so that alpha is scoped to this block only
		{
			y_offset = region->getSurface()->Hgt / 2;

			float alpha; // 0.0 == fully transparent (takes old color), 1.0 == solid color (takes new color)

			if (shadow) // draw the center of the light
			{
				alpha = 0.3 + 0.6 * light->getValue() * light->getLightness();
			}
			else // draw the edge of the light
			{
				alpha = 0.0;
			}

			glColor4f(light->getR(), light->getG(), light->getB(), alpha);
			break;
		}
		default:
			glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
			break;
	}

	// global coords -> region coords
	x += -region->getRegion().x;
	y += -region->getRegion().y + y_offset;

	glVertex2f(x, y);
}

void C4FoWDrawLightTextureStrategy::DrawDarkVertex(float x, float y)
{
	DrawVertex(x,y, false);
}

void C4FoWDrawLightTextureStrategy::DrawLightVertex(float x, float y)
{
	DrawVertex(x,y, true);
}

void C4FoWDrawWireframeStrategy::Begin(int32_t pass)
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

void C4FoWDrawWireframeStrategy::End(int32_t pass)
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glBlendEquation( GL_FUNC_ADD );
}

void C4FoWDrawWireframeStrategy::DrawVertex(float x, float y)
{
	// global coords -> screen pos and zoom
	x += screen->X - screen->TargetX;
	y += screen->Y - screen->TargetY;
	pGL->ApplyZoom(x,y);
	glVertex2f(x,y);
}

void C4FoWDrawWireframeStrategy::DrawDarkVertex(float x, float y)
{
	switch(phase)
	{
	case P_None:         return;
	case P_Fade:         glColor3f(0.0f, 0.5f, 0.0f); break;
	case P_Intermediate: glColor3f(0.0f, 0.0f, 0.5f); break;
	default:             assert(false); // only fade has dark vertices
	}
	DrawVertex(x, y);
}

void C4FoWDrawWireframeStrategy::DrawLightVertex(float x, float y)
{
	switch(phase)
	{
	case P_None:         return;
	case P_Fan:          glColor3f(1.0f, 0.0f, 0.0f); break;
	case P_FanMaxed:     glColor3f(1.0f, 1.0f, 0.0f); break;
	case P_Fade:         glColor3f(0.0f, 1.0f, 0.0f); break;
	case P_Intermediate: glColor3f(0.0f, 0.0f, 1.0f); break;
	}
	DrawVertex(x, y);
}

#endif
