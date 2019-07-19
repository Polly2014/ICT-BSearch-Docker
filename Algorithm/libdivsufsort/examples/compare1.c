
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
