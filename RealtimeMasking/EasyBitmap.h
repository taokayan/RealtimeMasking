
#include "stdafx.h"

#include <gdiplus.h>

class EasyBitmap: public Gdiplus::Bitmap
{
public:
	EasyBitmap(int width, int height, int format);
	BOOL EasyLock();
	void EasyUnlock();
	BOOL EasyGetPixel(const int& x, const int& y, COLORREF& colorout);
	BOOL EasySetPixel(const int& x, const int& y, const COLORREF& colorin);
	BOOL EasyFill(const int& x, const int& y, const int& width, const int& height,
		const COLORREF& colorin);
	BOOL EasyErase(int x0, int y0, int width, int height);
protected:
	std::auto_ptr<Gdiplus::BitmapData> pBitmapData;
	BOOL locked;
};

