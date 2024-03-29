#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <omp.h>
#include <stdbool.h>
#include <unistd.h>
#include "gdal/gdal.h"
#include "const.h"
#include "utilities.h"
#include "input.h"
#include "2d_array.h"
#include "misc.h"
#include "compositing.h"


int write_output_binary
(
    FILE *fptr,      /* I: pointer to the binary file */
    Output_t t          /* I: outputted structure     */
)
{
    int nvals;               /* number of values written to the file */
    char FUNC_NAME[] = "write_output_binary"; /* function name */

    nvals = fwrite(&t, sizeof(Output_t), 1, fptr);

    if (nvals != 1)
    {
        RETURN_ERROR("Incorrect amount of data written", FUNC_NAME, ERROR);
    }

    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    char in_dir[MAX_STR_LEN];
    char out_dir[MAX_STR_LEN];
    char out_path[MAX_STR_LEN];
    char msg_str[MAX_STR_LEN];        /* Input data scene name                  */
    int result;
    int i, j;
    FILE *fd;
    char FUNC_NAME[] = "main";        /* For printing error messages            */
    char errmsg[MAX_STR_LEN];   /* for printing error text to the log.  */
    char scene_list_filename[] = "scene_list.txt"; /* file name containing list of input sceneIDs */
    char scene_list_directory[MAX_STR_LEN]; /* full directory of scene list*/
    char tmpstr[MAX_STR_LEN];        /* char string for text manipulation      */
    char filename[MAX_STR_LEN];      /* file name constructed from sceneID   */
    char srsfilename[MAX_STR_LEN];               /* source file for outputted projection*/
    char *pszSRS_ref = NULL;
    time_t now;                      /* For logging the start, stop, and some     */
    char **scene_list;               /* 2-D array for list of scene IDs        */
    int num_scenes;                  /* Number of input scenes defined        */
    int *sdate;                      /* Pointer to list of acquisition dates  */
    int status;                      /* Return value from function call       */
    FILE **f_bip;                  /* Array of file pointers of BIP files    */
    input_meta_t *meta;              /* Structure for ENVI metadata hdr info  */
    short int **buf;                       /* This is the image bands buffer, valid pixel only*/
    int **valid_date_array_scanline;
    short int **fmask_buf_scanline;        /* fmask buf, valid pixels only*/
    int* valid_scene_count_scanline;
    short int **poutScanline;           /* outputted compositing results for four bands */
    short int **poutPoint;   /* outputted compositing results for mode = pixel-based */
    /* gdal related */
    GDALRasterBandH hBand[TOTAL_IMAGE_BANDS];
    GDALDatasetH hDstDS;
    char **papszOptions = NULL;
    double adfGeoTransform[6];
    GDALDriverH hDriver;
    GDALDatasetH  srsDataset;
    const char *pszFormat = "GTiff";
    time (&now);                      /*  intermediate times.                   */
    int mode;
    int row;
    int col;
    int tile_id;
    int lower_ordinal;
    int upper_ordinal;
    char **valid_scene_list = NULL;    /* 2-D array for list of filtered        */
    int *valid_date_array;             /* Sdate array after cfmask filtering    */
    short int *fmask_buf;              /* fmask buf, valid pixels only*/
    char pointTS_obs_path[MAX_STR_LEN]; /* output point-based time series csv, only for debug   */
    char pointTS_result_path[MAX_STR_LEN]; /* output point-based time series csv, only for debug   */
    char out_filename[MAX_STR_LEN];
    int valid_scene_count = 0;         /* x/y location specified is not valid be-   */
    short int **compositing_result;
    int method;
    int b_diagnosis = FALSE;
    Output_t* rec_c;
    // short int **buf1, **buf2, **buf3;

    // printf("argc = %d\n", argc);

    scene_list = (char **) allocate_2d_array (MAX_SCENE_LIST, ARD_STR_LEN,
                                               sizeof (char));
    if (scene_list == NULL)
    {
        RETURN_ERROR ("Allocating scene_list memory",
                                 FUNC_NAME, FAILURE);
    }

    snprintf (msg_str, sizeof(msg_str), "compositing start_time=%s\n", ctime (&now));

    LOG_MESSAGE (msg_str, FUNC_NAME);

    /**************************************************************/
    /*                                                            */
    /*   read inputting variables                                 */
    /*                                                            */
    /**************************************************************/
    result = get_args(argc, argv, in_dir, out_dir, &tile_id, &lower_ordinal,
                      &upper_ordinal, &mode, &row, &col, &method);

    if(result == ERROR)
    {
         RETURN_ERROR("Fail to read program variables. The program stops!", FUNC_NAME, FAILURE);
    }

    sprintf(scene_list_directory, "%s/%s", in_dir, scene_list_filename);

    if (access(scene_list_directory, F_OK) != 0) /* File does not exist */
    {
        status = create_scene_list(in_dir, &num_scenes, scene_list_filename);
        if(status != SUCCESS)
            RETURN_ERROR("Running create_scene_list file", FUNC_NAME, FAILURE);
    }
    else
    {
        num_scenes = MAX_SCENE_LIST;
    }

    /**************************************************************/
    /*                                                            */
    /* Fill the scene list array with full path names.            */
    /*                                                            */
    /**************************************************************/
    fd = fopen(scene_list_directory, "r");
    if (fd == NULL)
    {
        RETURN_ERROR("Opening scene_list file", FUNC_NAME, FAILURE);
    }

    for (i = 0; i < num_scenes; i++)
    {
        if (fscanf(fd, "%s", tmpstr) == EOF)
            break;
        strcpy(scene_list[i], tmpstr);
    }

    num_scenes = i;

    fclose(fd);

    f_bip = (FILE **)malloc(num_scenes * sizeof (FILE*));
    if (f_bip == NULL)
    {
        RETURN_ERROR ("Allocating f_bip memory", FUNC_NAME, FAILURE);
    }

    sdate = (int*)malloc(num_scenes * sizeof(int));
    if (sdate == NULL)
    {
        RETURN_ERROR("ERROR allocating sdate memory", FUNC_NAME, FAILURE);
    }


    /**************************************************************/
    /*                                                            */
    /* Sort scene_list based on year & julian_day, then do the    */
    /* swath filter, but read it above first.                     */
    /*                                                            */
    /**************************************************************/

    status = sort_scene_based_on_year_doy_row(scene_list, num_scenes, sdate);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Calling sort_scene_based_on_year_jday",
                      FUNC_NAME, FAILURE);
    }

    /**************************************************************/
    /*                                                            */
    /*    read metadata info                                      */
    /*                                                            */
    /**************************************************************/

    meta = (input_meta_t *)malloc(sizeof(input_meta_t));
    status = read_envi_header(in_dir, scene_list[0], meta);
    if (status != SUCCESS)
    {
       RETURN_ERROR ("Calling read_envi_header",
                          FUNC_NAME, FAILURE);
    }

//    for (i = 0; i < num_scenes; i++)
//    {
//        sprintf(filename, "%s/%s", in_dir, scene_list[i]);
//    }


    fmask_buf_scanline = (short int **)allocate_2d_array (meta->samples,
                                                          num_scenes, sizeof (short int));
    if (fmask_buf_scanline == NULL)
    {
        RETURN_ERROR("ERROR allocating fmask_buf_scanline memory", FUNC_NAME, FAILURE);
    }

    valid_scene_count_scanline = (int*)malloc(meta->samples * sizeof(int));
    if (valid_scene_count_scanline == NULL)
    {
        RETURN_ERROR("ERROR allocating valid_scene_count_scanline memory", FUNC_NAME, FAILURE);
    }

    valid_date_array_scanline = (int**)allocate_2d_array (meta->samples, num_scenes, sizeof (int));
    if (valid_date_array_scanline == NULL)
    {
        RETURN_ERROR("ERROR allocating valid_date_array_scanline memory", FUNC_NAME, FAILURE);
    }

    poutScanline = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, meta->samples,
                                               sizeof(short int));
    if (poutScanline == NULL)
    {
        RETURN_ERROR ("Allocating poutScanline memory",
                                 FUNC_NAME, FAILURE);
    }

    poutPoint = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, 1,
                                               sizeof(short int));
    if (poutPoint == NULL)
    {
        RETURN_ERROR ("Allocating poutPoint memory",
                                 FUNC_NAME, FAILURE);
    }

    /**************************************************************/
    /*                                                            */
    /*            Allocating memory finished.                     */
    /*                                                            */
    /**************************************************************/

    /* pixel-based detection */
    if(mode == 1)
    {
        buf = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, num_scenes, sizeof (short int));
        if (buf == NULL)
        {
            RETURN_ERROR ("Allocating buf memory", FUNC_NAME, FAILURE);
        }

        valid_scene_list = (char **) allocate_2d_array (num_scenes, MAX_STR_LEN,
                                             sizeof (char));
        if (valid_scene_list == NULL)
        {
            RETURN_ERROR ("Allocating valid_scene_list memory", FUNC_NAME, FAILURE);
        }


        valid_date_array = malloc(num_scenes * sizeof(int));
        if (valid_date_array == NULL)
        {
            RETURN_ERROR ("Allocating valid_date_array memory", FUNC_NAME, FAILURE);
        }

        fmask_buf = (short int *) malloc(num_scenes * sizeof (short int));
        if (fmask_buf == NULL)
        {
            RETURN_ERROR ("Allocating fmask_buf memory", FUNC_NAME, FAILURE);
        }
        /* temporally hard-coded*/

        compositing_result = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS, 1,
                                                   sizeof (short int));
        if (compositing_result == NULL)
        {
            RETURN_ERROR("ERROR allocating compositing_result memory", FUNC_NAME, FAILURE);
        }

        /* assign to be 0, meaning to produce output for diagnosis*/
        b_diagnosis = TRUE;
        rec_c = malloc(sizeof(Output_t));
        if (rec_c == NULL)
        {
            RETURN_ERROR("ERROR allocating rec_c memory", FUNC_NAME, FAILURE);
        }

        /*******************************************************/
        /******************* meta data result path ************/
        /*****************************************************/
        /* save output meta data csv, e.g.  "/home/su/Documents/Jupyter/source/LandsatARD/Plot23_coutput.csv";  */
        sprintf(out_filename, "coutput_%d_%d_obs.csv", row, col);
        sprintf(pointTS_obs_path, "%s/%s", out_dir, out_filename);

        sprintf(out_filename, "coutput_%d_%d_result", row, col);
        sprintf(pointTS_result_path, "%s/%s", out_dir, out_filename);

        valid_scene_count = 0;

        for (i = 0; i < num_scenes; i++)
        {
            read_bip(in_dir, scene_list, f_bip, i,
                     row, col, meta->samples, sdate, buf, fmask_buf, &valid_scene_count,
                     valid_scene_list, valid_date_array);
        }


        /*point_data output as csv*/
        fd = fopen(pointTS_obs_path, "w");
        for (i = 0; i < valid_scene_count; i++)
        {
            fprintf(fd, "%i, %d, %d, %d, %d, %d\n", valid_date_array[i], (short int)buf[0][i],
                    (short int)buf[1][i], (short int)buf[2][i], (short int)buf[3][i], (short int)fmask_buf[i]);

        }
        fclose(fd);

//        status = compositing_scanline(buf,valid_date_array, valid_scene_count, center_date,
//            half_interval, 1, num_scenes, compositing_result, method);
        /*weighted fitting*/
        if (1==method)
        {
            fitting_compositing(buf, valid_date_array,
                        valid_scene_count, lower_ordinal,
                        upper_ordinal, 0, compositing_result, TRUE, TRUE,
                                b_diagnosis, rec_c);
        }
        /*normal fitting*/
        else if (2==method)
        {
            fitting_compositing(buf, valid_date_array,
                                valid_scene_count, lower_ordinal,
                                upper_ordinal, 0, compositing_result,
                                TRUE, FALSE, b_diagnosis, rec_c);
        }
        else if (3==method)
        {
            hot_compositing(buf, valid_date_array,
                            valid_scene_count, lower_ordinal,
                            upper_ordinal, 0, compositing_result);
        }
        else if (4==method)
        {
            average_compositing(buf, valid_date_array,
                                valid_scene_count, lower_ordinal,
                                upper_ordinal, 0, compositing_result);
        }
        else if (5==method)
        {

            fitting_compositing(buf, valid_date_array,
                        valid_scene_count, lower_ordinal,
                                upper_ordinal, 0, compositing_result, FALSE, TRUE,
                                b_diagnosis, rec_c);
        }

        else if (6==method)
        {
            modified_hot_compositing(buf, valid_date_array,
                            valid_scene_count, lower_ordinal,
                                     upper_ordinal, 0, compositing_result);
        }

        else if (7==method)
        {
            medium_compositing(buf, valid_date_array,
                            valid_scene_count, lower_ordinal,
                               upper_ordinal, 0, compositing_result);
        }

        fd = fopen(pointTS_result_path, "w");
        result = write_output_binary(fd, *rec_c);
        if (result != SUCCESS)
        {
            RETURN_ERROR ("write_output_binary failed\n",
                          FUNC_NAME, FAILURE);
        }
        close(fd);

        free(fmask_buf);
        free(rec_c);
        status = free_2d_array ((void **) valid_scene_list);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: valid_scene_list\n",
                          FUNC_NAME, FAILURE);
        }
        status = free_2d_array((void **) compositing_result);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: compositing_result\n",
                          FUNC_NAME, FAILURE);
        }
        status = free_2d_array ((void **) buf);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: buf\n",
                          FUNC_NAME, FAILURE);
        }
        free(valid_date_array);
    }
    /* whole scene */
    else if (mode == 3)
    {

        for (i = 0; i < num_scenes; i++)
        {
            sprintf(filename, "%s/%s", in_dir, scene_list[i]);
            f_bip[i] = open_raw_binary(filename,"rb");
            if (f_bip[i] == NULL)
            {
                sprintf(errmsg, "Opening %d scene files\n", i);
                RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
            }
        }

        // outputted tif name
        sprintf(out_filename, "tile%d_%d_%d_pcs.tif", tile_id, lower_ordinal, upper_ordinal);

        //sprintf(out_filename, "composite_%d_%d.tif", center_date, half_interval);
        // create a complete path for output composite file
        sprintf(out_path, "%s/%s", out_dir, out_filename);

        buf = (short int **) allocate_2d_array (TOTAL_IMAGE_BANDS,
                                                num_scenes * meta->samples, sizeof (short int));
        if (buf == NULL)
        {
            RETURN_ERROR("ERROR allocating buf memory", FUNC_NAME, FAILURE);
        }


        /**************************************************************/
        /*                                                            */
        /*            create gdal dataset                             */
        /*                                                            */
        /**************************************************************/

        GDALAllRegister();

        hDriver = GDALGetDriverByName(pszFormat);
        hDstDS = GDALCreate(hDriver, out_path, meta->samples, meta->lines, TOTAL_IMAGE_BANDS,  GDT_Int16,
                             papszOptions);

        /**************************************************************/
        /*                                                            */
        /*            set projection from srs file                   */
        /*                                                            */
        /**************************************************************/

        sprintf(srsfilename, "%s/%s", in_dir, scene_list[0]);
        srsDataset = GDALOpen(srsfilename, GA_ReadOnly);
        pszSRS_ref = GDALGetProjectionRef(srsDataset);
        GDALSetProjection(hDstDS, pszSRS_ref);
        GDALClose(srsDataset);
        /**************************************************************/
        /*                                                            */
        /*            set geotransform from srs file                   */
        /*                                                            */
        /**************************************************************/
        adfGeoTransform[0] = meta->upper_left_x;
        adfGeoTransform[1] = PLANET_RES;
        adfGeoTransform[2] = 0;
        adfGeoTransform[3] = meta->upper_left_y;
        adfGeoTransform[4] = 0;
        adfGeoTransform[5] = -PLANET_RES;

        GDALSetGeoTransform( hDstDS, adfGeoTransform);

        for(i = 0; i < TOTAL_IMAGE_BANDS; i++)
            hBand[i] = GDALGetRasterBand(hDstDS, i+1);



        for (i = 0; i < meta->lines; i ++)
        {
            for(j = 0; j < meta->samples; j++)
            {
                valid_scene_count_scanline[j] = 0;
                // valid_scene_count_scanline_tmp[j] = 0;
            }
//            if(method == 3) // HOT index needs mediam filtering for each image
//            {
//                if(i == 0)
//                {
//                    result = read_bip_lines(f_bip, meta->samples,
//                                            num_scenes, sdate, buf1,
//                                            valid_scene_count_scanline,
//                                            valid_date_array_scanline, i);
//                    for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
//                        for(j = 0; j < num_scenes * meta->samples; j++)
//                            buf2[b][j] = buf1[b][j];

//                    result = read_bip_lines(f_bip, meta->samples,
//                                            num_scenes, sdate, buf3,
//                                            valid_scene_count_scanline_tmp,
//                                            valid_date_array_scanline_tmp, i + 1);

//                    median_filter(buf1, buf2, buf3, buf, valid_scene_count_scanline, num_scenes, meta->samples);
//                }
//                else if(i == meta->lines - 1) // last row doesn't need read buf 3
//                {
//                    //memcpy(buf1, buf2, TOTAL_IMAGE_BANDS * num_scenes * meta->samples *sizeof(short int));
//                    //memcpy(buf2, buf3, TOTAL_IMAGE_BANDS * num_scenes * meta->samples *sizeof(short int));

//                    for(j = 0; j < num_scenes; j++)
//                    {
//                         valid_scene_count_scanline[j] = valid_scene_count_scanline_tmp[j];
//                         for (b = 0; b < meta->samples; b++)
//                             valid_date_array_scanline[b][j] = valid_date_array_scanline_tmp[b][j];
//                    }

//                    //memcpy(valid_date_array_scanline, valid_date_array_scanline_tmp, num_scenes * meta->samples *sizeof(int));
//                    // median_filter(buf2, buf3, buf3, buf, meta->samples);
//                    median_filter(buf1, buf2, buf3, buf, valid_scene_count_scanline, num_scenes, meta->samples);

//                }
//                else
//                {
//                    for(b = 0; b < TOTAL_IMAGE_BANDS; b++)
//                    {
//                        for(j = 0; j < num_scenes * meta->samples; j++)
//                        {
//                            buf1[b][j] = buf2[b][j];
//                            buf2[b][j] = buf3[b][j];
//                        }
//                    }

//                    for(j = 0; j < num_scenes; j++)
//                    {
//                         valid_scene_count_scanline[j] = valid_scene_count_scanline_tmp[j];
//                         valid_scene_count_scanline_tmp[j] = 0;
//                         for (b = 0; b < meta->samples; b++)
//                             valid_date_array_scanline[b][j] = valid_date_array_scanline_tmp[b][j];
//                    }

//                    //memcpy(valid_scene_count_scanline, valid_scene_count_scanline_tmp, meta->samples *sizeof(int));
//                    //memcpy(valid_date_array_scanline, valid_date_array_scanline_tmp, num_scenes * meta->samples *sizeof(int));

//                    result = read_bip_lines(f_bip, meta->samples,
//                                            num_scenes, sdate, buf3,
//                                            valid_scene_count_scanline_tmp,
//                                            valid_date_array_scanline_tmp, i + 1);
//                    median_filter(buf1, buf2, buf3, buf, valid_scene_count_scanline, num_scenes, meta->samples);
//                }
//            }
//            else
//            {
                result = read_bip_lines(f_bip, meta->samples,
                                        num_scenes, sdate, buf,
                                        valid_scene_count_scanline,
                                        valid_date_array_scanline, i);
//            }

            if (result != SUCCESS)
            {
                sprintf(errmsg, "Error in reading ARD data for row_%d \n", i);
                RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
            }

            /**************************************************************/
            /*                                                            */
            /*            compositing based on scanline                   */
            /*                                                            */
            /**************************************************************/
            result = compositing_scanline(buf, valid_date_array_scanline, valid_scene_count_scanline,
                                          lower_ordinal, upper_ordinal, meta->samples, num_scenes,
                                          poutScanline, method);
            // printf("row_%d finished\n", i);
            if (result != SUCCESS)
            {
                sprintf(errmsg, "Error in compositing for row_%d \n", i);
                RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
            }

            for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
                GDALRasterIO(hBand[j], GF_Write, 0, i, meta->samples, 1,
                          poutScanline[j], meta->samples, 1, GDT_Int16,
                          0, 0 );


        }

        /**************************************************************/
        /*                                                            */
        /*                  Free memory                               */
        /*                                                            */
        /**************************************************************/

        for (i = 0; i < num_scenes; i++)
        {
            close_raw_binary(f_bip[i]);
        }

        status = free_2d_array ((void **) buf);
        if (status != SUCCESS)
        {
            RETURN_ERROR ("Freeing memory: buf\n",
                          FUNC_NAME, FAILURE);
        }


//        status = free_2d_array ((void **) buf1);
//        if (status != SUCCESS)
//        {
//            RETURN_ERROR ("Freeing memory: buf1\n",
//                          FUNC_NAME, FAILURE);
//        }


//        status = free_2d_array ((void **) buf2);
//        if (status != SUCCESS)
//        {
//            RETURN_ERROR ("Freeing memory: buf2\n",
//                          FUNC_NAME, FAILURE);
//        }


//        status = free_2d_array ((void **) buf3);
//        if (status != SUCCESS)
//        {
//            RETURN_ERROR ("Freeing memory: buf3\n",
//                          FUNC_NAME, FAILURE);
//        }

//        status = free_2d_array((void **)valid_date_array_scanline_tmp);
//        if (status != SUCCESS)
//        {
//            RETURN_ERROR ("Freeing memory: valid_date_array_scanline_tmp\n",
//                          FUNC_NAME, FAILURE);
//        }

//        free(valid_scene_count_scanline_tmp);
        GDALClose(hDstDS);

    }

    free(f_bip);

    status = free_2d_array ((void **) scene_list);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: scene_list\n",
                      FUNC_NAME, FAILURE);
    }
    free(meta);

    status = free_2d_array((void **) fmask_buf_scanline);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: fmask_buf_scanline\n",
                      FUNC_NAME, FAILURE);
    }

    status = free_2d_array((void **)valid_date_array_scanline);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: valid_date_array_scanline\n",
                      FUNC_NAME, FAILURE);
    }

    free(valid_scene_count_scanline);

    status = free_2d_array((void **)poutScanline);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: poutScanline\n",
                      FUNC_NAME, FAILURE);
    }

    status = free_2d_array((void **)poutPoint);
    if (status != SUCCESS)
    {
        RETURN_ERROR ("Freeing memory: poutPoint\n",
                      FUNC_NAME, FAILURE);
    }



    time(&now);
    snprintf (msg_str, sizeof(msg_str), "compositing end_time=%s\n", ctime (&now));
    LOG_MESSAGE (msg_str, FUNC_NAME);

    return SUCCESS;


}
