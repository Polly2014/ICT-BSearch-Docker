/*
 * mmtregex.c
 *
 *  Created on: Jul 27, 2016
 *      Author: Abe Wu
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mmtregex.h"

typedef struct mmtregex_s {
	size_t length;		/*<< the length of data, in unit of 256-bytes (representing 256 bits) */
	size_t match_len;	/*<< the length of the matched data */
	uint8_t *map;		/*<< The map. Each consecutive 256-byte map indicates the accepted character set */
#define MAP_UNIT_SIZE 256
} mmtregex_t, *mmtregex;

void delete_mmtregex(mmtregex preg)
{
	if (preg) {
		if (preg->map) {
			free(preg->map);
		}
		free(preg);
	}
}

/**
 * regex can only be '0', '1' and '.', where '.' matches both '0' and '1'
 * regex_str length must be multiple of 8
 */
mmtregex mmtregex_from_str(const uint8_t *regex_str, size_t regex_len)
{
	mmtregex preg;
	size_t i, j;
	uint8_t mask, value;

	if (!regex_str || regex_len == 0 || (regex_len % 8)) {
		return NULL;
	}

//	printf("%s: input pattern", __FUNCTION__);
//	for (i = 0; i < regex_len; i++) {
//		if (i != regex_len - 1 && (i % 8 == 0)) printf(" ");
//		printf("%c", regex_str[i]);
//	}
//	printf("\n");

	/* Allocate resource for the preg */
	preg = malloc(sizeof(*preg));
	if (!preg) {
		return NULL;
	}
	preg->length = regex_len / 8;
	preg->match_len = 0;
	preg->map = malloc(preg->length * MAP_UNIT_SIZE);
	if (!preg->map) {
		free(preg);
		return NULL;
	}

	/* Compile 8 characters of '0', '1' and '.' into a 256-bit map */
	for (i = 0; i < preg->length; i++) {
		/* Generate the mask and the value for the 8-bit, where '.' is unmasked */
		mask = 0;
		value = 0;
		for (j = i * 8; j < i * 8 + 8; j++) {
			switch (regex_str[j]) {
			case '0':
			case '1':
				value = (value << 1) + (regex_str[j] - '0');
				mask = (mask << 1) + 1;
				break;

			case '.':
				value = (value << 1);
				mask = (mask << 1);
				break;

			default:
				free(preg->map);
				free(preg);
				return NULL;
			}
		}

		/* Generate the map according to mask and value */
//		printf("%s: char %d, mask = %X, value = %X, allow", __FUNCTION__, i, mask, value);
		for (j = 0; j < MAP_UNIT_SIZE; j++) {
			preg->map[i * MAP_UNIT_SIZE + j] = ((j & mask) == value) ? 1 : 0;
//			if (preg->map[i * MAP_UNIT_SIZE + j])
//				printf(" %X", j);
		}
//		printf("\n");
	}

	return preg;
}

/**
 * @brief Try to match a string using compiled preg
 *
 * Unlike other regex, mmtregex only matches from the beginning of a string.
 * After match, mmtregex_get_matched_pos will return the number of characters matched.
 *
 * @param preg the compiled regular expression using mmtregex_comp
 * @param str the string to be matched with
 *
 * @retval MMTREGEX_MATCH_SUCCESS match found
 * @retval MMTREGEX_MATCH_FAIL not match
 */
int mmtregex_match(mmtregex preg, const uint8_t *str, size_t len)
{
	if (!preg || !preg->map || !str || len <= 0) {
		return MMTREGEX_MATCH_FAIL;
	}

	size_t i;

	for (i = 0; i < len && i < preg->length; i++) {
		if (!preg->map[i * MAP_UNIT_SIZE + str[i]]) {
//			printf("\t%s: match char index %d value %X [FAIL]\n", __FUNCTION__, i, str[i]);
			preg->match_len = i;
			return MMTREGEX_MATCH_FAIL;
		}
//		printf("\t%s: match char index %d value %X [MATCH]\n", __FUNCTION__, i, str[i]);
	}

	preg->match_len = i;
	if (preg->match_len == preg->length) {
		return MMTREGEX_MATCH_SUCCESS;
	} else {
		/* Input is too short to match */
		return MMTREGEX_MATCH_FAIL;
	}
}

int mmtregex_get_next_match_char(mmtregex preg, uint32_t offset, uint8_t charbeg)
{
	const int maxchar = 0x100;
	int i;
	if (!preg || offset >= preg->length) {
		return maxchar;
	}

	for (i = charbeg; i < maxchar; i++) {
		if (preg->map[offset * MAP_UNIT_SIZE + i]) {
			return i;
		}
	}

	return maxchar;
}

int mmtregex_get_matched_length(mmtregex preg)
{
	return preg ? preg->match_len : 0;
}

int mmtregex_get_maximum_length(mmtregex preg)
{
	return preg ? preg->length : 0;
}

//struct mmtregex_s {
//	size_t count;		/*<< Number of regex */
//	mmtregex regexs;	/*<< The regex array */
//};
//
//mmtregex new_mmtregex()
//{
//	mmtregex *preg = malloc(sizeof(*preg));
//	if (!preg) return NULL;
//
//	memset(preg, sizeof(*preg));
//
//	return preg;
//}
//
//void delete_mmtregex(mmtregex *preg)
//{
//	size_t i;
//	if (!preg) {
//		return;
//	}
//	if (preg->regexs) {
//		for (i = 0; i < preg->count; i++) {
//			delete_mmtregex_internal(preg->regexs[i]);
//		}
//		free(preg->regexs);
//	}
//	free(preg);
//}
//
//int mmtregex_comp_bit(mmtregex *preg, const uint8_t *regex_str, size_t regex_len)
//{
//	size_t i, padding;
//	uint8_t regex_buf;
//
//	if (!regex_str || !preg || regex_len <= 0) {
//		return MMTREGEX_EPARAM;
//	}
//	for (i = 0; i < regex_len; i++) {
//		if (regex_str[i] != '0' && regex_str[i] != '1' && regex_str[i] != '.') {
//			return MMTREGEX_EPARAM;
//		}
//	}
//
//	/* Allocate resource for regex_buf and fill before and after with '.' */
//	regex_buf = malloc(regex_len + 16);
//	memcpy(regex_buf + 8, regex_str, regex_len);
//	for (i = 0; i < 8; i++) {
//		regex_buf[i] = '.';
//		regex_buf[i + regex_len] = '.';
//	}
//
//	preg->count = 8;
//	preg->regexs = malloc(sizeof(*preg->regexs) * preg->count);
//	memset(preg->regexs, 0, sizeof(*preg->regexs) * preg->count);
//
//	for (i = 0; i < preg->count; i++) {
//		padding = (8 - ((regex_len + 8 - i) % 8)) % 8;
//		preg->regexs[i] = mmtregex_internal_from_str(regex_buf + 8 - i, regex_len + 8 - i + padding);
//		if (!preg->regexs[i]) {
//			delete_mmtregex(preg);
//			return NULL;
//		}
//	}
//
//	return preg;
//}

/**
 * @brief compile the regex
 *
 * REGEX format:
 *
 * REGEX ::= (REGEX_CHARSET)+
 * REGEX_CHARSET ::= REGEX_CHAR | '[' (REGEX_CHAR)+ ']'
 * REGEX_CHAR ::= \HH | CHAR | \. | \[ | \]
 * H ::= [0..9A..Ba..b]
 *
 * Encoding by '\'
 * 	\c means ASCII c, where c
 * 	\HH means ASCII with value 0xHH
 * 	\
 */
//int mmtregex_comp(mmtregex *preg, const char *regex, int cflags)
//{
//	if (!regex || !preg || strlen(regex)) {
//		return MMTREGEX_EPARAM;
//	}
//
//	size_t i, len;
//	size_t len = strlen(regex);
//
//	/* Allocate resource for the preg */
//	preg->length = len;
//	preg->match_len = 0;
//	preg->map = malloc(preg->length);
//	if (preg->map) {
//		return MMTREGEX_ENOMEM;
//	}
//
//	/* Compile the regex */
//	for (i = 0; i < len; i++) {
//		switch (regex[i]) {
//		case '0':
//			preg->map[i] = 1;
//			break;
//
//		case '1':
//			preg->map[i] = 2;
//			break;
//
//		case '.':
//			preg->map[i] = 3;
//			break;
//
//		default:
//			free(preg->map);
//			preg->length = 0;
//			return MMTREGEX_EREGEX;
//		}
//	}
//
//	return MMTREGEX_OK;
//}
