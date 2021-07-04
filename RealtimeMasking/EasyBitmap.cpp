
#include "stdafx.h"
#include "EasyBitmap.h"

EasyBitmap::EasyBitmap(int width, int height, int format)
	:Bitmap(width, height, format)
{
	locked = FALSE;
}

BOOL EasyBitmap::EasyLock()
{
	if( locked == TRUE) return FALSE;
	pBitmapData.reset(new Gdiplus::BitmapData());

	Gdiplus::Rect rec;
	rec.X = 0;
	rec.Width = this->GetWidth();
	rec.Y = 0;
	rec.Height = this->GetHeight();
	Gdiplus::Status r;
	if( (r = this->LockBits(&rec, Gdiplus::ImageLockModeWrite, 
		PixelFormat32bppRGB, &(*pBitmapData))) == 0)
	{
		locked = TRUE;
		return TRUE;
	}
	else
	{
		// r stores the fail reason...
		return FALSE;
	}

}

void EasyBitmap::EasyUnlock()
{
	if( locked == FALSE) return;
	this->UnlockBits(&(*pBitmapData));
	locked = FALSE;
}

BOOL EasyBitmap::EasyGetPixel(const int& x, const int& y, COLORREF& colorout)
{
	//if(locked == FALSE)return FALSE;
	//colorout = ((DWORD*)pBitmapData->Scan0ata)[y * (pBitmapData->Stride >> 2) + x];
	colorout = *(DWORD*)((char*)pBitmapData->Scan0 + y * pBitmapData->Stride + (x << 2));
	return TRUE;
}

BOOL EasyBitmap::EasySetPixel(const int& x, const int& y, const COLORREF& colorin)
{
	//if(locked == FALSE)return FALSE;
	//((DWORD*)pBitmapData->Scan0)[y * (pBitmapData->Stride >> 2) + x] = colorin;
	*(DWORD*)((char*)pBitmapData->Scan0 + y * pBitmapData->Stride + (x << 2)) = colorin;
	return TRUE;
}

BOOL EasyBitmap::EasyFill(const int& x, const int& y, const int& width, 
						  const int& height, const COLORREF& colorin)
{
	DWORD* start = (DWORD*)((char*)pBitmapData->Scan0 + y * pBitmapData->Stride + (x << 2));
	int dwordstride = ((pBitmapData->Stride) >> 2) - width;
	for(int py = 0; py < height; py++, start += dwordstride)
	{
		for(int px = 0; px < width; px++, start++)
		{
			*start = colorin;
		}
	}
	return TRUE;
}

BOOL EasyBitmap::EasyErase(int x0, int y0, int width, int height)
{
	int desty = y0 + height;
	char* p = (char*)pBitmapData->Scan0 + y0 * pBitmapData->Stride + (x0 << 2);
	if( locked == FALSE)return FALSE;
	for(int y = y0; y < desty; y++)
	{
		memset(p, 0, width * 4);
		p += pBitmapData->Stride;
	}
	return TRUE;
}
