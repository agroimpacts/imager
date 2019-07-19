#include <gsl/gsl_multifit.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include "2d_array.h"
#include "const.h"
#include "input.h"
#include "utilities.h"
#include "misc.h"

/******************************************************************************
MODULE:  greenband_test

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
)
{
    char FUNC_NAME[] = "greenband_test";
    int i;
    float **x;
    float pred;
    int nums;
    float coefs[ROBUST_COEFFS];

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

    *C0 = coefs[0];
    *C1 = coefs[1];

    /******************************************************************/
    /*                                                                */
    /* predict band 2 and band 4 refs, bl_ids value of 0 is clear and   */
    /* 1 otherwise                                                    */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < nums; i++)
    {
        pred = coefs[0] + coefs[1] * (float)clrx[i+start];
        if (clry[1][i+start]-pred > (n_t * rmse))
        {
            // int testy = clry[1][i+start];
            // int testx = clrx[i+start];
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
    x = NULL;

    return (SUCCESS);
}

/******************************************************************************
MODULE:  nirband_test

PURPOSE:  nirband_test used to filter out shadow

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
)
{
    char FUNC_NAME[] = "nirband_test";
    int i;
    float **x;
    float pred;
    int nums;
    float coefs[ROBUST_COEFFS];

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
    /* Do robust fitting for band 4 */
    /*                                                                */
    /******************************************************************/

    auto_robust_fit(x, clry, nums, start, 3, coefs);

    *C0 = coefs[0];
    *C1 = coefs[1];

    /******************************************************************/
    /*                                                                */
    /* predict band 2 and band 4 refs, bl_ids value of 0 is clear and   */
    /* 1 otherwise                                                    */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < nums; i++)
    {
        pred = coefs[0] + coefs[1] * (float)clrx[i+start];
        if (clry[3][i+start]-pred < -(n_t * rmse))
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
    x = NULL;

    return (SUCCESS);
}

/******************************************************************************
MODULE:  average_compositing

PURPOSE:  Running average compositing for pixel-based time series

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/02/2019   Su Ye         Original Development
******************************************************************************/

int average_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    int i, j;
    double index_sum[TOTAL_IMAGE_BANDS];
    int valid_count_window= 0;
    for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        index_sum[i] = 0;


    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                index_sum[j] = index_sum[j] +  buf[j][i];
            }
            valid_count_window++;

        }

    }

    if(valid_count_window==0)
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            out_compositing[i][i_col] = -9999;
        }
        return SUCCESS;
    }


    for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
    {
        out_compositing[j][i_col] = (short int)(index_sum[j] / valid_count_window);
    }
}

/******************************************************************************
MODULE:  average_compositing

PURPOSE:  Running average compositing for pixel-based time series

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/03/2019   Su Ye         Original Development
******************************************************************************/

int median_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    char FUNC_NAME[] = "median_compositing";
    short int *var;         /* pointer for allocation variable memory            */
    int i, j, m;

    if (valid_date_count == 1)
    {
        for (i = 0; i < TOTAL_IMAGE_BANDS; i++)
          out_compositing[i][i_col] = buf[i][0];
        return SUCCESS;
    }

    var = malloc(valid_date_count * sizeof(short int));
    if (var == NULL)
    {
        RETURN_ERROR ("Allocating var memory", FUNC_NAME, ERROR);
    }

    for (i = 0; i < TOTAL_IMAGE_BANDS; i++)
    {
        for (j = 0; j < valid_date_count; j++)
        {
            var[j] = buf[i][j];

        }
        quick_sort_float(var, 0, valid_date_count-1);
//            for (j = 0; j < dim2_end; j++)
//            {
//               printf("%f\n", var[j]);
//            }
        m = (valid_date_count) / 2;
        if (valid_date_count % 2 == 0)
        {
            //printf("%f\n", var[m-1]);
           //printf("%f\n", var[m]);
            out_compositing[i][i_col] = (short int)(var[m-1] + var[m]) / 2.0;
        }
        else
            out_compositing[i][i_col] = var[m];

    }

    free(var);
    return SUCCESS;

}

/******************************************************************************
MODULE:  hot_compositing

PURPOSE:  Running compositing for pixel-based time series

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019   Su Ye         Original Development
******************************************************************************/
int hot_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    int i, j;
    double wt;
    double index_sum[TOTAL_IMAGE_BANDS];
    for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        index_sum[i] = 0;
    double wt_sum = 0;
    int valid_count_window = 0;

    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            wt = (double)1.0/((buf[BLUE_INDEX][i] - 0.5 * buf[RED_INDEX][i]) * (buf[BLUE_INDEX][i] - 0.5 * buf[RED_INDEX][i]));
            //wt = (float)1/(abs(buf[BLUE_INDEX][i] - 0.5 * buf[RED_INDEX][i]));
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                index_sum[j] = index_sum[j] + buf[j][i] * wt;
            }
            wt_sum = wt_sum + wt;
            valid_count_window++;
        }
    }

    /********************************************/
    /*    condition 1: zero valid observation   */
    /********************************************/
    if(valid_count_window==0)
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            out_compositing[i][i_col] = -9999;
        }
        return SUCCESS;
    }

    /********************************************/
    /*    condition 2: standard procedures      */
    /********************************************/

    for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
    {
        out_compositing[j][i_col] = (short int)(index_sum[j] / wt_sum);
        //out_compositing[j][i_col] = (short int)valid_count_window;
    }

    return SUCCESS;

}


/******************************************************************************
MODULE:  modified_hot_compositing

PURPOSE:  Running compositing for pixel-based time series by adding shadow consideration

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/22/2019   Su Ye         Original Development
******************************************************************************/
int modified_hot_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    int i, j;
    int status;
    double wt;
    double wt_shadow;
    double wt_cloud;
    double index_sum[TOTAL_IMAGE_BANDS];
    for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        index_sum[i] = 0;
    double wt_sum = 0;
    int valid_count_window = 0;
    short int** ts_subset;
    short int* ts_subset_selected_shadow;
    short int* ts_subset_selected_blue;
    short int variogram_shadow;
    short int medium_shadow;

    char FUNC_NAME[] = "modified_hot_compositing";

    ts_subset = (short int**)allocate_2d_array(TOTAL_IMAGE_BANDS, valid_date_count, sizeof(short int));
    if(ts_subset == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset memory", FUNC_NAME, ERROR);
    }

    ts_subset_selected_shadow = (short int*)malloc(valid_date_count*sizeof(short int));
    if(ts_subset_selected_shadow == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset_selected_shadow memory", FUNC_NAME, ERROR);
    }

    ts_subset_selected_blue = (short int*)malloc(valid_date_count*sizeof(short int));
    if(ts_subset_selected_blue == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset_selected_blue memory", FUNC_NAME, ERROR);
    }


    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                ts_subset[j][valid_count_window] = buf[j][i];
                if(j == NIR_INDEX)
                {
                   //ts_subset_selected_shadow[valid_count_window] = (buf[BLUE_INDEX][i] + buf[RED_INDEX][i] + buf[GREEN_INDEX][i])/3;
		   ts_subset_selected_shadow[valid_count_window] = buf[NIR_INDEX][i];
                    //printf("%i\n", ts_subset_selected[valid_count_window]);
                }
                if(j == BLUE_INDEX)
                {
                    ts_subset_selected_blue[valid_count_window] = buf[BLUE_INDEX][i];
                    //printf("%i\n", ts_subset_selected[valid_count_window]);
                }
            }

            valid_count_window++;

        }
    }


    /********************************************/
    /*    condition 1: zero valid observation   */
    /********************************************/
    if(valid_count_window < 3)
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            out_compositing[i][i_col] = -9999;
        }
        return SUCCESS;
    }


    //single_mean_rmse(ts_subset_selected, 0, valid_count_window - 1, &rmse, &mean);
    //quick_sort_shortint(ts_subset_selected, 0, valid_count_window - 1);

    //m = valid_count_window / 2;

    //medium_shadow = (ts_subset_selected[m] + ts_subset_selected[m - 1] + ts_subset_selected[m + 1])/3;
    single_median_variogram(ts_subset_selected_shadow, 0, valid_count_window - 1, &variogram_shadow, &medium_shadow);
    //single_median_quantile(ts_subset_selected_blue, 0, valid_count_window - 1, &quantile_blue, &medium_blue);
    //single_median_variogram(ts_subset_selected_hot, 0, valid_count_window - 1, &variogram_hot, &medium_hot);
    //penalty_slope = - 9.0 / (1000.0 * variogram);
//    penalty_slope = - 33.0 / (50.0 * variogram);
//    penalty_intercept = PENALTY_INTERCEPT;

//    for(i = 0; i < valid_count_window; i++)
//    {

//        if (ts_subset_selected[i] - medium < - 1.5 * variogram)
//        {
//            ts_subset_id[i] = 1;
//        }
//        else if (ts_subset_selected[i] - medium < - 2 * variogram)
//        {
//            ts_subset_id[i] = 2;
//        }
//        else if (ts_subset_selected[i] - medium < - 3 * variogram)
//        {
//            ts_subset_id[i] = 3;
//        }
//        else
//        {
//            ts_subset_id[i] = 0;
//        }

//    }
//    for(i = 0; i < valid_count_window; i++)
//    {

//            if (ts_subset_selected[i] - medium < - 1.5 * variogram)
//            {
//                ts_subset_id[i] = 1;
//            }
//            else
//            {
//                ts_subset_id[i] = 0;
//            }

//    }

    /********************************************/
    /*    condition 2: standard procedures      */
    /********************************************/
    double ratio;
    for(i = 0; i < valid_count_window; i++)
    {
         wt_cloud = (double) 1.0 / (ts_subset_selected_blue[i] * ts_subset_selected_blue[i] * ts_subset_selected_blue[i]);

         if (ts_subset_selected_shadow[i] < medium_shadow)
         {
             ratio = (double)ts_subset_selected_shadow[i] / medium_shadow;
             wt_shadow = ratio * ratio * ratio * ratio * ratio;
         }
         else
             wt_shadow = 1.0;

         wt = wt_cloud * wt_shadow;
        //wt = (float)1/(abs(buf[BLUE_INDEX][i] - 0.5 * buf[RED_INDEX][i]));
        for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
        {
            index_sum[j] = index_sum[j] + ts_subset[j][i] * wt;
        }
        wt_sum = wt_sum + wt;
    }


    for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
    {
        out_compositing[j][i_col] = (short int)(index_sum[j] / wt_sum);
        //out_compositing[j][i_col] = (short int)valid_count_window;
    }

    /* free memory*/
    status = free_2d_array(ts_subset);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: ts_subset\n",
                      FUNC_NAME, FAILURE);
    }
    free(ts_subset_selected_shadow);
    free(ts_subset_selected_blue);

    return SUCCESS;

}

/******************************************************************************
MODULE:  valid_obs_count

PURPOSE:  count valid observation for each pixels

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/22/2019   Su Ye         Original Development
******************************************************************************/
int valid_obs_count
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    int index_sum[TOTAL_IMAGE_BANDS];
    int i, j, status;
    for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        index_sum[i] = 0;
    int valid_count_window = 0;
    short int** ts_subset;
    short int* ts_subset_selected;


    char FUNC_NAME[] = "modified_hot_compositing";

    ts_subset = (short int**)allocate_2d_array(TOTAL_IMAGE_BANDS, valid_date_count, sizeof(short int));
    if(ts_subset == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset memory", FUNC_NAME, ERROR);
    }

    ts_subset_selected = (short int*)malloc(valid_date_count*sizeof(short int));
    if(ts_subset_selected == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset_selected memory", FUNC_NAME, ERROR);
    }

    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                ts_subset[j][valid_count_window] = buf[j][i];
                if(j == NIR_INDEX)
                {
                    ts_subset_selected[valid_count_window] = buf[NIR_INDEX][i];
                    //printf("%i\n", ts_subset_selected[valid_count_window]);
                }
            }
            valid_count_window++;

        }
    }




    for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
    {

        out_compositing[j][i_col] = (short int)valid_count_window;
    }

    status = free_2d_array((void **)ts_subset);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: ts_subset\n",
                      FUNC_NAME, FAILURE);
    }
    free(ts_subset_selected);

    return SUCCESS;

}


/******************************************************************************
MODULE:  medium_compositing

PURPOSE:  Running compositing for pixel-based time series by adding shadow consideration

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/22/2019   Su Ye         Original Development
******************************************************************************/
int medium_compositing
(
    short int **buf,            /* I:  pixel-based time series           */
    int *valid_date_array,      /* I: valid date time series               */
    int valid_date_count,       /* I: the number of valid dates               */
    int lower_ordinal,
    int upper_ordinal,
    int i_col,
    short int **out_compositing           /* O: outputted compositing results for four bands */
)
{
    int i, j, m;
    int status;
    double wt;
    double index_sum[TOTAL_IMAGE_BANDS];
    for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        index_sum[i] = 0;
    double wt_sum = 0;
    int valid_count_window = 0;
    short int** ts_subset;
    short int* ts_subset_selected;
    int* ts_subset_selected_index;
    char FUNC_NAME[] = "medium_compositing";

    ts_subset = (short int*)allocate_2d_array(TOTAL_IMAGE_BANDS, valid_date_count, sizeof(short int));
    if(ts_subset == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset memory", FUNC_NAME, ERROR);
    }

    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                ts_subset[j][valid_count_window] = buf[j][i];
            }
            valid_count_window++;
        }
    }


    ts_subset_selected = (short int*)malloc(valid_count_window*sizeof(short int));
    if(ts_subset_selected == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset_selected memory", FUNC_NAME, ERROR);
    }

    ts_subset_selected_index = (int*)malloc(valid_count_window*sizeof(int));
    if(ts_subset_selected == NULL)
    {
        RETURN_ERROR ("Allocating ts_subset_selected_index memory", FUNC_NAME, ERROR);
    }

    for(i = 0; i < valid_count_window; i++)
    {

        ts_subset_selected[i] = ts_subset[NIR_INDEX][i];
        ts_subset_selected_index[i] = i;
            //printf("%i\n", ts_subset_selected[valid_count_window]);
    }



    /********************************************/
    /*    condition 1: zero valid observation   */
    /********************************************/
    if(valid_count_window == 0)
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            out_compositing[i][i_col] = -9999;
        }
        return SUCCESS;
    }


    quick_sort_shortint_index(ts_subset_selected, ts_subset_selected_index,
                              0, valid_count_window - 1);
//    for (j = 0; j < valid_count_window; j++)
//    {
//       printf("%i  %i\n", ts_subset_selected[j], ts_subset_selected_index[j]);
//    }

     m = valid_count_window / 2;
    if (valid_count_window % 2 == 0)
    {
        //printf("%f\n", var[m-1]);
       //printf("%f\n", var[m]);
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
           out_compositing[i][i_col] = (short int)((ts_subset[i][ts_subset_selected_index[m-1]] +
                   ts_subset[i][ts_subset_selected_index[m]]) / 2);
        }

    }
    else
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
           out_compositing[i][i_col] = (short int)(ts_subset[i][ts_subset_selected_index[m]]);
        }
        //printf("%i\n", out_compositing[i][i_col]);
    }


    status = free_2d_array((void**)ts_subset);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: ts_subset\n",
                      FUNC_NAME, FAILURE);
    }
    free(ts_subset_selected);

    return SUCCESS;

}

/******************************************************************************
MODULE:  compositing_scanline

PURPOSE:  Running compositing for scanline-based time series

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019   Su Ye         Original Development
******************************************************************************/
int compositing_scanline
(
    short int **buf,            /* I:  scanline-based time series           */
    int **valid_datearray_scanline,      /* I: valid date time series               */
    int *valid_datecount_scanline,       /* I: the number of valid dates               */
    int lower_ordinal,                /* I: lower ordinal date               */
    int upper_ordinal,                   /* I: upper_ordinal for temporal range of composition   */
    int num_samples,                /* I: the pixel number in a row              */
    int num_scenes,                 /* I: the number of scenes               */
    short int **out_compositing,           /* O: outputted compositing results for four bands */
    int method                       /* I: the compositing method{1 - fitting-weighted; 2 - fitting-normal; 3 - hot; 4 - average}  */
)
{
    int  j;
    int i_col;
    short int **tmp_buf;                   /* This is the image bands buffer, valid pixel only*/
    char FUNC_NAME[] = "compositing_scanline";
    int b_diagnosis = FALSE;
    Output_t* rec_c;

    tmp_buf = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, num_scenes, sizeof (short int));
    if(tmp_buf == NULL)
    {
        RETURN_ERROR("ERROR allocating tmp_buf memory", FUNC_NAME, FAILURE);
    }

    rec_c = malloc(sizeof(Output_t));
    if(rec_c == NULL)
    {
        RETURN_ERROR("ERROR allocating rec_c memory", FUNC_NAME, FAILURE);
    }

    for(i_col = 0; i_col < num_samples; i_col++)
    {
        for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
        {
           tmp_buf[j]  = buf[j] + i_col * num_scenes;
        }

        /*weighted fitting*/
        if (1==method)
        {
            fitting_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing, TRUE, TRUE, b_diagnosis, rec_c);
        }
        /*normal fitting*/
        else if (2==method)
        {
            fitting_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing, TRUE, FALSE, b_diagnosis, rec_c);
        }
        else if (3==method)
        {
            hot_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing);
        }
        else if (4==method)
        {
            average_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing);
        }
        else if (5==method)
        {
            fitting_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing, FALSE, FALSE, b_diagnosis, rec_c);
        }
        else if (6==method)
        {
            modified_hot_compositing(tmp_buf, valid_datearray_scanline[i_col],
                        valid_datecount_scanline[i_col], lower_ordinal,
                        upper_ordinal, i_col, out_compositing);
        }

        else if (7==method)
        {
            medium_compositing(tmp_buf, valid_datearray_scanline[i_col],
                               valid_datecount_scanline[i_col], lower_ordinal,
                               upper_ordinal, i_col, out_compositing);
        }
        else if (8==method)
        {
            valid_obs_count(tmp_buf, valid_datearray_scanline[i_col],
                            valid_datecount_scanline[i_col], lower_ordinal,
                            upper_ordinal, i_col, out_compositing);
        }
    }

    free(rec_c);
    free_2d_array((void **)tmp_buf);

    return SUCCESS;
}

/******************************************************************************
MODULE:  fitting_compositing

PURPOSE:  Running compositing for pixel-based time series

RETURN VALUE:
Type = int (SUCCESS, ERROR or FAILURE)

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019   Su Ye         Original Development
******************************************************************************/
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
)
{
    char FUNC_NAME[] = "fitting_compositing";
    int status;
    float date_vario;           /* I: median date                                          */
    float max_date_difference;   /* I: maximum difference between two neighbor dates        */
    float adj_rmse[TOTAL_IMAGE_BANDS]; /* Adjusted RMSE for all bands          */
    int *bl_ids;
    int n_clr;
    int n_clr_1;
    int n_outlier_1;
    int n_clr_2;
    int n_outlier_2;
    int k, b;
    int* clrx;
    float **clry;
    int* clrx_1;
    float **clry_1;
    int* clrx_2;
    float **clry_2;
    int i;
    float C0; // intercept from each test output
    float C1; // slope from each test output

    clrx = (int*)calloc(valid_date_count, sizeof(int));
    clry = (float **) allocate_2d_array (TOTAL_IMAGE_BANDS, valid_date_count,
                                             sizeof (float));
    if (clry == NULL)
    {
        RETURN_ERROR ("Allocating clry memory", FUNC_NAME, FAILURE);
    }

    /**************************************************************/
    /*                                                            */
    /*      select observations in the observation window          */
    /*                                                            */
    /**************************************************************/
    n_clr = 0;
    for(i = 0; i < valid_date_count; i++)
    {
        if((valid_date_array[i] > lower_ordinal - 1) && (valid_date_array[i] < upper_ordinal + 1))
        {
            clrx[n_clr] = valid_date_array[i];
            for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
            {
                clry[b][n_clr] = (float)buf[b][i];
            }
            n_clr++;
        }
    }

    /********************************************/
    /*    condition 1: zero valid observation   */
    /********************************************/
    if(n_clr==0)
    {
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
        {
            out_compositing[i][i_col] = -9999;
        }

        if(TRUE == b_diagnosis)
        {
            rec_c->condition = NOOBS_CONDITION;
        }

        free(clrx);
        status = free_2d_array((void **)clry);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: clry\n",
                          FUNC_NAME, FAILURE);
        }

        return SUCCESS;
    }

    /**********************************************/
    /*    condition 2: inefficient observations   */
    /**********************************************/
    else if (n_clr < MIN_SAMPLE)
    {
        median_compositing(buf, valid_date_array, valid_date_count,
                           i_col, out_compositing);

        if(TRUE == b_diagnosis)
        {
            rec_c->condition = INEFFICIENT_CONDITION;
        }

        free(clrx);
        status = free_2d_array((void **)clry);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: clry\n",
                          FUNC_NAME, FAILURE);
        }

        return SUCCESS;
    }
    else
    {
        if(TRUE == b_diagnosis)
        {
            rec_c->condition = NORMAL_CONDITION;
        }
    }

    /**********************************************/
    /*    condition 3: standard procedure         */
    /**********************************************/

    bl_ids = (int *)calloc(n_clr, sizeof(int));
    if (bl_ids == NULL)
    {
        RETURN_ERROR("ERROR allocating bl_ids memory", FUNC_NAME, FAILURE);
    }


    clrx_1 = (int *)calloc(n_clr, sizeof(int));
    clry_1 = (float **) allocate_2d_array (TOTAL_IMAGE_BANDS, n_clr,
                                             sizeof (float));
    if (clry_1 == NULL)
    {
        RETURN_ERROR ("Allocating clry_1 memory", FUNC_NAME, FAILURE);
    }

    clrx_2 = (int *)calloc(n_clr, sizeof(int));
    clry_2 = (float **) allocate_2d_array (TOTAL_IMAGE_BANDS, n_clr,
                                             sizeof (float));
    if (clry_2 == NULL)
    {
        RETURN_ERROR ("Allocating clry_2 memory", FUNC_NAME, FAILURE);
    }

    /**************************************************************/
    /*                                                            */
    /*      calculate variogram for each band and dates.          */
    /*                                                            */
    /**************************************************************/
    status = adjust_median_variogram(clrx, clry, TOTAL_IMAGE_BANDS,
                                     0, n_clr-1, &date_vario,
                                     &max_date_difference, adj_rmse);
    if (status != SUCCESS)
    {
            RETURN_ERROR("ERROR calling median_variogram routine", FUNC_NAME,
                         FAILURE);
    }


    status = greenband_test(clrx, clry, 0, n_clr-1, adj_rmse[1], T_CONST_SINGLETAIL_9999,
            bl_ids, &C0, &C1);

    if (status != SUCCESS)
    {
        RETURN_ERROR("ERROR calling greenband_test",
                      FUNC_NAME, FAILURE);
    }

    if(TRUE == b_diagnosis)
    {
        rec_c->C0_green = C0;
        rec_c->C1_green = C1;
    }

    /**************************************************/
    /*                                                */
    /*         remove outliers.                       */
    /*                                                */
    /**************************************************/
    n_clr_1 = 0;
    n_outlier_1 = 0;
    for(i = 0; i < n_clr; i++)
    {
        if(bl_ids[i] == 0)
        {
            clrx_1[n_clr_1] = clrx[i];
            for (b = 0; b < TOTAL_IMAGE_BANDS; b++)
            {
                clry_1[b][n_clr_1] = clry[b][i];
            }
            n_clr_1 = n_clr_1 + 1;
        }
        else
        {
            if(TRUE == b_diagnosis)
            {
                rec_c->outlier_dates_green[n_outlier_1] = clrx[i];
                n_outlier_1 = n_outlier_1 + 1;
            }
        }
    }

    rec_c->n_outlier_green = n_outlier_1;

    /* if n_clr_1 < MIN_SAMPLE, means that green test failed, need to reset*/
    if (n_clr_1 < MIN_SAMPLE)
    {
        n_clr_1 = 0;
        for(i = 0; i < n_clr; i++)
        {

            clrx_1[n_clr_1] = clrx[i];
            for (b = 0; b < TOTAL_IMAGE_BANDS; b++)
            {
                clry_1[b][n_clr_1] = (float)clry[b][i];
            }
            n_clr_1 = n_clr_1 + 1;

        }
        if(TRUE == b_diagnosis)
            rec_c->b_success_green = FAILURE;
    }
    else{
        if(TRUE == b_diagnosis)
            rec_c->b_success_green = SUCCESS;
    }


    /**************************************************/
    /*                                                */
    /* nir band test                                  */
    /*                                                */
    /**************************************************/

    for (k = 0; k < n_clr_1; k++)
       bl_ids[k] = 0;

    status = nirband_test(clrx_1, clry_1, 0, n_clr_1-1, adj_rmse[3], T_CONST_SINGLETAIL_9999,
            bl_ids, &C0, &C1);

    if (status != SUCCESS)
    {
        RETURN_ERROR("ERROR calling nirband_test",
                      FUNC_NAME, FAILURE);
    }

    if(TRUE == b_diagnosis)
    {
        rec_c->C0_nir = C0;
        rec_c->C1_nir = C1;
    }

    /**************************************************/
    /*                                                */
    /*         remove outliers.                       */
    /*                                                */
    /**************************************************/
    n_clr_2 = 0;
    n_outlier_2 = 0;
    for(i = 0; i < n_clr_1; i++)
    {
        if(bl_ids[i] == 0)
        {
            clrx_2[n_clr_2] = clrx_1[i];
            for (b = 0; b < TOTAL_IMAGE_BANDS; b++)
            {
                clry_2[b][n_clr_2] = clry_1[b][i];
            }
            n_clr_2 = n_clr_2 + 1;
        }
        else
        {
            if(TRUE == b_diagnosis)
            {
                rec_c->outlier_dates_nir[n_outlier_2] = clrx_1[i];
                n_outlier_2 = n_outlier_2 + 1;
            }
        }
    }

    rec_c->n_outlier_nir = n_outlier_2;

    /* if n_clr_1 < MIN_SAMPLE, means that nir test failed, reset*/
    if (n_clr_2 < MIN_SAMPLE)
    {
        for(i = 0; i < n_clr_1; i++)
        {
            n_clr_2 = 0;
            clrx_2[n_clr_2] = clrx_1[i];
            for (b = 0; b < TOTAL_IMAGE_BANDS; b++)
            {
                clry_2[b][n_clr_2] = clry_1[b][i];
            }
            n_clr_2 = n_clr_2 + 1;
        }

        if(TRUE == b_diagnosis)
            rec_c->b_success_nir = FAILURE;
    }
    else{
        if(TRUE == b_diagnosis)
            rec_c->b_success_nir = SUCCESS;
    }


    if(bfit == TRUE)
        linear_fit_centerdate(clrx_2, clry_2, n_clr_2, 0, (lower_ordinal + upper_ordinal)/2,i_col,
                          out_compositing, bweighted, rec_c->C0_final, rec_c->C1_final);
    else
    {
        float wt;
        int j;
        double index_sum[TOTAL_IMAGE_BANDS];
        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
            index_sum[i] = 0;
        float wt_sum = 0;


        for(i = 0; i < n_clr_2; i++)
        {
            wt = (float)1/((clry_2[BLUE_INDEX][i] - 0.5 * clry_2[RED_INDEX][i]) * (clry_2[BLUE_INDEX][i] - 0.5 * clry_2[RED_INDEX][i]));
            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
            {
                index_sum[j] = index_sum[j] + clry_2[j][i] * wt;
            }
            wt_sum = wt_sum + wt;
        }

        /********************************************/
        /*    condition 2: standard procedures      */
        /********************************************/

        for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
        {
            out_compositing[j][i_col] = (short int)(index_sum[j] / wt_sum);
        }

    }


    free(bl_ids);
    free(clrx);
    status = free_2d_array((void **)clry);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: clry\n",
                      FUNC_NAME, FAILURE);
    }

    free(clrx_1);
    status = free_2d_array((void **)clry_1);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: clry_1\n",
                      FUNC_NAME, FAILURE);
    }

    free(clrx_2);
    status = free_2d_array((void **)clry_2);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: clry_2\n",
                      FUNC_NAME, FAILURE);
    }

    return SUCCESS;

}

//int fitting_compositing_scanline
//(
//    short int **buf,            /* I:  scanline-based time series           */
//    int **valid_datearray_scanline,      /* I: valid date time series               */
//    int *valid_datecount_scanline,       /* I: the number of valid dates               */
//    int lower_ordinal,                /* I: lower ordinal dates               */
//    int upper_ordinal,                   /* I: upper_ordinal for temporal range of composition   */
//    int num_samples,                /* I: the pixel number in a row              */
//    int num_scenes,                 /* I: the number of scenes               */
//    short int **out_compositing           /* O: outputted compositing results for four bands */
//)
//{
//    int i, j;
//    int i_col;
//    short int **tmp_buf;                   /* This is the image bands buffer, valid pixel only*/
//    int result;
//    char FUNC_NAME[] = "compositing_scanline";
//    tmp_buf = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, num_scenes, sizeof (short int));
//    if(tmp_buf == NULL)
//    {
//        RETURN_ERROR("ERROR allocating tmp_buf memory", FUNC_NAME, FAILURE);
//    }

//    for(i_col = 0; i_col < num_samples; i_col++)
//    {
//        for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
//        {
//           tmp_buf[j]  = buf[j] + i_col * num_scenes;
//        }

//        fitting_compositing(tmp_buf, valid_datearray_scanline[i_col],
//                    valid_datecount_scanline[i_col], lower_ordinal,
//                    upper_ordinal, i_col, out_compositing);
//    }

//    free_2d_array((void **)tmp_buf);

//    return SUCCESS;
//}
