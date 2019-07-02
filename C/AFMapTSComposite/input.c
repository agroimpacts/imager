 #include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "input.h"
#include "utilities.h"
#include "const.h"

/******************************************************************************
MODULE:  sort_scene_based_on_year_doy_row

PURPOSE:  Sort scene list based on year and julian day of year and row number

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           error return
SUCCESS         No errors encountered

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            Original development

******************************************************************************/
int sort_scene_based_on_year_doy_row
(
    char **scene_list,      /* I/O: scene_list, sorted as output             */
    int num_scenes,         /* I: number of scenes in the scene list         */
    int *sdate              /* O: year plus date since 0000                  */
)
{
    int i;                  /* loop counter                                  */
    int status;             /* return of function calls for errror handling  */
    int year, doy;          /* to keep track of year, and day within year    */
    int *yeardoy;           /* combined year day of year as one string       */
    // int *row;               /* row of path/row for ordering from same swath  */
    char temp_string[8];    /* for string manipulation                       */
    char temp_string2[5];   /* for string manipulation                       */
    char temp_string3[4];   /* for string manipulation                       */
    char errmsg[MAX_STR_LEN]; /* for printing error messages                 */
    char FUNC_NAME[] = "sort_scene_based_on_year_doy_row"; /* function name  */
    int len; /* length of string returned from strlen for string manipulation*/

    /******************************************************************/
    /*                                                                */
    /* Allocate memory for yeardoy                                    */
    /*                                                                */
    /******************************************************************/

    yeardoy = malloc(num_scenes * sizeof(int));
    if (yeardoy == NULL)
    {
        RETURN_ERROR("Allocating yeardoy memory", FUNC_NAME, ERROR);
    }

    /******************************************************************/
    /*                                                                */
    /* Get year plus doy from scene name                              */
    /*                                                                */
    /******************************************************************/

    for (i = 0; i < num_scenes; i++)
    {
        len = strlen(scene_list[i]);
        //printf("%s\n",scene_list[i]);
        strncpy(temp_string, scene_list[i]+6, 7);
        yeardoy[i] = atoi(temp_string);
        strncpy(temp_string2, scene_list[i]+6, 4);
        year = atoi(temp_string2);
        strncpy(temp_string3, scene_list[i]+10, 3);
        doy = atoi(temp_string3);

        status = convert_year_doy_to_jday_from_0000(year, doy, &sdate[i]);

        if (status != SUCCESS)
        {
            sprintf(errmsg, "Converting year %d doy %d", year, doy);
            RETURN_ERROR (errmsg, FUNC_NAME, ERROR);
        }
    }

    /******************************************************************/
    /*                                                                */
    /* Sort the scene_list & sdate based on yeardoy                   */
    /*                                                                */
    /******************************************************************/

    quick_sort(yeardoy, scene_list, sdate, 0, num_scenes - 1);

    /******************************************************************/
    /*                                                                */
    /* Free memory                                                    */
    /*                                                                */
    /******************************************************************/

    free(yeardoy);

    return (SUCCESS);

}

/******************************************************************************
MODULE:  convert_year_doy_to_jday_from_0000

PURPOSE:  convert day of year in a year to julian day counted from year 0000

RETURN VALUE: int
ERROR           Error for year less than 1973 as input
SUCCESS         No errors encountered

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
5/02/2019   Su Ye            Original Development


NOTES:
******************************************************************************/
int convert_year_doy_to_jday_from_0000
(
    int year,      /* I: year */
    int doy,       /* I: day of the year */
    int *jday      /* O: julian date since year 0000 */
)
{
    char FUNC_NAME[] = "convert_year_doy_to_jday_from_0000";
    int i;
    int status;


    if (year < PLANET_START_YEAR)
    {
        RETURN_ERROR ("Planet data starts from 2015", FUNC_NAME, ERROR);
    }

    if (year != PLANET_START_YEAR)
    {
        *jday = JULIAN_DATE_LAST_DAY_2014;
        for (i = PLANET_START_YEAR; i < year; i++)
        {
            status = is_leap_year(i);
            if (status == TRUE)
            {
                *jday += LEAP_YEAR_DAYS;
            }
            else
            {
                *jday += NON_LEAP_YEAR_DAYS;
            }
        }
    }
    *jday += doy;

    return (SUCCESS);
}


/************************************************************************
FUNCTION: is_leap_year

PURPOSE:
Test if year given is a leap year.

RETURN VALUE:
Type = int
Value    Description
-----    -----------
TRUE     the year is a leap year
FALSE    the year is NOT a leap year

**************************************************************************/
int is_leap_year
(
    int year        /*I: Year to test         */
)
{
    if (((year % 4) != 0) || (((year % 100) == 0) && ((year % 400) != 0)))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*****************************************************************************
MODULE:  create_scene_list

PURPOSE:  Create scene list from existing files under working data
          directory and pop them into scene_list string array

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error getting the command-line arguments or a command-line
                argument and associated value were not specified
SUCCESS         No errors encountered

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            Original development

NOTES:
*****************************************************************************/

int create_scene_list
(
    const char *in_path,         /* I: string of ARD image directory          */
    int *num_scenes,          /* O: number of scenes                      */
    char *scene_list_filename /* I: file name of list of scene IDs        */
)
{
    DIR *dirp;
    struct dirent *dp;        /* structure for directory entries            */
    FILE *fd;                 /* file descriptor for scene list file        */
    char FUNC_NAME[] = "create_scene_list"; /* function name for messages   */
    int scene_counter = 0;     /* to record number of scenes                */
    char scene_list_directory[MAX_STR_LEN];    /* full directory of the scene list file*/
    char tmp_string[4];
    const char hdr_string[] = "hdr";
    sprintf(scene_list_directory, "%s/%s", in_path, scene_list_filename);
    int len;

    fd = fopen(scene_list_directory, "w");
    if (fd == NULL)
    {
        RETURN_ERROR("Opening scene_list file", FUNC_NAME, ERROR);
    }

    dirp = opendir(in_path);
    if (dirp != NULL)
    {
        while ((dp = readdir(dirp)) != NULL)
        {

            if (strcmp(dp->d_name,".")!=0 && strcmp(dp->d_name,"..")!=0 && strcmp(dp->d_name,scene_list_filename)!=0)
            {
                len = strlen(dp->d_name);
                strncpy(tmp_string, dp->d_name + len - 3, 3);
                tmp_string[3] = '\0';
                if(strcmp(tmp_string, hdr_string)!=0)
                {
                    fprintf(fd, "%s\n", dp->d_name);
                    scene_counter++;
                }
            }
        }

    }
    (void) closedir(dirp);
    fclose(fd);
    *num_scenes = scene_counter;

    return (SUCCESS);


//    DIR *d= opendir(in_path);;
//    struct dirent *dir;
//    if (d)
//    {
//        int i = 0;
//        while ((dir = readdir(d)) != NULL)
//        {
//            if (i<2){
//               i++;
//            }
//            else{
//               scene_list[i-2] = dir->d_name;
//               printf("dir %s\n", dir->d_name);
//               i++;
//            }

//        }
//        *num_scenes = i - 2;
//        closedir(d);
//    }

}

/******************************************************************************
MODULE: read_envi_header

PURPOSE: Reads envi header info into input structure

RETURN VALUE:
Type = None
NOTES:
*****************************************************************************/

int read_envi_header
(
    char *in_path,       /* I: Landsat ARD directory  */
    char *scene_name,      /* I: scene name             */
    input_meta_t *meta     /* O: saved header file info */
)
{
    char  buffer[MAX_STR_LEN] = "\0"; /* for retrieving fields        */
    char  *label = NULL;              /* for retrieving string tokens */
    char  *tokenptr = NULL;           /* for retrieving string tokens */
    char  *tokenptr2 = NULL;          /* for retrieving string tokens */
    char  *seperator = "=";           /* for retrieving string tokens */
    char  *seperator2 = ",";          /* for retrieving string tokens */
    FILE *in;                         /* file ptr to input hdr file   */
    int ib;                           /* loop index                   */
    char map_info[10][MAX_STR_LEN];   /* projection information fields*/
    char FUNC_NAME[] = "read_envi_header"; /* function name           */
    char filename[MAX_STR_LEN];       /* scene name                   */
    // char directory[MAX_STR_LEN];      /* for constucting path/file names */
    // char tmpstr[MAX_STR_LEN];         /* char string for text manipulation */
    // char scene_list_name[MAX_STR_LEN];/* char string for text manipulation */


    /******************************************************************/
    /*                                                                */
    /* Determine the file name.                                       */
    /*                                                                */
    /******************************************************************/



    sprintf(filename, "%s/%s.hdr", in_path, scene_name);


    in=fopen(filename, "r");
    if (in == NULL)
    {
        RETURN_ERROR ("opening header file", FUNC_NAME, FAILURE);
    }

    /* process line by line */
    while(fgets(buffer, MAX_STR_LEN, in) != NULL)
    {

        char *s;
        s = strchr(buffer, '=');
        if (s != NULL)
        {
            /* get string token */
            tokenptr = strtok(buffer, seperator);
            label=trimwhitespace(tokenptr);

            if (strcmp(label,"lines") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
                meta->lines = atoi(tokenptr);
            }

            if (strcmp(label,"data type") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
                meta->data_type = atoi(tokenptr);
            }

            if (strcmp(label,"byte order") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
                meta->byte_order = atoi(tokenptr);
            }

            if (strcmp(label,"samples") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
                meta->samples = atoi(tokenptr);
            }

            if (strcmp(label,"interleave") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
                strcpy(meta->interleave, tokenptr);
            }

            if (strcmp(label,"UPPER_LEFT_CORNER") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
            }

            if (strcmp(label,"map info") == 0)
            {
                tokenptr = trimwhitespace(strtok(NULL, seperator));
            }

            if (strcmp(label,"map info") == 0)
            {
                tokenptr2 = strtok(tokenptr, seperator2);
                ib = 0;
                while(tokenptr2 != NULL)
                {
                    strcpy(map_info[ib], tokenptr2);
                    if (ib == 3)
                        meta->upper_left_x = atoi(map_info[ib]);
                    if (ib == 4)
                        meta->upper_left_y = atoi(map_info[ib]);
                    if (ib == 5)
                        meta->pixel_size = atoi(map_info[ib]);
                    if(ib == 7)
                        meta->utm_zone = atoi(map_info[ib]);
                    tokenptr2 = strtok(NULL, seperator2);
                    ib++;
                }
            }
        }
    }
    fclose(in);

    return (SUCCESS);
}

/******************************************************************************
MODULE: trimwhitespace

PURPOSE: Trim leading spaces of a sting

RETURN VALUE:
Type = string without trailing space

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/16/2015    Su Ye            Modified from online code

NOTES:
*****************************************************************************/
char *trimwhitespace(char *str)
{
  char *end;

  /* Trim leading space */
  while(isspace(*str)) str++;

  if(*str == 0)
    return str;

  /* Trim trailing space */
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  /* Write new null terminator */
  *(end+1) = 0;

  return str;
}


/******************************************************************************
MODULE: read_bip_lines

PURPOSE: reading bip images by line

RETURN VALUE:
Type = success or fail

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
05/02/2019    Su Ye            Modified from online code

NOTES: it always read the next line for a open file
NOTES: today I was inspired by a coding pyschologist. Z.Q.C
*****************************************************************************/

int read_bip_lines
(
    FILE **f_bip,            /* I/O: file pointer array for BIP  file names */
    int  num_samples,         /* I:   number of image samples (X width)      */
    int  num_scenes,      /* I:   current num. in list of scenes to read */
    int *sdate,              /* I:   Original array of julian date values         */
    short int  **image_buf,          /* O:   pointer to a scanline for 2-D image band values array */
    int *valid_scene_count,           /* I/O: x/y is not always valid for gridded data,  */
    int **updated_sdate_array,         /* I/O: new buf of valid date values for each pixel */
    int cur_row
)
{
    int  i, j, k;                     /* band loop counter.                   */
    char errmsg[MAX_STR_LEN];   /* for printing error text to the log.  */
    short int *tmp_buf;
    char FUNC_NAME[] ="read_bip_lines";

    tmp_buf = malloc(sizeof(short int) * TOTAL_BANDS);

    /******************************************************************/
    /*                                                                */
    /* Read the image bands for this scene.                           */
    /*                                                                */
    /******************************************************************/

     /* read quality band, and check if the pixel is valid */

     for (i = 0; i < num_scenes; i++)
     {

         for(k = 0; k < num_samples; k++)
         {
             if (read_raw_binary(f_bip[i], 1, TOTAL_BANDS,
                                 sizeof(short int), tmp_buf) != 0)
             {
                 sprintf(errmsg, "error reading %d scene, %d row, %d col\n", i, cur_row, k);
                 RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
             }

             // if it is a valid pixel
             if ((tmp_buf[TOTAL_BANDS - 1] < MASK_FILL) && (tmp_buf[0]!= IMAGE_FILL))
             {
                 for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
                 {
                     //printf("%d\n", valid_scene_count[k]);
                     image_buf[j][k * num_scenes + valid_scene_count[k]] = tmp_buf[j];
                 }
                 updated_sdate_array[k][valid_scene_count[k]] = sdate[i];
                 valid_scene_count[k] = valid_scene_count[k] + 1;
             }
         }
     }

    free(tmp_buf);


    return (SUCCESS);

}

/******************************************************************************
MODULE: read_bip_lines

PURPOSE: reading bip images by line

RETURN VALUE:
Type = success or fail

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
05/27/2019    Su Ye            Modified from online code

******************************************************************************/
int read_bip
(
    char *in_path,       /* I: Landsat ARD directory  */
    char **scene_list,   /* I:   current scene name in list of sceneIDs       */
    FILE **fp_bip,            /* I/O: file pointer array for BIP  file names */
    int  curr_scene_num,      /* I:   current num. in list of scenes to read */
    int  row,                 /* I:   the row (Y) location within img/grid   */
    int  col,                 /* I:   the col (X) location within img/grid   */
    int  num_samples,         /* I:   number of image samples (X width)      */
    int  *sdate,              /* I:   Original array of julian date values         */
    short int  **image_buf,          /* I/O:   pointer to 2-D image band values array */
    short int  *fmask_buf,            /* I/O:   pointer to 1-D mask array */
    int  *valid_scene_count,   /* I/O: x/y is not always valid for gridded data,  */
    char **valid_scene_list,  /* I/O: 2-D array for list of filtered            */
    int  *updated_sdate_array /* I/O: new buf of valid date values            */
)
{

    int  k;                     /* band loop counter.                   */
    char filename[MAX_STR_LEN]; /* file name constructed from sceneID   */
    // char shorter_name[MAX_STR_LEN]; /* file name constructed from sceneID*/
    // char directory[MAX_STR_LEN];
    // char tmpstr[MAX_STR_LEN];   /* for string manipulation              */
    char errmsg[MAX_STR_LEN];   /* for printing error text to the log.  */
    char curr_scene_name[MAX_STR_LEN]; /* current scene name */
    short int* qa_val;           /* qa value*/
    bool debug = FALSE;          /* for debug printing                   */
    char FUNC_NAME[] = "read_bip"; /* function name */
    short int *tmp_buf;
    int j;

    tmp_buf = malloc(sizeof(short int) * TOTAL_BANDS);
    /******************************************************************/
    /*                                                                */
    /* Determine the BIP file name, open, fseek.                      */
    /*                                                                */
    /******************************************************************/
    sprintf(curr_scene_name, "%s", scene_list[curr_scene_num]);


    sprintf(filename, "%s/%s", in_path, curr_scene_name);

    fp_bip[curr_scene_num] = open_raw_binary(filename,"rb");
    if (fp_bip[curr_scene_num] == NULL)
    {
        sprintf(errmsg,  "Opening %d scene files\n", curr_scene_num);
        RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
    }



    /******************************************************************/
    /*                                                                */
    /* Read the image bands for this scene.                           */
    /*                                                                */
    /******************************************************************/

     /* read quality band, and check if the pixel is valid */
     fseek(fp_bip[curr_scene_num], (((row - 1)* num_samples + col - 1) *
         TOTAL_BANDS + TOTAL_BANDS - 1) * sizeof(short int), SEEK_SET);

     qa_val = malloc(sizeof(short int));
     read_raw_binary(fp_bip[curr_scene_num], 1, 1,
                            sizeof(short int), qa_val);

     if (*qa_val < MASK_FILL)
     {
         fseek(fp_bip[curr_scene_num], ((row - 1)* num_samples + col - 1) *
                       TOTAL_BANDS * sizeof(short int), SEEK_SET);

         if (read_raw_binary(fp_bip[curr_scene_num], 1, TOTAL_BANDS,
                             sizeof(short int), tmp_buf) != 0)
         {
             sprintf(errmsg, "error reading %d scene, %d row, %d col\n", curr_scene_num, row, col);
             RETURN_ERROR(errmsg, FUNC_NAME, ERROR);
         }
         if ((tmp_buf[TOTAL_BANDS - 1] < MASK_FILL) && (tmp_buf[0]!= IMAGE_FILL))
         {
             //printf("%d\n", tmp_buf[0]);
             for(j = 0; j < TOTAL_IMAGE_BANDS; j++)
             {
                 //printf("%d\n", valid_scene_count[k]);
                 image_buf[j][*valid_scene_count] = tmp_buf[j];
             }
             fmask_buf[*valid_scene_count] = (short int)(*qa_val);
             strcpy(valid_scene_list[*valid_scene_count], scene_list[curr_scene_num]);
             updated_sdate_array[*valid_scene_count] = sdate[curr_scene_num];
             (*valid_scene_count)++;
         }
    }
    free(tmp_buf);
    free(qa_val);

    close_raw_binary(fp_bip[curr_scene_num]);

    return (SUCCESS);
}

FILE *open_raw_binary
(
    char *infile,        /* I: name of the input file to be opened */
    char *access_type    /* I: string for the access type for reading the
                               input file; use the raw_binary_format
                               array at the top of this file */
)
{
    FILE *rb_fptr = NULL;    /* pointer to the raw binary file */
    char FUNC_NAME[] = "open_raw_binary"; /* function name */

    /* Open the file with the specified access type */
    rb_fptr = fopen (infile, access_type);
    if (rb_fptr == NULL)
    {
         ERROR_MESSAGE("Opening raw binary", FUNC_NAME);
     return NULL;
    }

    /* Return the file pointer */
    return rb_fptr;
}


void close_raw_binary
(
    FILE *fptr      /* I: pointer to raw binary file to be closed */
)
{
    fclose (fptr);
}

int read_raw_binary
(
    FILE *rb_fptr,      /* I: pointer to the raw binary file */
    int nlines,         /* I: number of lines to read from the file */
    int nsamps,         /* I: number of samples to read from the file */
    int size,           /* I: number of bytes per pixel (ex. sizeof(uint8)) */
    void *img_array     /* O: array of nlines * nsamps * size to be read from
                              the raw binary file (sufficient space should
                              already have been allocated) */
)
{
    int nvals;               /* number of values read from the file */
    char FUNC_NAME[] = "read_raw_binary"; /* function name */

    /* Read the data from the raw binary file */
    nvals = fread (img_array, size, nlines * nsamps, rb_fptr);
    if (nvals != nlines * nsamps)
    {
        RETURN_ERROR("Incorrect amount of data read", FUNC_NAME, ERROR);
    }

    return (SUCCESS);
}
