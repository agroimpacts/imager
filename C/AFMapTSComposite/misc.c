#include <math.h>
#include "2d_array.h"
#include "const.h"
#include "misc.h"
#include "utilities.h"
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_fit.h>

/******************************************************************************
MODULE:  rmse_from_square_root_mean

PURPOSE:  simulate matlab calculate rmse from square root mean

RETURN VALUE:
Type = void
Value           Description
-----           -----------


HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
5/28/2019   Su Ye            Original Development

******************************************************************************/
void rmse_from_square_root_mean
(
    float **array,      /* I: input array */
    float fit_cft,      /* I: input fit_cft value */
    int dim1_index,     /* I: dimension 1 index in input array */
    int dim2_len,       /* I: dimension 2 length */
    float *rmse         /* O: output rmse */
)
{
    int i;
    float sum = 0.0;

    for (i = 0; i < dim2_len; i++)
    {
         sum += (array[dim1_index][i] - fit_cft) *
                (array[dim1_index][i] - fit_cft);
    }
    *rmse = sqrt(sum / dim2_len);
}

/******************************************************************************
MODULE:  dofit

PURPOSE: Declare data type and allocate memory and do multiple linear robust
         fit used for auto_robust_fit

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
5/28/2019   Su Ye            Original Development

NOTES:
******************************************************************************/
void dofit(const gsl_multifit_robust_type *T,
      const gsl_matrix *X, const gsl_vector *y,
      gsl_vector *c, gsl_matrix *cov)
{
  gsl_multifit_robust_workspace * work
    = gsl_multifit_robust_alloc (T, X->size1, X->size2);
  gsl_multifit_robust (X, y, c, cov, work);
  gsl_multifit_robust_free (work);
  work = NULL;
}

void dofit_linear(const gsl_matrix *X, const gsl_vector *y,
      gsl_vector *c, gsl_matrix *cov)
{
    double chisq;
    gsl_multifit_linear_workspace * work
        = gsl_multifit_linear_alloc ( X->size1, X->size2);
    gsl_multifit_linear(X, y, c, cov, &chisq,work);
    gsl_multifit_linear_free (work);
    // work = NULL; // SY 03242019
}

/******************************************************************************
MODULE:  auto_robust_fit

PURPOSE:  Robust fit for one band

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
5/28/2019   Su Ye            Original Development

NOTES:
******************************************************************************/
void auto_robust_fit
(
    float **clrx,
    float **clry,
    int nums,
    int start,
    int band_index,
    float *coefs
)
{
    int i, j;
    const int p = 2; /* linear fit */
    gsl_matrix *x, *cov;
    gsl_vector *y, *c;

    /******************************************************************/
    /*                                                                */
    /* Defines the inputs/outputs for robust fitting                  */
    /*                                                                */
    /******************************************************************/

    x = gsl_matrix_alloc (nums, p);
    y = gsl_vector_alloc (nums);

    c = gsl_vector_alloc (p);
    cov = gsl_matrix_alloc (p, p);

    /******************************************************************/
    /*                                                                */
    /* construct design matrix x for linear fit                       */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < nums; ++i)
    {
        for (j = 0; j < p; j++)
        {
            if (j == 0)
            {
                gsl_matrix_set (x, i, j, 1.0);
            }
            else
            {
                gsl_matrix_set (x, i, j, (double)clrx[i][j-1]);
            }
        }
        gsl_vector_set(y,i,(double)clry[band_index][i+start]);
    }

    /******************************************************************/
    /*                                                                */
    /* perform robust fit                                             */
    /*                                                                */
    /******************************************************************/

    dofit(gsl_multifit_robust_bisquare, x, y, c, cov);

    for (j = 0; j < (int)c->size; j++)
    {
        coefs[j] = gsl_vector_get(c, j);
    }

    /******************************************************************/
    /*                                                                */
    /* Free the memories                                              */
    /*                                                                */
    /******************************************************************/

    gsl_matrix_free (x);
    x = NULL;
    gsl_vector_free (y);
    y = NULL;
    gsl_vector_free (c);
    c = NULL;
    gsl_matrix_free (cov);
    cov = NULL;
}


/******************************************************************************
MODULE:  auto_mask

PURPOSE:  Multitemporal cloud, cloud shadow, & snow masks (global version)

RETURN VALUE:
Type = int
ERROR error out due to memory allocation
SUCCESS no error encounted

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05282019    Su Ye

NOTES:
******************************************************************************/
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
)
{
    char FUNC_NAME[] = "auto_mask";
    int i;
    float **x;
    float pred_b2, pred_b4;
    int nums;
    float coefs[ROBUST_COEFFS];
    float coefs2[ROBUST_COEFFS];

    nums = end - start + 1;
    /* Allocate memory */
    x = (float **)allocate_2d_array(nums, ROBUST_COEFFS - 1, sizeof(float));
    if (x == NULL)
    {
        RETURN_ERROR("ERROR allocating x memory", FUNC_NAME, ERROR);
    }

    for (i = 0; i < nums; i++)
    {
        x[i][0] = (float)clrx[i+start];
    }

    /******************************************************************/
    /*                                                                */
    /* Do robust fitting for band 2 */
    /*                                                                */
    /******************************************************************/

    auto_robust_fit(x, clry, nums, start, 1, coefs);

    /******************************************************************/
    /*                                                                */
    /* Do robust fitting for band 4 */
    /*                                                                */
    /******************************************************************/

    auto_robust_fit(x, clry, nums, start, 3, coefs2);

    /******************************************************************/
    /*                                                                */
    /* predict band 2 and band 4 refs, bl_ids value of 0 is clear and   */
    /* 1 otherwise                                                    */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < nums; i++)
    {
        pred_b2 = coefs[0] + coefs[1] * (float)clrx[i+start];
        pred_b4 = coefs2[0] + coefs2[1] * (float)clrx[i+start];
        if (((clry[1][i+start]-pred_b2) > (n_t * t_b1)) ||
            ((clry[3][i+start]-pred_b4) < -(n_t * t_b2)))
        {
            bl_ids[i] = 1;
        }
        else
        {
            bl_ids[i] = 0;
        }
    }

    /* Free allocated memory */
    if (free_2d_array ((void **) x) != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: x\n", FUNC_NAME, ERROR);
    }

    return (SUCCESS);
}

/******************************************************************************
MODULE:  adjust_median_variogram

PURPOSE:  calculate absolute variogram for auto_mask

RETURN VALUE:
Type = void
Value           Description
-----           -----------


HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
28/05/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
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
)
{
    int i, j;           /* loop indecies                                     */
    float *var;         /* pointer for allocation variable memory            */
    int dim2_len = dim2_end - dim2_start + 1; /* perhaps should get defined  */
    char FUNC_NAME[] = "adjust_median_variogram"; /* for error messages             */
    int max_freq;
    int m;

//    for (i = 0; i < dim2_len; i++)
//    {
//        for (j = 0; j < TOTAL_IMAGE_BANDS; j++)
//        {
//           printf("%f\n", (float)array[j][i]);
//        }

//    }

    if (dim2_len == 1)
    {
        for (i = 0; i < dim1_len; i++)
        {
            output_array[i] = array[i][dim2_start];
            *date_vario = clrx[dim2_start];
            return (SUCCESS);
        }
    }

    var = malloc((dim2_len-1) * sizeof(float));
    if (var == NULL)
    {
        RETURN_ERROR ("Allocating var memory", FUNC_NAME, ERROR);
    }

    for (j = dim2_start; j < dim2_end; j++)
    {
        var[j] = abs(clrx[j+1] - clrx[j]);
    }
    quick_sort_float(var, dim2_start, dim2_end-1);
    m = (dim2_len-1) / 2;
    if ((dim2_len-1) % 2 == 0)
    {

        *date_vario = (var[m-1] + var[m]) / 2.0;
    }
    else
        *date_vario = var[m];

    *max_neighdate_diff = var[dim2_len-2];



    for (i = 0; i < dim1_len; i++)
    {
        for (j = dim2_start; j < dim2_end; j++)
        {
            var[j] = abs(array[i][j+1] - array[i][j]);
            //printf("%d var for band %d: %f\n", j, i+1, (float)var[j]);

        }
        quick_sort_float(var, dim2_start, dim2_end-1);
//            for (j = 0; j < dim2_end; j++)
//            {
//               printf("%f\n", var[j]);
//            }
        m = (dim2_len-1) / 2;
        if ((dim2_len-1) % 2 == 0)
        {
            //printf("%f\n", var[m-1]);
           //printf("%f\n", var[m]);
            output_array[i] = (var[m-1] + var[m]) / 2.0;
        }
        else
            output_array[i] = var[m];

    }

    free(var);


    return (SUCCESS);
}


/******************************************************************************
 *
MODULE:  single_median_variogram
PURPOSE:  calculate absolute variogram for a single band

RETURN VALUE:
Type = void
Value           Description
-----           -----------


HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
22/06/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
int single_median_variogram
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    short int *variogram,
    short int *mediam_value          /* O: outputted mediam value                         */
)
{
    int j, m;           /* loop indecies                                     */
    char FUNC_NAME[] = "single_median_variogram"; /* for error messages             */
    short int *var;         /* pointer for allocation variable memory            */
    int obs_num;
    int var_count = 0;
    short int *array_cpy;

    obs_num = i_end - i_start + 1;

//    for (j = 0; j < obs_num; j++)
//    {
//        printf("%i\n", array[j]);

//    }

    if (obs_num == 1)
    {
        *variogram = 0;
        *mediam_value = array[0];
        return (SUCCESS);

    }

    var = malloc((obs_num-1) * sizeof(short int));
    if (var == NULL)
    {
        RETURN_ERROR ("Allocating var memory", FUNC_NAME, ERROR);
    }

    array_cpy = malloc(obs_num * sizeof(short int));
    if (array_cpy == NULL)
    {
        RETURN_ERROR ("Allocating array_cpy memory", FUNC_NAME, ERROR);
    }


    for (j = i_start; j < i_end; j++)
    {
        var[j] = (short int)abs(array[j+1] - array[j]);
        //printf("%d var for band %d: %f\n", j, i+1, (float)var[j]);

    }

    for (j = i_start; j < i_end+1; j++)
    {
        array_cpy[j] = array[j];
        //printf("%d var for band %d: %f\n", j, i+1, (float)var[j]);

    }
    quick_sort_shortint(var, 0, obs_num-2);
//            for (j = 0; j < obs_num-1; j++)
//            {
//               printf("%d\n", var[j]);
//            }

    /* compute variogram */
    m = (obs_num-1) / 2;
    if ((obs_num-1) % 2 == 0)
    {
        *variogram = (short int) ((var[m-1] + var[m]) / 2.0);
    }
    else
        *variogram = var[m];


    /* compute mediam value */
    quick_sort_shortint(array_cpy, i_start, i_end);
//    for (j = 0; j < obs_num; j++)
//    {
//       printf("%d\n", array_cpy[j]);
//    }

    m = obs_num / 2;
    if (obs_num % 2 == 0)
    {
        *mediam_value = (short int)((array_cpy[m-1] + array_cpy[m]) / 2.0);
    }
    else
        *mediam_value = array_cpy[m];


    free(var);
    free(array_cpy);

    return (SUCCESS);
}


/******************************************************************************
 *
MODULE:  single_median_variogram
PURPOSE:  calculate absolute variogram for a single band

RETURN VALUE:
Type = void
Value           Description
-----           -----------


HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
22/06/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
int single_median_quantile
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    short int *quantile,          /* O: outputted quantile value                         */
    short int *mediam_value
)
{
    int j, m;           /* loop indecies                                     */
    char FUNC_NAME[] = "single_median_quantile"; /* for error messages             */
    short int *var;         /* pointer for allocation variable memory            */
    int obs_num;
    int var_count = 0;
    short int *array_cpy;

    obs_num = i_end - i_start + 1;

//    for (j = 0; j < obs_num; j++)
//    {
//        printf("%i\n", array[j]);

//    }

    if (obs_num == 1)
    {
        *mediam_value = array[0];
        *quantile = array[0];
        return (SUCCESS);

    }

    var = malloc((obs_num-1) * sizeof(short int));
    if (var == NULL)
    {
        RETURN_ERROR ("Allocating var memory", FUNC_NAME, ERROR);
    }

    array_cpy = malloc(obs_num * sizeof(short int));
    if (array_cpy == NULL)
    {
        RETURN_ERROR ("Allocating array_cpy memory", FUNC_NAME, ERROR);
    }




    for (j = i_start; j < i_end+1; j++)
    {
        array_cpy[j] = array[j];
        //printf("%d var for band %d: %f\n", j, i+1, (float)var[j]);

    }


    /* compute mediam value */
    quick_sort_shortint(array_cpy, i_start, i_end);
//    for (j = 0; j < obs_num; j++)
//    {
//       printf("%d\n", array_cpy[j]);
//    }

    m = obs_num / 2;
    if (obs_num % 2 == 0)
    {
        *mediam_value = (short int)((array_cpy[m-1] + array_cpy[m]) / 2.0);
    }
    else
        *mediam_value = array_cpy[m];

    *quantile = array_cpy[obs_num / 4];

    free(var);
    free(array_cpy);

    return (SUCCESS);
}
/******************************************************************************
 *
MODULE:  single_mean_rmse
PURPOSE:  calculate absolute variogram for a single band

RETURN VALUE:
Type = void
Value           Description
-----           -----------


HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
22/06/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
int single_mean_rmse
(
    short int *array,              /* I: input array                                    */
    int i_start,
    int i_end,
    float *rmse,
    float *mean          /* O: outputted mediam value                         */
)
{
    int j, m;           /* loop indecies                                     */
    char FUNC_NAME[] = "single_mean_rse"; /* for error messages             */
    short int *var;         /* pointer for allocation variable memory            */
    int obs_num;
    int sum = 0;
    int rmse_sum = 0;

    obs_num = i_end - i_start + 1;

//    for (j = 0; j < obs_num; j++)
//    {
//        printf("%i\n", array[j]);

//    }

    if (obs_num == 1)
    {
        *rmse = 0;
        *mean = array[0];
        return (SUCCESS);

    }

    var = malloc((obs_num-1) * sizeof(short int));
    if (var == NULL)
    {
        RETURN_ERROR ("Allocating var memory", FUNC_NAME, ERROR);
    }


    for (j = i_start; j < i_end + 1; j++)
    {
        sum = sum + (int)array[j];
    }

    *mean = (float)((float)sum / (float)obs_num);

    for (j = i_start; j < i_end + 1; j++)
    {
        rmse_sum = rmse_sum + (array[j] - *mean) * (array[j] - *mean);
    }

    *rmse = (float)sqrt((double)rmse_sum/(double)obs_num);
    return (SUCCESS);
}


/******************************************************************************
MODULE:  linear_fit_centerdate

PURPOSE:  Robust fit for one band

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
5/31/2019   Su Ye            Original Development

NOTES:
******************************************************************************/
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
    float *C0,
    float *C1
)
{
    char FUNC_NAME[] = "linear_fit_centerdate";
    double c0;
    double c1;
    double cov00;
    double cov01;
    double cov11;
    double sumsq;
    int i;
    double* x;
    double* y_b1;
    double* y_b2;
    double* y_b3;
    double* y_b4;
    double* w;
    float coefs[ROBUST_COEFFS];
    float** x_t;


    x = (double*)malloc(nums * sizeof(double));
    if (x == NULL)
    {
        RETURN_ERROR("ERROR allocating x memory", FUNC_NAME, FAILURE);
    }

    y_b1 = (double*)malloc(nums * sizeof(double));
    if (y_b1 == NULL)
    {
        RETURN_ERROR("ERROR allocating y_b1 memory", FUNC_NAME, FAILURE);
    }

    y_b2 = (double*)malloc(nums * sizeof(double));
    if (y_b2 == NULL)
    {
        RETURN_ERROR("ERROR allocating y_b2 memory", FUNC_NAME, FAILURE);
    }

    y_b3 = (double*)malloc(nums * sizeof(double));
    if (y_b3 == NULL)
    {
        RETURN_ERROR("ERROR allocating y_b3 memory", FUNC_NAME, FAILURE);
    }

    y_b4 = (double*)malloc(nums * sizeof(double));
    if (y_b4 == NULL)
    {
        RETURN_ERROR("ERROR allocating y_b4 memory", FUNC_NAME, FAILURE);
    }

    w = (double*)malloc(nums * sizeof(double));
    if (w == NULL)
    {
        RETURN_ERROR("ERROR allocating y_b4 memory", FUNC_NAME, FAILURE);
    }

    x_t = (float **)allocate_2d_array(nums, ROBUST_COEFFS - 1, sizeof(float));
    if (x_t == NULL)
    {
        RETURN_ERROR("ERROR allocating x_t memory", FUNC_NAME, ERROR);
    }

    for (i = 0; i < nums; i++)
    {
        x_t[i][0] = (float)clrx[i+start];
    }


    /******************************************************************/
    /*                                                                */
    /* Defines the inputs/outputs for robust fitting                  */
    /*                                                                */
    /******************************************************************/




    /******************************************************************/
    /*                                                                */
    /* construct design matrix x for linear fit                       */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < nums; ++i)
    {
        x[i] = (double)clrx[i];
        y_b1[i] = (double)clry[0][i];
        y_b2[i] = (double)clry[1][i];
        y_b3[i] = (double)clry[2][i];
        y_b4[i] = (double)clry[3][i];

    }

    /******************************************************************/
    /*                                                                */
    /* perform robust fit                                             */
    /*                                                                */
    /******************************************************************/

    if (bweighted == TRUE)
    {
        /* create hot weights */
        for (i = 0; i < nums; ++i)
        {
             w[i] = (double)1/(abs(y_b1[i] - 0.5 * y_b3[i]));
        }

        gsl_fit_wlinear (x, 1, w, 1, y_b1, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
        composites[0][i_col] = (short int)(c0 + c1 * center_date);
        C0[0] = (float)c0;
        C1[0] = (float)c1;

        gsl_fit_wlinear (x, 1, w, 1, y_b2, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
        composites[1][i_col] = (short int)(c0 + c1 * center_date);
        C0[1] = (float)c0;
        C1[1] = (float)c1;

        gsl_fit_wlinear (x, 1, w, 1, y_b3, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
        composites[2][i_col] = (short int)(c0 + c1 * center_date);
        C0[2] = (float)c0;
        C1[2] = (float)c1;

        gsl_fit_wlinear (x, 1, w, 1, y_b4, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
        composites[3][i_col] = (short int)(c0 + c1 * center_date);
        C0[3] = (float)c0;
        C1[3] = (float)c1;
    }
    else
    {
        /* OLS*/
//        gsl_fit_linear (x, 1, y_b1, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
//        composites[0][i_col] = (short int)(c0 + c1 * center_date);
//        C0[0] = (float)c0;
//        C1[0] = (float)c1;

//        gsl_fit_linear (x, 1, y_b2, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
//        composites[1][i_col] = (short int)(c0 + c1 * center_date);
//        C0[1] = (float)c0;
//        C1[1] = (float)c1;

//        gsl_fit_linear (x, 1, y_b3, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
//        composites[2][i_col] = (short int)(c0 + c1 * center_date);
//        C0[2] = (float)c0;
//        C1[2] = (float)c1;

//        gsl_fit_linear (x, 1, y_b4, 1, nums, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
//        composites[3][i_col] = (short int)(c0 + c1 * center_date);
//        C0[3] = (float)c0;
//        C1[3] = (float)c1;
        /* robust regression */
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            auto_robust_fit(x_t, clry, nums, start, i, coefs);
            composites[i][i_col] = (short int)(coefs[0] + coefs[1] * center_date);
            C0[i] = (float)coefs[0];
            C1[i] = (float)coefs[1];

        }
    }






    /******************************************************************/
    /*                                                                */
    /* Free the memories                                              */
    /*                                                                */
    /******************************************************************/

    free (x);
    free (y_b1);
    free (y_b2);
    free (y_b3);
    free (y_b4);
    free (w);
    /* Free allocated memory */
    if (free_2d_array ((void **) x_t) != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: x_t\n", FUNC_NAME, ERROR);
    }

}

/******************************************************************************
MODULE:  median_filter

PURPOSE:  calculate mediam filtering for three buf scanline

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
6/12/2019   Su Ye            Original Development

NOTES:
******************************************************************************/
int median_filter
(
    short int **buf1,
    short int **buf2,
    short int **buf3,
    short int **buf,
    int *valid_datecount_scanline,       /* I: the number of valid dates               */
    int num_scenes,
    int n_cols
)
{
    int i, b, j;
    short int window[9];

    for(i = 0; i < n_cols; i++)
    {
        for(j = 0; j < valid_datecount_scanline[i]; j++)
        {
            if(i==0)
            {
                for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
                {
                    window[0] = buf1[b][i * num_scenes + j];
                    window[1] = buf1[b][i * num_scenes + j];
                    window[2] = buf1[b][(i+1) * num_scenes + j];
                    window[3] = buf2[b][i * num_scenes + j];
                    window[4] = buf2[b][i * num_scenes + j];
                    window[5] = buf2[b][(i+1) * num_scenes + j];
                    window[6] = buf3[b][i * num_scenes + j];
                    window[7] = buf3[b][i * num_scenes + j];
                    window[8] = buf3[b][(i+1) * num_scenes + j];

                    quick_sort_shortint(window, 0, 8);

                    buf[b][i * num_scenes + j] = window[4];

                }
            }
            else if (i == n_cols - 1)
            {
                for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
                {
                    window[0] = buf1[b][(i-1) * num_scenes + j];
                    window[1] = buf1[b][i * num_scenes + j];
                    window[2] = buf1[b][i * num_scenes + j];
                    window[3] = buf2[b][(i-1) * num_scenes + j];
                    window[4] = buf2[b][i * num_scenes + j];
                    window[5] = buf2[b][i * num_scenes + j];
                    window[6] = buf3[b][(i-1) * num_scenes + j];
                    window[7] = buf3[b][i * num_scenes + j];
                    window[8] = buf3[b][i * num_scenes + j];

                    quick_sort_shortint(window, 0, 8);

                    buf[b][i * num_scenes + j] = window[4];

                }
            }
            else
            {
                for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
                {
                    window[0] = buf1[b][(i-1) * num_scenes + j];
                    window[1] = buf1[b][i * num_scenes + j];
                    window[2] = buf1[b][(i+1) * num_scenes + j];
                    window[3] = buf2[b][(i-1) * num_scenes + j];
                    window[4] = buf2[b][i * num_scenes + j];
                    window[5] = buf2[b][(i+1) * num_scenes + j];
                    window[6] = buf3[b][(i-1) * num_scenes + j];
                    window[7] = buf3[b][i * num_scenes + j];
                    window[8] = buf3[b][(i+1) * num_scenes + j];

                    quick_sort_shortint(window, 0, 8);

                    buf[b][i * num_scenes + j] = window[4];

                }
            }

        }

    }


}
