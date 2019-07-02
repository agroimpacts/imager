#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>


#define LOG_MESSAGE(message, module) \
            write_message((message), (module), "INFO", \
                          __FILE__, __LINE__, stdout);


#define WARNING_MESSAGE(message, module) \
            write_message((message), (module), "WARNING", \
                          __FILE__, __LINE__, stdout);


#define ERROR_MESSAGE(message, module) \
            write_message((message), (module), "ERROR", \
                          __FILE__, __LINE__, stdout);


#define RETURN_ERROR(message, module, status) \
           {write_message((message), (module), "ERROR", \
                          __FILE__, __LINE__, stdout); \
            return (status);}

void write_message
(
    const char *message, /* I: message to write to the log */
    const char *module,  /* I: module the message is from */
    const char *type,    /* I: type of the error */
    char *file,          /* I: file the message was generated in */
    int line,            /* I: line number in the file where the message was
                               generated */
    FILE * fd            /* I: where to write the log message */
);

int partition
(
    int arr[],
    char *brr[],
    int crr[],
    int left,
    int right
);

int partition_float
(
    float arr[],
    int left,
    int right
);

int partition_shortint
(
    short int arr[],
    int left,
    int right
);

void quick_sort
(
    int arr[],
    char *brr[],
    int crr[],
    int left,
    int right
);

void quick_sort_float
(
    float arr[],
    int left,
    int right
);

void quick_sort_shortint
(
    short int arr[],
    int left,
    int right
);

int get_args
(
    int argc,              /* I: number of cmd-line args                    */
    char *argv[],          /* I: string of cmd-line args                    */
    char *in_path,         /* O: directory locaiton for input data          */
    char *out_path,        /* O: directory location for output files        */
    int *grid,             /* O: outputted grid number        */
    int *lower_ordinal,       /* O: lower bound for compositing window        */
    int *upper_ordinal,     /* O: upper bound for compositing window    */
    int *mode,               /* O: the mode                */
    int *row,
    int *col,
    int *method
);

void quick_sort_shortint_index(short int arr[], int index_list[], int left, int right);

int partition_shortint_index (short int arr[], int index[], int left, int right);

#endif /* UTILITIES_H */

