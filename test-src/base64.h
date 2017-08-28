/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SRC_B64_H
#define SRC_B64_H

#include <stdlib.h>

/**
 * base64_encode - Base64 encode
 * @param[in] src Data to be encoded
 * @param[in] len Length of the data to be encoded
 * @param[out] out_len Pointer to output length variable, or %NULL if not used
 * @return    Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char * base64_encode(const unsigned char *src, size_t len,
			      size_t *out_len);

/**
 * base64_decode - Base64 decode
 * @param[in] src Data to be decoded
 * @param[in] len Length of the data to be decoded
 * @param[out] out_len Pointer to output length variable
 * @return   Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char * base64_decode(const unsigned char *src, size_t len,
			      size_t *out_len);
#endif

