#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <glide.h>
#include <math.h>
#include <texus.h>
#include <assert.h>
#include <graph.h>
#include "build.h"
#include "texcache.h"
#include "text.h"
#include <sys/stat.h>

float global_oow1 = 1;
float global_oow2 = 1;
int glideNoPaletteLookups = 0;
static int numOfTris = 0;
static int showTriCount = 0;
static int showVersionNumber = 0;
static int showFPS = 0;
int glideXRes = 0;
int glideYRes = 0;
static int useTexus = 0;
static int convertTextures = 0;
static int glideLargeMemory = 0;
int noWallFog = 0;
int use512x384 = 1;
static char glideInit = 0;
static FxU32 memAllocated = 0;
static FxU32* glideReserveMem = NULL;
//static char permanentlock = 255;
static FxU32 glideWinOpened = 0;
//static int splashFrameNum = 0;

typedef struct
{
	FxBool converted;
	FxBool resized;
	FxBool clamp;
	long id;
} textureId;

typedef struct
{
	int width;
	int height;
	void* data;
} ImgInfo;

int global_resize;
float global_z;
float global_t1;
float global_t2;
int animationFrameNumber;
float global_s1;
float global_s2;
static float CORRECTIONSCALARFOR8X6;
static float CORRECTIONSCALAR;
static TexCache texCache;
static GrHwConfiguration hwConfig;
static TexCache texCache2;
static FxU32 mirrorColor;
FxU16* current_texture_data;
static float CORRECTION;
GrAspectRatio_t current_aspect_ratio;
long current_texture_height;
long current_texture_width;
static char tribuff[120];
static FxU16 txBuff[16384];
static char version[40];
static FxU8 gammaTable[256];

extern int _wp1, _wp2, _wp3, _wp4, _wp5, _wp6, _wp7, _wp8, _wp9, _wp10;

static FxU32 CLAMP_STATE;
static FxU32 AF_STATE;
static FxU32 BI_STATE;
static FxU32 CC_STATE;
static FxU32 paletteState;
static FxU32 AC_STATE;

static textureId picnumToTexid[MAXTILES];
static FxU32 global_palette[256];

//static FILE* fp;

long GetLargePow2(long value);

void setGlideStateBilinear(int b)
{
	if (b)
	{
		if (BI_STATE != 1)
		{
			grTexFilterMode(0, 1, 1);
			BI_STATE = 1;
		}
	}
	else
	{
		if (BI_STATE != 2)
		{
			grTexFilterMode(0, 0, 0);
		}
		BI_STATE = 2;
	}
}

void setGlideClampState(void)
{
	if (CLAMP_STATE != 1)
		grTexClampMode(0, 1, 1);
	CLAMP_STATE = 1;
}

void setGlideWrapState(void)
{
	if (CLAMP_STATE != 2)
		grTexClampMode(0, 0, 0);
	CLAMP_STATE = 2;
}

void setMirrorMode(void)
{
	grConstantColorValue(mirrorColor);
	if (CC_STATE != 5)
		grColorCombine(3, 1, 1, 1, 0);
	CC_STATE = 5;
}

void clearMirrorMode(void)
{
}

void setGlideConstantColorState(FxU32 color)
{
	grConstantColorValue(color);
	if (CC_STATE != 1)
		grColorCombine(1, 0, 1, 2, 0);
	CC_STATE = 1;
	if (AF_STATE != 1)
		grAlphaBlendFunction(4, 0, 4, 0);
	AF_STATE = 1;
}

void setGlideVertexColorState(char col)
{
	if (CC_STATE != 3)
		grColorCombine(1, 0, 0, 2, 0);
	CC_STATE = 3;
	if (AF_STATE != 1)
		grAlphaBlendFunction(4, 0, 4, 0);
	AF_STATE = 1;
}

void setGlideTextureDecalState(void)
{
	if (CC_STATE != 2)
		grColorCombine(3, 8, 1, 1, 0);
	CC_STATE = 2;
	if (AF_STATE != 1)
		grAlphaBlendFunction(4, 0, 4, 0);
	AF_STATE = 1;
}

void setGlideTextureMaskState(void)
{
	if (CC_STATE != 2)
		grColorCombine(3, 8, 1, 1, 0);
	CC_STATE = 2;
	if (AC_STATE != 2)
		grAlphaCombine(3, 8, 1, 1, 0);
	AC_STATE = 2;
	if (AF_STATE != 2)
		grAlphaBlendFunction(1, 5, 4, 0);
	AF_STATE = 2;
}

void setGlideTransMaskWallState(void)
{
	if (CC_STATE != 2)
		grColorCombine(3, 8, 1, 1, 0);
	CC_STATE = 2;
	if (AC_STATE != 3)
		grAlphaCombine(3, 1, 1, 1, 0);
	grConstantColorValue(0x80000000);
	AC_STATE = 3;
	if (AF_STATE != 2)
		grAlphaBlendFunction(1, 5, 4, 0);
	AF_STATE = 2;
}

void setGlideTransMaskFloorState(void)
{
	if (CC_STATE != 2)
		grColorCombine(3, 8, 1, 1, 0);
	CC_STATE = 2;
	if (AC_STATE != 3)
		grAlphaCombine(3, 1, 1, 1, 0);
	grConstantColorValue(0x80000000);
	AC_STATE = 3;
	if (AF_STATE != 2)
		grAlphaBlendFunction(1, 5, 4, 0);
	AF_STATE = 2;
}

void setGlideTransMaskWallReverseState(void)
{
	if (CC_STATE != 2)
		grColorCombine(3, 8, 1, 1, 0);
	CC_STATE = 2;
	if (AC_STATE != 2)
		grAlphaCombine(3, 8, 1, 1, 0);
	AC_STATE = 2;
	if (AF_STATE != 3)
		grAlphaBlendFunction(1, 5, 4, 0);
	AF_STATE = 3;
}

void setGlideBufferFront(void)
{
	grRenderBuffer(0);
}

void setGlideBufferBack(void)
{
	grRenderBuffer(1);
}

void setGlideDefaultState(void)
{
	grTexCombine(0, 1, 0, 0, 0, 0, 0);
	guTexCombineFunction(0, 1);
	grTexClampMode(0, 0, 0);
	grDepthMask(0);
	grDepthBufferMode(0);
	grTexFilterMode(0, 1, 1);
	grClipWindow(1, 1, glideXRes - 1, glideYRes - 1);
	CC_STATE = -1;
	AF_STATE = -1;
	AC_STATE = -1;
	BI_STATE = 1;
	CLAMP_STATE = 2;
}

void terminateGlide(void)
{
	if (glideInit)
	{
		grSstWinClose();
		grGlideShutdown();
	}
	glideInit = 0;
}

#pragma aux setvmode =\
	"int 0x10",\
	parm [eax]\

void initglide(void)
{
	if (glideInit)
		return;
	printf("\r\nIf you get a \"Fatal Error,\" you need to download the Glide drivers from \n\rhttp://www.3dfx.com\n\n");
	grGlideInit();
	_clearscreen(0);
	printf("Glide initialized.\n");
	if (!grSstQueryHardware(&hwConfig))
	{
		setvmode(3);
		fprintf(stderr, "Failed grSstQueryHardware");
		exit(-1);
	}
	if (hwConfig.SSTs[0].type == GR_SSTTYPE_SST96)
	{
		setvmode(3);
		fprintf(stderr, "Only Voodoo Graphics is supported.\n");
		exit(-1);
	}
	grSstSelect(0);
	if (grTexMaxAddress(0) > 0x200000)
		glideLargeMemory = 1;
	else if (hwConfig.num_sst > 1)
		glideLargeMemory = 1;
	if (glideLargeMemory)
	{
		if (hwConfig.num_sst > 1)
		{
			if (!TexCacheInit(&texCache, grTexMinAddress(0), grTexMaxAddress(0), MAXTILES, 0, "buildCache"))
			{
				setvmode(3);
				fprintf(stderr, "Failed to initialize texture cache.\n");
				exit(-1);
			}
			if (!TexCacheInit(&texCache2, grTexMinAddress(1), grTexMaxAddress(1), MAXTILES, 1, "buildCache2"))
			{
				setvmode(3);
				fprintf(stderr, "Failed to initialize texture cache.\n");
				exit(-1);
			}
		}
		else
		{
			if (!TexCacheInit(&texCache, grTexMinAddress(0), 0x200000, MAXTILES, 0, "buildCache"))
			{
				setvmode(3);
				fprintf(stderr, "Failed to initialize texture cache.\n");
				exit(-1);
			}
			if (!TexCacheInit(&texCache2, 0x200000, grTexMaxAddress(0), MAXTILES, 0, "buildCache2"))
			{
				setvmode(3);
				fprintf(stderr, "Failed to initialize texture cache.\n");
				exit(-1);
			}
		}
	}
	else
	{
		if (!TexCacheInit(&texCache, grTexMinAddress(0), grTexMaxAddress(0), MAXTILES, 0, "buildCache"))
		{
			setvmode(3);
			fprintf(stderr, "Failed to initialize texture cache.\n");
			exit(-1);
		}
	}

	if (atoi(getenv("BUILD_640X480")))
		use512x384 = 0;

	glideInit = 1;
	atexit(terminateGlide);
}

void glideBufferSwap()
{
	if (showVersionNumber)
	{
		glideWriteString(15, glideYRes-15, "BUILD ENGINE - 3DFX VERSION 1.0i - 11/04/1997");
		glideWriteString(15, glideYRes-7, version);
	}
	if (showFPS)
	{
		sprintf(tribuff, "clock  = %d", totalclock);
		glideWriteString(glideXRes - 150, 10, tribuff);
		sprintf(tribuff, "frames = %d", numframes);
		glideWriteString(glideXRes - 150, 18, tribuff);
		sprintf(tribuff, "fps    = %f", (120.f / totalclock) * numframes);
		glideWriteString(glideXRes - 150, 26, tribuff);
	}
	if (showTriCount)
	{
		sprintf(tribuff, "NUMTRIS:%d", numOfTris);
		glideWriteString(glideXRes - 150, 34, tribuff);
		numOfTris = 0;
		sprintf(tribuff, "RESOLUTION:%dx%d", glideXRes, glideYRes);
		glideWriteString(glideXRes - 150, 42, tribuff);
		numOfTris = 0;
	}
	grBufferSwap(1);
}

extern int renderingMirror;

void drawGlideLineVert(long x1, long y1, long x2, long y2)
{
	GrVertex a, b, c;

	if (renderingMirror)
	{
		x1 = glideXRes - 1 - x1;
		x2 = glideXRes - 1 - x2;
		setMirrorMode();
	}

	a.x = x1 + 0.5;
	a.y = y1 + 0.5;
	a.a = global_z;
	a.oow = global_oow1;
	b.x = x2 + 0.5;
	b.y = y2 + 0.5;
	b.a = global_z;
	b.oow = c.oow = global_oow2;
	c.x = x1 + 1.5f;
	c.y = y1 + 0.5;
	c.a = global_z;
	a.tmuvtx[0].sow = global_s1;
	a.tmuvtx[0].tow = global_t1;
	b.tmuvtx[0].sow = global_s2;
	b.tmuvtx[0].tow = global_t2;
	c.tmuvtx[0].sow = global_s2;
	c.tmuvtx[0].tow = global_t2;
	grDrawTriangle(&a, &c, &b);
	if (renderingMirror)
		clearMirrorMode();

	numOfTris++;
}

void drawGlideLineHoriz(long x1, long y1, long x2, long y2)
{
	GrVertex a, b, c;

	if (renderingMirror)
	{
		x1 = glideXRes - 1 - x1;
		x2 = glideXRes - 1 - x2;
		setMirrorMode();
	}

	a.x = x1 + 0.5;
	a.y = y1 + 0.5;
	a.a = global_z;
	a.oow = 1;
	b.x = x2 + 0.5;
	b.y = y2 + 0.5;
	b.a = global_z;
	b.oow = c.oow = 1;
	c.x = x2 + 0.5f;
	c.y = y2 + 1.5;
	c.a = global_z;
	a.tmuvtx[0].sow = global_s1;
	a.tmuvtx[0].tow = global_t1;
	b.tmuvtx[0].sow = global_s2;
	b.tmuvtx[0].tow = global_t2;
	c.tmuvtx[0].sow = global_s2;
	c.tmuvtx[0].tow = global_t2;
	grDrawTriangle(&a, &c, &b);
	if (renderingMirror)
		clearMirrorMode();

	numOfTris++;
}

void drawGlidePoint(FxU32 x1, FxU32 y1)
{
	GrVertex a;

	a.x = x1 + 0xc0000;
	a.y = y1 + 0xc0000;
	grDrawPoint(&a);
}

GrAspectRatio_t DimensionsToAspectRatio(int width, int height)
{
	int aspect;

	aspect = (width * 8) / height;
	switch (aspect)
	{
		case 64:
			return GR_ASPECT_8x1;
		case 32:
			return GR_ASPECT_4x1;
		case 16:
			return GR_ASPECT_2x1;
		case 8:
			return GR_ASPECT_1x1;
		case 4:
			return GR_ASPECT_1x2;
		case 2:
			return GR_ASPECT_1x4;
		case 1:
			return GR_ASPECT_1x8;
	}
	return GR_ASPECT_1x8;
}

GrLOD_t GetLod(long width, long height)
{
	if (height > width)
	{
		width = height;
	}

	switch (width)
	{
		case 256:
			return GR_LOD_256;
		case 128:
			return GR_LOD_128;
		case 64:
			return GR_LOD_64;
		case 32:
			return GR_LOD_32;
		case 16:
			return GR_LOD_16;
		case 8:
			return GR_LOD_8;
		case 4:
			return GR_LOD_4;
		case 2:
			return GR_LOD_2;
		case 1:
			return GR_LOD_1;
	}

	printf("GetLod: texture too large\n");
	printf("Width = %d,  Height = %d\n", width, height);
	terminateGlide();

	return GR_LOD_1;
}

char* strtoupper(char* str)
{
	unsigned int i;
	char* pt;

	pt = str;
	for (i = 0; i < strlen(str); i++, pt++)
	{
		*pt = toupper(*pt);
	}

	return str;
}

void glideWriteString(int xpos, int ypos, char* name)
{
	int i;
	GrLfbInfo_t info;

	strtoupper(name);

	for (i = 0; name[i]; i++)
	{
		grLfbWriteRegion(1, xpos + i * 6, ypos, 0, 4, 5, 8,
			&txtChar[(name[i] - 32) * 10]);
		grLfbWriteRegion(0, xpos + i * 6, ypos, 0, 4, 5, 8,
			&txtChar[(name[i] - 32) * 10]);
	}
}

void blitTexture(FxU8* texaddr, int width, int height, int offsetx, int offsety)
{
	GrLfbInfo_t info;
	int x, y;

	info.size = sizeof(GrLfbInfo_t);
	if (!grLfbLock(1, 0, 4, 0, 0, &info))
		return;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			FxU32* destpixel;
			FxU8* srcpixel;

			destpixel = (FxU32*)((char*)info.lfbPtr + (y + offsety) * info.strideInBytes + (x + offsetx) * 4);
			srcpixel = texaddr + y * width + x;
			*destpixel = global_palette[*srcpixel];
		}
	}

	grLfbUnlock(1, 0);
}

extern int viewTileNumber;

FxBool setGlideTexInfo(long width, long height, FxU8* texaddr, short picnum, int mask)
{
	GrTexInfo* texInfo;
	Gu3dfInfo info;
	FxU32 tex_mem_required;
	int i, j, h, w;
	FxU32 currMemAllocated = 0;
	FILE* tdf;
	char filename[32];
	char dirname[80];

	if (glideReserveMem)
	{
		free(glideReserveMem);
		glideReserveMem = NULL;
		preCacheBigTextures();
	}

	if (width == 0 || height == 0)
		return FXFALSE;

	if (picnumToTexid[picnum].id == 0 || picnum == viewTileNumber || animationFrameNumber)
	{
		int target_width = GetLargePow2(width);
		int target_height = GetLargePow2(height);
		if (memAllocated + sizeof(GrTexInfo) > 0x500000)
		{
			TexCacheClear(&texCache);
			if (glideLargeMemory)
				TexCacheClear(&texCache2);
			memset(picnumToTexid, 0, sizeof(picnumToTexid));
			memAllocated = currMemAllocated;
		}
		texInfo = (GrTexInfo*)myAlloc(sizeof(GrTexInfo));
		currMemAllocated += sizeof(GrTexInfo);
		memAllocated += sizeof(GrTexInfo);
		if (!texInfo)
		{
			setvmode(3);
			fprintf(stderr, "Out of memory (texInfo) in setGlideTexInfo\n");
			exit(-1);
		}
		if (target_width > 256 || target_height > 256)
		{
			tex_mem_required = txInit3dfInfo(&info, 5, &target_height, &target_width, 1, 0x2000);
		}
		else
		{
			tex_mem_required = txInit3dfInfo(&info, 5, &target_height, &target_width, 1, 0x1000);
		}

		if (memAllocated + tex_mem_required > 0x500000)
		{
			TexCacheClear(&texCache);
			if (glideLargeMemory)
				TexCacheClear(&texCache2);
			memset(picnumToTexid, 0, sizeof(picnumToTexid));
			memAllocated = currMemAllocated;
		}
		info.data = myAlloc(tex_mem_required);
		currMemAllocated += tex_mem_required;
		memAllocated += tex_mem_required;
		if (!info.data)
		{
			setvmode(3);
			fprintf(stderr, "Out of memory (info.data) in setGlideTexInfo.\n");
			exit(-1);
		}
		if (memAllocated + tex_mem_required * 2 > 0x500000)
		{
			TexCacheClear(&texCache);
			if (glideLargeMemory)
				TexCacheClear(&texCache2);
			memset(picnumToTexid, 0, sizeof(picnumToTexid));
			memAllocated = currMemAllocated;
		}
		current_texture_data = (FxU16*)myAlloc(tex_mem_required * 2);
		if (!current_texture_data)
		{
			setvmode(3);
			fprintf(stderr, "Out of memory for current_texture_data in setGlideTexInfo\n");
			exit(-1);
		}

		currMemAllocated += tex_mem_required * 2;
		memAllocated += tex_mem_required * 2;
		memset(current_texture_data, 0, tex_mem_required * 2);

		if (target_height < height || target_width < width)
		{
			if (!convertTextures)
			{
				if (animationFrameNumber)
				{
					sprintf(dirname, "anim%d", picnum);
					mkdir(dirname);
					sprintf(filename, "anim%d\\tx%d.3df", picnum, animationFrameNumber);
				}
				else
				{
					sprintf(filename, "tx3dfx\\tx%d.3df", picnum);
				}

				if (!gu3dfLoad(filename, &info))
				{
					if (useTexus)
					{
						txConvert(&info, GR_TEXFMT_P_8, height, width, texaddr,
							0x10000, global_palette);
					}
					else
					{
						resample(texaddr, info.data, height, width, target_height, target_width);
					}
					tdf = fopen(filename, "wb");
					if (tdf)
					{
						txWrite(&info, tdf, 0);
						fclose(tdf);
					}
					else
					{
						setvmode(3);
						fprintf(stderr, "Can't open the file %s for saving textures\n", filename);
						exit(-1);
					}
				}

				picnumToTexid[picnum].resized = FXTRUE;
				processTexture(target_width, target_height, target_width, target_height,
					info.data, mask, current_texture_data, picnum, 0);
			}
			else
			{
				resample(texaddr, info.data, height, width, target_height, target_width);
				picnumToTexid[picnum].resized = FXTRUE;
				processTexture(target_width, target_height, target_width, target_height,
					info.data, mask, current_texture_data, picnum, 0);
			}
		}
		else if (picnum == 328 || picnum == 3338 || picnum == 3353)
		{
			if (useTexus)
			{
				txConvert(&info, GR_TEXFMT_P_8, height, width, texaddr,
					0x10000, global_palette);
			}
			else
			{
				resample(texaddr, info.data, height, width, target_height, target_width);
				processTexture(target_width, target_height, target_width, target_height,
					info.data, mask, current_texture_data, picnum, 0);
				picnumToTexid[picnum].resized = FXTRUE;
			}
		}
		else
		{
			picnumToTexid[picnum].resized = FXFALSE;
			processTexture(width, height, target_width, target_height,
				texaddr, mask, current_texture_data, picnum, 0);
		}

		myFree(info.data);
		currMemAllocated -= tex_mem_required;
		memAllocated -= tex_mem_required;

		if (width < target_width)
		{
			for (i = 0; i < target_height; i++)
			{
				current_texture_data[width * target_height + i] = current_texture_data[(width - 1) * target_height + i];
			}
		}
		if (height < target_height)
		{
			for (i = 0; i < target_width; i++)
			{
				*(current_texture_data + i * target_height + height) = *(current_texture_data + i * target_height + height - 1);
			}
		}
		if (height < target_height)
		{
			for (i = 0; i < target_width; i++)
			{
				current_texture_data[i * target_height + target_height - 1] = current_texture_data[i * target_height] & 0xff;
			}
		}
		if (width < target_width)
		{
			for (i = 0; i < target_height; i++)
			{
				current_texture_data[(target_width - 1) * target_height + i] = current_texture_data[i] & 0xff;
			}
		}

		texInfo->smallLod = info.header.small_lod;
		texInfo->largeLod = info.header.large_lod;
		texInfo->aspectRatio = info.header.aspect_ratio;
		texInfo->format = 14;
		texInfo->data = current_texture_data;

		if (glideLargeMemory && picnum % 2 != 0)
		{
			picnumToTexid[picnum].id = TexCacheInsertTexture(&texCache2, texInfo, target_width, target_height);
		}
		else
		{
			picnumToTexid[picnum].id = TexCacheInsertTexture(&texCache, texInfo, target_width, target_height);
		}

		if (picnumToTexid[picnum].id == -1)
		{
			setvmode(3);
			fprintf(stderr, "Not enough space left in texture cache");
			exit(-1);
		}
	}

	if (glideLargeMemory && picnum % 2 != 0)
	{
		TexCacheSetCurrent(&texCache2, picnumToTexid[picnum].id);
	}
	else
	{
		TexCacheSetCurrent(&texCache, picnumToTexid[picnum].id);
	}

	global_resize = picnumToTexid[picnum].resized;
	current_aspect_ratio = DimensionsToAspectRatio(current_texture_height, current_texture_width);

	return FXTRUE;
}

void convertAndDownloadPaletteLookup(char* lookupTable)
{
	int i;
	FxU32 newPalette[256];

	if (glideNoPaletteLookups)
		return;

	if ((FxU32)lookupTable == paletteState)
		return;

	paletteState = (FxU32)lookupTable;

	for (i = 0; i < 256; i++)
	{
		newPalette[i] = global_palette[lookupTable[i]];
	}

	grFogColorValue(newPalette[0]);
	grTexDownloadTable(0, 2, newPalette);
}

typedef struct
{
	FxU8 r;
	FxU8 g;
	FxU8 b;
} palType;

void convertAndDownloadPalette(void* dlpalette)
{
	int i;

	FxU32 newpal[256];
	palType* convertpal;

	convertpal = (palType*)dlpalette;

	for (i = 0; i < 256; i++)
	{
		newpal[i] = (gammaTable[convertpal[i].r] << 16) | (gammaTable[convertpal[i].g] << 8)
			| gammaTable[convertpal[i].b];

		global_palette[i] = newpal[i];
	}

	global_palette[255] = newpal[255] = 0;

	grTexDownloadTable(0, 2, newpal);
}

void drawSampleQuad(void)
{
	GrVertex a, b, c, d;

	a.oow = 1;
	b = c = d = a;
	a.x = c.x = 0;
	a.y = b.y = 0;
	b.x = d.x = 255;
	c.y = d.y = 255;
	a.tmuvtx[0].sow = c.tmuvtx[0].sow = 0;
	b.tmuvtx[0].sow = d.tmuvtx[0].sow = 255;
	a.tmuvtx[0].tow = b.tmuvtx[0].tow = 0;
	c.tmuvtx[0].tow = d.tmuvtx[0].tow = 255;

	grDrawTriangle(&a, &d, &c);
	grDrawTriangle(&a, &b, &d);

	numOfTris += 2;
}

void drawSwatch(void)
{
	int i;
	GrVertex a, b;

	a.oow = 1;
	b.oow = 1;
	a.x = b.x = 300;
	a.y = 300;
	b.y = current_texture_height + 300;
	a.tmuvtx[0].sow = 0;
	a.tmuvtx[0].tow = 0;
	b.tmuvtx[0].sow = 0;
	b.tmuvtx[0].tow = 256;

	setGlideTextureMaskState();
	grRenderBuffer(0);
	for (i = 0; i < current_texture_width; i++)
	{
		grDrawLine(&a, &b);
		a.x += 1;
		b.x += 1;
		b.tmuvtx[0].sow = a.tmuvtx[0].sow = (float)i / (float)current_texture_width * 255.f;
	}
	grRenderBuffer(1);
}

long GetLargePow2(long value)
{
	long i;
	if ((value & (value - 1)) == 0)
		return value;

	for (i = 1; i < value; i += i)
	{
	}

	return i;
}

FILE* debugfp;

void myprintf(const char* fmt, ...)
{
	char msg[80];
	va_list argptr;

	if (!debugfp)
		return;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);
	fprintf(debugfp, msg);
	fflush(debugfp);
}

void myprintftx(const char* fmt, ...)
{
	char msg[80];
	FILE* fp;
	va_list argptr;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	fp = fopen("debugtx.txt", "a");
	if (!fp)
	{
		fprintf(stderr, "Could not open debug file\n");
	}
	fprintf(fp, msg);
	fclose(fp);
}

void PrintCacheStats(void)
{
	TexCachePrintState(&texCache);
}

int doGlideTransparency(short p1, short p2, short picnum)
{
	return picnum >= p1 && picnum <= p2;
}

void processTexture(int src_width, int src_height, int dst_width, int dst_height,
	void* src, int mask, FxU16* dest, short picnum, int hackTexture)
{
	int w, h;
	int doTrans = 0;

	mask = 1;

	for (h = 0; h < src_width; h++)
	{
		for (w = 0; w < src_height; w++)
		{
			FxU8 pixel = *((FxU8*)src + (h * src_height + w));
			if (!mask)
			{
				*(dest + (h * dst_height + w)) = 0xff00 | pixel;
			}
			else
			{
				if (pixel != 255)
				{
					if (hackTexture)
					{
						FxU8* texelLeft = NULL;
						FxU8* texelRight = NULL;
						if (w)
						{
							texelLeft = (FxU8*)src + (h * src_height + w - 1);
						}
						if (w != src_height - 1)
						{
							texelRight = (FxU8*)src + (h * src_height + w + 1);
						}
						if (*texelLeft == 255 || *texelRight == 255)
						{
							*(dest + (h * dst_height + w)) = pixel;
						}
						else
						{
							*(dest + (h * dst_height + w)) = 0xff00 | pixel;
						}
					}
					else
					{
						if (doTrans)
						{
							*(dest + (h * dst_height + w)) = (calculateTransparentLevel(pixel) << 8) | pixel;
						}
						else
						{
							*(dest + (h * dst_height + w)) = 0xff00 | pixel;
						}
					}
				}
				else
				{
					FxU8* transTexelLeft = NULL;
					FxU8* transTexelRight = NULL;
					FxU8* transTexelAbove = NULL;
					FxU8* transTexelBelow = NULL;
					if (w)
					{
						transTexelLeft = (FxU8*)src + (h * src_height + w - 1);
					}
					if (w != src_height - 1)
					{
						transTexelRight = (FxU8*)src + (h * src_height + w + 1);
					}
					if (h)
					{
						transTexelAbove = (FxU8*)src + ((h - 1) * src_height + w);
					}
					if (h != src_width - 1)
					{
						transTexelBelow = (FxU8*)src + ((h + 1) * src_height + w);
					}

					if (transTexelRight)
						if (*transTexelRight != 255)
					{
						*(dest + (h * dst_height + w)) = *transTexelRight;
						continue;
					}
					if (transTexelLeft)
						if (*transTexelLeft != 255)
					{
						*(dest + (h * dst_height + w)) = *transTexelLeft;
						continue;
					}
					if (transTexelAbove)
						if (*transTexelAbove != 255)
					{
						*(dest + (h * dst_height + w)) = *transTexelAbove;
						continue;
					}
					if (transTexelBelow)
						if (*transTexelBelow != 255)
					{
						*(dest + (h * dst_height + w)) = *transTexelBelow;
						continue;
					}
					*(dest + (h * dst_height + w)) = pixel;
				}
			}
		}
	}
}

void glideClearview(long color)
{
	grBufferClear(color, 0, 0);
}

void 
PokeStringMono(char Attr, char* String)
    {
    // Scrolls the monochrome display up one line, then prints the
    // string with the desired attribute on the bottom line.

    // EXAMPLE: PokeStringMono(MDA_NORMAL, "Hello, world.");

	char* Src, * Dest;
    long* s, * d;
    static char MonoBuf[4000];
    
    #define BASE (MonoBuf)
    #define LINE_SIZE 160
    #define LINE_OFFSET(num) ((80*2)*(num))
    
    // First scroll the screen up one line.
    // copy lines 1-24 up to 0
    s = (long*)(BASE + LINE_SIZE);
    d = (long*)BASE;
    memmove(d,s,LINE_OFFSET(24));

    // clear bottom line
    Dest = BASE + LINE_OFFSET(24);
    //memset(Dest,0,LINE_SIZE);
    memset(Dest,'.',LINE_SIZE);

    // Now print the string on the bottom line.
    Src = String;
    Dest = BASE + LINE_OFFSET(24);
    
    while (*Src)
        {
        *Dest++ = *Src++;
        *Dest++ = Attr;
        }
    
    memcpy((char *)0xB0000, MonoBuf, sizeof(MonoBuf));    
    }

int calculateTransparentLevel(int pixel)
{
	float col;

	col = global_palette[pixel];
	col /= 16777215.f;
	col *= 255.f;
	return col;
}

void drawGlideQuad(long x1, long y1, long x2, long y2, long x3, long y3, long x4, long y4,
	float s1, float t1, float s2, float t2, float s3, float t3, float s4, float t4)
{
	GrVertex v1, v2, v3, v4;
	char buff[255];

	if (renderingMirror)
	{
		x1 = glideXRes - 1 - x1;
		x2 = glideXRes - 1 - x2;
		x3 = glideXRes - 1 - x3;
		x4 = glideXRes - 1 - x4;
		setMirrorMode();
	}

	v1.oow = 1;
	v1.a = global_z;

	v2 = v3 = v4 = v1;

	v1.x = x1;
	v1.y = y1;
	v1.tmuvtx[0].sow = s1;
	v1.tmuvtx[0].tow = t1;
	v2.x = x2;
	v2.y = y2;
	v2.tmuvtx[0].sow = s2;
	v2.tmuvtx[0].tow = t2;
	v3.x = x3;
	v3.y = y3;
	v3.tmuvtx[0].sow = s3;
	v3.tmuvtx[0].tow = t3;
	v4.x = x4;
	v4.y = y4;
	v4.tmuvtx[0].sow = s4;
	v4.tmuvtx[0].tow = t4;

	grDrawTriangle(&v1, &v4, &v3);
	grDrawTriangle(&v1, &v2, &v4);
	if (renderingMirror)
		clearMirrorMode();
	numOfTris += 2;
}

void drawGlideOverheadLine(long x1, long y1, long x2, long y2, char col)
{
	GrVertex a, b;
	float myx1, myy1, myx2, myy2;

	myx1 = x1 * (1.f/4096.f);
	myy1 = y1 * (1.f/4096.f);
	myx2 = x2 * (1.f/4096.f);
	myy2 = y2 * (1.f/4096.f);

	a.oow = 1;

	b = a;

	a.x = myx1 + 786432;
	a.y = myy1 + 786432;
	b.x = myx2 + 786432;
	b.y = myy2 + 786432;

	setGlideConstantColorState(global_palette[col]);

	grDrawLine(&a, &b);

	numOfTris += 2;
}

int setGlideWindow(long xdim, long ydim)
{
	char dirname[15];
	int error = 0, i;
	FxU32 tmp;
	GrScreenResolution_t res;

	if (glideWinOpened)
	{
		grSstWinClose();
		glideWinOpened = 0;
	}

	switch (xdim)
	{
		case 512:
			if (ydim == 384)
				res = GR_RESOLUTION_512x384;
			else
				error = 1;
			break;
		case 640:
			if (ydim == 480)
				res = GR_RESOLUTION_640x480;
			else
				error = 1;
			break;
		case 800:
			if (ydim == 600)
				res = GR_RESOLUTION_800x600;
			else
				error = 1;
			break;
		default:
			error = 1;
			break;
	}
	if (error)
	{
		setvmode(3);
		fprintf(stderr, "Resolution %dx%d is unsupported", xdim, ydim);
		return 0;
	}
	if (!grSstWinOpen(0, res, 0, 1, 0, 2, 0))
	{
		setvmode(3);
		fprintf(stderr, "Could not open 3Dfx context. Exiting...\n");
		return 0;
	}

	glideXRes = xdim;
	glideYRes = ydim;
	if (xdim == 512)
		use512x384 = 1;

	grGlideGetVersion(version);
	setGlideDefaultState();

	grRenderBuffer(1);

	glideNoPaletteLookups = atoi(getenv("BUILD_NOPAL"));
	showTriCount = atoi(getenv("BUILD_TRICOUNT"));
	showVersionNumber = atoi(getenv("BUILD_VERSION"));
	showFPS = atoi(getenv("BUILD_FPS"));
	useTexus = atoi(getenv("BUILD_RESAMPLE"));
	noWallFog = atoi(getenv("BUILD_NOFOG"));
	convertTextures = atoi(getenv("BUILD_CONVTEXTURES"));
	mirrorColor = 0xff000000;
	if (atoi(getenv("BUILD_MRED")))
		mirrorColor |= atoi(getenv("BUILD_MRED"));
	else
		mirrorColor |= 200;
	if (atoi(getenv("BUILD_MGREEN")))
		mirrorColor |= atoi(getenv("BUILD_MGREEN")) << 8;
	else
		mirrorColor |= 200 << 8;
	if (atoi(getenv("BUILD_MBLUE")))
		mirrorColor |= atoi(getenv("BUILD_MBLUE")) << 16;
	else
		mirrorColor |= 255 << 16;

	if (getenv("BUILD_GAMMA"))
		CORRECTION = strtod(getenv("BUILD_GAMMA"), 0);
	else
		CORRECTION = 1.3;

	if (getenv("BUILD_8X6GAMMASCALE"))
		CORRECTIONSCALARFOR8X6 = strtod(getenv("BUILD_8X6GAMMASCALE"), 0);
	else
		CORRECTIONSCALARFOR8X6 = 1.1;

	if (res == GR_RESOLUTION_800x600)
		CORRECTIONSCALAR = CORRECTIONSCALARFOR8X6;
	else
		CORRECTIONSCALAR = 1.0;

	for (i = 0; i < 256; i++)
	{
		tmp = pow(i, CORRECTION) * CORRECTIONSCALAR;
		if (tmp > 255)
			tmp = 255;
		gammaTable[i] = tmp;
	}

	glideWinOpened = 1;
	return 1;
}

int glideDumpScreen(char* filename)
{
	return tlScreenDump(filename, glideXRes, glideYRes);
}

FxBool tlScreenDump(const char* filename, FxU16 width, FxU16 height)
{
	FILE* fp;
	FxU16* pixel, * region;
	FxU32 count, signature;
	FxU8 type, depth;
	FxU8 table1[] = { 0, 0, 2, 0, 0, 0, 0, 0 };
	FxU16 table2[] = { 0, 0, 0, 0 };
	FxU8 table3[] = { 24, 32 };
	int rc = 0;
	GrLfbInfo_t info;
	ImgInfo imgInfo;
	static FxU16 buf[800 * 600];
	static FxU32 buf2[800 * 600];
	long i, scanline;
	FxU16 * readPtr, * writePtr;
	long readPtrInc;

	table2[2] = width;
	table2[3] = height;
	info.size = sizeof(GrLfbInfo_t);

	fp = fopen(filename, "w+b");

	if (!fp)
		return FXFALSE;

	if (!grLfbLock(0, 1, 255, 0, 0, &info))
		return FXFALSE;

	{
		FxU16* surf = (FxU16*)info.lfbPtr;
		int i, j;

		for (i = 0; i < height; i++)
		{
			FxU16* surf2 = surf;
			for (j = 0; j < width; j++)
			{
				buf[i * width + j] = *surf2;
				surf2++;
			}
			surf += info.strideInBytes >> 1;
		}
	}

	grLfbUnlock(0, 1);

	for (i = 0; i < width * height; i++)
	{
		*((FxU8*)buf2 + 4 * i + 2) = (buf[i] & 0xf800) >> 8;
		*((FxU8*)buf2 + 4 * i + 1) = (buf[i] & 0x7e0) >> 3;
		*((FxU8*)buf2 + 4 * i + 0) = (buf[i] & 0x1f) << 3;
	}

	imgInfo.width = width;
	imgInfo.height = height;
	imgInfo.data = buf2;

	_glide_s1(&imgInfo, 5, 0, 1);
	_glide_f1(&imgInfo, 5, 4, 1);
	_glide_e1(&imgInfo, 5);
	_glide_g1(&imgInfo, 1.7);

	fwrite(table1, 1, 8, fp);
	fwrite(table2, 2, 4, fp);
	fwrite(table3, 1, 2, fp);

	{
		FxU32* surf = (FxU32*)buf2;
		int i, j;

		for (i = 0; i < height; i++)
		{
			FxU32* surf2 = surf;
			for (j = 0; j < width; j++)
			{
				fwrite(surf2, 3, 1, fp);
				surf2++;
			}
			surf += width;
		}
	}

	fclose(fp);
	return 1;
}

void resample(char* src, char* dst, int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
	int dx, dy, error, i, x, y;

	error = 0;
	dy = dstHeight - 1;
	dx = srcHeight;
	x = 0;
	y = 0;
	for (i = 0; i < dx; i++)
	{
		Plot(x, y, src, dst, srcWidth, dstWidth);
		while (error >= 0)
		{
			y++;
			Plot(x, y, src, dst, srcWidth, dstWidth);
			error -= dx * 2;
		}
		x++;
		error += dy * 2;
	}
}













































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































void Plot(int srcRow, int dstRow, char* src, char* dst, int srcWidth, int dstWidth)
{
	int dx, dy, x, y, i, error;

	error = 0;
	dy = dstWidth - 1;
	dx = srcWidth;
	x = 0;
	y = 0;
	for (i = 0; i < dx; i++)
	{
		assert(x < srcWidth);
		assert(y < dstWidth);

		*(dst + dstRow * dstWidth + y) = *(src + srcRow * srcWidth + x);
		while (error >= 0)
		{
			y++;
			*(dst + dstRow * dstWidth + y) = *(src + srcRow * srcWidth + x);
			error -= dx * 2;
		}
		x++;
		error += dy * 2;
	}
}

void resample2(FxU16* src, FxU16* dst, int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
	int dx, dy, error, i, x, y;

	error = 0;
	dy = dstHeight - 1;
	dx = srcHeight;
	x = 0;
	y = 0;
	for (i = 0; i < dx; i++)
	{
		Plot2(x, y, src, dst, srcWidth, dstWidth);
		while (error >= 0)
		{
			y++;
			Plot2(x, y, src, dst, srcWidth, dstWidth);
			error -= dx * 2;
		}
		x++;
		error += dy * 2;
	}
}




























void Plot2(int srcRow, int dstRow, FxU16* src, FxU16* dst, int srcWidth, int dstWidth)
{
	int dx, dy, x, y, i, error;

	error = 0;
	dy = dstWidth - 1;
	dx = srcWidth;
	x = 0;
	y = 0;
	for (i = 0; i < dx; i++)
	{
		assert(x < srcWidth);
		assert(y < dstWidth);

		*(dst + dstRow * dstWidth + y) = *(src + srcRow * srcWidth + x);
		while (error >= 0)
		{
			y++;
			*(dst + dstRow * dstWidth + y) = *(src + srcRow * srcWidth + x);
			error -= dx * 2;
		}
		x++;
		error += dy * 2;
	}
}

int setglidepic(short picnum, int mask);

void preCacheBigTextures(void)
{
	char buff[82];
	char buff2[82];
	int picnum, i, row;
	struct stat sbuf;
	float f;

	row = 20;

	if (stat("tx3dfx", &sbuf) != -1)
		return;

	mkdir("tx3dfx");

	sprintf(buff, "Converting textures... This could take a while...");
	printext256(5, row, 0, 0, buff, 0);
	row += 8;
	row += 8;
	sprintf(buff, "This process is caching the 3dfx textures to the game directory");
	printext256(5, row, 0, 0, buff, 0);
	row += 8;
	sprintf(buff, "so it will not take so long in the future.");
	printext256(5, row, 0, 0, buff, 0);
	row += 8;
	row += 8;
	sprintf(buff, "If you want to recreate the textures, then delete the tx3dfx subdirectory");
	printext256(5, row, 0, 0, buff, 0);
	row += 8;
	sprintf(buff, "from the game subdirectory.");
	printext256(5, row, 0, 0, buff, 0);
	row += 8;
	row += 8;

	for (picnum = 0; picnum < MAXTILES; picnum++)
	{
		if (tilesizx[picnum] > 256 || tilesizy[picnum] > 256)
		{
			if (!waloff[picnum])
				loadtile(picnum);

			setglidepic(picnum, 1);

			f = (float)(picnum / MAXTILES * 48);
			i = f;
			memset(buff2 + 1, '.', i);
			memset(buff2 + 1 + i, ' ', 48 - i);
		}
	}
}
