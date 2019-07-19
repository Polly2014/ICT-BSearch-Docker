/*
 * sasearch.c for libdivsufsort
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


static
void
print_help(const char *progname, int status) {
  fprintf(stderr,
          "sasearch, a simple SA-based full-text search tool, version %s\n",
          divsufsort_version());
  fprintf(stderr, "usage: %s PATTERN FILE SAFILE\n\n", progname);
  exit(status);
}

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

/*
 * @param argv[1] the pattern to be matched
 * @param argv[2] the file to be searched
 * @param argv[3] the suffix array file, which is built by from argv[2]
 */
int
main(int argc, const char *argv[]) {
  FILE *SAF;		/*<< The file for suffix array */
  FILE *TF;	/*<< The file to be searched */
  const char *P;
  LFS_OFF_T n;		/*<< length of SAF */
  LFS_OFF_T n_suf;	/*<< length of TF */
  size_t Psize;
  const int result_size = 65536;
  saidx_t i, size, result[result_size];
  clock_t start, finish;

  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS); }
  if(argc != 4) { print_help(argv[0], EXIT_FAILURE); }

  P = argv[1];
  Psize = strlen(P);

  /* Open the raw file */
#if HAVE_FOPEN_S
  if(fopen_s(&TF, argv[2], "rb") != 0) {
#else
  if((TF = LFS_FOPEN(argv[2], "rb")) == NULL) {
#endif
    fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], argv[2]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Get the file size. */
  if(LFS_FSEEK(TF, 0, SEEK_END) == 0) {
    n = LFS_FTELL(TF);
    rewind(TF);
    if(n < 0) {
      fprintf(stderr, "%s: Cannot ftell `%s': ", argv[0], argv[2]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "%s: Cannot fseek `%s': ", argv[0], argv[2]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }


  /* Open the suffix file */
#if HAVE_FOPEN_S
  if(fopen_s(&SAF, argv[3], "rb") != 0) {
#else
  if((SAF = LFS_FOPEN(argv[3], "rb")) == NULL) {
#endif
    fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], argv[2]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Get the file size. */
  if(LFS_FSEEK(SAF, 0, SEEK_END) == 0) {
    n_suf = LFS_FTELL(SAF);
    rewind(SAF);
    if(n_suf < 0) {
      fprintf(stderr, "%s: Cannot ftell `%s': ", argv[0], argv[3]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
	if (n_suf != n * 4){
	  fprintf(stderr, "%s: suffix array file %s is not correct", argv[0], argv[3]);
      exit(EXIT_FAILURE);
	}
  } else {
    fprintf(stderr, "%s: Cannot fseek `%s': ", argv[0], argv[3]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Search and print */
  start = clock();
  //for (i = 0; i < 1000; i++)
  size = sa_bitsearch_file(TF, (saidx_t)n,
                   (const sauchar_t *)P, (saidx_t)Psize,
                   SAF, (saidx_t)n,
  	  	  	  	   result, result_size);
  if (size > 0)
	  qsort(result, size, sizeof(*result), compare_saidx);
  finish = clock();
  for(i = 0; i < size; ++i) {
    fprintf(stdout, "0x%x\n", result[i]);
  }
  fprintf(stderr, "%.3f ms or %d CPU ticks\n", 1000 * (double)(finish - start) / (double)CLOCKS_PER_SEC, (int)(finish - start));

  /* Deallocate memory. */
  fclose(SAF);
  fclose(TF);

  return 0;
}
