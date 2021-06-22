#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

#include "utilities.h"
#include "const.h"

/*****************************************************************************
  NAME:  write_message

  PURPOSE:  Writes a formatted log message to the specified file handle.

  RETURN VALUE:  None

  NOTES:
      - Log Message Format:
            yyyy-mm-dd HH:mm:ss pid:module [filename]:line message
*****************************************************************************/

void write_message
(
    const char *message, /* I: message to write to the log */
    const char *module,  /* I: module the message is from */
    const char *type,    /* I: type of the error */
    char *file,          /* I: file the message was generated in */
    int line,            /* I: line number in the file where the message was
                               generated */
    FILE *fd             /* I: where to write the log message */
)
{
    time_t current_time;
    struct tm *time_info;
    int year;
    pid_t pid;

    time (&current_time);
    time_info = localtime (&current_time);
    year = time_info->tm_year + 1900;

    pid = getpid ();

    fprintf (fd, "%04d:%02d:%02d %02d:%02d:%02d %d:%s [%s]:%d [%s]:%s\n",
             year,
             time_info->tm_mon,
             time_info->tm_mday,
             time_info->tm_hour,
             time_info->tm_min,
             time_info->tm_sec,
             pid, module, basename (file), line, type, message);
}


/******************************************************************************
MODULE:  quick_sort

PURPOSE:  sort the scene_list & sdate based on yeardoy string

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            original development

NOTES:
******************************************************************************/
void quick_sort (int arr[], char *brr[], int crr[], int left, int right)
{
    int index = partition (arr, brr, crr, left, right);

    if (left < index - 1)
    {
        quick_sort (arr, brr, crr, left, index - 1);
    }
    if (index < right)
    {
        quick_sort (arr, brr, crr, index, right);
    }
}

/******************************************************************************
MODULE:  quick_sort_float

PURPOSE:  sort the scene_list based on yeardoy string

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            original development

NOTES:
******************************************************************************/
void quick_sort_float(float arr[], int left, int right)
{
    int index = partition_float (arr, left, right);

    if (left < index - 1)
    {
        quick_sort_float (arr, left, index - 1);
    }
    if (index < right)
    {
        quick_sort_float (arr, index, right);
    }
}

/******************************************************************************
MODULE:  quick_sort_int

PURPOSE:  sort the scene_list based on yeardoy string

RETURN VALUE: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
06/12/2019  Su Ye            original development

NOTES:
******************************************************************************/
void quick_sort_shortint(short int arr[], int left, int right)
{
    int index = partition_shortint (arr, left, right);

    if (left < index - 1)
    {
        quick_sort_shortint (arr, left, index - 1);
    }
    if (index < right)
    {
        quick_sort_shortint (arr, index, right);
    }
}


void quick_sort_shortint_index(short int arr[], int index_list[], int left, int right)
{
    int index = partition_shortint_index (arr, index_list, left, right);

    if (left < index - 1)
    {
        quick_sort_shortint_index (arr, index_list, left, index - 1);
    }
    if (index < right)
    {
        quick_sort_shortint_index (arr, index_list, index, right);
    }
}

/******************************************************************************
MODULE:  partition

PURPOSE:  partition used for the quick_sort routine

RETURN VALUE:
Type = int
Value           Description
-----           -----------
i               partitioned value

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            original development

NOTES:
******************************************************************************/
int partition (int arr[], char *brr[], int crr[], int left, int right)
{
    int i = left, j = right;
    int tmp, tmp2;
    char temp[MAX_STR_LEN];
    int pivot = arr[(left + right) / 2];

    while (i <= j)
    {
        while (arr[i] < pivot)
    {
            i++;
    }
        while (arr[j] > pivot)
    {
            j--;
    }
        if (i <= j)
        {
            tmp = arr[i];
            strcpy(&temp[0], brr[i]);
            tmp2 = crr[i];
            arr[i] = arr[j];
            strcpy(brr[i], brr[j]);
            crr[i] = crr[j];
            arr[j] = tmp;
            strcpy(brr[j],&temp[0]);
            crr[j] = tmp2;
            i++;
            j--;
        }
    }

    return i;
}

/******************************************************************************
MODULE:  partition_float

PURPOSE:  partition the sorted list

RETURN VALUE:
Type = int
Value           Description
-----           -----------
i               partitioned value

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
int partition_float (float arr[], int left, int right)
{
    int i = left, j = right;
    float tmp;
    float pivot = arr[(left + right) / 2];

    while (i <= j)
    {
        while (arr[i] < pivot)
    {
            i++;
    }
        while (arr[j] > pivot)
    {
            j--;
    }
        if (i <= j)
        {
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i++;
            j--;
        }
    }

    return i;
}


/******************************************************************************
MODULE:  partition_shortint

PURPOSE:  partition the sorted list

RETURN VALUE:
Type = int
Value           Description
-----           -----------
i               partitioned value

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019   Su Ye         Original Development

NOTES:
******************************************************************************/
int partition_shortint (short int arr[], int left, int right)
{
    int i = left, j = right;
    short int tmp;
    short int pivot = (short int) arr[(left + right) / 2];

    while (i <= j)
    {
        while (arr[i] < pivot)
    {
            i++;
    }
        while (arr[j] > pivot)
    {
            j--;
    }
        if (i <= j)
        {
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i++;
            j--;
        }
    }

    return i;
}

int partition_shortint_index (short int arr[], int index[], int left, int right)
{
    int i = left, j = right;
    short int tmp;
    short int pivot = (short int) arr[(left + right) / 2];

    while (i <= j)
    {
        while (arr[i] < pivot)
    {
            i++;
    }
        while (arr[j] > pivot)
    {
            j--;
    }
        if (i <= j)
        {
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;

            tmp = index[i];
            index[i] = index[j];
            index[j] = tmp;

            i++;
            j--;
        }
    }

    return i;
}


/******************************************************************************
MODULE: get_args
PURPOSE:  Gets the command-line arguments and validates that the required
arguments were specified.
RETURN VALUE:
Type = int
Value           Description
-----           -----------
FAILURE         Error getting the command-line arguments or a command-line
                argument and associated value were not specified
SUCCESS         No errors encountered
HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
05/02/2019  Su Ye            original development
******************************************************************************/
int get_args
(
    int argc,              /* I: number of cmd-line args                    */
    char *argv[],          /* I: string of cmd-line args                    */
    char *in_path,         /* O: directory locaiton for input data          */
    char *out_path,        /* O: directory location for output files        */
    int *tile_id,             /* O: outputted grid number        */
    int *lower_ordinal,       /* O: lower bound for compositing window        */
    int *upper_ordinal,     /* O: upper bound for compositing window    */
    int *mode,               /* O: the mode                */
    int *row,
    int *col,
    int *method
)
{
    char cwd[MAX_STR_LEN]; // current directory path
    char var_path[MAX_STR_LEN];
    FILE *var_fp;
    char line1[MAX_STR_LEN], line2[MAX_STR_LEN], line3[MAX_STR_LEN],
            line4[MAX_STR_LEN], line5[MAX_STR_LEN], line6[MAX_STR_LEN],
            line7[MAX_STR_LEN], line8[MAX_STR_LEN], line9[MAX_STR_LEN];
    char FUNC_NAME[] = "get_args";

    // when there is no variable command-line argument,
    // use the default variable text path
    if(argc < 2)
    {
        getcwd(cwd, sizeof(cwd));
        //printf("getvariable");
        sprintf(var_path, "%s/%s", cwd, "variables");
    }
    // for production
    else if(argc == 6)
    {
        // printf("argc == 6 \n");
        strcpy(in_path, argv[1]);
        strcpy(out_path, argv[2]);
        *tile_id = atoi(argv[3]);
        *lower_ordinal = atoi(argv[4]);
        *upper_ordinal = atoi(argv[5]);
        *mode = 3; // on-production, mode =3 means that image-based processing
        *row = 0;
        *col = 0;
        *method = DEFAULT_COMPOSITING_METHOD;
        return SUCCESS;
    }
    else
    {
        RETURN_ERROR("Inputted arg parameter number has to be 0 or 5 ", FUNC_NAME, ERROR);
    }

    var_fp = fopen(var_path, "r");

    if(var_fp == NULL)
    {
        RETURN_ERROR("There is no variable file in the bin folder", FUNC_NAME, ERROR);
    }

    fscanf(var_fp, "%s\n", line1);
    strcpy(in_path, strchr(line1, '=') + 1);
    if (*in_path == '\0')
    {
        RETURN_ERROR("Variables error: 'in_path' "
                     "cannot be empty in 'Variables' file!", FUNC_NAME, ERROR);

    }

    fscanf(var_fp, "%s\n", line2);
    strcpy(out_path, strchr(line2, '=') + 1);
    if (*out_path == '\0')
    {
        RETURN_ERROR("Variables error: 'out_path' "
                     "cannot be empty in 'Variables' file!", FUNC_NAME, ERROR);
    }

    fscanf(var_fp, "%s\n", line3);
    *tile_id = atoi(strchr(line3, '=') + 1);

    fscanf(var_fp, "%s\n", line4);
    *lower_ordinal = atoi(strchr(line4, '=') + 1);

    fscanf(var_fp, "%s\n", line5);
    *upper_ordinal = atoi(strchr(line5, '=') + 1);

    fscanf(var_fp, "%s\n", line6);
    *mode = atoi(strchr(line6, '=') + 1);

    fscanf(var_fp, "%s\n", line7);
    *row = atoi(strchr(line7, '=') + 1);

    fscanf(var_fp, "%s\n", line8);
    *col = atoi(strchr(line8, '=') + 1);

    fscanf(var_fp, "%s\n", line9);
    *method = atoi(strchr(line9, '=') + 1);


    return SUCCESS;

}
