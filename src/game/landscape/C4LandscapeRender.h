
#ifndef C4LANDSCAPE_RENDER_H
#define C4LANDSCAPE_RENDER_H

#include "StdSurface2.h"
#include "C4FacetEx.h"

// Data we want to store per landscape pixel
enum C4LR_Byte {
	C4LR_Material,
	C4LR_BiasX,
	C4LR_BiasY,

	C4LR_ByteCount
};

// How much data we want to store per landscape pixel
const int C4LR_BytesPerPx = 3;

// How much data we can hold per surface, how much surfaces we therefore need.
const int C4LR_BytesPerSurface = 4;
const int C4LR_SurfaceCount = (C4LR_ByteCount + C4LR_BytesPerSurface - 1) / C4LR_BytesPerSurface;

class C4Landscape; class C4TextureMap;

class C4LandscapeRender
{
public:
	C4LandscapeRender()
		: iWidth(0), iHeight(0), pTexs(NULL) { }
	virtual ~C4LandscapeRender()
		{}

protected:
	int32_t iWidth, iHeight;
	C4TextureMap *pTexs;

public:

	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics) = 0;
	virtual void Clear() = 0;

	// Returns the rectangle of pixels that must be updated on changes in the given rect
	virtual C4Rect GetAffectedRect(C4Rect Rect) = 0;

	// Updates the landscape rendering to reflect the landscape contents in
	// the given rectangle
	virtual void Update(C4Rect Rect, C4Landscape *pSource) = 0;

	virtual void Draw(const C4TargetFacet &cgo) = 0;
};

#ifdef USE_GL
class C4LandscapeRenderGL : public C4LandscapeRender
{
public:
	C4LandscapeRenderGL();
	~C4LandscapeRenderGL();

private:
	// surfaces
	CSurface *Surfaces[C4LR_SurfaceCount];

	// shader sources
	StdStrBuf LandscapeShader;
	StdStrBuf LandscapeShaderPath;
	int iLandscapeShaderTime;
	// shaders
	GLhandleARB hVert, hFrag, hProg;
	// shader variables
	GLhandleARB hLandscapeUnit, hScalerUnit, hMaterialUnit;
	GLhandleARB hResolutionUniform, hMatTexMapUniform;

	// Texture count
	int32_t iTexCount;
	// 3D material textures
	GLuint hMaterialTexture;
	// material map
	GLfloat MatTexMap[256];

	// scaler image
	C4FacetSurface fctScaler;

public:
	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pMap, C4GroupSet *pGraphics);
	virtual void Clear();

	virtual C4Rect GetAffectedRect(C4Rect Rect);
	virtual void Update(C4Rect Rect, C4Landscape *pSource);

	virtual void Draw(const C4TargetFacet &cgo);

private:
	bool InitMaterialTexture(C4TextureMap *pMap);
	bool LoadShaders(C4GroupSet *pGraphics);
	bool LoadScaler(C4GroupSet *pGraphics);

	void DumpInfoLog(const char *szWhat, GLhandleARB hShader);
	int GetObjectStatus(GLhandleARB hObj, GLenum type);
	GLhandleARB CreateShader(GLenum iShaderType, const char *szWhat, const char *szCode);

	bool InitShaders();
	void ClearShaders();
};
#endif

class C4LandscapeRenderClassic : public C4LandscapeRender
{
public:
	C4LandscapeRenderClassic();
	~C4LandscapeRenderClassic();

private:
	CSurface *Surface32;

public:
	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pMap, C4GroupSet *pGraphics);
	virtual void Clear();

	virtual C4Rect GetAffectedRect(C4Rect Rect);
	virtual void Update(C4Rect Rect, C4Landscape *pSource);

	virtual void Draw(const C4TargetFacet &cgo);

};

#endif // C4LANDSCAPE_RENDER_H
