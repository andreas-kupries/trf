#ifndef SHA_H
#define SHA_H

#include <tcl.h>

/* NIST Secure Hash Algorithm */
/* heavily modified from Peter C. Gutmann's implementation */

/* Useful defines & typedefs */

typedef unsigned char BYTE;
#ifndef __WIN32__
typedef unsigned long LONG;
#endif

#define SHA_BLOCKSIZE		64
#define SHA_DIGESTSIZE		20

typedef struct {
    LONG digest[5];		/* message digest */
    LONG count_lo, count_hi;	/* 64-bit bit count */
    LONG data[16];		/* SHA data buffer */
} SHA_INFO;

void sha_init   _ANSI_ARGS_ ((SHA_INFO *));
void sha_update _ANSI_ARGS_ ((SHA_INFO *, BYTE *, int));
void sha_final  _ANSI_ARGS_ ((SHA_INFO *));
void sha_stream _ANSI_ARGS_ ((SHA_INFO *, FILE *));
void sha_print  _ANSI_ARGS_ ((SHA_INFO *));

#endif /* SHA_H */
