#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glide.h>
#define TWOMEGFIELD
#include "texcache.h"
#include "texus.h"

char sbuff[80];

extern long current_texture_width;
extern long current_texture_height;

void TexCachePrintState(TexCache* cache)
{
	int i;
	printf("TexCache State:\n-------\n");
	printf("min_address = %ld\n", cache->min_address);
	printf("max_address = %ld\n", cache->max_address);
	printf("max_entries = %ld\n", cache->max_entries);
	printf("num_entries = %ld\n", cache->num_entries);
	printf("prev_id = %ld\n", cache->prev_id);
	printf("next_address = %ld\n", cache->next_address);
}

void TexCacheReset(TexCache* cache)
{
	TexCacheEntry* dummy_entry;

	cache->num_entries = 1;
	cache->prev_id = 0;
	dummy_entry = cache->entries;
	dummy_entry->texinfo = NULL;
	dummy_entry->min_address = cache->min_address;
	dummy_entry->mem_required = 0;
	dummy_entry->next_id = 0;
}

FxBool TexCacheInit(TexCache* cache, FxU32 min_address, FxU32 max_address, FxU32 max_entries, GrChipID_t chip, char* name)
{
	cache->chip = chip;
	cache->name = strdup(name);
	cache->entries = (TexCacheEntry*)malloc((max_entries + 1) * sizeof(TexCacheEntry));
	if (!cache->entries)
	{
		return FXFALSE;
	}
	cache->min_address = ((min_address + 7) >> 3) << 3;
	cache->max_address = max_address;
	cache->two_meg_boundary = cache->min_address + 0x200000;
	sprintf(sbuff, "MIN_ADDR = %x", cache->min_address);
	sprintf(sbuff, "MAX_ADDR = %x", cache->max_address);
	cache->max_entries = max_entries + 1;
	TexCacheReset(cache);

	return FXTRUE;
}

FxU32 TexCacheInsertTexture(TexCache* cache, GrTexInfo* texinfo, FxU32 width, FxU32 height)
{
	TexCacheEntry* new_entry;

	if (cache->num_entries == cache->max_entries)
		return -1;

	new_entry = cache->entries + cache->num_entries;
	new_entry->texinfo = texinfo;
	new_entry->min_address = -1;
	new_entry->next_id = -1;
	new_entry->mem_required = grTexCalcMemRequired(texinfo->smallLod, texinfo->largeLod, texinfo->aspectRatio, texinfo->format);
	new_entry->width = width;
	new_entry->height = height;

	cache->num_entries++;

	return cache->num_entries - 1;
}

void TexCacheClear(TexCache* cache)
{
	TexCacheEntry* entry;
	int i;
	for (i = 1; i < cache->num_entries; i++)
	{
		entry = cache->entries + i;
		free(entry->texinfo->data);
		free(entry->texinfo);
	}
	TexCacheReset(cache);
}

FxBool TexCacheForceTexture(TexCache* cache, TexCacheId_t entry_id)
{
	TexCacheEntry* entry, * prev_entry, * next_entry;
	FxU32 download_address;
	FxBool wrapped = FXFALSE;
	TexCacheId_t next_id;

	entry = cache->entries + entry_id;
	if (entry->min_address != -1)
		return FXTRUE;

	prev_entry = cache->entries + cache->prev_id;
	if (cache->max_address - cache->min_address < entry->mem_required)
		return FXFALSE;

	download_address = prev_entry->min_address + prev_entry->mem_required;
	download_address = ((download_address + 7) >> 3) << 3;
	if (download_address < cache->two_meg_boundary && download_address + entry->mem_required > cache->two_meg_boundary)
	{
		download_address = cache->two_meg_boundary;
	}

	if (download_address + entry->mem_required > cache->max_address)
	{
		download_address = cache->min_address;
		wrapped = FXTRUE;
	}

	next_id = prev_entry->next_id;
	next_entry = cache->entries + next_id;

	if (wrapped)
	{
		while (next_entry->min_address > download_address)
		{
			if (next_entry->min_address == -1)
			{
				next_id = next_entry->next_id;
				next_entry = cache->entries + next_id;
			}
			else
			{
				next_entry->min_address = -1;
				next_id = next_entry->next_id;
				next_entry = cache->entries + next_id;
			}
		}
	}

	while (download_address + entry->mem_required > next_entry->min_address)
	{
		if (next_entry->min_address + next_entry->mem_required < download_address)
			break;
		next_entry->min_address = -1;
		next_id = next_entry->next_id;
		next_entry = cache->entries + next_id;
	}

	prev_entry->next_id = entry_id;
	entry->min_address = download_address;
	entry->next_id = next_id;

	grTexDownloadMipMap(cache->chip, download_address, 3, entry->texinfo);

	cache->prev_id = entry_id;
	return FXTRUE;
}

FxBool TexCacheSetCurrent(TexCache* cache, TexCacheId_t entry_id)
{
	if (!TexCacheForceTexture(cache, entry_id))
		return FXFALSE;

	grTexSource(cache->chip, cache->entries[entry_id].min_address, 3, cache->entries[entry_id].texinfo);

	current_texture_width = cache->entries[entry_id].width;
	current_texture_height = cache->entries[entry_id].height;

	return FXTRUE;
}
