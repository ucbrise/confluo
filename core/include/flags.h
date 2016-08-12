#ifndef FLAGS_H_
#define FLAGS_H_

#define HASH3
#define BSR

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
