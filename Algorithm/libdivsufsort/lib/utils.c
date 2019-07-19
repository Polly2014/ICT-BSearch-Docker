/*
 * utils.c for libdivsufsort
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

#include "divsufsort_private.h"

#include "mmtregex.h"

#define MAXPATTERNSIZE 4096

/*- Private Function -*/

/* Binary search for inverse bwt. */
static
saidx_t
binarysearch_lower(const saidx_t *A, saidx_t size, saidx_t value) {
  saidx_t half, i;
  for(i = 0, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    if(A[i + half] < value) {
      i += half + 1;
      half -= (size & 1) ^ 1;
    }
  }
  return i;
}


/*- Functions -*/

/* Burrows-Wheeler transform. */
saint_t
bw_transform(const sauchar_t *T, sauchar_t *U, saidx_t *SA,
             saidx_t n, saidx_t *idx) {
  saidx_t *A, i, j, p, t;
  saint_t c;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (idx == NULL)) { return -1; }
  if(n <= 1) {
    if(n == 1) { U[0] = T[0]; }
    *idx = n;
    return 0;
  }

  if((A = SA) == NULL) {
    i = divbwt(T, U, NULL, n);
    if(0 <= i) { *idx = i; i = 0; }
    return (saint_t)i;
  }

  /* BW transform. */
  if(T == U) {
    t = n;
    for(i = 0, j = 0; i < n; ++i) {
      p = t - 1;
      t = A[i];
      if(0 <= p) {
        c = T[j];
        U[j] = (j <= p) ? T[p] : (sauchar_t)A[p];
        A[j] = c;
        j++;
      } else {
        *idx = i;
      }
    }
    p = t - 1;
    if(0 <= p) {
      c = T[j];
      U[j] = (j <= p) ? T[p] : (sauchar_t)A[p];
      A[j] = c;
    } else {
      *idx = i;
    }
  } else {
    U[0] = T[n - 1];
    for(i = 0; A[i] != 0; ++i) { U[i + 1] = T[A[i] - 1]; }
    *idx = i + 1;
    for(++i; i < n; ++i) { U[i] = T[A[i] - 1]; }
  }

  if(SA == NULL) {
    /* Deallocate memory. */
    free(A);
  }

  return 0;
}

/* Inverse Burrows-Wheeler transform. */
saint_t
inverse_bw_transform(const sauchar_t *T, sauchar_t *U, saidx_t *A,
                     saidx_t n, saidx_t idx) {
  saidx_t C[ALPHABET_SIZE];
  sauchar_t D[ALPHABET_SIZE];
  saidx_t *B;
  saidx_t i, p;
  saint_t c, d;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (idx < 0) ||
     (n < idx) || ((0 < n) && (idx == 0))) {
    return -1;
  }
  if(n <= 1) { return 0; }

  if((B = A) == NULL) {
    /* Allocate n*sizeof(saidx_t) bytes of memory. */
    if((B = (saidx_t *)malloc((size_t)n * sizeof(saidx_t))) == NULL) { return -2; }
  }

  /* Inverse BW transform. */
  for(c = 0; c < ALPHABET_SIZE; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, d = 0, i = 0; c < ALPHABET_SIZE; ++c) {
    p = C[c];
    if(0 < p) {
      C[c] = i;
      D[d++] = (sauchar_t)c;
      i += p;
    }
  }
  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
  for(i = 0, p = idx; i < n; ++i) {
    U[i] = D[binarysearch_lower(C, d, p)];
    p = B[p - 1];
  }

  if(A == NULL) {
    /* Deallocate memory. */
    free(B);
  }

  return 0;
}

/* Checks the suffix array SA of the string T. */
saint_t
sufcheck(const sauchar_t *T, const saidx_t *SA,
         saidx_t n, saint_t verbose) {
  saidx_t C[ALPHABET_SIZE];
  saidx_t i, p, q, t;
  saint_t c;

  if(verbose) { fprintf(stderr, "sufcheck: "); }

  /* Check arguments. */
  if((T == NULL) || (SA == NULL) || (n < 0)) {
    if(verbose) { fprintf(stderr, "Invalid arguments.\n"); }
    return -1;
  }
  if(n == 0) {
    if(verbose) { fprintf(stderr, "Done.\n"); }
    return 0;
  }

  /* check range: [0..n-1] */
  for(i = 0; i < n; ++i) {
    if((SA[i] < 0) || (n <= SA[i])) {
      if(verbose) {
        fprintf(stderr, "Out of the range [0,%" PRIdSAIDX_T "].\n"
                        "  SA[%" PRIdSAIDX_T "]=%" PRIdSAIDX_T "\n",
                        n - 1, i, SA[i]);
      }
      return -2;
    }
  }

  /* check first characters. */
  for(i = 1; i < n; ++i) {
    if(T[SA[i - 1]] > T[SA[i]]) {
      if(verbose) {
        fprintf(stderr, "Suffixes in wrong order.\n"
                        "  T[SA[%" PRIdSAIDX_T "]=%" PRIdSAIDX_T "]=%d"
                        " > T[SA[%" PRIdSAIDX_T "]=%" PRIdSAIDX_T "]=%d\n",
                        i - 1, SA[i - 1], T[SA[i - 1]], i, SA[i], T[SA[i]]);
      }
      return -3;
    }
  }

  /* check suffixes. */
  for(i = 0; i < ALPHABET_SIZE; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(i = 0, p = 0; i < ALPHABET_SIZE; ++i) {
    t = C[i];
    C[i] = p;
    p += t;
  }

  q = C[T[n - 1]];
  C[T[n - 1]] += 1;
  for(i = 0; i < n; ++i) {
    p = SA[i];
    if(0 < p) {
      c = T[--p];
      t = C[c];
    } else {
      c = T[p = n - 1];
      t = q;
    }
    if((t < 0) || (p != SA[t])) {
      if(verbose) {
        fprintf(stderr, "Suffix in wrong position.\n"
                        "  SA[%" PRIdSAIDX_T "]=%" PRIdSAIDX_T " or\n"
                        "  SA[%" PRIdSAIDX_T "]=%" PRIdSAIDX_T "\n",
                        t, (0 <= t) ? SA[t] : -1, i, SA[i]);
      }
      return -4;
    }
    if(t != q) {
      ++C[c];
      if((n <= C[c]) || (T[SA[C[c]]] != c)) { C[c] = -1; }
    }
  }

  if(1 <= verbose) { fprintf(stderr, "Done.\n"); }
  return 0;
}


static
int
_compare(const sauchar_t *T, saidx_t Tsize,
         const sauchar_t *P, saidx_t Psize,
         saidx_t suf, saidx_t *match) {
  saidx_t i, j;
  saint_t r;
  printf("idx = %8d, match_idx = %3d, ", suf, *match);
  for(i = suf + *match, j = *match, r = 0;
      (i < Tsize) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j)
  {
  }
  printf("compare = %3d\n", (r == 0) ? -(j != Psize) : r);
  *match = j;
  return (r == 0) ? -(j != Psize) : r;
}

/* Search for the pattern P in the string T. */
saidx_t
sa_search(const sauchar_t *T, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          const saidx_t *SA, saidx_t SAsize,
          saidx_t *idx) {
  saidx_t size, lsize, rsize, half;
  saidx_t match, lmatch, rmatch;
  saidx_t llmatch, lrmatch, rlmatch, rrmatch;
  saidx_t i, j, k;
  saint_t r;

  if(idx != NULL) { *idx = -1; }
  if((T == NULL) || (P == NULL) || (SA == NULL) ||
     (Tsize < 0) || (Psize < 0) || (SAsize < 0)) { return -1; }
  if((Tsize == 0) || (SAsize == 0)) { return 0; }
  if(Psize == 0) { if(idx != NULL) { *idx = 0; } return SAsize; }

  for(i = j = k = 0, lmatch = rmatch = 0, size = SAsize, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    match = MIN(lmatch, rmatch);
    printf(" match(%8d %8d): ", i,  half);
    r = _compare(T, Tsize, P, Psize, SA[i + half], &match);
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
        printf("lmatch: ");
        r = _compare(T, Tsize, P, Psize, SA[j + half], &lmatch);
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
        printf("rmatch: ");
        r = _compare(T, Tsize, P, Psize, SA[k + half], &rmatch);
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

/* Search for the character c in the string T. */
saidx_t
sa_simplesearch(const sauchar_t *T, saidx_t Tsize,
                const saidx_t *SA, saidx_t SAsize,
                saint_t c, saidx_t *idx) {
  saidx_t size, lsize, rsize, half;
  saidx_t i, j, k, p;
  saint_t r;

  if(idx != NULL) { *idx = -1; }
  if((T == NULL) || (SA == NULL) || (Tsize < 0) || (SAsize < 0)) { return -1; }
  if((Tsize == 0) || (SAsize == 0)) { return 0; }

  for(i = j = k = 0, size = SAsize, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    p = SA[i + half];
    r = (p < Tsize) ? T[p] - c : -1;
    if(r < 0) {
      i += half + 1;
      half -= (size & 1) ^ 1;
    } else if(r == 0) {
      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

      /* left part */
      for(half = lsize >> 1;
          0 < lsize;
          lsize = half, half >>= 1) {
        p = SA[j + half];
        r = (p < Tsize) ? T[p] - c : -1;
        if(r < 0) {
          j += half + 1;
          half -= (lsize & 1) ^ 1;
        }
      }

      /* right part */
      for(half = rsize >> 1;
          0 < rsize;
          rsize = half, half >>= 1) {
        p = SA[k + half];
        r = (p < Tsize) ? T[p] - c : -1;
        if(r <= 0) {
          k += half + 1;
          half -= (rsize & 1) ^ 1;
        }
      }

      break;
    }
  }

  if(idx != NULL) { *idx = (0 < (k - j)) ? j : i; }
  return k - j;
}

saidx_t _get_saidx(FILE *SAF, saidx_t idx)
{
	saidx_t value;
	fseek(SAF, idx * sizeof(idx), SEEK_SET);
	fread(&value, sizeof(saidx_t), 1, SAF);
	return value;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


static
int
_compare_file(FILE *TF, saidx_t Tsize,
         const sauchar_t *P, saidx_t Psize,
         saidx_t suf, saidx_t *match) {
  saidx_t i, j;
  saint_t r, read_count;
  static sauchar_t T[MAXPATTERNSIZE];

  //printf("idx = %8d, match_idx = %3d, ", suf, *match);

  /* read content from file */
  read_count = min(Psize - *match, Tsize - suf - *match);
  fseek(TF, suf + *match, SEEK_SET);
  fread(T, 1, read_count, TF);

  for(i = 0, j = *match, r = 0;
      (i < read_count) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j)
  {
  }
  //printf("compare = %3d\n", (r == 0) ? -(j != Psize) : r);
  *match = j;
  return (r == 0) ? -(j != Psize) : r;
}

/* Search for the pattern P in the FILE TF and SAF. */
saidx_t
sa_search_file(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          FILE *SAF, saidx_t SAsize,
          saidx_t *idx) {
  saidx_t size, lsize, rsize, half;
  saidx_t match, lmatch, rmatch;
  saidx_t llmatch, lrmatch, rlmatch, rrmatch;
  saidx_t i, j, k;
  saint_t r;

  if(idx != NULL) { *idx = -1; }
  if((TF == NULL) || (P == NULL) || (SAF == NULL) ||
     (Tsize < 0) || (Psize < 0) || (Psize > MAXPATTERNSIZE) || (SAsize < 0)) { return -1; }
  if((Tsize == 0) || (SAsize == 0)) { return 0; }
  if(Psize == 0) { if(idx != NULL) { *idx = 0; } return SAsize; }

  for(i = j = k = 0, lmatch = rmatch = 0, size = SAsize, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    match = MIN(lmatch, rmatch);
    //printf(" match(%8d %8d): ", i, half);
    r = _compare_file(TF, Tsize, P, Psize, _get_saidx(SAF, i + half), &match);
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
        //printf("lmatch: ");
        r = _compare_file(TF, Tsize, P, Psize, _get_saidx(SAF, j + half), &lmatch);
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
        //printf("rmatch: ");
        r = _compare_file(TF, Tsize, P, Psize, _get_saidx(SAF, k + half), &rmatch);
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

sauchar_t _get_char_from_file(FILE *TF, saidx_t idx)
{
	sauchar_t c;
	fseek(TF, idx, SEEK_SET);
	fread(&c, sizeof(c), 1, TF);
	return c;
}

/* Search for the character c with mask. */
saidx_t
sa_bitsearch_bytemask(FILE *TF, saidx_t Tsize,
                FILE *SAF, saidx_t SAstart, saidx_t SAcount,
				sauchar_t c, sauchar_t cmask, saidx_t coffset, saidx_t *idx) {
  saidx_t size, lsize, rsize, half;
  saidx_t i, j, k, p;
  saint_t r;
  sauchar_t cf;

  if(idx != NULL) { *idx = -1; }
  if((TF == NULL) || (SAF == NULL) || (Tsize < 0) || (SAstart < 0)) { return -1; }
  if((Tsize == 0) || (SAcount == 0)) { return 0; }

//  printf("%s: search for 0x%x from %8d to %8d count %8d\n", __FUNCTION__, c, SAstart, SAstart + SAcount, SAcount);
  for(i = j = k = 0, size = SAcount, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    p = _get_saidx(SAF, SAstart + i + half);
    cf = _get_char_from_file(TF, p + coffset) & cmask;
//    printf("%s: binary search M@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + i + half, p, coffset, cf);
    r = (p < Tsize) ? cf - c : -1;
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
        cf = _get_char_from_file(TF, p + coffset) & cmask;
//        printf("%s: binary search L@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + j + half, p, coffset, cf);
        r = (p < Tsize) ? cf - c : -1;
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
        cf = _get_char_from_file(TF, p + coffset) & cmask;
//        printf("%s: binary search R@%8d index %8d + %2d, value 0x%x\n", __FUNCTION__, SAstart + k + half, p, coffset, cf);
        r = (p < Tsize) ? cf - c : -1;
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

saidx_t _binary_search_times(saidx_t length)
{
	saidx_t i;
	for (i = 0; length != 0; i++, length>>=1) ;
	return i + 1;
}

saidx_t
sa_search_file_mask(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize, sauchar_t mask,
          FILE *SAF, saidx_t SAsize,
          saidx_t *idx)
{
	saidx_t count;

	if (Psize == 0) {
		*idx = 0;
		return SAsize;
	}

	if (mask != 0xFF) {
		/* search for file without the last byte */
		if (Psize == 1) {
			*idx = 0;
			count = SAsize;
		} else {
			count = sa_search_file(TF, Tsize, P, Psize - 1, SAF, SAsize, idx);
			if (count <= 0) {
				return count;
			}
		}

		/* search for file from current search result with the last byte */
		count = sa_bitsearch_bytemask(TF, Tsize, SAF, *idx, count, P[Psize-1], mask, Psize-1, idx);
	} else {
		/* There is no last byte need masked search, search once to fasten the process */
		count = sa_search_file(TF, Tsize, P, Psize, SAF, SAsize, idx);
	}
	return count;
}

/**
 * @brief Search for the exact bit-level string P in the file TF with support of suffix array.
 *
 * @param TF the original file to be searched
 * @param Tsize the size of TF (do we need this?)
 * @Param P a bit level pattern which includes '0' and '1' only. (may extend to support regex)
 * @param Psize the size of array P, should > 16
 * @param SAF the sorted suffix array file
 * @param SAsize the size of SAF
 * @param[out] result_array the array storing the index of TF that matches query
 * @param[out] result_size the size of the result_array
 *
 * @return the number of actual matches
 *
 * @warning At most 512K (= 8*64K) potential matched strings are evaluated
 */
saidx_t
sa_bitsearch_file(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          FILE *SAF, saidx_t SAsize,
          saidx_t *result_array, saidx_t result_size)
{
	const int minimum_bits = 16;
	saidx_t i, b, k;
	sauchar_t *PB, the_byte, pre_mask, post_mask, buf8[8];
	saidx_t pre_bytes, mid_bytes, post_bytes;
	saidx_t pre_bits, mid_bits, post_bits;
	saidx_t idx, count, result_pos;
	saidx_t *indices;
	saidx_t search_cost; 	/*<< The cost of one search in SAF and TF */
	saidx_t permutate_cost; /*<< The cost of permutating pre_bits */
	saidx_t permutate_times;

	/* Calculate search_cost, in the unit of file access
	 * Search for two boundary usually takes 1.5 times binary search
	 * and visit both SAF and TF takes another 2 times.
	 */
	search_cost = _binary_search_times(Tsize) * 3;

	/* Validate that P contains bit only (may be extended to REGEX)
	 * For bit search, 16 bits (2 bytes) is the minimum we'd like to search
	 */
	if (P && Psize >= minimum_bits) {
		for (i = 0; i < Psize; i++) {
			if (P[i] != '0' && P[i] != '1') {
				return -1;
			}
		}
	} else {
		return -1;
	}

	/* A large enough buffer */
	PB = malloc(Psize / 8 + 2);

	/* Assume b is the valid start point of a byte
	 * i.e. the number of bits that cannot be searched
	 */
	result_pos = 0;
	for (b = 0; b < 8; b++) {
		/* Calculate how many bits/bytes for each part */
		pre_bits = b;
		post_bits = (Psize - pre_bits) % 8;
		mid_bits = Psize - pre_bits - post_bits;
		pre_bytes = pre_bits ? 1 : 0;
		post_bytes = post_bits ? 1 : 0;
		mid_bytes = mid_bits / 8;

		pre_mask = 0xFF >> (8 - pre_bits);
		/* If no post_bits, post_mask should set to 0xFF to avoid overlap with mid_bits */
		post_mask = post_bits ? 0xFF << (8 - post_bits) : 0xFF;

		/* Convert the bits array P to bytes array PB*/
		if (pre_bits) {
			for (i = 0; i < 8 - pre_bits; i++) {
				buf8[i] = '0';
			}
			for (; i < 8; i++) {
				buf8[i] = P[i + pre_bits - 8];
			}
			PB[0] = bits_to_byte(buf8);
		}
		if (mid_bits) {
			for (i = 0; i < mid_bytes; i++) {
				PB[i + pre_bytes] = bits_to_byte(P + i*8 + pre_bits);
			}
		}
		if (post_bits) {
			for (i = 0; i < post_bits; i++) {
				buf8[i] = P[i + mid_bits + pre_bits];
			}
			for (; i < 8; i++) {
				buf8[i] = '0';
			}
			PB[pre_bytes + mid_bytes] = bits_to_byte(buf8);
		}
		printf("pre: {%d bits, mask = %x, byte = %x}, mid: %d bits, post: {%d bits, mask = %x, byte = %x}\n",
				pre_bits, pre_mask, PB[0], mid_bits, post_bits, post_mask, PB[pre_bytes + mid_bytes]);


		/* search for mid and post */
		count = sa_search_file_mask(TF, Tsize, PB+pre_bytes, mid_bytes+post_bytes, post_mask, SAF, SAsize, &idx);
		if (count <= 0) {
			continue;
		}

		/* Calculating how many file access are needed if we permutate bits before pre_bits */
		permutate_times = 1 << (8-pre_bits);
		permutate_cost = search_cost * permutate_times + count / (1<<pre_bits) ;
		printf("%s: Candidate %d, permutate cost %d = %d*%d+%d/%d, use %s\n", __FUNCTION__,
				count, permutate_cost, permutate_times, search_cost, count, (1<<pre_bits), permutate_cost < count ? "permutate":"walk");
		if (pre_bits > 0 && permutate_cost < count) {
			/* Permutating PB[0] and search would be a better choice */
			for (k = 0; k < permutate_times; k++, PB[0] += (1 << pre_bits)) {
				count = sa_search_file_mask(TF, Tsize, PB, pre_bytes + mid_bytes + post_bytes, post_mask, SAF, SAsize, &idx);
				if (count > 0) {
					/* read indices from file */
					indices = malloc(sizeof(*indices) * count);
					fseek(SAF, idx * sizeof(idx), SEEK_SET);
					fread(indices, sizeof(*indices), count, SAF);

					/* All these indices are matches */
					for (i = 0; i < count; i++) {
						result_array[result_pos++] = indices[i];
						printf("match found at %d\n", indices[i]);
						if (result_pos >= result_size) goto result_oversize;
					}
					free(indices);
				}
			}
			/* Restore PB[0] to its original value */
			PB[0] -= ((permutate_times-1) << pre_bits);
 		} else {
			/* read candidate indices from file and match with PB[0] would be a better choice*/
			indices = malloc(sizeof(*indices) * count);
			fseek(SAF, idx * sizeof(idx), SEEK_SET);
			fread(indices, sizeof(*indices), count, SAF);

			/* Validate the result for pre_bits */
			for (i = 0; i < count; i++) {
				/* read the pre_bits byte from raw file */
				if (pre_bits > 0) {
					if (indices[i] == 0) {
						/* no match since it's at the beginning of a file */
						continue;
					}
					fseek(TF, indices[i] - 1, SEEK_SET);
					fread(&the_byte, 1, 1, TF);
					if ((the_byte & pre_mask) != PB[0]) {
						/* No match */
						continue;
					}
				}

				/* we find a match */
				result_array[result_pos++] = indices[i] - (pre_bits ? 1 : 0);
				printf("match found at %d\n", indices[i]);
				if (result_pos >= result_size) goto result_oversize;
			}

			free(indices);
		}
	}

	free(PB);
	return result_pos;

result_oversize:
	free(PB);
	free(indices);
	return result_pos;
}

/*******************************************************************************
 * Regex bit-level search
 *******************************************************************************/

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

	/* For debug trace */
	if (bytes_len > 0) {
		printf("%s: (", __FUNCTION__);
		for (m = 0; m < K; m++) {
			printf("%c", bits[m]);
		}
		printf(")");
		for ( ; m < bits_len; m++) {
			printf("%c", bits[m]);
		}
		printf(" is converted to:");
		for (m = 0; m < bytes_len; m++) {
			printf(" %X", bytes[m]);
		}
		printf(", with mask %X\n", *mask);
	}

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
//		printf("[%6d] = %9d, [%6d] = %9d\n", i1, array1->indices[i1], i2, array2->indices[i2]);
		if (array1->indices[i1] + maximum_distance < array2->indices[i2]) {
			i1++;
		} else if (array1->indices[i1] - maximum_distance > array2->indices[i2]) {
			i2++;
		} else {
//			printf("match found!!!!!");
			parray->indices[parray->count++] = array1->indices[i1++];
			/* We do not increase i2 here since it may be used to match the next array1->indices */
		}
	}

	return parray;
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
 * @param Tsize the size of TF (do we need this?)
 * @Param P a bit level pattern which includes '0' and '1' only. (may extend to support regex)
 * @param Psize the size of array P, should > 16
 * @param SAF the sorted suffix array file
 * @param SAsize the size of SAF
 * @param[out] result_array the array storing the index of TF that matches query
 * @param[out] result_size the size of the result_array
 *
 * @return the number of actual matches
 *
 * @warning At most 512K (= 8*64K) potential matched strings are evaluated
 */
saidx_t
sa_bitgrep_file(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          FILE *SAF, saidx_t SAsize,
          saidx_t *result_array, saidx_t result_size)
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
		printf("%s: Input parameter invalid.\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < Psize && P[i] == '.'; i++) ;
	for (j = Psize - 1; j >= 0 && P[j] == '.'; j--) ;
	P = P + i;
	Psize = j - i + 1;
	i = 0;
	j = 0;

	/* Sanity check */
	if (Psize <= minimum_bits || Psize > sizeof(regex_buf) - 16) {
		printf("%s: Input parameter invalid.\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < Psize; i++) {
		if (P[i] != '0' && P[i] != '1' && P[i] != '.') {
			printf("%s: Input parameter invalid, can only be '0', '1', '.'\n", __FUNCTION__);
			return -1;
		}
	}

	/* Allocate resource for regex_buf and fill before and after with '.' */
	memcpy(regex_buf + 8, P, Psize);
	for (i = 0; i < 8; i++) {
		regex_buf[i] = '.';
		regex_buf[i + 8 + Psize] = '.';
	}

	for (i = 0, result_pos = 0; i < 8 && result_pos < result_size; i++) {
		printf("%s: ====================================================================\n", __FUNCTION__);
		/* Calculate the regex to be used, from regex to regex + length */
		regex = regex_buf + 8 - i;
		length = (Psize + i) / 8 + ((Psize + i) % 8 ? 1 : 0);

		match_min.count = SAsize;
		match_min2.count = SAsize;
		for (j = 0, match_count = 0; j < length; j += max(1, k)) {
			k = bits_to_bytes(regex + j*8, (length-j)*8, buf, &mask);
			if (k <= 0) {
				continue;
			}
			count = sa_search_file_mask(TF, Tsize, buf, k, mask, SAF, SAsize, &idx);
			printf("%s: search found index %d(0x%X), count %d\n", __FUNCTION__, idx, idx, count);
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
			printf("%s: no match found, break out\n", __FUNCTION__);
			continue;
		}

		/* Try to joint the potential matches */
		candidates = NULL;
		joint_cost = (match_min.count + match_min2.count) * sizeof(match_min.index) / 4096;
		if (match_count == 0) {
			/* We need to fallback to other ways to move forward... */
			printf("%s: !!!!!!!!! TODO: Fallback should be implemented !!!!!!!!!!!\n", __FUNCTION__);
		} else if (match_count == 1 || match_min.count < joint_cost) {
			/* TODO: Should implement sa_bitsearch_file similar trick to reduce candidates */
			match_min.count = min(match_min.count, maximum_evaluations);
			candidates = _get_saindices(SAF, &match_min);
			if (match_count == 1)
				printf("%s: only one valid match found with %d indices\n", __FUNCTION__, match_min.count);
			else
				printf("%s: It would be efficient to evaluate %d indices than joint %d and %d indices\n",
						__FUNCTION__, match_min.count, match_min.count, match_min2.count);
		} else {
			/* Calculate which approach is better: walk match_min or join match_min and match_min2 */
			/* We are sure that both match_min and match_min2 do not exceed maximum_evaluations */
			candidates1 = _get_saindices(SAF, &match_min);
			candidates2 = _get_saindices(SAF, &match_min2);

			/* Although the match_min and match_min2 are binary searched ranges, adding
			 * offset from the start of the match make them not range anymore.
			 */
			candidates = _joint_array(candidates1, candidates2, 0);
			printf("%s: joint two potential matches %d and %d indices get %d indices\n",
					__FUNCTION__, candidates1->count, candidates2->count, candidates->count);

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
				if (candidates->indices[j] + length > Tsize) {
					/* not enough bytes to read, definitely not match*/
					continue;
				}
				fseek(TF, candidates->indices[j], SEEK_SET);
				fread(buf, sizeof(*buf), length, TF);
				if (mmtregex_match(preg, buf, length) == MMTREGEX_OK) {
					if (result_pos >= result_size) {
						/* not enough room to fill result */
						goto result_full;
					}
					result_array[result_pos++] = candidates->indices[j];
				}
			}
			delete_mmtregex(preg);

			printf("%s: walk through %d potential indices get %d indices\n",
					__FUNCTION__, candidates->count, result_pos - last_result_pos);

			delete_index_array(candidates);
		}

	}

result_full:
	return result_pos;
}

/**
 * @brief Grep for the exact bit-level string P in the file TF with support of suffix array.
 *
 * By now, only '0', '1' and '.' (which matches both '0' and '1') are supported
 *
 * @param TF the original file to be searched
 * @param Tsize the size of TF (do we need this?)
 * @Param P a bit level pattern which includes '0' and '1' only. (may extend to support regex)
 * @param Psize the size of array P, should > 16
 * @param SAF the sorted suffix array file
 * @param SAsize the size of SAF
 * @param[out] result_array the array storing the index of TF that matches query
 * @param[out] result_size the size of the result_array
 *
 * @return the number of actual matches
 *
 * @warning At most 512K (= 8*64K) potential matched strings are evaluated
 */
saidx_t
sa_bitgrep2_file(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          FILE *SAF, saidx_t SAsize,
          saidx_t *result_array, saidx_t result_size)
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
		printf("%s: Input parameter invalid.\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < Psize && P[i] == '.'; i++) ;
	for (j = Psize - 1; j >= 0 && P[j] == '.'; j--) ;
	P = P + i;
	Psize = j - i + 1;
	i = 0;
	j = 0;

	/* Sanity check */
	if (Psize <= minimum_bits || Psize > sizeof(regex_buf) - 16) {
		printf("%s: Input parameter invalid.\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < Psize; i++) {
		if (P[i] != '0' && P[i] != '1' && P[i] != '.') {
			printf("%s: Input parameter invalid, can only be '0', '1', '.'\n", __FUNCTION__);
			return -1;
		}
	}

	/* Allocate resource for regex_buf and fill before and after with '.' */
	memcpy(regex_buf + 8, P, Psize);
	for (i = 0; i < 8; i++) {
		regex_buf[i] = '.';
		regex_buf[i + 8 + Psize] = '.';
	}

	for (i = 0, result_pos = 0; i < 8 && result_pos < result_size; i++) {
		printf("%s: ====================================================================\n", __FUNCTION__);
		/* Calculate the regex to be used, from regex to regex + length */
		regex = regex_buf + 8 - i;
		length = (Psize + i) / 8 + ((Psize + i) % 8 ? 1 : 0);

		match_min.count = SAsize;
		match_min2.count = SAsize;
		for (j = 0, match_count = 0; j < length; j += max(1, k)) {
			k = bits_to_bytes(regex + j*8, (length-j)*8, buf, &mask);
			if (k <= 0) {
				continue;
			}
			count = sa_search_file_mask(TF, Tsize, buf, k, mask, SAF, SAsize, &idx);
			printf("%s: search found index %d(0x%X), count %d\n", __FUNCTION__, idx, idx, count);
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
			printf("%s: no match found, break out\n", __FUNCTION__);
			continue;
		}

		/* Try to joint the potential matches */
		candidates = NULL;
		joint_cost = (match_min.count + match_min2.count) * sizeof(match_min.index) / 4096;
		if (match_count == 0) {
			/* We need to fallback to other ways to move forward... */
			printf("%s: !!!!!!!!! TODO: Fallback should be implemented !!!!!!!!!!!\n", __FUNCTION__);
		} else if (match_count == 1 || match_min.count < joint_cost) {
			/* TODO: Should implement sa_bitsearch_file similar trick to reduce candidates */
			match_min.count = min(match_min.count, maximum_evaluations);
			candidates = _get_saindices(SAF, &match_min);
			printf("%s: only one valid match found with %d indices\n", __FUNCTION__, match_min.count);
		} else {
			/* Calculate which approach is better: walk match_min or join match_min and match_min2 */
			/* We are sure that both match_min and match_min2 do not exceed maximum_evaluations */
			candidates1 = _get_saindices(SAF, &match_min);
			candidates2 = _get_saindices(SAF, &match_min2);

			/* Although the match_min and match_min2 are binary searched ranges, adding
			 * offset from the start of the match make them not range anymore.
			 */
			candidates = _joint_array(candidates1, candidates2, 0);
			printf("%s: joint two potential matches %d and %d indices get %d indices\n",
					__FUNCTION__, candidates1->count, candidates2->count, candidates->count);

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
				if (candidates->indices[j] + length > Tsize) {
					/* not enough bytes to read, definitely not match*/
					continue;
				}
				fseek(TF, candidates->indices[j], SEEK_SET);
				fread(buf, sizeof(*buf), length, TF);
				if (mmtregex_match(preg, buf, length) == MMTREGEX_OK) {
					if (result_pos >= result_size) {
						/* not enough room to fill result */
						goto result_full;
					}
					result_array[result_pos++] = candidates->indices[j];
				}
			}
			delete_mmtregex(preg);

			printf("%s: walk through %d potential indices get %d indices\n",
					__FUNCTION__, candidates->count, result_pos - last_result_pos);

			delete_index_array(candidates);
		}

	}

result_full:
	return result_pos;
}

/**
 * @brief Grep for the exact bit-level string P in the file TF with support of suffix array.
 *
 * By now, only '0', '1' and '.' (which matches both '0' and '1') are supported
 *
 * @param TF the original file to be searched
 * @param Tsize the size of TF (do we need this?)
 * @Param P a bit level pattern which includes '0' and '1' only. (may extend to support regex)
 * @param Psize the size of array P, should > 16
 * @param SAF the sorted suffix array file
 * @param SAsize the size of SAF
 * @param[out] result_array the array storing the index of TF that matches query
 * @param[out] result_size the size of the result_array
 *
 * @return the number of actual matches
 *
 * @warning At most 512K (= 8*64K) potential matched strings are evaluated
 */
saidx_t
sa_bitgrep3_file(FILE *TF, saidx_t Tsize,
          const sauchar_t *P, saidx_t Psize,
          FILE *SAF, saidx_t SAsize,
          saidx_t *result_array, saidx_t result_size)
{
	const int minimum_bits = 16;
	int nextchar;
	saidx_t i, j;
	saidx_t idx, count, result_pos, tfidx, readsize, padding, matchlen;

	mmtregex preg;

	uint8_t regex_buf[4096*8], buf[4096];

	/* Sanity check */
	if (!P || Psize <= minimum_bits || Psize > sizeof(regex_buf) - 16) {
		printf("%s: Input parameter invalid.\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < Psize; i++) {
		if (P[i] != '0' && P[i] != '1' && P[i] != '.') {
			printf("%s: Input parameter invalid, can only be '0', '1', '.'\n", __FUNCTION__);
			return -1;
		}
	}

	/* Allocate resource for regex_buf and fill before and after with '.' */
	memcpy(regex_buf + 8, P, Psize);
	for (i = 0; i < 8; i++) {
		regex_buf[i] = '.';
		regex_buf[i + 8 + Psize] = '.';
	}

	result_pos = 0;

	for (i = 0; i < 8; i++) {
		/* Compile the regex */
		padding = (8 - ((Psize + i) % 8)) % 8;
//		printf("Psize = %d, Psize + i = %d, (Psize + i) %% 8 = %d, padding = %d\n",
//				Psize, Psize + i, (Psize + i) % 8, padding);
		preg = mmtregex_from_str(regex_buf + 8 - i, Psize + i + padding);
		if (!preg) {
			printf("%s: build regex failed\n", __FUNCTION__);
			return -1;
		}

		/* Determine the number of bytes need to read from file each time */
		readsize = mmtregex_get_maximum_length(preg);
		if (readsize > sizeof(buf)) {
			/* Too big pattern: are they really exist? */
			printf("%s: pattern is too big\n", __FUNCTION__);
			return -1;
		}

		/* Search in the file */
		for (idx = 0; idx < SAsize; ) {
			/* Retrive some data from source file for match */
			tfidx = _get_saidx(SAF, idx);
			fseek(TF, tfidx, SEEK_SET);
			fread(buf, 1, readsize, TF);
			printf("%s: read %d bytes from SA @%d(0x%X) raw @%X: ", __FUNCTION__, readsize, idx, idx, tfidx);
			for (j = 0; j < readsize; j++) {
				printf(" %X", buf[j]);
			}
			printf("\n");

			/* Try match */
			if (mmtregex_match(preg, buf, readsize) == MMTREGEX_OK) {
				/* buf[0..readsize-1] is the prefix that matches */
				count = sa_search_file(TF, Tsize, buf, readsize, SAF, SAsize, &idx);

				printf("%s: match found from %d(0x%X) with count %d\n", __FUNCTION__, idx, idx, count);

				/* read indices from file */
				fseek(SAF, idx * sizeof(idx), SEEK_SET);
				fread(result_array + result_pos, sizeof(idx), count, SAF);
				result_pos += count;
				idx += count;
			} else {
				/* buf[0..matchlen-1] is the prefix that matches
				 * buf[matchlen] is the character that not matches
				 */
				matchlen = mmtregex_get_matched_length(preg);
				nextchar = mmtregex_get_next_match_char(preg, matchlen, buf[matchlen]);

				if (nextchar > 0xFF) {
					/* No next match char, back one character */
					matchlen--;
					if (matchlen < 0 || matchlen >= readsize) {
						idx = SAsize;
						printf("%s: No valid string can be matched, set idx = SAsize\n", __FUNCTION__);
					} else {
						/* start from end of boundary */
						printf("%s: back one character and search for", __FUNCTION__);
						for (j = 0; j < matchlen + 1; j++) {
							printf(" %X", buf[j]);
						}
						count = sa_search_file(TF, Tsize, buf, matchlen + 1, SAF, SAsize, &idx);
						printf(", %s at %d(0x%X) with count %d, ", count > 0 ? "found" : "not found", idx, idx, count);
						idx += count;
						printf(", set idx to the end of boundary %d(0x%X)\n", idx, idx);
					}
				} else {
					buf[matchlen] = nextchar;

					/* Guess which is larger? nextchar or the next*/
					printf("%s: search for", __FUNCTION__);
					for (j = 0; j < matchlen + 1; j++) {
						printf(" %X", buf[j]);
					}
					count = sa_search_file(TF, Tsize, buf, matchlen + 1, SAF, SAsize, &idx);
					printf(", %s at %d(0x%X) with count %d\n", count > 0 ? "found" : "not found", idx, idx, count);
				}
			}
		}

		delete_mmtregex(preg);
	}

	return result_pos;
}


