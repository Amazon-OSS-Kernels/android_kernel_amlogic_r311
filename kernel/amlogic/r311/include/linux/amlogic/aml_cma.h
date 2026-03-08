
#ifndef __AMLOGIC_CMA_H__
#define __AMLOGIC_CMA_H__

#include <linux/migrate_mode.h>
#include <linux/pagemap.h>

#define __GFP_NO_CMA	(__GFP_BDEV | __GFP_WRITE)
enum migrate_type {
	COMPACT_NORMAL,
	COMPACT_CMA,
};
/* copy from mm/internal.h, must keep same as it */
struct compact_control {
	struct list_head freepages;	/* List of free pages to migrate to */
	struct list_head migratepages;	/* List of pages being migrated */
	unsigned long nr_freepages;	/* Number of isolated free pages */
	unsigned long nr_migratepages;	/* Number of pages to migrate */
	unsigned long free_pfn;		/* isolate_freepages search base */
	unsigned long migrate_pfn;	/* isolate_migratepages search base */
	enum migrate_mode mode;		/* Async or sync migration mode */
	enum migrate_type page_type;
	bool ignore_skip_hint;		/* Scan blocks even if marked skip */
	bool finished_update_free;	/* True when the zone cached pfns are
					 * no longer being updated
					 */
	bool finished_update_migrate;
	bool forbid_to_cma;

	int order;			/* order a direct compactor needs */
	int migratetype;		/* MOVABLE, RECLAIMABLE etc */
	struct zone *zone;
	bool contended;			/* True if a lock was contended, or
					 * need_resched() true during async
					 * compaction
					 */
};

static inline bool cma_forbidden_mask(gfp_t gfp_flags)
{
	if ((gfp_flags & __GFP_NO_CMA) || !(gfp_flags & __GFP_MOVABLE))
		return true;
	return false;
}

extern void cma_page_count_update(long size);
extern void aml_cma_alloc_pre_hook(int *, int);
extern void aml_cma_alloc_post_hook(int *, int, struct page *);
extern void aml_cma_release_hook(int, struct page *);
extern unsigned long compact_to_free_cma(struct zone *zone);
extern int cma_alloc_ref(void);
extern bool can_use_cma(gfp_t gfp_flags);
extern void get_cma_alloc_ref(void);
extern void put_cma_alloc_ref(void);
extern bool cma_page(struct page *page);
extern unsigned long get_cma_allocated(void);
extern unsigned long get_total_cmapages(void);
extern spinlock_t cma_iso_lock;
extern int aml_cma_alloc_range(unsigned long start, unsigned long end);

extern void aml_cma_free(unsigned long pfn, unsigned nr_pages);
extern bool cma_first_wm_low;

extern unsigned long reclaim_clean_pages_from_list(struct zone *zone,
		struct list_head *page_list);

extern unsigned long isolate_freepages_range(struct compact_control *cc,
					     unsigned long start_pfn,
					     unsigned long end_pfn);
extern unsigned long isolate_migratepages_range(struct zone *zone,
						struct compact_control *cc,
						unsigned long low_pfn,
						unsigned long end_pfn,
						bool unevictable);

static inline struct page *page_cache_alloc_rwahead(struct address_space *x)
{
	return __page_cache_alloc(mapping_gfp_mask(x) | __GFP_WRITE |
				  __GFP_COLD | __GFP_NORETRY | __GFP_NOWARN);
}
#endif /* __AMLOGIC_CMA_H__ */
