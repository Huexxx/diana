/*
 * aes.h
 * AES encrypt/decrypt wrapper functions used around Rijndael reference
 * implementation
 *
 * Copyright (C) 1999-2010, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: aes.h,v 1.17.68.2.2.2.2.1.170.1 2010/09/03 21:28:04 Exp $
 */

#ifndef _AES_H_
#define _AES_H_

#include <typedefs.h>
#ifdef BCMDRIVER
#include <osl.h>
#else
#include <stddef.h>  /* For size_t */
#endif
#include <bcmcrypto/rijndael-alg-fst.h>

/* forward declaration */
typedef struct dot11_header dot11_header_t;

#define AES_BLOCK_SZ    	16
#define AES_BLOCK_BITLEN	(AES_BLOCK_SZ * 8)
#define AES_KEY_BITLEN(kl)  	(kl * 8)
#define AES_ROUNDS(kl)		((AES_KEY_BITLEN(kl) / 32) + 6)
#define AES_MAXROUNDS		14

enum {
	NO_PADDING,
	PAD_LEN_PADDING /* padding with padding length  */
};


void aes_encrypt(const size_t KL, const uint8 *K, const uint8 *ptxt, uint8 *ctxt);
void aes_decrypt(const size_t KL, const uint8 *K, const uint8 *ctxt, uint8 *ptxt);

#define aes_block_encrypt(nr, rk, ptxt, ctxt) rijndaelEncrypt(rk, nr, ptxt, ctxt)
#define aes_block_decrypt(nr, rk, ctxt, ptxt) rijndaelDecrypt(rk, nr, ctxt, ptxt)

int aes_cbc_encrypt_pad(uint32 *rk, const size_t key_len, const uint8 *nonce,
                              const size_t data_len, const uint8 *ptxt, uint8 *ctxt,
                              uint8 pad_type);
int aes_cbc_encrypt(uint32 *rk, const size_t key_len, const uint8 *nonce,
                              const size_t data_len, const uint8 *ptxt, uint8 *ctxt);
int aes_cbc_decrypt(uint32 *rk, const size_t key_len, const uint8 *nonce,
                              const size_t data_len, const uint8 *ctxt, uint8 *ptxt);
int aes_cbc_decrypt_pad(uint32 *rk, const size_t key_len, const uint8 *nonce,
                                  const size_t data_len, const uint8 *ctxt, uint8 *ptxt,
                                  uint8 pad_type);

#define AES_CTR_MAXBLOCKS	(1<<16)

int aes_ctr_crypt(unsigned int *rk, const size_t key_len, const uint8 *nonce,
                            const size_t data_len, const uint8 *ptxt, uint8 *ctxt);

/* only support the 2 octet AAD length encoding */
#define AES_CCM_LEN_FIELD_LEN	2
#define AES_CCM_AAD_MAX_LEN	0xFEFF
#define AES_CCM_NONCE_LEN	13

#define AES_CCM_CRYPT_FLAGS	(AES_CCM_LEN_FIELD_LEN-1)

/* only support the 8 octet auth field */
#define AES_CCM_AUTH_LEN	8

#define AES_CCM_AUTH_FLAGS	(4*(AES_CCM_AUTH_LEN-2) + (AES_CCM_LEN_FIELD_LEN-1))
#define AES_CCM_AUTH_AAD_FLAG	0x40

#define AES_CCMP_AAD_MIN_LEN	22
#define AES_CCMP_AAD_MAX_LEN	30
#define AES_CCMP_NONCE_LEN	AES_CCM_NONCE_LEN
#define AES_CCMP_AUTH_LEN	AES_CCM_AUTH_LEN

#define AES_CCMP_FC_RETRY		0x800
#define AES_CCMP_FC_PM			0x1000
#define AES_CCMP_FC_MOREDATA		0x2000
#define AES_CCMP_FRAGNUM_MASK		0x000f
#define AES_CCMP_QOS_TID_MASK		0x000f

/* mute PM */
#define	AES_CCMP_LEGACY_FC_MASK		~(AES_CCMP_FC_RETRY)
#define	AES_CCMP_LEGACY_SEQ_MASK	0x0000
#define	AES_CCMP_LEGACY_QOS_MASK	0xffff

/* mute MoreData, PM, Retry, Low 3 bits of subtype */
#define AES_CCMP_SUBTYPE_LOW_MASK	0x0070
#define	AES_CCMP_FC_MASK 		~(AES_CCMP_SUBTYPE_LOW_MASK | AES_CCMP_FC_RETRY | \
					  AES_CCMP_FC_PM | AES_CCMP_FC_MOREDATA)
#define	AES_CCMP_SEQ_MASK		AES_CCMP_FRAGNUM_MASK
#define	AES_CCMP_QOS_MASK		AES_CCMP_QOS_TID_MASK

#define AES_CCMP_ENCRYPT_SUCCESS	0
#define AES_CCMP_ENCRYPT_ERROR		-1

/* AES-CCMP mode encryption algorithm
	- encrypts in-place
	- packet buffer must be contiguous
	- packet buffer must have sufficient tailroom for CCMP MIC
	- returns AES_CCMP_ENCRYPT_SUCCESS on success
	- returns AES_CCMP_ENCRYPT_ERROR on error
*/
int aes_ccmp_encrypt(unsigned int *rk, const size_t key_len,
	const size_t data_len, uint8 *p, bool legacy, uint8 nonce_1st_byte);

#define AES_CCMP_DECRYPT_SUCCESS	0
#define AES_CCMP_DECRYPT_ERROR		-1
#define AES_CCMP_DECRYPT_MIC_FAIL	-2

/* AES-CCMP mode decryption algorithm
	- decrypts in-place
	- packet buffer must be contiguous
	- packet buffer must have sufficient tailroom for CCMP MIC
	- returns AES_CCMP_DECRYPT_SUCCESS on success
	- returns AES_CCMP_DECRYPT_ERROR on decrypt protocol/format error
	- returns AES_CCMP_DECRYPT_MIC_FAIL on message integrity check failure
*/
int aes_ccmp_decrypt(unsigned int *rk, const size_t key_len,
	const size_t data_len, uint8 *p, bool legacy, uint8 nonce_1st_byte);

void aes_ccmp_cal_params(dot11_header_t *h, bool legacy, uint8 nonce_1st_byte,
                               uint8 *nonce, uint8 *aad, uint *la, uint *lh);

int aes_ccm_mac(unsigned int *rk, const size_t key_len, const uint8 *nonce,
                          const size_t aad_len, const uint8 *aad, const size_t data_len,
                          const uint8 *ptxt, uint8 *mac);
int aes_ccm_encrypt(unsigned int *rk, const size_t key_len, const uint8 *nonce,
                              const size_t aad_len, const uint8 *aad, const size_t data_len,
                              const uint8 *ptxt, uint8 *ctxt, uint8 *mac);
int aes_ccm_decrypt(unsigned int *rk, const size_t key_len, const uint8 *nonce,
                              const size_t aad_len, const uint8 *aad, const size_t data_len,
                              const uint8 *ctxt, uint8 *ptxt);

#if defined(WLFBT)
#define AES_CMAC_AUTH_LEN	AES_BLOCK_SZ

void aes_cmac_gen_subkeys(const size_t kl, const uint8 *K, uint8 *K1, uint8 *K2);

void aes_cmac(const size_t key_len, const uint8* K, const uint8 *K1,
              const uint8 *K2, const size_t data_len, const uint8 *ptxt, uint8 *mac);
void aes_cmac_calc(const uint8 *data, const size_t data_length,
                   const uint8 *mic_key, const size_t key_len, uint8 *mic);
#endif /* defined(WLFBT) */

#ifdef BCMAES_TEST
int aes_test_cbc(void);
int aes_test_ctr(void);
int aes_test_ccm(void);
int aes_test_ccmp(void);
int aes_test_ccmp_timing(int *t);
int aes_test_cmac(void);
int aes_test(int *t);
#endif /* BCMAES_TEST */

#endif /* _AES_H_ */
