/*****************************************************************************
     S T A T I S T I C A L  T E S T  F U N C T I O N  P R O T O T Y P E S
 *****************************************************************************/

/*
 * This code has been heavily modified by the following people:
 *
 *      Landon Curt Noll
 *      Tom Gilgan
 *      Riccardo Paccagnella
 *
 * See the README.md and the initial comment in sts.c for more information.
 *
 * WE (THOSE LISTED ABOVE WHO HEAVILY MODIFIED THIS CODE) DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL WE (THOSE LISTED ABOVE
 * WHO HEAVILY MODIFIED THIS CODE) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * chongo (Landon Curt Noll, http://www.isthe.com/chongo/index.html) /\oo/\
 *
 * Share and enjoy! :-)
 */

#ifndef STAT_FNCS_H
#   define STAT_FNCS_H

/*
 * Initialize functions
 */
extern void Frequency_init(struct state *state);
extern void BlockFrequency_init(struct state *state);
extern void CumulativeSums_init(struct state *state);
extern void Runs_init(struct state *state);
extern void LongestRunOfOnes_init(struct state *state);
extern void Rank_init(struct state *state);
extern void DiscreteFourierTransform_init(struct state *state);
extern void NonOverlappingTemplateMatchings_init(struct state *state);
extern void OverlappingTemplateMatchings_init(struct state *state);
extern void Universal_init(struct state *state);
extern void ApproximateEntropy_init(struct state *state);
extern void RandomExcursions_init(struct state *state);
extern void RandomExcursionsVariant_init(struct state *state);
extern void LinearComplexity_init(struct state *state);
extern void Serial_init(struct state *state);

/*
 * iterate functions
 */
extern void Frequency_iterate(struct state *state);
extern void BlockFrequency_iterate(struct state *state);
extern void CumulativeSums_iterate(struct state *state);
extern void Runs_iterate(struct state *state);
extern void LongestRunOfOnes_iterate(struct state *state);
extern void Rank_iterate(struct state *state);
extern void DiscreteFourierTransform_iterate(struct state *state);
extern void NonOverlappingTemplateMatchings_iterate(struct state *state);
extern void OverlappingTemplateMatchings_iterate(struct state *state);
extern void Universal_iterate(struct state *state);
extern void ApproximateEntropy_iterate(struct state *state);
extern void RandomExcursions_iterate(struct state *state);
extern void RandomExcursionsVariant_iterate(struct state *state);
extern void LinearComplexity_iterate(struct state *state);
extern void Serial_iterate(struct state *state);

/*
 * Print functions
 */
extern void Frequency_print(struct state *state);
extern void BlockFrequency_print(struct state *state);
extern void CumulativeSums_print(struct state *state);
extern void Runs_print(struct state *state);
extern void LongestRunOfOnes_print(struct state *state);
extern void Rank_print(struct state *state);
extern void DiscreteFourierTransform_print(struct state *state);
extern void NonOverlappingTemplateMatchings_print(struct state *state);
extern void OverlappingTemplateMatchings_print(struct state *state);
extern void Universal_print(struct state *state);
extern void ApproximateEntropy_print(struct state *state);
extern void RandomExcursions_print(struct state *state);
extern void RandomExcursionsVariant_print(struct state *state);
extern void LinearComplexity_print(struct state *state);
extern void Serial_print(struct state *state);

/*
 * Compute metrics functions
 */
extern void Frequency_metrics(struct state *state);
extern void BlockFrequency_metrics(struct state *state);
extern void CumulativeSums_metrics(struct state *state);
extern void Runs_metrics(struct state *state);
extern void LongestRunOfOnes_metrics(struct state *state);
extern void Rank_metrics(struct state *state);
extern void DiscreteFourierTransform_metrics(struct state *state);
extern void NonOverlappingTemplateMatchings_metrics(struct state *state);
extern void OverlappingTemplateMatchings_metrics(struct state *state);
extern void Universal_metrics(struct state *state);
extern void ApproximateEntropy_metrics(struct state *state);
extern void RandomExcursions_metrics(struct state *state);
extern void RandomExcursionsVariant_metrics(struct state *state);
extern void LinearComplexity_metrics(struct state *state);
extern void Serial_metrics(struct state *state);

/*
 * destroy functions
 */
extern void Frequency_destroy(struct state *state);
extern void BlockFrequency_destroy(struct state *state);
extern void CumulativeSums_destroy(struct state *state);
extern void Runs_destroy(struct state *state);
extern void LongestRunOfOnes_destroy(struct state *state);
extern void Rank_destroy(struct state *state);
extern void DiscreteFourierTransform_destroy(struct state *state);
extern void NonOverlappingTemplateMatchings_destroy(struct state *state);
extern void OverlappingTemplateMatchings_destroy(struct state *state);
extern void Universal_destroy(struct state *state);
extern void ApproximateEntropy_destroy(struct state *state);
extern void RandomExcursions_destroy(struct state *state);
extern void RandomExcursionsVariant_destroy(struct state *state);
extern void LinearComplexity_destroy(struct state *state);
extern void Serial_destroy(struct state *state);

#endif				/* STAT_FNCS_H */
