#ifndef SOE_KVS_SRC_C_API_H_
#define SOE_KVS_SRC_C_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types */

typedef struct LzKVStore_t LzKVStore_t;
typedef struct LzKVStore_cache_t LzKVStore_cache_t;
typedef struct LzKVStore_comparator_t LzKVStore_comparator_t;
typedef struct LzKVStore_env_t LzKVStore_env_t;
typedef struct LzKVStore_filelock_t LzKVStore_filelock_t;
typedef struct LzKVStore_filterpolicy_t LzKVStore_filterpolicy_t;
typedef struct LzKVStore_iterator_t LzKVStore_iterator_t;
typedef struct LzKVStore_logger_t LzKVStore_logger_t;
typedef struct LzKVStore_options_t LzKVStore_options_t;
typedef struct LzKVStore_randomfile_t LzKVStore_randomfile_t;
typedef struct LzKVStore_readoptions_t LzKVStore_readoptions_t;
typedef struct LzKVStore_seqfile_t LzKVStore_seqfile_t;
typedef struct LzKVStore_snapshot_t LzKVStore_snapshot_t;
typedef struct LzKVStore_writablefile_t LzKVStore_writablefile_t;
typedef struct LzKVStore_writebatch_t LzKVStore_writebatch_t;
typedef struct LzKVStore_writeoptions_t LzKVStore_writeoptions_t;

/* KVDatabase operations */

extern LzKVStore_t* LzKVStore_open(const LzKVStore_options_t* options,
        const char* name, char** errptr);

extern void LzKVStore_close(LzKVStore_t* db);

extern void LzKVStore_put(LzKVStore_t* db,
        const LzKVStore_writeoptions_t* options, const char* key, size_t keylen,
        const char* val, size_t vallen, char** errptr);

extern void LzKVStore_delete(LzKVStore_t* db,
        const LzKVStore_writeoptions_t* options, const char* key, size_t keylen,
        char** errptr);

extern void LzKVStore_write(LzKVStore_t* db,
        const LzKVStore_writeoptions_t* options, LzKVStore_writebatch_t* batch,
        char** errptr);

/* Returns NULL if not found.  A malloc()ed array otherwise.
 Stores the length of the array in *vallen. */
extern char* LzKVStore_get(LzKVStore_t* db,
        const LzKVStore_readoptions_t* options, const char* key, size_t keylen,
        size_t* vallen, char** errptr);

extern LzKVStore_iterator_t* LzKVStore_create_iterator(LzKVStore_t* db,
        const LzKVStore_readoptions_t* options);

extern const LzKVStore_snapshot_t* LzKVStore_create_snapshot(LzKVStore_t* db);

extern void LzKVStore_release_snapshot(LzKVStore_t* db,
        const LzKVStore_snapshot_t* snapshot);

/* Returns NULL if property name is unknown.
 Else returns a pointer to a malloc()-ed null-terminated value. */
extern char* LzKVStore_property_value(LzKVStore_t* db, const char* propname);

extern void LzKVStore_approximate_sizes(LzKVStore_t* db, int num_ranges,
        const char* const * range_start_key, const size_t* range_start_key_len,
        const char* const * range_limit_key, const size_t* range_limit_key_len,
        uint64_t* sizes);

extern void LzKVStore_compact_range(LzKVStore_t* db, const char* start_key,
        size_t start_key_len, const char* limit_key, size_t limit_key_len);

/* Management operations */

extern void LzKVStore_destroy_db(const LzKVStore_options_t* options,
        const char* name, char** errptr);

extern void LzKVStore_repair_db(const LzKVStore_options_t* options,
        const char* name, char** errptr);

/* Iterator */

extern void LzKVStore_iter_destroy(LzKVStore_iterator_t*);
extern unsigned char LzKVStore_iter_valid(const LzKVStore_iterator_t*);
extern void LzKVStore_iter_seek_to_first(LzKVStore_iterator_t*);
extern void LzKVStore_iter_seek_to_last(LzKVStore_iterator_t*);
extern void LzKVStore_iter_seek(LzKVStore_iterator_t*, const char* k,
        size_t klen);
extern void LzKVStore_iter_next(LzKVStore_iterator_t*);
extern void LzKVStore_iter_prev(LzKVStore_iterator_t*);
extern const char* LzKVStore_iter_key(const LzKVStore_iterator_t*,
        size_t* klen);
extern const char* LzKVStore_iter_value(const LzKVStore_iterator_t*,
        size_t* vlen);
extern void LzKVStore_iter_get_error(const LzKVStore_iterator_t*,
        char** errptr);

/* Write batch */

extern LzKVStore_writebatch_t* LzKVStore_writebatch_create();
extern void LzKVStore_writebatch_destroy(LzKVStore_writebatch_t*);
extern void LzKVStore_writebatch_clear(LzKVStore_writebatch_t*);
extern void LzKVStore_writebatch_put(LzKVStore_writebatch_t*, const char* key,
        size_t klen, const char* val, size_t vlen);
extern void LzKVStore_writebatch_delete(LzKVStore_writebatch_t*,
        const char* key, size_t klen);
extern void LzKVStore_writebatch_iterate(LzKVStore_writebatch_t*, void* state,
        void (*put)(void*, const char* k, size_t klen, const char* v,
                size_t vlen),
        void (*deleted)(void*, const char* k, size_t klen));

/* Options */

extern LzKVStore_options_t* LzKVStore_options_create();
extern void LzKVStore_options_destroy(LzKVStore_options_t*);
extern void LzKVStore_options_set_comparator(LzKVStore_options_t*,
        LzKVStore_comparator_t*);
extern void LzKVStore_options_set_filter_policy(LzKVStore_options_t*,
        LzKVStore_filterpolicy_t*);
extern void LzKVStore_options_set_create_if_missing(LzKVStore_options_t*,
        unsigned char);
extern void LzKVStore_options_set_error_if_exists(LzKVStore_options_t*,
        unsigned char);
extern void LzKVStore_options_set_full_checks(LzKVStore_options_t*,
        unsigned char);
extern void LzKVStore_options_set_env(LzKVStore_options_t*, LzKVStore_env_t*);
extern void LzKVStore_options_set_info_log(LzKVStore_options_t*,
        LzKVStore_logger_t*);
extern void LzKVStore_options_set_write_buffer_size(LzKVStore_options_t*,
        size_t);
extern void LzKVStore_options_set_max_open_files(LzKVStore_options_t*, int);
extern void LzKVStore_options_set_cache(LzKVStore_options_t*,
        LzKVStore_cache_t*);
extern void LzKVStore_options_set_block_size(LzKVStore_options_t*, size_t);
extern void LzKVStore_options_set_block_restart_interval(LzKVStore_options_t*,
        int);

enum {
    LzKVStore_no_compression = 0, LzKVStore_snappy_compression = 1
};
extern void LzKVStore_options_set_compression(LzKVStore_options_t*, int);

/* Equalcomp */

extern LzKVStore_comparator_t* LzKVStore_comparator_create(void* state,
        void (*destructor)(void*),
        int (*compare)(void*, const char* a, size_t alen, const char* b,
                size_t blen), const char* (*name)(void*));
extern void LzKVStore_comparator_destroy(LzKVStore_comparator_t*);

/* Filter policy */

extern LzKVStore_filterpolicy_t* LzKVStore_filterpolicy_create(void* state,
        void (*destructor)(void*),
        char* (*create_filter)(void*, const char* const * key_array,
                const size_t* key_length_array, int num_keys,
                size_t* filter_length),
        unsigned char (*key_may_match)(void*, const char* key, size_t length,
                const char* filter, size_t filter_length),
        const char* (*name)(void*));
extern void LzKVStore_filterpolicy_destroy(LzKVStore_filterpolicy_t*);

extern LzKVStore_filterpolicy_t* LzKVStore_filterpolicy_create_bloom(
        int bits_per_key);

/* Read options */

extern LzKVStore_readoptions_t* LzKVStore_readoptions_create();
extern void LzKVStore_readoptions_destroy(LzKVStore_readoptions_t*);
extern void LzKVStore_readoptions_set_verify_checksums(LzKVStore_readoptions_t*,
        unsigned char);
extern void LzKVStore_readoptions_set_fill_cache(LzKVStore_readoptions_t*,
        unsigned char);
extern void LzKVStore_readoptions_set_snapshot(LzKVStore_readoptions_t*,
        const LzKVStore_snapshot_t*);

/* Write options */

extern LzKVStore_writeoptions_t* LzKVStore_writeoptions_create();
extern void LzKVStore_writeoptions_destroy(LzKVStore_writeoptions_t*);
extern void LzKVStore_writeoptions_set_sync(LzKVStore_writeoptions_t*,
        unsigned char);

/* Accell */

extern LzKVStore_cache_t* LzKVStore_cache_create_lru(size_t capacity);
extern void LzKVStore_cache_destroy(LzKVStore_cache_t* cache);

/* Configuration */

extern LzKVStore_env_t* LzKVStore_create_default_env();
extern void LzKVStore_env_destroy(LzKVStore_env_t*);

/* Utility */

/* Calls free(ptr).
 REQUIRES: ptr was malloc()-ed and returned by one of the routines
 in this file.  Note that in certain cases (typically on Windows), you
 may need to call this routine instead of free(ptr) to dispose of
 malloc()-ed memory returned by this library. */
extern void LzKVStore_free(void* ptr);

/* Return the major version number for this release. */
extern int LzKVStore_major_version();

/* Return the minor version number for this release. */
extern int LzKVStore_minor_version();

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  /* SOE_KVS_SRC_C_API_H_ */
