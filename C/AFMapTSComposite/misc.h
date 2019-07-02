#ifndef MISC_H
#define MISC_H
#include "const.h"

/* Output structure for fitting compositing approach */
typedef struct
{
    float C0_green;             /* intercept for green-band test  */
    float C1_green;             /* slope for green-band test    */
    float C0_nir;             /* intercept for green-band test  */
    float C1_nir;             /* slope for green-band test    */
    float C0_final[TOTAL_IMAGE_BANDS];             /* intercept for green-band test  */
    float C1_final[TOTAL_IMAGE_BANDS];             /* slope for green-band test    */
    int outlier_dates_green[MAX_NUM_OUTLIERS];
    int outlier_dates_nir[MAX_NUM_OUTLIERS];
    int b_success_green;      /* 0-success; 1-fail */
    int b_success_nir;        /* 0-success; 1-fail */
    int condition;            /* 0-normal; 1-no valid obs; 2-obs less than MIN_SAMPLE*/
    int n_outlier_green;
    int n_outlier_nir;
} Output_t;

int auto_mask
(
    int *clrx,
    float **clry,
    int start,
    int end,
    float years,
    float t_b1,
    float t_b2,
    float n_t,
    int *bl_ids
);

int adjust_median_variogram
(
    int *clrx,                  /* I: dates                                          */
    float **array,              /* I: input array                                    */
    int dim1_len,               /* I: dimension 1 length in input array              */
    int dim2_start,             /* I: dimension 2 start index                        */
    int dim2_end,               /* I: dimension 2 end index                          */
    float *date_vario,          /* O: outputted median variogran for dates           */
    float *max_neighdate_diff,  /* O: maximum difference for two neighbor times       */
    float *output_array        /* O: output array                                   */
);

void auto_robust_fit
(
    float **clrx,
    float **clry,
    int nums,
    int start,
    int band_index,
    float *coefs
);

void linear_fit_centerdate
(
    int *clrx,
    float **clry,
    int nums,
    int start,
    int center_date,
    int i_col,
    short int **composites,
    int bweighted,
    float* C0,
    float* C1
);

int median_filter
(
    short int **buf1,
    short int **buf2,
    short int **buf3,
    short int **buf,
    int *valid_datecount_scanline,       /* I: the number of valid dates               */
    int num_scenes,
    int n_cols
);

int single_median_quantile
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    short int *quantile,          /* O: outputted quantile value                         */
    short int *mediam_value
);

int single_median_variogram
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    short int *variogram,
    short int *mediam_value          /* O: outputted mediam value                         */
);

int single_mean_rmse
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    float *rmse,
    float *mean          /* O: outputted mediam value                         */
);
#endif // MISH.h
