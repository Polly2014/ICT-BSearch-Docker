
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
