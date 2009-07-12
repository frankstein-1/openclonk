/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2005-2006  Sven Eberhardt
 * Copyright (c) 2005-2008  Günther Brammer
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* OpenGL implementation of NewGfx */

#include <Standard.h>
#include <StdGL.h>
#include <StdSurface2.h>
#include <StdWindow.h>

#ifdef USE_GL

#include <stdio.h>
#include <math.h>
#include <limits.h>

static void glColorDw(DWORD dwClr)
	{
	glColor4ub(GLubyte(dwClr>>16), GLubyte(dwClr>>8), GLubyte(dwClr), GLubyte(dwClr>>24));
	}

// GLubyte (&r)[4] is a reference to an array of four bytes named r.
static void DwTo4UB(DWORD dwClr, GLubyte (&r)[4])
	{
	//unsigned char r[4];
	r[0] = GLubyte(dwClr>>16);
	r[1] = GLubyte(dwClr>>8);
	r[2] = GLubyte(dwClr);
	r[3] = GLubyte(dwClr>>24);
	}

CStdGL::CStdGL()
	{
	Default();
	// global ptr
	pGL = this;
	shaders[0] = 0;
	vbo = 0;
	}

CStdGL::~CStdGL()
	{
	Clear();
	pGL=NULL;
	}

void CStdGL::Clear()
	{
	#ifndef USE_SDL_MAINLOOP
	CStdDDraw::Clear();
	#endif
	NoPrimaryClipper();
	if (pTexMgr) pTexMgr->IntUnlock();
	DeleteDeviceObjects();
	MainCtx.Clear();
	pCurrCtx=NULL;
	#ifndef USE_SDL_MAINLOOP
	CStdDDraw::Clear();
	#endif
	}

bool CStdGL::PageFlip(RECT *pSrcRt, RECT *pDstRt, CStdWindow * pWindow)
	{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// safety
	if (!pCurrCtx) return FALSE;
	// end the scene and present it
	if (!pCurrCtx->PageFlip()) return FALSE;
	// success!
  return TRUE;
	}

void CStdGL::FillBG(DWORD dwClr)
	{
	if (!pCurrCtx) if (!MainCtx.Select()) return;
	glClearColor((float)GetBValue(dwClr)/255.0f, (float)GetGValue(dwClr)/255.0f, (float)GetRValue(dwClr)/255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

bool CStdGL::UpdateClipper()
	{
	// no render target? do nothing
	if (!RenderTarget || !Active) return true;
	// negative/zero?
	int iWdt=Min(iClipX2, RenderTarget->Wdt-1)-iClipX1+1;
	int iHgt=Min(iClipY2, RenderTarget->Hgt-1)-iClipY1+1;
	int iX=iClipX1; if (iX<0) { iWdt+=iX; iX=0; }
	int iY=iClipY1; if (iY<0) { iHgt+=iY; iY=0; }
	if (iWdt<=0 || iHgt<=0)
		{
		ClipAll=true;
		return true;
		}
	ClipAll=false;
	// set it
	glViewport(iX, RenderTarget->Hgt-iY-iHgt, iWdt, iHgt);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D((GLdouble) iX, (GLdouble) (iX+iWdt), (GLdouble) (iY+iHgt), (GLdouble) iY);
	//gluOrtho2D((GLdouble) 0, (GLdouble) xRes, (GLdouble) yRes, (GLdouble) yRes-iHgt);
	return true;
	}

bool CStdGL::PrepareRendering(SURFACE sfcToSurface)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// device?
	if (!pCurrCtx) if (!MainCtx.Select()) return false;
	// not ready?
	if (!Active)
		//if (!RestoreDeviceObjects())
			return false;
	// target?
	if (!sfcToSurface) return false;
	// target locked?
	if (sfcToSurface->Locked) return false;
	// target is already set as render target?
	if (sfcToSurface != RenderTarget)
		{
		// target is a render-target?
		if (!sfcToSurface->IsRenderTarget()) return false;
		// set target
		RenderTarget=sfcToSurface;
		// new target has different size; needs other clipping rect
		UpdateClipper();
		}
	// done
	return true;
	}

void CStdGL::PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact)
	{
	// clipping
	if (DDrawCfg.ClipManually && rBltData.pTransform) ClipPoly(rBltData);
	// global modulation map
	int i;
	bool fAnyModNotBlack = !!dwModClr;
	if (!shaders[0] && fUseClrModMap && dwModClr)
		{
		fAnyModNotBlack = false;
		for (i=0; i<rBltData.byNumVertices; ++i)
			{
			float x = rBltData.vtVtx[i].ftx;
			float y = rBltData.vtVtx[i].fty;
			if (rBltData.pTransform)
				{
				rBltData.pTransform->TransformPoint(x,y);
				}
			DWORD c = pClrModMap->GetModAt(int(x), int(y));
			ModulateClr(c, dwModClr);
			if (c) fAnyModNotBlack = true;
			DwTo4UB(c, rBltData.vtVtx[i].color);
			}
		}
	else
		{
		for (i=0; i<rBltData.byNumVertices; ++i)
			DwTo4UB(dwModClr, rBltData.vtVtx[i].color);
		}
	// reset MOD2 for completely black modulations
	if (fMod2 && !fAnyModNotBlack) fMod2 = 0;
	if (shaders[0])
		{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		GLuint s = fMod2 ? 1 : 0;
		if (Saturation < 255)
			{
			s += 3;
			}
		if (fUseClrModMap)
			{
			s += 6;
			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, (*pClrModMap->GetSurface()->ppTex)->texName);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glActiveTexture(GL_TEXTURE0);
			}
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[s]);
		if (Saturation < 255)
			{
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, bla);
			}
		}
	else
		{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, fMod2 ? GL_ADD_SIGNED : GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, fMod2 ? 2.0f : 1.0f);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
		}
	// set texture+modes
	glShadeModel((fUseClrModMap && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glBindTexture(GL_TEXTURE_2D, pTex->texName);
	if (!fExact && !DDrawCfg.PointFiltering)
		{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

	glMatrixMode(GL_TEXTURE);
	float matrix[16];
	matrix[0]=rBltData.TexPos.mat[0];  matrix[1]=rBltData.TexPos.mat[3];  matrix[2]=0;  matrix[3]=rBltData.TexPos.mat[6];
	matrix[4]=rBltData.TexPos.mat[1];  matrix[5]=rBltData.TexPos.mat[4];  matrix[6]=0;  matrix[7]=rBltData.TexPos.mat[7];
	matrix[8]=0;                       matrix[9]=0;                       matrix[10]=1; matrix[11]=0;
	matrix[12]=rBltData.TexPos.mat[2]; matrix[13]=rBltData.TexPos.mat[5]; matrix[14]=0;	matrix[15]=rBltData.TexPos.mat[8];
	glLoadMatrixf(matrix);

	if (shaders[0] && fUseClrModMap)
		{
		glActiveTexture(GL_TEXTURE3);
		glLoadIdentity();
		CSurface * pSurface = pClrModMap->GetSurface();
		glScalef(1.0f/(pClrModMap->GetResolutionX()*(*pSurface->ppTex)->iSize), 1.0f/(pClrModMap->GetResolutionY()*(*pSurface->ppTex)->iSize), 1.0f);
		glTranslatef(float(-pClrModMap->OffX), float(-pClrModMap->OffY), 0.0f);
		}
	if (rBltData.pTransform)
		{
		float * mat = rBltData.pTransform->mat;
		matrix[0]=mat[0];  matrix[1]=mat[3];  matrix[2]=0;  matrix[3]=mat[6];
		matrix[4]=mat[1];  matrix[5]=mat[4];  matrix[6]=0;  matrix[7]=mat[7];
		matrix[8]=0;       matrix[9]=0;       matrix[10]=1; matrix[11]=0;
		matrix[12]=mat[2]; matrix[13]=mat[5]; matrix[14]=0;	matrix[15]=mat[8];
		if (shaders[0] && fUseClrModMap)
			{
			glMultMatrixf(matrix);
			}
		glActiveTexture(GL_TEXTURE0);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(matrix);
		}
	else
		{
		glActiveTexture(GL_TEXTURE0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		}

	// draw polygon
	for (i=0; i<rBltData.byNumVertices; ++i)
		{
		rBltData.vtVtx[i].tx = rBltData.vtVtx[i].ftx;
		rBltData.vtVtx[i].ty = rBltData.vtVtx[i].fty;
		//if (rBltData.pTransform) rBltData.pTransform->TransformPoint(rBltData.vtVtx[i].ftx, rBltData.vtVtx[i].fty);
		rBltData.vtVtx[i].ftz = 0;
		}
	if (vbo)
		{
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, rBltData.byNumVertices*sizeof(CBltVertex), rBltData.vtVtx, GL_STREAM_DRAW_ARB);
		glInterleavedArrays(GL_T2F_C4UB_V3F, sizeof(CBltVertex), 0);
		}
	else
		{
		glInterleavedArrays(GL_T2F_C4UB_V3F, sizeof(CBltVertex), rBltData.vtVtx);
		}
	if (shaders[0] && fUseClrModMap)
		{
		glClientActiveTexture(GL_TEXTURE3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(CBltVertex), &rBltData.vtVtx[0].ftx);
		glClientActiveTexture(GL_TEXTURE0);
		}
	glDrawArrays(GL_POLYGON, 0, rBltData.byNumVertices);
	glLoadIdentity();
	if (shaders[0])
		{
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		}
	if (!fExact && !DDrawCfg.PointFiltering)
		{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

void CStdGL::PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt)
{
	const StdMesh& mesh = instance.Mesh;
	const StdMeshBox& box = mesh.GetBoundingBox();

	glPushMatrix();
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);

	// TODO: Zoom, ClrMod, ...

	//glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
	
	// Scale so that the mesh fits in (tx,ty,twdt,thgt)
	float rx = -box.x1 / (box.x2 - box.x1);
	float ry = -box.y1 / (box.y2 - box.y1);
	glTranslatef(tx + rx*twdt, ty + ry*thgt, 0.0f);

	// Put a light source in front of the object
	GLfloat light_position[] = { 0.0f, 0.0f, 15.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);

	float scx = twdt/(box.x2 - box.x1);
	float scy = thgt/(box.y2 - box.y1);
	// Keep aspect ratio:
	//if(scx < scy) scy = scx;
	//else scx = scy;
	glScalef(scx, scy, 1.0f);

	// TODO: Find a working technique, we currently always use the
	// first one:
	const StdMeshMaterial& material = mesh.GetMaterial();
	const StdMeshMaterialTechnique& technique = material.Techniques[0];

	// Render each pass
	for(unsigned int i = 0; i < technique.Passes.size(); ++i)
	{
		const StdMeshMaterialPass& pass = technique.Passes[i];

		// Set up material
		glMaterialfv(GL_FRONT, GL_AMBIENT, pass.Ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, pass.Diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, pass.Specular);
		glMaterialfv(GL_FRONT, GL_EMISSION, pass.Emissive);
		glMaterialf(GL_FRONT, GL_SHININESS, pass.Shininess);

		// TODO: Set up texture units
		// Render mesh
		glBegin(GL_TRIANGLES);
		for(unsigned int j = 0; j < mesh.GetNumFaces(); ++j)
		{
			const StdMeshFace& face = mesh.GetFace(j);
			const StdMeshVertex& vtx1 = instance.GetVertex(face.Vertices[0]);
			const StdMeshVertex& vtx2 = instance.GetVertex(face.Vertices[1]);
			const StdMeshVertex& vtx3 = instance.GetVertex(face.Vertices[2]);

			glTexCoord2f(vtx1.u, vtx1.v);
			glNormal3f(vtx1.nx, vtx1.ny, vtx1.nz);
			glVertex3f(vtx1.x, vtx1.y, vtx1.z);
			glTexCoord2f(vtx2.u, vtx2.v);
			glNormal3f(vtx2.nx, vtx2.ny, vtx2.nz);
			glVertex3f(vtx2.x, vtx2.y, vtx2.z);
			glTexCoord2f(vtx3.u, vtx3.v);
			glNormal3f(vtx3.nx, vtx3.ny, vtx3.nz);
			glVertex3f(vtx3.x, vtx3.y, vtx3.z);
		}
		glEnd(); // GL_TRIANGLES
	}

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);
	glPopMatrix();

	// TODO: glScissor, so that we only clear the area the mesh covered.
	glClear(GL_DEPTH_BUFFER_BIT);
}

void CStdGL::BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcLiquidAnimation, float fx, float fy,
								SURFACE sfcTarget, float tx, float ty, float wdt, float hgt)
	{
	Blit(sfcSource, fx, fy, wdt, hgt, sfcTarget, tx, ty, wdt, hgt);return;
	// safety
	if (!sfcSource || !sfcTarget || !wdt || !hgt) return;
	assert(sfcTarget->IsRenderTarget());
	assert(!(dwBlitMode & C4GFXBLIT_MOD2));
	// Apply Zoom
	float twdt = wdt;
	float thgt = hgt;
	tx = (tx - ZoomX) * Zoom + ZoomX;
	ty = (ty - ZoomY) * Zoom + ZoomY;
	twdt *= Zoom;
	thgt *= Zoom;
	// bound
	if (ClipAll) return;
	// manual clipping? (primary surface only)
	if (DDrawCfg.ClipManuallyE)
		{
		int iOver;
		// Left
		iOver=int(tx)-iClipX1;
		if (iOver<0)
			{
			wdt+=iOver;
			twdt+=iOver*Zoom;
			fx-=iOver;
			tx=float(iClipX1);
			}
		// Top
		iOver=int(ty)-iClipY1;
		if (iOver<0)
			{
			hgt+=iOver;
			thgt+=iOver*Zoom;
			fy-=iOver;
			ty=float(iClipY1);
			}
		// Right
		iOver=iClipX2+1-int(tx+twdt);
		if (iOver<0)
			{
			wdt+=iOver/Zoom;
			twdt+=iOver;
			}
		// Bottom
		iOver=iClipY2+1-int(ty+thgt);
		if (iOver<0)
			{
			hgt+=iOver/Zoom;
			thgt+=iOver;
			}
		}
	// inside screen?
	if (wdt<=0 || hgt<=0) return;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return;
	// texture present?
	if (!sfcSource->ppTex)
		{
		return;
		}
	// get involved texture offsets
	int iTexSize=sfcSource->iTexSize;
	int iTexX=Max(int(fx/iTexSize), 0);
	int iTexY=Max(int(fy/iTexSize), 0);
	int iTexX2=Min((int)(fx+wdt-1)/iTexSize +1, sfcSource->iTexX);
	int iTexY2=Min((int)(fy+hgt-1)/iTexSize +1, sfcSource->iTexY);
	// blit from all these textures
	SetTexture();
	if (sfcSource2)
		{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, (*sfcLiquidAnimation->ppTex)->texName);
		glActiveTexture(GL_TEXTURE0);
		}
	DWORD dwModMask = 0;
	if (shaders[0])
		{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		GLuint s = sfcSource2 ? 2 : 0;
		if (Saturation < 255)
			{
			s += 3;
			}
		if (fUseClrModMap)
			{
			s += 6;
			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, (*pClrModMap->GetSurface()->ppTex)->texName);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glActiveTexture(GL_TEXTURE0);
			}
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[s]);
		if (Saturation < 255)
			{
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, bla);
			}
		if (sfcSource2)
			{
			static GLfloat bla[4] = { -0.6f/3, 0.0f, 0.6f/3, 0.0f };
			bla[0] += 0.05f; bla[1] += 0.05f; bla[2] += 0.05f;
			GLfloat mod[4];
			for (int i = 0; i < 3; ++i)
				{
				if (bla[i] > 0.9f) bla[i] = -0.3f;
				mod[i] = (bla[i] > 0.3f ? 0.6f - bla[i] : bla[i]) / 3.0f;
				}
			mod[3] = 0;
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, mod);
			}
		dwModMask = 0;
		}
	// texture environment
	else
		{
		if (DDrawCfg.NoAlphaAdd)
			{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
			dwModMask = 0xff000000;
			}
		else
			{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
			dwModMask = 0;
			}
		}
	// set texture+modes
	glShadeModel((fUseClrModMap && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	for (int iY=iTexY; iY<iTexY2; ++iY)
		{
		for (int iX=iTexX; iX<iTexX2; ++iX)
			{
			// blit
			DWORD dwModClr = BlitModulated ? BlitModulateClr : 0xffffff;

			if (sfcSource2)
				glActiveTexture(GL_TEXTURE0);
			CTexRef *pTex = *(sfcSource->ppTex + iY * sfcSource->iTexX + iX);
			glBindTexture(GL_TEXTURE_2D, pTex->texName);
			if (Zoom != 1.0 && !DDrawCfg.PointFiltering)
				{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			if (sfcSource2)
				{
				CTexRef *pTex = *(sfcSource2->ppTex + iY * sfcSource2->iTexX + iX);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, pTex->texName);
				if (Zoom != 1.0 && !DDrawCfg.PointFiltering)
					{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}
				}

			// get current blitting offset in texture (beforing any last-tex-size-changes)
			int iBlitX=iTexSize*iX;
			int iBlitY=iTexSize*iY;
			// size changed? recalc dependant, relevant (!) values
			if (iTexSize != pTex->iSize)
				{
				iTexSize = pTex->iSize;
				}
			// get new texture source bounds
			FLOAT_RECT fTexBlt;
			// get new dest bounds
			FLOAT_RECT tTexBlt;
			// set up blit data as rect
			fTexBlt.left  = Max<float>((float)(fx - iBlitX), 0.0f);
			tTexBlt.left  = (fTexBlt.left  + iBlitX - fx) * Zoom + tx;
			fTexBlt.top   = Max<float>((float)(fy - iBlitY), 0.0f);
			tTexBlt.top   = (fTexBlt.top   + iBlitY - fy) * Zoom + ty;
			fTexBlt.right = Min<float>((float)(fx + wdt - iBlitX), (float)iTexSize);
			tTexBlt.right = (fTexBlt.right + iBlitX - fx) * Zoom + tx;
			fTexBlt.bottom= Min<float>((float)(fy + hgt - iBlitY), (float)iTexSize);
			tTexBlt.bottom= (fTexBlt.bottom+ iBlitY - fy) * Zoom + ty;
			CBltVertex Vtx[4];
			// blit positions
			Vtx[0].ftx = tTexBlt.left;  Vtx[0].fty = tTexBlt.top;
			Vtx[1].ftx = tTexBlt.right; Vtx[1].fty = tTexBlt.top;
			Vtx[2].ftx = tTexBlt.right; Vtx[2].fty = tTexBlt.bottom;
			Vtx[3].ftx = tTexBlt.left;  Vtx[3].fty = tTexBlt.bottom;
			// blit positions
			Vtx[0].tx = fTexBlt.left;  Vtx[0].ty = fTexBlt.top;
			Vtx[1].tx = fTexBlt.right; Vtx[1].ty = fTexBlt.top;
			Vtx[2].tx = fTexBlt.right; Vtx[2].ty = fTexBlt.bottom;
			Vtx[3].tx = fTexBlt.left;  Vtx[3].ty = fTexBlt.bottom;

			// color modulation
			// global modulation map
			int i;
			if (fUseClrModMap && dwModClr)
				{
				for (i=0; i<4; ++i)
					{
					DWORD c = pClrModMap->GetModAt(int(Vtx[i].ftx), int(Vtx[i].fty));
					ModulateClr(c, dwModClr);
					DwTo4UB(c | dwModMask, Vtx[i].color);
					}
				}
			else
				{
				for (i=0; i<4; ++i)
					DwTo4UB(dwModClr | dwModMask, Vtx[i].color);
				}
			for (i=0; i<4; ++i)
				{
				Vtx[i].tx /= iTexSize;
				Vtx[i].ty /= iTexSize;
				Vtx[i].ftx += DDrawCfg.fBlitOff;
				Vtx[i].fty += DDrawCfg.fBlitOff;
				Vtx[i].ftz = 0;
				}

			glInterleavedArrays(GL_T2F_C4UB_V3F, sizeof(CBltVertex), Vtx);
			glDrawArrays(GL_POLYGON, 0, 4);
			}
		}
	if (shaders[0])
		{
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		}
	if (sfcSource2)
		{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		}
	// reset texture
	ResetTexture();
	}

BOOL CStdGL::CreateDirectDraw()
	{
	Log("  Using OpenGL...");
	return TRUE;
	}

CStdGLCtx *CStdGL::CreateContext(CStdWindow * pWindow, CStdApp *pApp)
	{
	// safety
	if (!pWindow) return NULL;
	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	if (!pCtx->Init(pWindow, pApp))
		{
		delete pCtx; Error("  gl: Error creating secondary context!"); return NULL;
		}
	// reselect current context
	//if (pCurrCtx) pCurrCtx->Select(); else pCurrCtx=pCtx;
	// done
	return pCtx;
	}

#ifdef _WIN32
CStdGLCtx *CStdGL::CreateContext(HWND hWindow, CStdApp *pApp)
	{
	// safety
	if (!hWindow) return NULL;
	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	if (!pCtx->Init(NULL, pApp, hWindow))
		{
		delete pCtx; Error("  gl: Error creating secondary context!"); return NULL;
		}
	// reselect current context
	//if (pCurrCtx) pCurrCtx->Select(); else pCurrCtx=pCtx;
	// done
	return pCtx;
	}
#endif

bool CStdGL::CreatePrimarySurfaces(BOOL Playermode, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor)
	{
	//remember fullscreen setting
	fFullscreen = Playermode && !DDrawCfg.Windowed;

	// Set window size only in playermode
	if (Playermode)
		{
		// Always search for display mode, in case the user decides to activate fullscreen later
		if (!pApp->SetVideoMode(iXRes, iYRes, iColorDepth, iMonitor, fFullscreen))
			{
			Error("  gl: No Display mode found; leaving current!");
			fFullscreen = false;
			}
		if (!fFullscreen)
			{
			pApp->pWindow->SetSize(iXRes, iYRes);
			}
		}

	// store options
	byByteCnt=4; iClrDpt=iColorDepth;

	// create lpPrimary and lpBack (used in first context selection)
	lpPrimary=lpBack=new CSurface();
	lpPrimary->fPrimary=true;
	lpPrimary->AttachSfc(NULL, iXRes, iYRes);
	lpPrimary->byBytesPP=byByteCnt;

	// create+select gl context
	if (!MainCtx.Init(pApp->pWindow, pApp)) return Error("  gl: Error initializing context");

	// done, init device stuff
	return InitDeviceObjects();
	}

void CStdGL::DrawQuadDw(SURFACE sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4)
	{
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;
	// apply global modulation
	ClrByCurrentBlitMod(dwClr1);
	ClrByCurrentBlitMod(dwClr2);
	ClrByCurrentBlitMod(dwClr3);
	ClrByCurrentBlitMod(dwClr4);
	// apply modulation map
	if (fUseClrModMap)
		{
		ModulateClr(dwClr1, pClrModMap->GetModAt(int(ipVtx[0]), int(ipVtx[1])));
		ModulateClr(dwClr2, pClrModMap->GetModAt(int(ipVtx[2]), int(ipVtx[3])));
		ModulateClr(dwClr3, pClrModMap->GetModAt(int(ipVtx[4]), int(ipVtx[5])));
		ModulateClr(dwClr4, pClrModMap->GetModAt(int(ipVtx[6]), int(ipVtx[7])));
		}
	// no clr fading supported
	if (DDrawCfg.NoBoxFades)
		{
		NormalizeColors(dwClr1, dwClr2, dwClr3, dwClr4);
		glShadeModel(GL_FLAT);
		}
	else
		glShadeModel((dwClr1 == dwClr2 && dwClr1 == dwClr3 && dwClr1 == dwClr4) ? GL_FLAT : GL_SMOOTH);
	// set blitting state
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, iAdditive ? GL_ONE : GL_SRC_ALPHA);
	// draw two triangles
	glInterleavedArrays(GL_V2F, sizeof(float)*2, ipVtx);
	GLubyte colors[4][4];
	DwTo4UB(dwClr1,colors[0]);
	DwTo4UB(dwClr2,colors[1]);
	DwTo4UB(dwClr3,colors[2]);
	DwTo4UB(dwClr4,colors[3]);
	glColorPointer(4,GL_UNSIGNED_BYTE,0,colors);
	glEnableClientState(GL_COLOR_ARRAY);
	glDrawArrays(GL_POLYGON, 0, 4);
	glShadeModel(GL_FLAT);
	}

void CStdGL::PerformLine(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr)
	{
	// render target?
	if (sfcTarget->IsRenderTarget())
		{
		// prepare rendering to target
		if (!PrepareRendering(sfcTarget)) return;
		// set blitting state
		int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
		// use a different blendfunc here, because GL_LINE_SMOOTH expects this one
		glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
		// draw one line
		glBegin(GL_LINES);
		// global clr modulation map
		DWORD dwClr1 = dwClr;
		if (fUseClrModMap)
			{
			ModulateClr(dwClr1, pClrModMap->GetModAt((int)x1, (int)y1));
			}
		// convert from clonk-alpha to GL_LINE_SMOOTH alpha
		glColorDw(InvertRGBAAlpha(dwClr1));
		glVertex2f(x1 + 0.5f, y1 + 0.5f);
		if (fUseClrModMap)
			{
			ModulateClr(dwClr, pClrModMap->GetModAt((int)x2, (int)y2));
			glColorDw(InvertRGBAAlpha(dwClr));
			}
		glVertex2f(x2 + 0.5f, y2 + 0.5f);
		glEnd();
		}
	else
		{
		// emulate
		if (!LockSurfaceGlobal(sfcTarget)) return;
		ForLine((int32_t)x1,(int32_t)y1,(int32_t)x2,(int32_t)y2,&DLineSPix,(int) dwClr);
		UnLockSurfaceGlobal(sfcTarget);
		}
	}

void CStdGL::PerformPix(SURFACE sfcTarget, float tx, float ty, DWORD dwClr)
  {
	// render target?
	if (sfcTarget->IsRenderTarget())
		{
		if (!PrepareRendering(sfcTarget)) return;
		int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
		// use a different blendfunc here because of GL_POINT_SMOOTH
		glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
		// convert the alpha value for that blendfunc
		glBegin(GL_POINTS);
		glColorDw(InvertRGBAAlpha(dwClr));
		glVertex2f(tx + 0.5f, ty + 0.5f);
		glEnd();
		}
	else
		{
		// emulate
		sfcTarget->SetPixDw((int)tx, (int)ty, dwClr);
		}
  }

bool CStdGL::InitDeviceObjects()
	{
	// BGRA Pixel Formats, Multitexturing, Texture Combine Environment Modes
	if (!GLEW_VERSION_1_3)
		{
		Log("  gl: OpenGL Version 1.3 or higher required.");
		return false;
		}
	MaxTexSize = 64;
	GLint s = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s);
	if (s>0) MaxTexSize = s;
	return RestoreDeviceObjects();
	}

static void DefineShaderARB(const char * p, GLuint & s)
	{
	glBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, s);
	glProgramStringARB (GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(p), p);
	if (GL_INVALID_OPERATION == glGetError())
		{
		GLint errPos; glGetIntegerv (GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		fprintf (stderr, "ARB program%d:%d: Error: %s\n", s, errPos, glGetString (GL_PROGRAM_ERROR_STRING_ARB));
		s = 0;
		}
	}

bool CStdGL::RestoreDeviceObjects()
	{
	// safety
	if (!lpPrimary) return false;
	// delete any previous objects
	InvalidateDeviceObjects();
	bool fSuccess=true;
	// restore primary/back
	RenderTarget=lpPrimary;
	//lpPrimary->AttachSfc(NULL);
	lpPrimary->byBytesPP=byByteCnt;

	// set states
	fSuccess = pCurrCtx ? (pCurrCtx->Select()) : MainCtx.Select();
	// activate if successful
	Active=fSuccess;
	// restore gamma if active
	if (Active)
		EnableGamma();
	// reset blit states
	dwBlitMode = 0;

	if (0 && GLEW_ARB_vertex_buffer_object)
		{
		glGenBuffersARB(1, &vbo);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, 8 * sizeof(CBltVertex), 0, GL_STREAM_DRAW_ARB);
		}

	if(!DDrawCfg.Shader)
		{
		}
	else if (!shaders[0] && GLEW_ARB_fragment_program)
		{
		glGenProgramsARB (sizeof(shaders)/sizeof(*shaders), shaders);
		const char * preface =
		"!!ARBfp1.0\n"
		"TEMP tmp;\n"
		// sample the texture
		"TXP tmp, fragment.texcoord[0], texture, 2D;\n";
		const char * alpha_add =
		// perform the modulation
		"MUL tmp.rgb, tmp, fragment.color.primary;\n"
		"ADD_SAT tmp.a, tmp, fragment.color.primary;\n";
		const char * funny_add =
		// perform the modulation
		"ADD tmp, tmp, fragment.color.primary;\n"
		"MAD_SAT tmp, tmp, { 2.0, 2.0, 2.0, 1.0 }, { -1.0, -1.0, -1.0, 0.0 };\n";
		const char * grey =
		"TEMP grey;\n"
		"DP3 grey, tmp, { 0.299, 0.587, 0.114, 1.0 };\n"
		"LRP tmp.rgb, program.local[0], tmp, grey;\n";
		const char * liquid =
		"TEMP mask;\n"
		"TEMP liquid;\n"
		"TXP mask, fragment.texcoord, texture[1], 2D;\n"
		"TXP liquid, fragment.texcoord, texture[2], 2D;\n"
		// animation
		"SUB liquid.rgb, liquid, {0.5, 0.5, 0.5, 0};\n"
		"DP3 liquid.rgb, liquid, program.local[1];\n"
		//"MAD_SAT tmp.rgb, mask.aaa, liquid, tmp;\n"
		"MUL liquid.rgb, mask.aaaa, liquid;\n"
		"ADD_SAT tmp.rgb, liquid, tmp;\n";
		const char * fow =
		"TEMP fow;\n"
		// sample the texture
		"TXP fow, fragment.texcoord[3], texture[3], 2D;\n"
		"LRP tmp.rgb, fow.aaaa, tmp, fow;\n";
		const char * end =
		"MOV result.color, tmp;\n"
		"END\n";
		DefineShaderARB(FormatString("%s%s%s",       preface,         alpha_add,            end).getData(), shaders[0]);
		DefineShaderARB(FormatString("%s%s%s",       preface,         funny_add,            end).getData(), shaders[1]);
		DefineShaderARB(FormatString("%s%s%s%s",     preface, liquid, alpha_add,            end).getData(), shaders[2]);
		DefineShaderARB(FormatString("%s%s%s%s",     preface,         alpha_add, grey,      end).getData(), shaders[3]);
		DefineShaderARB(FormatString("%s%s%s%s",     preface,         funny_add, grey,      end).getData(), shaders[4]);
		DefineShaderARB(FormatString("%s%s%s%s%s",   preface, liquid, alpha_add, grey,      end).getData(), shaders[5]);
		DefineShaderARB(FormatString("%s%s%s%s",     preface,         alpha_add,       fow, end).getData(), shaders[6]);
		DefineShaderARB(FormatString("%s%s%s%s",     preface,         funny_add,       fow, end).getData(), shaders[7]);
		DefineShaderARB(FormatString("%s%s%s%s%s",   preface, liquid, alpha_add,       fow, end).getData(), shaders[8]);
		DefineShaderARB(FormatString("%s%s%s%s%s",   preface,         alpha_add, grey, fow, end).getData(), shaders[9]);
		DefineShaderARB(FormatString("%s%s%s%s%s",   preface,         funny_add, grey, fow, end).getData(), shaders[10]);
		DefineShaderARB(FormatString("%s%s%s%s%s%s", preface, liquid, alpha_add, grey, fow, end).getData(), shaders[11]);
		}
	// done
	return Active;
	}

bool CStdGL::InvalidateDeviceObjects()
{
	bool fSuccess=true;
	// clear gamma
	#ifndef USE_SDL_MAINLOOP
	DisableGamma();
	#endif
	// deactivate
	Active=false;
	// invalidate font objects
	// invalidate primary surfaces
	if (lpPrimary) lpPrimary->Clear();
	if (shaders[0])
		{
		glDeleteProgramsARB(sizeof(shaders)/sizeof(*shaders), shaders);
		shaders[0] = 0;
		}
	if (vbo)
		{
		glDeleteBuffersARB(1, &vbo);
		vbo = 0;
		}
	return fSuccess;
}

bool CStdGL::StoreStateBlock()
	{
	return true;
	}

void CStdGL::SetTexture()
	{
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, (dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	}

void CStdGL::ResetTexture()
	{
	// disable texturing
	glDisable(GL_TEXTURE_2D);
	}

bool CStdGL::RestoreStateBlock()
	{
	return true;
	}

bool CStdGL::CheckGLError(const char *szAtOp)
	{
	GLenum err = glGetError();
	if (!err) return true;
	Log(szAtOp);
	switch (err)
		{
		case GL_INVALID_ENUM:				Log("GL_INVALID_ENUM"); break;
		case GL_INVALID_VALUE:			Log("GL_INVALID_VALUE"); break;
		case GL_INVALID_OPERATION:	Log("GL_INVALID_OPERATION"); break;
		case GL_STACK_OVERFLOW:			Log("GL_STACK_OVERFLOW"); break;
		case GL_STACK_UNDERFLOW:		Log("GL_STACK_UNDERFLOW"); break;
		case GL_OUT_OF_MEMORY:			Log("GL_OUT_OF_MEMORY"); break;
		default: Log("unknown error"); break;
		}
	return false;
	}

CStdGL *pGL=NULL;

bool CStdGL::DeleteDeviceObjects()
	{
	InvalidateDeviceObjects();
	NoPrimaryClipper();
	// del font objects
	// del main surfaces
	if (lpPrimary) delete lpPrimary;
	lpPrimary=lpBack=NULL;
	RenderTarget=NULL;
	// clear context
	if (pCurrCtx) pCurrCtx->Deselect();
	MainCtx.Clear();
	return true;
	}

void CStdGL::TaskOut()
	{
	// deactivate
	// backup textures
	if (pTexMgr && fFullscreen) pTexMgr->IntLock();
	// shotdown gl
	InvalidateDeviceObjects();
	if (pCurrCtx) pCurrCtx->Deselect();
	}

void CStdGL::TaskIn()
	{
	// restore gl
	//if (!DeviceReady()) MainCtx.Init(pWindow, pApp);
	// restore textures
	if (pTexMgr && fFullscreen) pTexMgr->IntUnlock();
	// restore device stuff
	RestoreDeviceObjects();
	}

bool CStdGL::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
	{
	InvalidateDeviceObjects();
	RestoreDeviceObjects();
	// Re-create primary clipper to adapt to new size.
	CreatePrimaryClipper(iXRes, iYRes);
	return true;
	}

void CStdGL::Default()
	{
	CStdDDraw::Default();
	//pCurrCtx = NULL;
	iPixelFormat=0;
	sfcFmt=0;
	iClrDpt=0;
	MainCtx.Clear();
	}

#endif // USE_GL
