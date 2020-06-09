/*
 * sabgrep.c for libdivsufsort
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <time.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MEMORY_H
# include <memory.h>
#endif
#if HAVE_STDDEF_H
# include <stddef.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_IO_H && HAVE_FCNTL_H
# include <io.h>
# include <fcntl.h>
#endif
#include <divsufsort.h>
#include "lfs.h"
#include "mmtregex.h"
#include "smartfile.h"

const char *program_name;
#define MAXPATTERNSIZE 4096

saidx_t _get_saidx(FILE *SAF, saidx_t idx)
{
	saidx_t value;
	fseek(SAF, idx * sizeof(idx), SEEK_SET);
	fread(&value, sizeof(saidx_t), 1, SAF);
	return value;
}

int compare_saidx(const void *p1, const void *p2)
{
	saidx_t s1, s2;
	s1 = *((const saidx_t*)p1);
	s2 = *((const saidx_t*)p2);
	return (s1<s2) ? -1 : ((s1==s2)?0:1);
}

/********************************************************************
 * REGEX bit level search
 ********************************************************************/
unsigned char bits_to_byte(const unsigned char *buf)
{
	return ((buf[0] - '0') << 7)
			+  ((buf[1] - '0') << 6)
			+  ((buf[2] - '0') << 5)
			+  ((buf[3] - '0') << 4)
			+  ((buf[4] - '0') << 3)
			+  ((buf[5] - '0') << 2)
			+  ((buf[6] - '0') << 1)
			+  ((buf[7] - '0'));
}
/**
 * @brief Converts consecutive 0,1 bits into bytes, stops at non-0,1 characters
 *
 * If the number of 0,1 bits is not the 8 time an integer, the last few(<8) bits
 * will be filled with 0 and a mask is provided to specify the number of bits
 * that are valid 0,1 bits
 */
saidx_t bits_to_bytes(const uint8_t *bits, saidx_t bits_len, uint8_t *bytes, uint8_t *mask)
{
	saidx_t K, L, m, bytes_len;
	uint8_t buf[8], mask_bits[8];
	/* Find the consecutive deterministic string */
	for (K = 0; K < bits_len; K++) {
		if (bits[K] != '0' && bits[K] != '1') {
			break;
		}
	}
	bytes_len = K / 8;
	/* Search for full determined characters */
	for (m = 0; m < bytes_len; m++) {
		bytes[m] = bits_to_byte(bits + m*8);
	}
	L = K % 8;
	if (L == 0) {
		*mask = 255;
	} else {
		for (m = 0; m < L; m++) {
			buf[m] = bits[m + bytes_len*8];
			mask_bits[m] = '1';
		}
		for ( ; m < 8; m++) {
			buf[m] = '0';
			mask_bits[m] = '0';
		}
		bytes[bytes_len] = bits_to_byte(buf);
		*mask = bits_to_byte(mask_bits);
		bytes_len++;
	}

//	/* For debug trace */
//	if (bytes_len > 0) {
//		printf("%s: (", __FUNCTION__);
//		for (m = 0; m < K; m++) {
//			printf("%c", bits[m]);
//		}
//		printf(")");
//		for ( ; m < bits_len; m++) {
//			printf("%c", bits[m]);
//		}
//		printf(" is converted to:");
//		for (m = 0; m < bytes_len; m++) {
//			printf(" %X", bytes[m]);
//		}
//		printf(", with mask %X\n", *mask);
//	}

	return bytes_len;
}

typedef struct {
	saidx_t index;
	saidx_t count;
	saidx_t offset;
} _potential_match_t, *_potential_match;

typedef struct {
	saidx_t *indices;
	saidx_t count;
} index_array_t, *index_array;

typedef struct {
	saidx_t off_byte;	/*<< the byte offset in the raw file */
	saidx_t off_bit;	/*<< the bit offset of first byte */
	saidx_t len_bit;	/*<< length of the match, in bits */
} *raw_match, raw_match_t;

index_array new_index_array(saidx_t count)
{
	index_array parray;

	parray = malloc(sizeof(*parray));
	if (!parray) {
		return NULL;
	}

	parray->count = count;
	parray->indices = malloc(parray->count * sizeof(*parray->indices));
	if (!parray->indices) {
		free(parray);
		return NULL;
	}

	return parray;
}

void delete_index_array(index_array parray)
{
	if (parray) {
		if (parray->indices) {
			free (parray->indices);
		}
		free (parray);
	}
}

index_array _get_saindices(FILE *SAF, _potential_match match)
{
	index_array parray;
	saidx_t i;

	parray = new_index_array(match->count);
	if (!parray) {
		return NULL;
	}

	fseek(SAF, match->index * sizeof(*parray->indices), SEEK_SET);
	fread(parray->indices, sizeof(*parray->indices), match->count, SAF);

	for (i = 0; i < match->count; i++) {
		parray->indices[i] -= match->offset;
	}

	return parray;
}

int compare_saidx_t(const void *p1, const void *p2)
{
	saidx_t v1, v2;
	v1 = *((const saidx_t*)p1);
	v2 = *((const saidx_t*)p2);
	return v1==v2 ? 0 : (v1<v2?-1:1);
}

static
int
_compare_file(file_list raw_files,
		const sauchar_t *P, saidx_t Psize,
		saidx_t suf, saidx_t *match)
{
	saidx_t i, j;
	saint_t r, read_count;
	static sauchar_t T[MAXPATTERNSIZE];

	/* read content from file */
	read_count = MIN(Psize - *match, raw_files->total_len - suf - *match);
	smfile_list_read(T, 1, read_count, raw_files, suf + *match);

	for(i = 0, j = *match, r = 0;
			(i < read_count) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j) {
		// Do nothing here
	}

	*match = j;
	return (r == 0) ? -(j != Psize) : r;
}


/**
 * @brief Joint arrays: The indices in array1 is put into the result if they are
 * within maximum distance of any indices in array2
 *
 * @warning This function sorts indices in both array1 and array2 for performance
 */
index_array _joint_array(index_array array1, index_array array2, saidx_t maximum_distance)
{
	saidx_t i1, i2;
	index_array parray;

	if (!array1 || !array2) {
		return NULL;
	}

	parray = new_index_array(array1->count);
	if (!parray) {
		return NULL;
	}

	/* TODO: Use hash table to accelerate the joint process when maximum distance == 0 */
	qsort(array1->indices, array1->count, sizeof(*array1->indices), compare_saidx_t);
	qsort(array2->indices, array2->count, sizeof(*array2->indices), compare_saidx_t);

	for (parray->count = 0, i1 = 0, i2 = 0; i1 < array1->count && i2 < array2->count; ) {
		if (array1->indices[i1] + maximum_distance < array2->indices[i2]) {
			i1++;
		} else if (array1->indices[i1] - maximum_distance > array2->indices[i2]) {
			i2++;
		} else {
			parray->indices[parray->count++] = array1->indices[i1++];
			/* We do not increase i2 here since it may be used to match the next array1->indices */
		}
	}

	return parray;
}


/* Search for the pattern P in the FILE TF and SAF. */
saidx_t
sa_search_filelist(file_list raw_files, const sauchar_t *P, saidx_t Psize, FILE *SAF, saidx_t *idx)
{
	saidx_t size, lsize, rsize, half;
	saidx_t match, lmatch, rmatch;
	saidx_t llmatch, lrmatch, rlmatch, rrmatch;
	saidx_t i, j, k;
	saint_t r;

	if(idx != NULL) { *idx = -1; }
	if((raw_files == NULL) || (P == NULL) || (SAF == NULL) || (Psize < 0) || (Psize > MAXPATTERNSIZE) ) { return -1; }
	if(raw_files->total_len == 0) { return 0; }
	if(Psize == 0) { if(idx != NULL) { *idx = 0; } return raw_files->total_len; }

	for(i = j = k = 0, lmatch = rmatch = 0, size = raw_files->total_len, half = size >> 1;
			0 < size;
			size = half, half >>= 1) {
		match = MIN(lmatch, rmatch);
		r = _compare_file(raw_files, P, Psize, _get_saidx(SAF, i + half), &match);
		if(r < 0) {
			i += half + 1;
			half -= (size & 1) ^ 1;
			lmatch = match;
		} else if(r > 0) {
			rmatch = match;
		} else {
			lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

			/* left part */
			for(llmatch = lmatch, lrmatch = match, half = lsize >> 1;
					0 < lsize;
					lsize = half, half >>= 1) {
				lmatch = MIN(llmatch, lrmatch);
				r = _compare_file(raw_files, P, Psize, _get_saidx(SAF, j + half), &lmatch);
				if(r < 0) {
					j += half + 1;
					half -= (lsize & 1) ^ 1;
					llmatch = lmatch;
				} else {
					lrmatch = lmatch;
				}
			}

			/* right part */
			for(rlmatch = match, rrmatch = rmatch, half = rsize >> 1;
					0 < rsize;
					rsize = half, half >>= 1) {
				rmatch = MIN(rlmatch, rrmatch);
				r = _compare_file(raw_files, P, Psize, _get_saidx(SAF, k + half), &rmatch);
				if(r <= 0) {
					k += half + 1;
					half -= (rsize & 1) ^ 1;
					rlmatch = rmatch;
				} else {
					rrmatch = rmatch;
				}
			}

			break;
		}
	}

	if(idx != NULL) { *idx = (0 < (k - j)) ? j : i; }
	return k - j;
}


/* Search for the character c with mask. */
saidx_t
sa_bitsearch_bytemask(file_list raw_files, FILE *SAF, saidx_t SAstart, saidx_t SAcount,
		sauchar_t c, sauchar_t cmask, saidx_t coffset, saidx_t *idx) {
	saidx_t size, lsize, rsize, half;
	saidx_t i, j, k, p;
	saint_t r;
	sauchar_t cf;

	for(i = j = k = 0, size = SAcount, half = size >> 1;
			0 < size;
			size = half, half >>= 1) {
		p = _get_saidx(SAF, SAstart + i + half);
		smfile_list_read(&cf, 1, 1, raw_files, p + coffset);
		cf &= cmask;
		//    printf("%s: binary search M@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + i + half, p, coffset, cf);
		r = (p < raw_files->total_len) ? cf - c : -1;
		if(r < 0) {
			i += half + 1;
			half -= (size & 1) ^ 1;
		} else if(r == 0) {
			lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

			/* left part */
			for(half = lsize >> 1;
					0 < lsize;
					lsize = half, half >>= 1) {
				p = _get_saidx(SAF, SAstart + j + half);
				smfile_list_read(&cf, 1, 1, raw_files, p + coffset);
				cf &= cmask;
				//        printf("%s: binary search L@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + j + half, p, coffset, cf);
				r = (p < raw_files->total_len) ? cf - c : -1;
				if(r < 0) {
					j += half + 1;
					half -= (lsize & 1) ^ 1;
				}
			}

			/* right part */
			for(half = rsize >> 1;
					0 < rsize;
					rsize = half, half >>= 1) {
				p = _get_saidx(SAF, SAstart + k + half);
				smfile_list_read(&cf, 1, 1, raw_files, p + coffset);
				cf &= cmask;
				//        printf("%s: binary search R@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + k + half, p, coffset, cf);
				r = (p < raw_files->total_len) ? cf - c : -1;
				if(r <= 0) {
					k += half + 1;
					half -= (rsize & 1) ^ 1;
				}
			}

			break;
		}
	}

	if(idx != NULL) { *idx = SAstart + ((0 < (k - j)) ? j : i); }
	return k - j;
}

saidx_t
sa_search_filelist_mask(file_list raw_files, const sauchar_t *P, saidx_t Psize, sauchar_t mask, FILE *SAF, saidx_t *idx)
{
	saidx_t count;

	if (Psize == 0) {
		*idx = 0;
		return raw_files->total_len;
	}

	if (mask != 0xFF) {
		/* search for file without the last byte */
		if (Psize == 1) {
			*idx = 0;
			count = raw_files->total_len;
		} else {
			count = sa_search_filelist(raw_files, P, Psize - 1, SAF, idx);
			if (count <= 0) {
				return count;
			}
		}

		/* search for file from current search result with the last byte */
		count = sa_bitsearch_bytemask(raw_files, SAF, *idx, count, P[Psize-1], mask, Psize-1, idx);
	} else {
		/* There is no last byte need masked search, search once to fasten the process */
		count = sa_search_filelist(raw_files, P, Psize, SAF, idx);
	}
	return count;
}

/**
 * @brief GREP bit-level pattern P in the file TF with support of suffix array.
 *
 * By now, only '0', '1' and '.' (which matches both '0' and '1') in P are supported.
 *
 * For each byte, assume the number of '.' is n and the number of '0' or '1' is m. Obviously, m+n = 8
 * For each '.' in a byte, current search cost will be doubled (enumerate '.' to '0' and '1).
 * For each '0' or '1' in a byte, the search result will be reduced by s times. If evenly distributed,
 * s = 2. However, in real case the distribution of data is never even, and we set s to be some value
 * between 1 and 2, like, 1.5.
 *
 * According to above statement, the byte will have below values
 * 		Search Cost (SC): 2^n
 * 		Search Scope Reduction (SSR): s^m
 *
 * If a search pattern contains too many '.', the search cost will be very high and unacceptable. Some-
 * times the cost is even higher than grep bit-level raw data. Under this scenario, we need to search
 * sub-pattern and joint the candidate results of sub-patterns, to decrease the search cost.
 *
 * In another way, even the search cost of the original pattern is not that high, we may still want to
 * search sub-pattern and joint to get a better performance. Generally, we want to find some of the sub-
 * patterns from the original pattern, joint candidate indices and walk through the raw data of each
 * indices to confirm the raw data really matches. We can do this by fixing the number of sub-patterns
 * each time, and estimating the cost value and scope reduction. By selecting the possible solutions
 * with minimum search cost and maximum search reduction, we'll find the (probability) best way of doing
 * the search.
 *
 * The SC and SSR of a search pattern can be calculated as
 * 		Search Cost (SC): 2^(n1+n2+...nN)
 * 		Search Scope Reduction (SSR): s^(m1+m2+...+mM)
 *
 * The joint cost of two sub-pattern candidate indices is (assume the number of indices are L and K)
 * 		O(L*logL + K*logK)*(Memory access time) + O(L+K) * (continuous HDD access time)
 *
 * Walk through final candidates costs (assume the number of candidate indices is X, P is the pattern length)
 * 		O(X) * (random HDD access time)
 *
 * Any input pattern can be converted into an array of SCs and SSRs. We'd like to find subpattern that
 * have large SSR and small SC.
 *
 * @param TF the original file to be searched
 * @param raw_files->total_len the size of TF (do we need this?)
 * @Param P a bit level pattern which includes '0' and '1' only. (may extend to support regex)
 * @param Psize the size of array P, should > 16
 * @param SAF the sorted suffix array file
 * @param raw_files->total_len the size of SAF
 * @param[out] result_array the array storing the index of TF that matches query
 * @param[out] result_size the size of the result_array
 *
 * @return the number of actual matches
 *
 * @warning At most 512K (= 8*64K) potential matched strings are evaluated
 */
saidx_t
sa_bitgrep_filelist(file_list raw_files, const sauchar_t *P, saidx_t Psize, FILE *SAF,
		raw_match result_array, saidx_t result_size)
{
	const int minimum_bits = 16;
	const int maximum_evaluations = 1024*1024; /*<< maximum suffices we evaluate */
	saidx_t i, j, k, length;
	saidx_t idx, count, result_pos, last_result_pos;

	mmtregex preg;

	uint8_t regex_buf[4096*8], buf[4096], *regex, mask;
	_potential_match_t match_min, match_min2;
	saidx_t match_count, joint_cost;

	index_array candidates, candidates1, candidates2;

	/* Remove the first and last '.' in P. ist and ied are the first/last character in P that is not '.' */
	if (!P) {
		exit_result(1, "Input parameter invalid");
	}
	for (i = 0; i < Psize && P[i] == '.'; i++) ;
	for (j = Psize - 1; j >= 0 && P[j] == '.'; j--) ;
	P = P + i;
	Psize = j - i + 1;
	i = 0;
	j = 0;

	/* Sanity check */
	if (Psize < minimum_bits || Psize > sizeof(regex_buf) - 16) {
		exit_result(1, "Input parameter invalid: too short or too long");
	}
	for (i = 0; i < Psize; i++) {
		if (P[i] != '0' && P[i] != '1' && P[i] != '.') {
			exit_result(1, "Input parameter invalid, can only be '0', '1', '.'");
		}
	}

	/* Allocate resource for regex_buf and fill before and after with '.' */
	memcpy(regex_buf + 8, P, Psize);
	for (i = 0; i < 8; i++) {
		regex_buf[i] = '.';
		regex_buf[i + 8 + Psize] = '.';
	}

	for (i = 0, result_pos = 0; i < 8 && result_pos < result_size; i++) {
//		printf("%s: ====================================================================\n", __FUNCTION__);
		/* Calculate the regex to be used, from regex to regex + length */
		regex = regex_buf + 8 - i;
		length = (Psize + i) / 8 + ((Psize + i) % 8 ? 1 : 0);

		match_min.count = raw_files->total_len;
		match_min2.count = raw_files->total_len;
		for (j = 0, match_count = 0; j < length; j += MAX(1, k)) {
			k = bits_to_bytes(regex + j*8, (length-j)*8, buf, &mask);
			if (k <= 0) {
				continue;
			}
			count = sa_search_filelist_mask(raw_files, buf, k, mask, SAF, &idx);
//			printf("%s: search found index %d(0x%X), count %d\n", __FUNCTION__, idx, idx, count);
			if (count <= 0) {
				/* No match found */
				break;
			} else {
				/* Now we find a potential match from idx to idx+count, keep two minimum matches only */
				if (count < match_min.count) {
					memcpy(&match_min2, &match_min, sizeof(match_min));
					match_min.count = count;
					match_min.index = idx;
					match_min.offset = j;
				} else if (count < match_min2.count) {
					match_min2.count = count;
					match_min2.index = idx;
					match_min2.offset = j;
				}
				match_count++;
			}
			//			printf("%s: potential matches, {offset, index, count} = {%d, %d(0x%X), %d}, {%d, %d(0x%X), %d}\n",
			//					__FUNCTION__,
			//					match_min.offset, match_min.index, match_min.index, match_min.count,
			//					match_min2.offset, match_min2.index, match_min2.index, match_min2.count);
		}
		if (j < length) {
			/* No match found since previous loop breaks out */
//			printf("%s: no match found, break out\n", __FUNCTION__);
			continue;
		}

		/* Try to joint the potential matches */
		candidates = NULL;
		joint_cost = (match_min.count + match_min2.count) * sizeof(match_min.index) / 4096;
		if (match_count == 0) {
			/* We need to fallback to other ways to move forward... */
//			printf("%s: !!!!!!!!! TODO: Fallback should be implemented !!!!!!!!!!!\n", __FUNCTION__);
		} else if (match_count == 1 || match_min.count < joint_cost) {
			/* TODO: Should implement sa_bitsearch_file similar trick to reduce candidates */
			match_min.count = MIN(match_min.count, maximum_evaluations);
			candidates = _get_saindices(SAF, &match_min);
//			if (match_count == 1)
//				printf("%s: only one valid match found with %d indices\n", __FUNCTION__, match_min.count);
//			else
//				printf("%s: It would be efficient to evaluate %d indices than joint %d and %d indices\n",
//						__FUNCTION__, match_min.count, match_min.count, match_min2.count);
		} else {
			/* Calculate which approach is better: walk match_min or join match_min and match_min2 */
			/* We are sure that both match_min and match_min2 do not exceed maximum_evaluations */
			candidates1 = _get_saindices(SAF, &match_min);
			candidates2 = _get_saindices(SAF, &match_min2);

			/* Although the match_min and match_min2 are binary searched ranges, adding
			 * offset from the start of the match make them not range anymore.
			 */
			candidates = _joint_array(candidates1, candidates2, 0);
//			printf("%s: joint two potential matches %d and %d indices get %d indices\n",
//					__FUNCTION__, candidates1->count, candidates2->count, candidates->count);

			delete_index_array(candidates1);
			delete_index_array(candidates2);
			if (candidates->count == 0) {
				delete_index_array(candidates);
				candidates = NULL;
			}
		}

		/* Finally, walk through candidates to decide final match */
		if (candidates) {
			last_result_pos = result_pos;
			preg = mmtregex_from_str(regex, length * 8);
			for (j = 0; j < candidates->count; j++) {
				if (candidates->indices[j] + length > raw_files->total_len) {
					/* not enough bytes to read, definitely not match*/
					continue;
				}
				smfile_list_read(buf, sizeof(*buf), length, raw_files, candidates->indices[j]);
				if (mmtregex_match(preg, buf, length) == MMTREGEX_OK) {
					if (result_pos >= result_size) {
						/* not enough room to fill result */
						goto result_full;
					}
					result_array[result_pos].off_byte = candidates->indices[j];
					result_array[result_pos].off_bit = i;
					result_array[result_pos].len_bit = Psize;
					result_pos++;
				}
			}
			delete_mmtregex(preg);

//			printf("%s: walk through %d potential indices get %d indices\n",
//					__FUNCTION__, candidates->count, result_pos - last_result_pos);

			delete_index_array(candidates);
		}

	}

	result_full:
	return result_pos;
}

/********************************************************************
 * result to json
 ********************************************************************/

JSON_Value *create_json_result(file_list raw_files, raw_match results, saidx_t size, saidx_t match_size)
{
	/* Local variables */
	saidx_t i, actual_match;
	char *serialized_string;

	/* Init root value/object */
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	JSON_Value *result_array_value = json_value_init_array();
	JSON_Array *result_array = json_array(result_array_value);
	JSON_Value *temp_value;
	JSON_Object *temp_object;
	file_info finfo;

	for (i = 0, actual_match = 0; i < size; i++) {
		/* Get the fileinfo from the offset */
		finfo = smfile_list_get(raw_files, results[i].off_byte);
		if (finfo == NULL) {
			/* ERROR here, what we have matched in the file list is not in the file list ???? */

			continue;
		}

		/* Check whether the match passed the boundary of the file */
		if (results[i].off_byte - finfo->offset_in_list + match_size > finfo->length) {
			continue;
		}

		/* Create the result object */
		temp_value = json_value_init_object();
		temp_object = json_object(temp_value);
		json_object_set_string(temp_object, "name", finfo->name);
		json_object_set_number(temp_object, "offset", finfo->offset + results[i].off_byte - finfo->offset_in_list);
		json_object_set_number(temp_object, "offset_bit", results[i].off_bit);
		json_object_set_number(temp_object, "length", results[i].len_bit);

		/* Append the match value */
		json_array_append_value(result_array, temp_value);
		actual_match++;
	}

	/* Set root values */
	json_object_set_number(root_object, "code", 0);
	json_object_set_value(root_object, "matches", result_array_value);

	/* just put results */
	serialized_string = json_serialize_to_string_pretty(root_value);
	puts(serialized_string);
	json_free_serialized_string(serialized_string);

	return root_value;
}

/********************************************************************
 * main function
 ********************************************************************/
/*
 * @param argv[1] the pattern to be matched
 * @param argv[2] the JSON config file for the raw files and the index file
 */
int
main(int argc, const char *argv[]) {
	file_list raw_files;	/*<< The file to be searched */
	FILE *SAF;				/*<< The file for suffix array */
	LFS_OFF_T n_suf;		/*<< length of SAF */
	const char *P;
	size_t Psize;
	const int result_size = 65536;
	saidx_t i, size;
	raw_match_t result[result_size];
	clock_t start, finish;

	JSON_Value *config;
	const char *index_file;

	program_name = argv[0];

	/* Check arguments */
	if (argc != 3)
		exit_result(1, "Invalid argument");

	/* first parameter is the matched pattern */
	P = argv[1];
	Psize = strlen(P);

	/* Check JSON format. */
	config = smfile_open_config(argv[2]);
	if (config == NULL) {
		exit_result(2, "json config file %s error", argv[2]);
	}

	/* Convert JSON file into a structure */
	raw_files = smfile_list_create(config);
	index_file = smfile_get_index_file(config);
	if (raw_files->total_len > MAX_RAW_FILE_LENGTH) {
		/* We only support 4gB file length at maximum */
		exit_result(6, "overall input file length too huge: %zu, should less than: %zu", raw_files->total_len, MAX_RAW_FILE_LENGTH);
	}

	/* open index file */
	if((SAF = LFS_FOPEN(index_file, "rb")) == NULL) {
		exit_result(2, "cannot open index file %s", index_file);
	}

	/* Get the file size. */
	if(LFS_FSEEK(SAF, 0, SEEK_END) == 0) {
		n_suf = LFS_FTELL(SAF);
		rewind(SAF);
		if(n_suf < 0) {
			exit_result(2, "cannot ftell index file %s", index_file);
		}
		if (n_suf < raw_files->total_len * 4){
			exit_result(2, "index file %s is not correct", index_file);
		}
	} else {
		exit_result(2, "cannot fseek index file %s", index_file);
	}

	/* Search and print */
	start = clock();
	//for (i = 0; i < 1000; i++)
	size = sa_bitgrep_filelist(raw_files, (const sauchar_t *)P, (saidx_t)Psize, SAF, result, result_size);
	if (size > 0)
		qsort(result, size, sizeof(*result), compare_saidx);
	finish = clock();
//	for(i = 0; i < size; ++i) {
//		fprintf(stdout, "0x%x\n", result[i]);
//	}
//	fprintf(stderr, "%.6f ms or %d CPU ticks\n", 1000000 * (double)(finish - start) / (double)CLOCKS_PER_SEC, (int)(finish - start));


	create_json_result(raw_files, result, size, Psize);

	/* Deallocate memory. */
	fclose(SAF);

	return 0;
}
