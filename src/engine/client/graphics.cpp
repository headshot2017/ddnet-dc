/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/detect.h>
#include <base/math.h>

#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <math.h> // cosf, sinf

#include "graphics.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glkos.h"

#undef GL_MAX_TEXTURE_SIZE
#define GL_MAX_TEXTURE_SIZE 256

void CGraphics_PVR::Flush()
{
	if(m_NumVertices == 0)
		return;

	if(m_RenderEnable)
	{
		glBegin(g_Config.m_GfxQuadAsTriangle ? GL_TRIANGLES : GL_QUADS);

		for (int i=0; i<m_NumVertices; i++)
		{
			float r = m_aVertices[i].m_Color.r;
			float g = m_aVertices[i].m_Color.g;
			float b = m_aVertices[i].m_Color.b;
			float a = m_aVertices[i].m_Color.a;

			float x = m_aVertices[i].m_Pos.x;
			float y = m_aVertices[i].m_Pos.y;
			float z = m_aVertices[i].m_Pos.z;

			float u = m_aVertices[i].m_Tex.u;
			float v = m_aVertices[i].m_Tex.v;

			glColor4f(r, g, b, a);
			glTexCoord2f(u, v);
			glVertex3f(x, y, z);
		}

		glEnd();
	}

	// Reset pointer
	m_NumVertices = 0;
}

void CGraphics_PVR::AddVertices(int Count)
{
	m_NumVertices += Count;
	if((m_NumVertices + Count) >= MAX_VERTICES)
		Flush();
}

void CGraphics_PVR::Rotate(const CPoint &rCenter, CVertex *pPoints, int NumPoints)
{
	float c = cosf(m_Rotation);
	float s = sinf(m_Rotation);
	float x, y;
	int i;

	for(i = 0; i < NumPoints; i++)
	{
		x = pPoints[i].m_Pos.x - rCenter.x;
		y = pPoints[i].m_Pos.y - rCenter.y;
		pPoints[i].m_Pos.x = x * c - y * s + rCenter.x;
		pPoints[i].m_Pos.y = x * s + y * c + rCenter.y;
	}
}

unsigned char CGraphics_PVR::Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp)
{
	int Value = 0;
	for(int x = 0; x < ScaleW; x++)
		for(int y = 0; y < ScaleH; y++)
			Value += pData[((v+y)*w+(u+x))*Bpp+Offset];
	return Value/(ScaleW*ScaleH);
}

unsigned char *CGraphics_PVR::Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData)
{
	unsigned char *pTmpData;
	int ScaleW = Width/NewWidth;
	int ScaleH = Height/NewHeight;

	int Bpp = 3;
	if(Format == CImageInfo::FORMAT_RGBA)
		Bpp = 4;

	pTmpData = (unsigned char *)mem_alloc(NewWidth*NewHeight*Bpp, 1);

	int c = 0;
	for(int y = 0; y < NewHeight; y++)
		for(int x = 0; x < NewWidth; x++, c++)
		{
			pTmpData[c*Bpp] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 0, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+1] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 1, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+2] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 2, ScaleW, ScaleH, Bpp);
			if(Bpp == 4)
				pTmpData[c*Bpp+3] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 3, ScaleW, ScaleH, Bpp);
		}

	return pTmpData;
}

CGraphics_PVR::CGraphics_PVR()
{
	m_NumVertices = 0;

	m_ScreenX0 = 0;
	m_ScreenY0 = 0;
	m_ScreenX1 = 0;
	m_ScreenY1 = 0;

	m_ScreenWidth = -1;
	m_ScreenHeight = -1;

	m_Rotation = 0;
	m_Drawing = 0;
	m_InvalidTexture = 0;

	m_TextureMemoryUsage = 0;

	m_RenderEnable = true;
	m_DoScreenshot = false;
}

void CGraphics_PVR::ClipEnable(int x, int y, int w, int h)
{
	if(x < 0)
		w += x;
	if(y < 0)
		h += y;

	x = clamp(x, 0, ScreenWidth());
	y = clamp(y, 0, ScreenHeight());
	w = clamp(w, 0, ScreenWidth()-x);
	h = clamp(h, 0, ScreenHeight()-y);

	glScissor(x, ScreenHeight()-(y+h), w, h);
	glEnable(GL_SCISSOR_TEST);
}

void CGraphics_PVR::ClipDisable()
{
	glDisable(GL_SCISSOR_TEST);
}

void CGraphics_PVR::BlendNone()
{
	glDisable(GL_BLEND);
}

void CGraphics_PVR::BlendNormal()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CGraphics_PVR::BlendAdditive()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void CGraphics_PVR::WrapNormal()
{
	
}

void CGraphics_PVR::WrapClamp()
{
	
}

int CGraphics_PVR::MemoryUsage() const
{
	return m_TextureMemoryUsage;
}

void CGraphics_PVR::MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY)
{
	m_ScreenX0 = TopLeftX;
	m_ScreenY0 = TopLeftY;
	m_ScreenX1 = BottomRightX;
	m_ScreenY1 = BottomRightY;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(TopLeftX, BottomRightX, BottomRightY, TopLeftY, 1.0f, 10.f);
}

void CGraphics_PVR::GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY)
{
	*pTopLeftX = m_ScreenX0;
	*pTopLeftY = m_ScreenY0;
	*pBottomRightX = m_ScreenX1;
	*pBottomRightY = m_ScreenY1;
}

void CGraphics_PVR::LinesBegin()
{
	dbg_assert(m_Drawing == 0, "called Graphics()->LinesBegin twice");
	m_Drawing = DRAWING_LINES;
	SetColor(1,1,1,1);
}

void CGraphics_PVR::LinesEnd()
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called Graphics()->LinesEnd without begin");
	Flush();
	m_Drawing = 0;
}

void CGraphics_PVR::LinesDraw(const CLineItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called Graphics()->LinesDraw without begin");

	for(int i = 0; i < Num; ++i)
	{
		m_aVertices[m_NumVertices + 2*i].m_Pos.x = pArray[i].m_X0;
		m_aVertices[m_NumVertices + 2*i].m_Pos.y = pArray[i].m_Y0;
		m_aVertices[m_NumVertices + 2*i].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices + 2*i].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.x = pArray[i].m_X1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.y = pArray[i].m_Y1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 2*i + 1].m_Color = m_aColor[1];

		m_aVertices[m_NumVertices + 2*i + 2].m_Pos.x = pArray[i].m_X1;
		m_aVertices[m_NumVertices + 2*i + 2].m_Pos.y = pArray[i].m_Y1;
		m_aVertices[m_NumVertices + 2*i + 2].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 2*i + 2].m_Color = m_aColor[1];
	}

	AddVertices(3*Num);
}

int CGraphics_PVR::UnloadTexture(int Index)
{
	if(Index == m_InvalidTexture)
		return 0;

	if(Index < 0)
		return 0;

	m_aTextures[Index].m_Next = m_FirstFreeTexture;
	m_TextureMemoryUsage -= m_aTextures[Index].m_MemSize;
	m_FirstFreeTexture = Index;
	return 0;
}

int CGraphics_PVR::LoadTextureRawSub(int TextureID, int x, int y, int Width, int Height, int Format, const void *pData)
{
    return 0;
}

int CGraphics_PVR::LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags)
{
	uint8_t* pTexData = (uint8_t*)pData;
	uint8_t* pTmpData = 0;

	int Tex = 0;

	// don't waste memory on texture if we are stress testing
	if(g_Config.m_DbgStress)
		return 	m_InvalidTexture;

	// grab texture
	Tex = m_FirstFreeTexture;
	m_FirstFreeTexture = m_aTextures[Tex].m_Next;
	m_aTextures[Tex].m_Next = -1;

	if(!(Flags&TEXLOAD_NORESAMPLE) && (Format == CImageInfo::FORMAT_RGBA || Format == CImageInfo::FORMAT_RGB))
	{
		if(Width > GL_MAX_TEXTURE_SIZE || Height > GL_MAX_TEXTURE_SIZE)
		{
			int NewWidth = min(Width, GL_MAX_TEXTURE_SIZE);
			float div = NewWidth/(float)Width;
			int NewHeight = Height * div;
			pTmpData = Rescale(Width, Height, NewWidth, NewHeight, Format, pTexData);
			pTexData = pTmpData;
			Width = NewWidth;
			Height = NewHeight;
		}
		else if(Width > 16 && Height > 16 && g_Config.m_GfxTextureQuality == 0)
		{
			pTmpData = Rescale(Width, Height, Width/2, Height/2, Format, pTexData);
			pTexData = pTmpData;
			Width /= 2;
			Height /= 2;
		}
	}

	int PixelSize = 4;
	if(StoreFormat == CImageInfo::FORMAT_RGB)
		PixelSize = 3;
	else if(StoreFormat == CImageInfo::FORMAT_ALPHA)
		PixelSize = 1;

	int Oglformat = GL_RGBA;
	if(Format == CImageInfo::FORMAT_RGB)
		Oglformat = GL_RGB;
	else if(Format == CImageInfo::FORMAT_ALPHA)
		Oglformat = GL_ALPHA;

	glGenTextures(1, &m_aTextures[Tex].m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_aTextures[Tex].m_Tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, Oglformat, Width, Height, 0, Oglformat, GL_UNSIGNED_BYTE, pTexData);

	if (pTmpData) mem_free(pTmpData);

	// calculate memory usage
	{
		m_aTextures[Tex].m_MemSize = Width*Height*PixelSize;
	}

	m_TextureMemoryUsage += m_aTextures[Tex].m_MemSize;
	return Tex;
}

// simple uncompressed RGBA loaders
int CGraphics_PVR::LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags)
{
	int l = str_length(pFilename);
	int ID;
	CImageInfo Img;

	if(l < 3)
		return m_InvalidTexture;
	if(LoadPNG(&Img, pFilename, StorageType))
	{
		if (StoreFormat == CImageInfo::FORMAT_AUTO)
			StoreFormat = Img.m_Format;

		ID = LoadTextureRaw(Img.m_Width, Img.m_Height, Img.m_Format, Img.m_pData, StoreFormat, Flags);
		mem_free(Img.m_pData);
		if(ID != m_InvalidTexture && g_Config.m_Debug)
			dbg_msg("graphics/texture", "loaded %s", pFilename);
		return ID;
	}

	return m_InvalidTexture;
}

int CGraphics_PVR::LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", pFilename);
		return 0;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return 0;
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return 0;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return 1;
}

void CGraphics_PVR::ScreenshotDirect(const char *pFilename)
{
	
}

void CGraphics_PVR::TextureSet(int TextureID)
{
	dbg_assert(m_Drawing == 0, "called Graphics()->TextureSet within begin");

	if(TextureID == -1)
	{
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_aTextures[TextureID].m_Tex);
	}
}

void CGraphics_PVR::Clear(float r, float g, float b)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(r, g, b, 1.0f);
    glClearDepth(1.0f);
	
}

void CGraphics_PVR::QuadsBegin()
{
	dbg_assert(m_Drawing == 0, "called Graphics()->QuadsBegin twice");
	m_Drawing = DRAWING_QUADS;

	QuadsSetSubset(0,0,1,1);
	QuadsSetRotation(0);
	SetColor(1,1,1,1);
}

void CGraphics_PVR::QuadsEnd()
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsEnd without begin");
	Flush();
	m_Drawing = 0;
}

void CGraphics_PVR::QuadsSetRotation(float Angle)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetRotation without begin");
	m_Rotation = Angle;
}

void CGraphics_PVR::SetColorVertex(const CColorVertex *pArray, int Num)
{
	dbg_assert(m_Drawing != 0, "called Graphics()->SetColorVertex without begin");

	for(int i = 0; i < Num; ++i)
	{
		m_aColor[pArray[i].m_Index].r = pArray[i].m_R;
		m_aColor[pArray[i].m_Index].g = pArray[i].m_G;
		m_aColor[pArray[i].m_Index].b = pArray[i].m_B;
		m_aColor[pArray[i].m_Index].a = pArray[i].m_A;
	}
}

void CGraphics_PVR::SetColor(float r, float g, float b, float a)
{
	dbg_assert(m_Drawing != 0, "called Graphics()->SetColor without begin");
	CColorVertex Array[4] = {
		CColorVertex(0, r, g, b, a),
		CColorVertex(1, r, g, b, a),
		CColorVertex(2, r, g, b, a),
		CColorVertex(3, r, g, b, a)};
	SetColorVertex(Array, 4);
}

void CGraphics_PVR::QuadsSetSubset(float TlU, float TlV, float BrU, float BrV)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetSubset without begin");

	m_aTexture[0].u = TlU;	m_aTexture[1].u = BrU;
	m_aTexture[0].v = TlV;	m_aTexture[1].v = TlV;

	m_aTexture[3].u = TlU;	m_aTexture[2].u = BrU;
	m_aTexture[3].v = BrV;	m_aTexture[2].v = BrV;
}

void CGraphics_PVR::QuadsSetSubsetFree(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
	m_aTexture[0].u = x0; m_aTexture[0].v = y0;
	m_aTexture[1].u = x1; m_aTexture[1].v = y1;
	m_aTexture[2].u = x2; m_aTexture[2].v = y2;
	m_aTexture[3].u = x3; m_aTexture[3].v = y3;
}

void CGraphics_PVR::QuadsDraw(CQuadItem *pArray, int Num)
{
	for(int i = 0; i < Num; ++i)
	{
		pArray[i].m_X -= pArray[i].m_Width/2;
		pArray[i].m_Y -= pArray[i].m_Height/2;
	}

	QuadsDrawTL(pArray, Num);
}

void CGraphics_PVR::QuadsDrawTL(const CQuadItem *pArray, int Num)
{
	CPoint Center;
	Center.z = 0;

	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsDrawTL without begin");

	if(g_Config.m_GfxQuadAsTriangle)
	{
		for(int i = 0; i < Num; ++i)
		{
			// first triangle
			m_aVertices[m_NumVertices + 6*i].m_Pos.x = pArray[i].m_X;
			m_aVertices[m_NumVertices + 6*i].m_Pos.y = pArray[i].m_Y;
			m_aVertices[m_NumVertices + 6*i].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 6*i].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 6*i + 1].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
			m_aVertices[m_NumVertices + 6*i + 1].m_Pos.y = pArray[i].m_Y;
			m_aVertices[m_NumVertices + 6*i + 1].m_Tex = m_aTexture[1];
			m_aVertices[m_NumVertices + 6*i + 1].m_Color = m_aColor[1];

			m_aVertices[m_NumVertices + 6*i + 2].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
			m_aVertices[m_NumVertices + 6*i + 2].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
			m_aVertices[m_NumVertices + 6*i + 2].m_Tex = m_aTexture[2];
			m_aVertices[m_NumVertices + 6*i + 2].m_Color = m_aColor[2];

			// second triangle
			m_aVertices[m_NumVertices + 6*i + 3].m_Pos.x = pArray[i].m_X;
			m_aVertices[m_NumVertices + 6*i + 3].m_Pos.y = pArray[i].m_Y;
			m_aVertices[m_NumVertices + 6*i + 3].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 6*i + 3].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 6*i + 4].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
			m_aVertices[m_NumVertices + 6*i + 4].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
			m_aVertices[m_NumVertices + 6*i + 4].m_Tex = m_aTexture[2];
			m_aVertices[m_NumVertices + 6*i + 4].m_Color = m_aColor[2];

			m_aVertices[m_NumVertices + 6*i + 5].m_Pos.x = pArray[i].m_X;
			m_aVertices[m_NumVertices + 6*i + 5].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
			m_aVertices[m_NumVertices + 6*i + 5].m_Tex = m_aTexture[3];
			m_aVertices[m_NumVertices + 6*i + 5].m_Color = m_aColor[3];

			if(m_Rotation != 0)
			{
				Center.x = pArray[i].m_X + pArray[i].m_Width/2;
				Center.y = pArray[i].m_Y + pArray[i].m_Height/2;

				Rotate(Center, &m_aVertices[m_NumVertices + 6*i], 6);
			}
		}

		AddVertices(3*2*Num);
	}
	else
	{
		for(int i = 0; i < Num; ++i)
		{
			m_aVertices[m_NumVertices + 4*i].m_Pos.x = pArray[i].m_X;
			m_aVertices[m_NumVertices + 4*i].m_Pos.y = pArray[i].m_Y;
			m_aVertices[m_NumVertices + 4*i].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 4*i].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 4*i + 1].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
			m_aVertices[m_NumVertices + 4*i + 1].m_Pos.y = pArray[i].m_Y;
			m_aVertices[m_NumVertices + 4*i + 1].m_Tex = m_aTexture[1];
			m_aVertices[m_NumVertices + 4*i + 1].m_Color = m_aColor[1];

			m_aVertices[m_NumVertices + 4*i + 2].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
			m_aVertices[m_NumVertices + 4*i + 2].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
			m_aVertices[m_NumVertices + 4*i + 2].m_Tex = m_aTexture[2];
			m_aVertices[m_NumVertices + 4*i + 2].m_Color = m_aColor[2];

			m_aVertices[m_NumVertices + 4*i + 3].m_Pos.x = pArray[i].m_X;
			m_aVertices[m_NumVertices + 4*i + 3].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
			m_aVertices[m_NumVertices + 4*i + 3].m_Tex = m_aTexture[3];
			m_aVertices[m_NumVertices + 4*i + 3].m_Color = m_aColor[3];

			if(m_Rotation != 0)
			{
				Center.x = pArray[i].m_X + pArray[i].m_Width/2;
				Center.y = pArray[i].m_Y + pArray[i].m_Height/2;

				Rotate(Center, &m_aVertices[m_NumVertices + 4*i], 4);
			}
		}

		AddVertices(4*Num);
	}
}

void CGraphics_PVR::QuadsDrawFreeform(const CFreeformItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsDrawFreeform without begin");

	if(g_Config.m_GfxQuadAsTriangle)
	{
		for(int i = 0; i < Num; ++i)
		{
			m_aVertices[m_NumVertices + 6*i].m_Pos.x = pArray[i].m_X0;
			m_aVertices[m_NumVertices + 6*i].m_Pos.y = pArray[i].m_Y0;
			m_aVertices[m_NumVertices + 6*i].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 6*i].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 6*i + 1].m_Pos.x = pArray[i].m_X1;
			m_aVertices[m_NumVertices + 6*i + 1].m_Pos.y = pArray[i].m_Y1;
			m_aVertices[m_NumVertices + 6*i + 1].m_Tex = m_aTexture[1];
			m_aVertices[m_NumVertices + 6*i + 1].m_Color = m_aColor[1];

			m_aVertices[m_NumVertices + 6*i + 2].m_Pos.x = pArray[i].m_X3;
			m_aVertices[m_NumVertices + 6*i + 2].m_Pos.y = pArray[i].m_Y3;
			m_aVertices[m_NumVertices + 6*i + 2].m_Tex = m_aTexture[3];
			m_aVertices[m_NumVertices + 6*i + 2].m_Color = m_aColor[3];

			m_aVertices[m_NumVertices + 6*i + 3].m_Pos.x = pArray[i].m_X0;
			m_aVertices[m_NumVertices + 6*i + 3].m_Pos.y = pArray[i].m_Y0;
			m_aVertices[m_NumVertices + 6*i + 3].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 6*i + 3].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 6*i + 4].m_Pos.x = pArray[i].m_X3;
			m_aVertices[m_NumVertices + 6*i + 4].m_Pos.y = pArray[i].m_Y3;
			m_aVertices[m_NumVertices + 6*i + 4].m_Tex = m_aTexture[3];
			m_aVertices[m_NumVertices + 6*i + 4].m_Color = m_aColor[3];

			m_aVertices[m_NumVertices + 6*i + 5].m_Pos.x = pArray[i].m_X2;
			m_aVertices[m_NumVertices + 6*i + 5].m_Pos.y = pArray[i].m_Y2;
			m_aVertices[m_NumVertices + 6*i + 5].m_Tex = m_aTexture[2];
			m_aVertices[m_NumVertices + 6*i + 5].m_Color = m_aColor[2];
		}

		AddVertices(3*2*Num);
	}
	else
	{
		for(int i = 0; i < Num; ++i)
		{
			m_aVertices[m_NumVertices + 4*i].m_Pos.x = pArray[i].m_X0;
			m_aVertices[m_NumVertices + 4*i].m_Pos.y = pArray[i].m_Y0;
			m_aVertices[m_NumVertices + 4*i].m_Tex = m_aTexture[0];
			m_aVertices[m_NumVertices + 4*i].m_Color = m_aColor[0];

			m_aVertices[m_NumVertices + 4*i + 1].m_Pos.x = pArray[i].m_X1;
			m_aVertices[m_NumVertices + 4*i + 1].m_Pos.y = pArray[i].m_Y1;
			m_aVertices[m_NumVertices + 4*i + 1].m_Tex = m_aTexture[1];
			m_aVertices[m_NumVertices + 4*i + 1].m_Color = m_aColor[1];

			m_aVertices[m_NumVertices + 4*i + 2].m_Pos.x = pArray[i].m_X3;
			m_aVertices[m_NumVertices + 4*i + 2].m_Pos.y = pArray[i].m_Y3;
			m_aVertices[m_NumVertices + 4*i + 2].m_Tex = m_aTexture[3];
			m_aVertices[m_NumVertices + 4*i + 2].m_Color = m_aColor[3];

			m_aVertices[m_NumVertices + 4*i + 3].m_Pos.x = pArray[i].m_X2;
			m_aVertices[m_NumVertices + 4*i + 3].m_Pos.y = pArray[i].m_Y2;
			m_aVertices[m_NumVertices + 4*i + 3].m_Tex = m_aTexture[2];
			m_aVertices[m_NumVertices + 4*i + 3].m_Color = m_aColor[2];
		}

		AddVertices(4*Num);
	}
}

void CGraphics_PVR::QuadsText(float x, float y, float Size, const char *pText)
{
	float StartX = x;

	while(*pText)
	{
		char c = *pText;
		pText++;

		if(c == '\n')
		{
			x = StartX;
			y += Size;
		}
		else
		{
			QuadsSetSubset(
				(c%16)/16.0f,
				(c/16)/16.0f,
				(c%16)/16.0f+1.0f/16.0f,
				(c/16)/16.0f+1.0f/16.0f);

			CQuadItem QuadItem(x, y, Size, Size);
			QuadsDrawTL(&QuadItem, 1);
			x += Size/2;
		}
	}
}

int CGraphics_PVR::Init()
{
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_ScreenWidth = g_Config.m_GfxScreenWidth = 640;
	m_ScreenHeight = g_Config.m_GfxScreenHeight = 480;

	// Set all z to -5.0f
	for(int i = 0; i < MAX_VERTICES; i++)
		m_aVertices[i].m_Pos.z = -5.0f;

	// init textures
	m_FirstFreeTexture = 0;
	for(int i = 0; i < MAX_TEXTURES; i++)
		m_aTextures[i].m_Next = i+1;
	m_aTextures[MAX_TEXTURES-1].m_Next = -1;

	glKosInit();

	// set some default settings
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(0);

	// create null texture, will get id=0
	static const unsigned char aNullTextureData[] = {
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff,
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff,
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff,
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff,
	};

	m_InvalidTexture = LoadTextureRaw(4,4,CImageInfo::FORMAT_RGBA,aNullTextureData,CImageInfo::FORMAT_RGBA,TEXLOAD_NORESAMPLE);

	return 0;
}

void CGraphics_PVR::Shutdown()
{
	
}

void CGraphics_PVR::Minimize()
{
	
}

void CGraphics_PVR::Maximize()
{
	
}

int CGraphics_PVR::WindowActive()
{
	return 1;
}

int CGraphics_PVR::WindowOpen()
{
	return 1;
}

void CGraphics_PVR::NotifyWindow()
{
	
}

void CGraphics_PVR::TakeScreenshot(const char *pFilename)
{
	
}

void CGraphics_PVR::TakeCustomScreenshot(const char *pFilename)
{
	
}


void CGraphics_PVR::Swap()
{
	glKosSwapBuffers();
}


int CGraphics_PVR::GetVideoModes(CVideoMode *pModes, int MaxModes)
{
	pModes[0].m_Width = 640;
	pModes[0].m_Height = 480;
	pModes[0].m_Red = 8;
	pModes[0].m_Green = 8;
	pModes[0].m_Blue = 8;
	return 1;
}

// syncronization
void CGraphics_PVR::InsertSignal(semaphore *pSemaphore)
{
	//pSemaphore->signal();
}

bool CGraphics_PVR::IsIdle()
{
	return true;
}

void CGraphics_PVR::WaitForIdle()
{
}

extern IEngineGraphics *CreateEngineGraphics() { return new CGraphics_PVR(); }
