// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
// This file has been modified from Ken Silverman's original release

#include <glide.h>
#include "build.h"

extern int turnOn2dDrawing;

extern float global_s1, global_s2;
extern float global_t1, global_t2;
extern int global_resize;
extern int current_texture_height;
extern int current_texture_width;
extern int current_aspect_ratio;
extern float global_oow1;
extern float global_oow2;
extern unsigned short globalpicnum;

static float static_global_s1, static_global_s2;
static float static_global_t1, static_global_t2;

static int first_coord = 1;

float aspectToTexCoordFactors[7][2] = {
	256, 32,
	256, 64,
	256, 128,
	256, 256,
	128, 256,
	64, 256,
	32, 256
};

#define BITSOFPRECISION 3
#define BITSOFPRECISIONPOW 8

extern long asm1, asm2, asm3, asm4, fpuasm, globalx3, globaly3;
extern void *reciptable;

extern int _wp1;
extern int _wp2;
extern int _wp3;
extern int _wp4;
extern int _wp5;
extern int _wp6;
extern int _wp7;

static long bpl, transmode = 0;
static long glogx, glogy, gbxinc, gbyinc, gpinc;
static char *gbuf, *gpal, *ghlinepal, *gtrans;

	//Global variable functions
setvlinebpl(long dabpl) { bpl = dabpl; }
fixtransluscence(long datransoff) { gtrans = (char *)datransoff; }
settransnormal() { transmode = 0; setGlideTransMaskWallReverseState(); }
settransreverse() { transmode = 1; setGlideTransMaskWallState(); }


	//Ceiling/floor horizontal line functions
sethlinesizes(long logx, long logy, long bufplc)
	{ glogx = logx; glogy = logy; gbuf = (char *)bufplc; }
setpalookupaddress(char *paladdr) { ghlinepal = paladdr; }
setuphlineasm4(long bxinc, long byinc) { gbxinc = bxinc; gbyinc = byinc; }
hlineasm4(long cnt, long skiploadincs, long paloffs, unsigned long by, unsigned long bx, long p)
{
	char *palptr;

	palptr = (char *)&ghlinepal[paloffs];
	if (!skiploadincs) { gbxinc = asm1; gbyinc = asm2; }
	convertAndDownloadPaletteLookup(palptr);
	setupS1S2T1T2TexCoordsHoriz(by, gbyinc, bx, gbxinc, cnt);
	if (turnOn2dDrawing)
	{
		for(;cnt>=0;cnt--)
		{
			*((char *)p) = palptr[gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))]];
			bx -= gbxinc;
			by -= gbyinc;
			p--;
		}
	}
}

setupS1S2T1T2TexCoordsHoriz(long by, long byi, long bx, long bxi, long cnt)
{
	global_s1 = static_global_s1;
	global_s2 = static_global_s2;

	global_s1 *= by;
	global_s2 *= -byi;
	global_s2 *= cnt;
	global_s2 += global_s1;

	global_t1 = static_global_t1;
	global_t2 = static_global_t2;

	global_t1 *= bx;
	global_t2 *= -bxi;
	global_t2 *= cnt;
	global_t2 += global_t1;
}

setupS1S2T1T2TexCoordsHorizStaticStuff()
{
	static_global_s1 = 1.0 / (1 << (32 - glogy));
	if (!global_resize)
		static_global_s1 /= current_texture_height;
	else
		static_global_s1 /= tilesizy[globalpicnum];
	static_global_s1 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	static_global_s2 = 1.0 / (1 << (32 - glogy));
	if (!global_resize)
		static_global_s2 /= current_texture_height;
	else
		static_global_s2 /= tilesizy[globalpicnum];
	static_global_s2 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	static_global_t1 = 1.0 / (1 << (32 - glogx));
	if (!global_resize)
		static_global_t1 /= current_texture_width;
	else
		static_global_t1 /= tilesizx[globalpicnum];
	static_global_t1 *= aspectToTexCoordFactors[current_aspect_ratio][1];

	static_global_t2 = 1.0 / (1 << (32 - glogx));
	if (!global_resize)
		static_global_t2 /= current_texture_width;
	else
		static_global_t2 /= tilesizx[globalpicnum];
	static_global_t2 *= aspectToTexCoordFactors[current_aspect_ratio][1];
}

setupS1S2T1T2TexCoordsSlopeStaticStuff()
{
	static_global_s2 = 1.0;
	static_global_s2 /= (1 << (32 - glogy));
	if (!global_resize)
		static_global_s2 /= current_texture_height;
	else
		static_global_s2 /= tilesizy[globalpicnum];
	static_global_s2 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	static_global_t2 = 1.0;
	static_global_t2 /= (1 << (32 - glogx));
	if (!global_resize)
		static_global_t2 /= current_texture_width;
	else
		static_global_t2 /= tilesizx[globalpicnum];
	static_global_t2 *= aspectToTexCoordFactors[current_aspect_ratio][1];

	static_global_s1 = 1.0;
	static_global_s1 /= (1 << (32 - glogy));
	if (!global_resize)
		static_global_s1 /= current_texture_height;
	else
		static_global_s1 /= tilesizy[globalpicnum];
	static_global_s1 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	static_global_t1 = 1.0;
	static_global_t1 /= (1 << (32 - glogx));
	if (!global_resize)
		static_global_t1 /= current_texture_width;
	else
		static_global_t1 /= tilesizx[globalpicnum];
	static_global_t1 *= aspectToTexCoordFactors[current_aspect_ratio][1];
}

setupS1S2T1T2TexCoordsSlope(long bz, long bx, long by, long bzinc, long cnt)
{
	unsigned long u, v;
	int i;

	i = krecip(bz>>6);
	u = bx+globalx3*i;
	v = by+globaly3*i;

	global_oow1 = abs(i) * 0.00000001f;

	global_s2 = (float)by+(float)globaly3*(float)i;
	global_s2 *= static_global_s2;

	global_t2 = (float)bx+(float)globalx3*(float)i;
	global_t2 *= static_global_t2;

	bz += bzinc * (cnt - 1);

	i = krecip(bz >> 6);

	global_oow2 = abs(i) * 0.00000001f;

	global_s1 = (float)by+(float)globaly3*(float)i;
	global_s1 *= static_global_s1;

	global_t1 = (float)bx+(float)globalx3*(float)i;
	global_t1 *= static_global_t1;
}


	//Sloped ceiling/floor vertical line functions
setupslopevlin(long logylogx, long bufplc, long pinc)
{
	glogx = (logylogx&255); glogy = (logylogx>>8);
	gbuf = (char *)bufplc; gpinc = pinc;
}
slopevlin(long p, long i, long slopaloffs, long cnt, long bx, long by)
{
	long *slopalptr, bz, bzinc;
	unsigned long u, v;
	float fu, fv, fi, fbx, fby, fgbx3, fgby3, tempfloat;
	char buff[80];
	//int c2;

	bz = asm3; bzinc = (asm1>>3);
	setupS1S2T1T2TexCoordsSlope(bz, bx, by, bzinc, cnt);
	global_s1 *= global_oow1;
	global_t1 *= global_oow1;
	global_s2 *= global_oow2;
	global_t2 *= global_oow2;
	if (turnOn2dDrawing)
	{
		slopalptr = (long *)slopaloffs;
		//c2 = cnt;
		for(;cnt>0;cnt--)
		{
			i = krecip(bz>>6); bz += bzinc;
			u = bx+globalx3*i;
			v = by+globaly3*i;
			(*(char *)p) = *(char *)(slopalptr[0]+gbuf[((u>>(32-glogx))<<glogy)+(v>>(32-glogy))]);
			slopalptr--;
			p += gpinc;
		}
		//if (c2 == 25)
		//	first_coord = 0;
	}
}


	//Wall,face sprite/wall sprite vertical line functions
setupvlineasm(long neglogy) { glogy = neglogy; }
vlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	if (turnOn2dDrawing)
	{
		for(;cnt>=0;cnt--)
		{
			*((char *)p) = gpal[gbuf[vplc>>glogy]];
			p += bpl;
			vplc += vinc;
		}
	}
}

setupmvlineasm(long neglogy) { glogy = neglogy; }
mvlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	if (turnOn2dDrawing)
	{
		for(;cnt>=0;cnt--)
		{
			ch = gbuf[vplc>>glogy]; if (ch != 255) *((char *)p) = gpal[ch];
			p += bpl;
			vplc += vinc;
		}
	}
}

setuptvlineasm(long neglogy) { glogy = neglogy; }
tvlineasm1(long vinc, long paloffs, long cnt, unsigned long vplc, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	gpal = (char *)paloffs;
	if (turnOn2dDrawing)
	{
		if (transmode)
		{
			for(;cnt>=0;cnt--)
			{
				ch = gbuf[vplc>>glogy];
				if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
				p += bpl;
				vplc += vinc;
			}
		}
		else
		{
			for(;cnt>=0;cnt--)
			{
				ch = gbuf[vplc>>glogy];
				if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
				p += bpl;
				vplc += vinc;
			}
		}
	}
}

	//Floor sprite horizontal line functions
msethlineshift(long logx, long logy) { glogx = logx; glogy = logy; }
mhline(long bufplc, unsigned long bx, long cntup16, long junk, unsigned long by, long p)
{
	char ch;

	int t = cntup16 >> 16;

	gbuf = (char *)bufplc;
	gpal = (char *)asm3;
	convertAndDownloadPaletteLookup(gpal);
	global_s1 = by / (float)(1 << (32 - glogy));
	global_s1 /= current_texture_height;
	global_s1 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	global_s2 = asm2 / (float)(1 << (32 - glogy));
	global_s2 /= current_texture_height;
	global_s2 *= t * aspectToTexCoordFactors[current_aspect_ratio][0];
	global_s2 += global_s1;
	
	global_t1 = bx / (float)(1 << (32 - glogx));
	global_t1 /= current_texture_width;
	global_t1 *= aspectToTexCoordFactors[current_aspect_ratio][1];

	global_t2 = asm1 / (float)(1 << (32 - glogx));
	global_t2 /= current_texture_width;
	global_t2 *= t * aspectToTexCoordFactors[current_aspect_ratio][1];
	global_t2 += global_t1;



	if (turnOn2dDrawing)
	{
		for(cntup16>>=16;cntup16>0;cntup16--)
		{
			ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
			if (ch != 255) *((char *)p) = gpal[ch];
			bx += asm1;
			by += asm2;
			p++;
		}
	}
}

tsethlineshift(long logx, long logy) { glogx = logx; glogy = logy; }
thline(long bufplc, unsigned long bx, long cntup16, long junk, unsigned long by, long p)
{
	char ch;

	int t = cntup16 >> 16;

	gbuf = (char *)bufplc;
	gpal = (char *)asm3;
	convertAndDownloadPaletteLookup(gpal);
	global_s1 = by / (float)(1 << (32 - glogy));
	global_s1 /= current_texture_height;
	global_s1 *= aspectToTexCoordFactors[current_aspect_ratio][0];

	global_s2 = asm2 / (float)(1 << (32 - glogy));
	global_s2 /= current_texture_height;
	global_s2 *= t * aspectToTexCoordFactors[current_aspect_ratio][0];
	global_s2 += global_s1;
	
	global_t1 = bx / (float)(1 << (32 - glogx));
	global_t1 /= current_texture_width;
	global_t1 *= aspectToTexCoordFactors[current_aspect_ratio][1];

	global_t2 = asm1 / (float)(1 << (32 - glogx));
	global_t2 /= current_texture_width;
	global_t2 *= t * aspectToTexCoordFactors[current_aspect_ratio][1];
	global_t2 += global_t1;

	if (turnOn2dDrawing)
	{
		if (transmode)
		{
			for(cntup16>>=16;cntup16>0;cntup16--)
			{
				ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
				if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
				bx += asm1;
				by += asm2;
				p++;
			}
		}
		else
		{
			for(cntup16>>=16;cntup16>0;cntup16--)
			{
				ch = gbuf[((bx>>(32-glogx))<<glogy)+(by>>(32-glogy))];
				if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
				bx += asm1;
				by += asm2;
				p++;
			}
		}
	}
}


	//Rotatesprite vertical line functions
setupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
	setGlideTextureDecalState();
	convertAndDownloadPaletteLookup(gpal);
}
spritevline(long bx, long by, long cnt, long bufplc, long p)
{
	gbuf = (char *)bufplc;
	if (turnOn2dDrawing)
	{
		for(;cnt>1;cnt--)
		{
			(*(char *)p) = gpal[gbuf[(bx>>16)*glogy+(by>>16)]];
			bx += gbxinc;
			by += gbyinc;
			p += bpl;
		}
	}
}

	//Rotatesprite vertical line functions
msetupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
	setGlideTextureMaskState();
	convertAndDownloadPaletteLookup(gpal);
}
mspritevline(long bx, long by, long cnt, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	if (turnOn2dDrawing)
	{
		for(;cnt>1;cnt--)
		{
			ch = gbuf[(bx>>16)*glogy+(by>>16)];
			if (ch != 255) (*(char *)p) = gpal[ch];
			bx += gbxinc;
			by += gbyinc;
			p += bpl;
		}
	}
}

tsetupspritevline(long paloffs, long bxinc, long byinc, long ysiz)
{
	gpal = (char *)paloffs;
	gbxinc = bxinc;
	gbyinc = byinc;
	glogy = ysiz;
	setGlideTransMaskWallState();
	convertAndDownloadPaletteLookup(gpal);
}
tspritevline(long bx, long by, long cnt, long bufplc, long p)
{
	char ch;

	gbuf = (char *)bufplc;
	if (turnOn2dDrawing)
	{
		if (transmode)
		{
			for(;cnt>1;cnt--)
			{
				ch = gbuf[(bx>>16)*glogy+(by>>16)];
				if (ch != 255) *((char *)p) = gtrans[(*((char *)p))+(gpal[ch]<<8)];
				bx += gbxinc;
				by += gbyinc;
				p += bpl;
			}
		}
		else
		{
			for(;cnt>1;cnt--)
			{
				ch = gbuf[(bx>>16)*glogy+(by>>16)];
				if (ch != 255) *((char *)p) = gtrans[((*((char *)p))<<8)+gpal[ch]];
				bx += gbxinc;
				by += gbyinc;
				p += bpl;
			}
		}
	}
}

vlineasm4(int a, int b, int c, int d, int e, int f) {}
mvlineasm4(int a, int b, int c, int d, int e, int f) {}
voxoff(void) {}
