/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2003, 2005-2006  Sven Eberhardt
 * Copyright (c) 2006  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
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

/* A piece of a DirectDraw surface */

#ifndef INC_C4Facet
#define INC_C4Facet

#include <StdDDraw2.h>

const int32_t C4FCT_None   = 0,

                             C4FCT_Left   = 1,
                                            C4FCT_Right  = 2,
                                                           C4FCT_Top    = 4,
                                                                          C4FCT_Bottom = 8,
                                                                                         C4FCT_Center = 16,

                                                                                                        C4FCT_Alignment = C4FCT_Left | C4FCT_Right | C4FCT_Top | C4FCT_Bottom | C4FCT_Center,

                                                                                                                          C4FCT_Half   = 32,
                                                                                                                                         C4FCT_Double = 64,
                                                                                                                                                        C4FCT_Triple = 128;

// tuple of two integers
struct C4Vec2D
{
	int32_t x,y;

	C4Vec2D(int32_t x=0, int32_t y=0) : x(x), y(y) {}
};

class C4DrawTransform : public CBltTransform
{
public:
	int32_t FlipDir; // +1 or -1; multiplied as x-flip

	C4DrawTransform(C4DrawTransform &rCopy, float iOffX, float iOffY) // ctor doing transform at given offset - doesn't init FlipDir (for temp use only)
	{
		SetTransformAt(rCopy, iOffX, iOffY);
	}

	C4DrawTransform()
	{
		// ctor without matrix initialization
		FlipDir = 1;
	}

	C4DrawTransform(int32_t iFlipDir)
	{
		// ctor setting flipdir only
		FlipDir = iFlipDir;
		// set identity
		Set(1,0,0,0,1,0,0,0,1);
	}

	~C4DrawTransform()
	{
	}

	// do transform at given offset - doesn't init FlipDir (for temp use only)
	void SetTransformAt(C4DrawTransform &rCopy, float iOffX, float iOffY);

	void Set(float fA, float fB, float fC, float fD, float fE, float fF, float fG, float fH, float fI)
	{
		// set values; apply flipdir
		CBltTransform::Set(fA*FlipDir, fB, fC, fD, fE, fF, fG, fH, fI);
	}

	void SetFlipDir(int32_t iNewFlipDir)
	{
		// no change?
		if (iNewFlipDir == FlipDir) return;
		// set and apply in matrix
		FlipDir = iNewFlipDir; mat[0] = -mat[0];
	}

	bool IsIdentity()
	{
		return (mat[0]==1.0f) && (mat[1]==0.0f) && (mat[2]=0.0f)
		       && (mat[3]==0.0f) && (mat[4]==1.0f) && (mat[5]=0.0f)
		       && (mat[6]==0.0f) && (mat[7]==0.0f) && (mat[8]=1.0f)
		       && (FlipDir==1); // flipdir must be 1, because otherwise matrices flipped by action+script would be removed
	}

	// default comparison op won't work :(
	bool operator == (const C4DrawTransform &rCmp) const
	{
		return (mat[0]==rCmp.mat[0]) && (mat[1]==rCmp.mat[1]) && (mat[2]==rCmp.mat[2])
		       && (mat[3]==rCmp.mat[3]) && (mat[4]==rCmp.mat[4]) && (mat[5]==rCmp.mat[5])
		       && (mat[6]==rCmp.mat[6]) && (mat[7]==rCmp.mat[7]) && (mat[8]==rCmp.mat[8]) && (FlipDir == rCmp.FlipDir);
	}
	C4DrawTransform * operator&() { return this; }
	void CompileFunc(StdCompiler *pComp);

	// rounded pixel offsets generated by this transformation
	int32_t GetXOffset() const { return static_cast<int32_t>(mat[2]); }
	int32_t GetYOffset() const { return static_cast<int32_t>(mat[5]); }

};

class C4Facet
{
public:
	SURFACE Surface;
	int32_t X,Y,Wdt,Hgt;
public:
	C4Facet();
	C4Facet(SURFACE pSfc, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
			: Surface(pSfc), X(iX), Y(iY), Wdt(iWdt), Hgt(iHgt) {  }
public:
	void Default();
	void Set(CSurface &rSfc);
	void Set(SURFACE nsfc, int32_t nx, int32_t ny, int32_t nwdt, int32_t nhgt);
	void Set(const C4Facet &cpy) { *this=cpy; }
	void Wipe();
	void Expand(int32_t iLeft=0, int32_t iRight=0, int32_t iTop=0, int32_t iBottom=0);
	void DrawTile(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt);
	void DrawEnergyLevel(int32_t iLevel, int32_t iRange, int32_t iColor=45); // draw energy level using solid colors
	void DrawEnergyLevelEx(int32_t iLevel, int32_t iRange, const C4Facet &gfx, int32_t bar_idx); // draw energy level using graphics
	void DrawX(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iPhaseX=0, int32_t iPhaseY=0) const;
	void DrawXFloat(SURFACE sfcTarget, float fX, float fY, float fWdt, float fHgt) const;
	void DrawValue(C4Facet &cgo, int32_t iValue, int32_t iPhaseX=0, int32_t iPhaseY=0, int32_t iAlign=C4FCT_Center);
	void DrawValue2(C4Facet &cgo, int32_t iValue1, int32_t iValue2, int32_t iPhaseX=0, int32_t iPhaseY=0, int32_t iAlign=C4FCT_Center, int32_t *piUsedWidth=NULL);
	void Draw(C4Facet &cgo, bool fAspect=true, int32_t iPhaseX=0, int32_t iPhaseY=0, bool fTransparent=true);
	void DrawFullScreen(C4Facet &cgo);
	void DrawT(SURFACE sfcTarget, float iX, float iY, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform); // draw with transformation (if pTransform is assigned)
	void DrawT(C4Facet &cgo, bool fAspect, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform);
	void DrawXT(SURFACE sfcTarget, float iX, float iY, int32_t iWdt, int32_t iHgt, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform);
	void DrawClr(C4Facet &cgo, bool fAspect=true, DWORD dwClr=0); // set surface color and draw
	void DrawXClr(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, DWORD dwClr); // set surface color and draw
	void DrawValue2Clr(C4Facet &cgo, int32_t iValue1, int32_t iValue2, DWORD dwClr); // set surface color and draw
	void DrawXR(SURFACE sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iPhaseX=0, int32_t iPhaseY=0, int32_t r=0); // draw rotated
	void Draw(SURFACE sfcTarget, float iX, float iY, int32_t iPhaseX=0, int32_t iPhaseY=0);
	void DrawSectionSelect(C4Facet &cgo, int32_t iSelection, int32_t iMaxSelect);
	bool GetPhaseNum(int32_t &rX, int32_t &rY);   // return number of phases in this graphic
	C4Facet GetSection(int32_t iSection);
	C4Facet GetPhase(int iPhaseX=0, int iPhaseY=0);
	C4Facet GetFraction(int32_t percentWdt, int32_t percentHgt=0, int32_t alignX=C4FCT_Left, int32_t alignY=C4FCT_Top);
	C4Facet TruncateSection(int32_t iAlign=C4FCT_Left);
	C4Facet Truncate(int32_t iAlign, int32_t iSize);
	int32_t GetSectionCount();
	int32_t GetWidthByHeight(int32_t iHeight) // calc width so it matches facet aspect to height
	{ return iHeight * Wdt / (Hgt ? Hgt : 1); }
	int32_t GetHeightByWidth(int32_t iWidth) // calc height so it matches facet aspect to width
	{ return iWidth * Hgt / (Wdt ? Wdt : 1); }
#ifdef _WIN32
	void Draw(HWND hWnd, int32_t iTx, int32_t iTy, int32_t iTWdt, int32_t iTHgt, bool fAspect=true, int32_t iPhaseX=0, int32_t iPhaseY=0);
#endif
};

#endif // INC_C4Facet
