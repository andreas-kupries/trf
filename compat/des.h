/* lib/des/des.h */
/* Copyright (C) 1995 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 * 
 * This file is part of an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL
 * specification.  This library and applications are
 * FREE FOR COMMERCIAL AND NON-COMMERCIAL USE
 * as long as the following conditions are aheared to.
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.  If this code is used in a product,
 * Eric Young should be given attribution as the author of the parts used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Eric Young (eay@mincom.oz.au)
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#ifndef HEADER_DES_H
#define HEADER_DES_H

#include <tcl.h>	/* _ANSI_ARGS_ ... */
#include <stdio.h>

/* If this is set to 'unsigned int' on a DEC Alpha, this gives about a
 * %20 speed up (longs are 8 bytes, int's are 4). */
#ifndef DES_LONG
#define DES_LONG unsigned long
#endif

typedef unsigned char des_cblock[8];
typedef struct des_ks_struct
	{
	union	{
		des_cblock _;
		/* make sure things are correct size on machines with
		 * 8 byte longs */
		DES_LONG pad[2];
		} ks;
#undef _
#define _	ks._
	} des_key_schedule[16];

#define DES_KEY_SZ 	(sizeof(des_cblock))
#define DES_SCHEDULE_SZ (sizeof(des_key_schedule))

#define DES_ENCRYPT	1
#define DES_DECRYPT	0

#define DES_CBC_MODE	0
#define DES_PCBC_MODE	1

#define des_ecb2_encrypt(i,o,k1,k2,e) \
	des_ecb3_encrypt((i),(o),(k1),(k2),(k1),(e))

#define des_ede2_cbc_encrypt(i,o,l,k1,k2,iv,e) \
	des_ede3_cbc_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(e))

#define des_ede2_cfb64_encrypt(i,o,l,k1,k2,iv,n,e) \
	des_ede3_cfb64_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(n),(e))

#define des_ede2_ofb64_encrypt(i,o,l,k1,k2,iv,n) \
	des_ede3_ofb64_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(n))

#define C_Block des_cblock
#define Key_schedule des_key_schedule
#define ENCRYPT DES_ENCRYPT
#define DECRYPT DES_DECRYPT
#define KEY_SZ DES_KEY_SZ
#define string_to_key des_string_to_key
#define read_pw_string des_read_pw_string
#define random_key des_random_key
#define pcbc_encrypt des_pcbc_encrypt
#define set_key des_set_key
#define key_sched des_key_sched
#define ecb_encrypt des_ecb_encrypt
#define cbc_encrypt des_cbc_encrypt
#define ncbc_encrypt des_ncbc_encrypt
#define cbc_cksum des_cbc_cksum
#define quad_cksum des_quad_cksum

/* For compatibility with the MIT lib - eay 20/05/92 */
typedef des_key_schedule bit_64;
#define des_fixup_key_parity des_set_odd_parity
#define des_check_key_parity check_parity

extern int des_check_key;	/* defaults to false */
extern int des_rw_mode;		/* defaults to DES_PCBC_MODE */


void des_ecb3_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
				    des_key_schedule ks1,des_key_schedule ks2,
				    des_key_schedule ks3, int enc));

DES_LONG des_cbc_cksum _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
				     long length,des_key_schedule schedule,des_cblock *ivec));

void des_cbc_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,long length,
				   des_key_schedule schedule,des_cblock *ivec,int enc));

void des_ncbc_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,long length,
				    des_key_schedule schedule,des_cblock *ivec,int enc));

void des_3cbc_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,long length,
				    des_key_schedule sk1,des_key_schedule sk2,
				    des_cblock *ivec1,des_cblock *ivec2,int enc));

void des_cfb_encrypt _ANSI_ARGS_ ((unsigned char *in,unsigned char *out,int numbits,
				   long length,des_key_schedule schedule,des_cblock *ivec,int enc));

void des_ecb_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
				   des_key_schedule ks,int enc));

void des_encrypt _ANSI_ARGS_ ((DES_LONG *data,des_key_schedule ks, int enc));

void des_encrypt2 _ANSI_ARGS_ ((DES_LONG *data,des_key_schedule ks, int enc));

void des_ede3_cbc_encrypt _ANSI_ARGS_ ((des_cblock *input, des_cblock *output, 
					long length, des_key_schedule ks1, des_key_schedule ks2, 
					des_key_schedule ks3, des_cblock *ivec, int enc));

void des_ede3_cfb64_encrypt _ANSI_ARGS_ ((unsigned char *in, unsigned char *out,
					  long length, des_key_schedule ks1, des_key_schedule ks2,
					  des_key_schedule ks3, des_cblock *ivec, int *num,
					  int encrypt));

void des_ede3_ofb64_encrypt _ANSI_ARGS_ ((unsigned char *in, unsigned char *out,
					  long length, des_key_schedule ks1, des_key_schedule ks2,
					  des_key_schedule ks3, des_cblock *ivec, int *num));

int des_enc_read _ANSI_ARGS_ ((int fd,char *buf,int len,des_key_schedule sched,
			       des_cblock *iv));
int des_enc_write _ANSI_ARGS_ ((int fd,char *buf,int len,des_key_schedule sched,
				des_cblock *iv));
#ifdef PERL5
char *des_crypt _ANSI_ARGS_ ((const char *buf,const char *salt));
#else
/* some stupid compilers complain because I have declared char instead
 * of const char */
#ifdef HEADER_DES_LOCL_H
char *crypt _ANSI_ARGS_ ((const char *buf,const char *salt));
#else
char *crypt _ANSI_ARGS_ (());
#endif
#endif

void des_ofb_encrypt _ANSI_ARGS_ ((unsigned char *in,unsigned char *out,
				   int numbits,long length,des_key_schedule schedule,
				   des_cblock *ivec));

void des_pcbc_encrypt _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,long length,
				    des_key_schedule schedule,des_cblock *ivec,int enc));

DES_LONG des_quad_cksum _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
				      long length,int out_count,des_cblock *seed));

void des_random_seed _ANSI_ARGS_ ((des_cblock key));

void des_random_key _ANSI_ARGS_ ((des_cblock ret));

int des_read_password _ANSI_ARGS_ ((des_cblock *key,char *prompt,int verify));

int des_read_2passwords _ANSI_ARGS_ ((des_cblock *key1,des_cblock *key2,
				      char *prompt,int verify));

int des_read_pw_string _ANSI_ARGS_ ((char *buf,int length,char *prompt,int verify));

void des_set_odd_parity _ANSI_ARGS_ ((des_cblock *key));

int des_is_weak_key _ANSI_ARGS_ ((des_cblock *key));

int des_set_key _ANSI_ARGS_ ((des_cblock *key,des_key_schedule schedule));

int des_key_sched _ANSI_ARGS_ ((des_cblock *key,des_key_schedule schedule));

void des_string_to_key _ANSI_ARGS_ ((char *str,des_cblock *key));

void des_string_to_2keys _ANSI_ARGS_ ((char *str,des_cblock *key1,des_cblock *key2));

void des_cfb64_encrypt _ANSI_ARGS_ ((unsigned char *in, unsigned char *out, long length,
				     des_key_schedule schedule, des_cblock *ivec, int *num, int enc));

void des_ofb64_encrypt _ANSI_ARGS_ ((unsigned char *in, unsigned char *out, long length,
				     des_key_schedule schedule, des_cblock *ivec, int *num));

/* Extra functions from Mark Murray <mark@grondar.za> */

void des_cblock_print_file _ANSI_ARGS_ ((des_cblock *cb, FILE *fp));

/* The following functions are not in the normal unix build or the
 * SSLeay build.  When using the SSLeay build, use RAND_seed()
 * and RAND_bytes() instead. */

int des_new_random_key _ANSI_ARGS_ ((des_cblock *key));

void des_init_random_number_generator _ANSI_ARGS_ ((des_cblock *key));

void des_set_random_generator_seed _ANSI_ARGS_ ((des_cblock *key));

void des_set_sequence_number _ANSI_ARGS_ ((des_cblock new_sequence_number));

void des_generate_random_block _ANSI_ARGS_ ((des_cblock *block));

#endif
