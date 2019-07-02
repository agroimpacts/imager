#ifndef COMPOSITING_H
#define COMPOSITING_H
#include "stdbool.h"

int hot_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);

int average_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);

int compositing_scanline
(
    short int **buf,            /* I:  scanline-based time series           */
    int **valid_datearray_scanline,      /* I: valid date time series               */
    int *valid_datecount_scanline,       /* I: the number of valid dates               */
    int lower_ordinal,                /* I: center date               */
    int upper_ordinal,                   /* I: interval for temporal range of composition   */
    int num_samples,                /* I: the pixel number in a row              */
    int num_scenes,                 /* I: the number of scenes               */
    short int **out_compositing,           /* O: outputted compositing results for four bands */
    int method                       /* I: the compositing method{1 - fitting-weighted; 2 - fitting-normal; 3 - hot; 4 - average}*/
);

//int fitting_compositing_scanline
//(
//    short int **buf,            /* I:  scanline-based time series           */
//    int **valid_datearray_scanline,      /* I: valid date time series               */
//    int *valid_datecount_scanline,       /* I: the number of valid dates               */
//    int center_date,                /* I: center date               */
//    int interval,                   /* I: interval for temporal range of composition   */
//    int num_samples,                /* I: the pixel number in a row              */
//    int num_scenes,                 /* I: the number of scenes               */
//    short int **out_compositing           /* O: outputted compositing results for four bands */
//);

int greenband_test
(
    int *clrx,
    float **clry,
    int start,
    int end,
    float rmse,
    float n_t,
    int *bl_ids,
    float *C0,
    float *C1
);

int nirband_test
(
    int *clrx,
    float **clry,
    int start,
    int end,
    float rmse,
    float n_t,
    int *bl_ids,
    float *C0,
    float *C1
);

int fitting_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing,           /* O: outputted compositing results for four bands */
    int bfit,
    int bweighted,
    int b_diagnosis,
    Output_t* rec_c
);

int median_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);


int modified_hot_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int ordinal_lower,
    int ordinal_upper,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);

int medium_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);

int valid_obs_count
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
);

#endif // COMPOSITING_H
