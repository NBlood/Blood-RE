#ifndef _TEXCACHE_H_
#define _TEXCACHE_H_

typedef FxU32 TexCacheId_t;

typedef struct
{
	GrTexInfo* texinfo;
	FxU32 min_address;
	FxU32 mem_required;
	FxU32 next_id;
	FxU32 width;
	FxU32 height;
	GrAspectRatio_t aspect_ratio;
} TexCacheEntry;

typedef struct
{
	GrChipID_t chip;
	FxU32 min_address;
	FxU32 max_address;
#ifdef TWOMEGFIELD
	FxU32 two_meg_boundary;
#endif
	FxU32 max_entries;
	FxU32 num_entries;
	FxU32 prev_id;
	FxU32 next_address;
	TexCacheEntry* entries;
	char* name;
} TexCache;


void TexCachePrintState(TexCache* cache);
void TexCacheReset(TexCache* cache);
FxBool TexCacheInit(TexCache* cache, FxU32 min_address, FxU32 max_address, FxU32 max_entries, GrChipID_t chip, char* name);
FxU32 TexCacheInsertTexture(TexCache* cache, GrTexInfo* texinfo, FxU32 width, FxU32 height);
void TexCacheClear(TexCache* cache);
FxBool TexCacheForceTexture(TexCache* cache, TexCacheId_t entry_id);
FxBool TexCacheSetCurrent(TexCache* cache, TexCacheId_t entry_id);

#endif
