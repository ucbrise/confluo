#ifndef FLAGS_H_
#define FLAGS_H_

#define USE_INT_HASH
#define USE_STL_HASHMAP_NGRAM
#define HASH3
#define STL_LOCKS
#define BSR
#define LOCK_FREE_OFFSET_LIST
#define CUCKOO_HASHTABLE

#ifdef HASH4
#define NGRAM_N 4
#else
#ifdef HASH3
#define NGRAM_N 3
#else
#define NGRAM_N 2
#endif
#endif

#endif /* FLAGS_H_ */
