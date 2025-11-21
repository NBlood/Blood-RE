#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "glide.h"
#include <math.h>
#include "texus.h"


static int dithmat2x2[2][2] = {
	2, 10,
	14, 6
};

static int dithmat4x4[4][4] = {
	0, 8, 2, 10,
	12, 4, 14, 6,
	3, 11, 1, 9,
	15, 7, 13, 5
};

static int dithmat16x2[2][16] = {
	0, 20, 15, 27, 2, 22, 13, 25, 1, 21, 14, 26, 3, 23,
	12, 24, 31, 11, 16, 4, 29, 9, 18, 6, 30, 10, 17, 5,
	28, 8, 19, 7
};

typedef struct
{
	int width;
	int height;
	void* data;
} ImgInfo;

int rotate;

void _glide_g1(ImgInfo* info, double gamma)
{
	FxU32 x, y;
	FxU32 r, g, b;
	FxU32 rgb;
	FxU32* data;
	FxU32 gamtable[256];
	char buff[80];

	for (x = 0; x < 256; x++)
	{
		gamtable[x] = (int)(pow((double)x / 255, 1.0 / gamma) * 255 + 0.5);
	}

	data = info->data;

	for (y = 0; y < info->height; y++)
	{
		for (x = 0; x < info->width; x++)
		{
			rgb = *data;
			r = (rgb >> 16) & 255;
			g = (rgb >> 8) & 255;
			b = rgb & 255;

			r = gamtable[r];
			g = gamtable[g];
			b = gamtable[b];

			rgb = (r << 16) | (g << 8) | b;
			*data++ = rgb;
		}
	}
}

void _glide_e1(ImgInfo* inf, int bits)
{
	unsigned long rgb;
	unsigned int x, y;
	int r, g, b;
	unsigned long* data;

	data = (unsigned long*)inf->data;

	if (bits == 8)
		return;

	for (y = 0; y < inf->height; y++)
	{
		for (x = 0; x < inf->width; x++)
		{
			rgb = *data;
			r = (rgb >> 16) & 255;
			g = (rgb >> 8) & 255;
			b = rgb & 255;

			if (bits == 3)
			{
				r += (r >> 3) + (r >> 6);
				g += (g >> 3) + (g >> 6);
				b += (b >> 2) + (b >> 4) + (b >> 6);
			}
			else if (bits == 4)
			{
				r += (r >> 4);
				g += (g >> 4);
				b += (b >> 4);
			}
			else if (bits == 5)
			{
				r += (r >> 5);
				g += (g >> 6);
				b += (b >> 5);
			}
			else if (bits == 6)
			{
				r += (r >> 6);
				g += (g >> 6);
				b += (b >> 6);
			}
			else if (bits == 15)
			{
				r += (r >> 5);
				g += (g >> 5);
				b += (b >> 5);
			}
			else
			{
				fprintf(stderr, "expand8", "invalid bit size %d\n", bits);
			}
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;

			rgb = (r << 16) | (g << 8) | b;
			*data++ = rgb;
		}
	}
}

void _glide_f1(ImgInfo* inf, int bits, int fwidth, int smart)
{
	long rgb;
	long wr, wg, wb;
	int x, y, z;
	int r[2048];
	int g[2048];
	int b[2048];
	int a[2048];
	int red;
	int grn;
	int blu;
	int maxRed;
	int maxGrn;
	int maxBlu;
	int OFF;
	unsigned long* data;

	OFF = fwidth / 2;

	data = (unsigned long*)inf->data;
	if (bits == 3)
	{
		maxRed = 48;
		maxGrn = 48;
		maxBlu = 64;
	}
	else if (bits == 4 || bits == 8)
	{
		maxRed = 32;
		maxGrn = 32;
		maxBlu = 32;
	}
	else if (bits == 5)
	{
		maxRed = 12;
		maxGrn = 6;
		maxBlu = 12;
	}
	else if (bits == 15)
	{
		maxRed = 12;
		maxGrn = 12;
		maxBlu = 12;
	}
	else
	{
		fprintf(stderr, "filter", "invalid bit size %d\n", bits);
	}

	for (y = 0; y < inf->height; y++)
	{
		int dr;
		int dg;
		int db;
		int or;
		int og;
		int ob;

		for (z = 0; z < inf->width; z++)
		{
			rgb = data[z];
			a[z] = (rgb >> 24) & 255;
			r[z] = (rgb >> 16) & 255;
			g[z] = (rgb >> 8) & 255;
			b[z] = rgb & 255;
		}
		for (x = 0; x < inf->width; x++)
		{
			red = grn = blu = 0;
			wr = wg = wb = fwidth;
			if (smart)
			{
				or = r[x];
				og = g[x];
				ob = b[x];

				red = or * fwidth;
				grn = og * fwidth;
				blu = ob * fwidth;

				for (z = 0; z < fwidth; z++)
				{
					int t;

					t = x + z - OFF;
					if (t >= 0 && t < inf->width)
					{
						dr = r[t] - or;
						dg = g[t] - og;
						db = b[t] - ob;
					}
					else
					{
						dr = dg = db = 0;
					}

					if (smart < 0)
					{
						if (dr > maxRed || dr < -maxRed)
						{
							dr = 0;
							wr--;
							red -= or;
						}
						if (dg > maxGrn || dg < -maxGrn)
						{
							dg = 0;
							wg--;
							grn -= og;
						}
						if (db > maxBlu || db < -maxBlu)
						{
							db = 0;
							wb--;
							blu -= ob;
						}
					}
					if (dr > maxRed)
						dr = maxRed;
					else if (dr < -maxRed)
						dr = -maxRed;
					red += dr;
					if (dg > maxGrn)
						dg = maxGrn;
					else if (dg < -maxGrn)
						dg = -maxGrn;
					grn += dg;
					if (db > maxBlu)
						db = maxBlu;
					else if (db < -maxBlu)
						db = -maxBlu;
					blu += db;
				}
			}
			else
			{
				for (z = 0; z < fwidth; z++)
				{
					int t;
					t = x + z - OFF;
					if (t < 0)
						t = 0;
					if (t >= inf->width)
						t = inf->width - 1;

					red += r[t];
					grn += g[t];
					blu += b[t];
				}
			}

			if (red < 0)
				red = 0;
			else
				red /= wr;
			if (grn < 0)
				grn = 0;
			else
				grn /= wg;
			if (blu < 0)
				blu = 0;
			else
				blu /= wb;

			if (a[x] & 1)
				red = or;
			if (a[x] & 2)
				grn = og;
			if (a[x] & 4)
				blu = ob;

			rgb = (red << 16) | (grn << 8) | blu;
			*data++ = rgb;
		}
	}
}

void _glide_s1(ImgInfo* inf, int bits, int dit, int subdit)
{
	unsigned long rgb;
	unsigned int x, y;
	int r, g, b;
	int dm;
	unsigned long* data;

	data = (unsigned long*)inf->data;

	if (bits == 8)
		return;

	for (y = 0; y < inf->height; y++)
	{
		for (x = 0; x < inf->width; x++)
		{
			rgb = *data;
			r = (rgb >> 16) & 255;
			g = (rgb >> 8) & 255;
			b = rgb & 255;
			if (dit < 0)
			{
				dm = dithmat2x2[y & 1][x & 1];
			}
			else if (rotate)
			{
				dm = dithmat4x4[x & 3][y & 3];
			}
			else
			{
				dm = dithmat4x4[y & 3][x & 3];
			}
			dm = 8 - dm;
			if (subdit < 0)
				dm >>= 1;
			if (bits == 3)
			{
				if (dit < 0)
					dm = 8 - dithmat16x2[y & 1][x & 15];
				r += dm << 1;
				g += dm << 1;
				b += dm << 2;
			}
			else if (bits == 4)
			{
				r += dm;
				g += dm;
				b += dm;
			}
			else if (bits == 5)
			{
				r += dm >> 1;
				g += dm >> 2;
				b += dm >> 1;
			}
			else if (bits == 6)
			{
				r += dm >> 2;
				g += dm >> 2;
				b += dm >> 2;
			}
			else if (bits == 15)
			{
				r += dm >> 1;
				g += dm >> 1;
				b += dm >> 1;
			}
			else
			{
				fprintf(stderr, "sdtr", "invalid bit size %d\n", bits);
			}
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;

			rgb = (r << 16) | (g << 8) | b;
			*data++ = rgb;
		}
	}
}
