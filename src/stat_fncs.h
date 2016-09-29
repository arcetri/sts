/*****************************************************************************
     S T A T I S T I C A L  T E S T  F U N C T I O N  P R O T O T Y P E S
 *****************************************************************************/

#ifndef STAT_FNCS_H
#   define STAT_FNCS_H

/*
 * initialize functions
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
 * interate functions
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

#endif				/* STAT_FNCS_H */
