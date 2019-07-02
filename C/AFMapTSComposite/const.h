#ifndef CONST_H
#define CONST_H

#ifndef SUCCESS
    #define SUCCESS  0
#endif

#ifndef ERROR
    #define ERROR -1
#endif


#ifndef FAILURE
    #define FAILURE 1
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define JULIAN_DATE_LAST_DAY_2014 735598
#define PLANET_START_YEAR 2015
#define LEAP_YEAR_DAYS 366
#define NON_LEAP_YEAR_DAYS 365
#define AVE_DAYS_IN_A_YEAR 365.25
#define IMAGE_FILL -9999

#define MAX_STR_LEN 512
#define MAX_SCENE_LIST 3922
#define ARD_STR_LEN 100

#define TOTAL_BANDS 5
#define TOTAL_IMAGE_BANDS 4
#define MASK_FILL 10000

#define BLUE_INDEX 0
#define GREEN_INDEX 1
#define RED_INDEX 2
#define NIR_INDEX 3

#define PLANET_RES 3
#define ROBUST_COEFFS 2

/* from condition of output_t*/
#define NORMAL_CONDITION 0
#define NOOBS_CONDITION 1
#define INEFFICIENT_CONDITION 2

#define T_CONST_SINGLETAIL_99 2.32      /* Threshold for cloud, shadow, and snow detection (0.999) */
#define T_CONST_SINGLETAIL_999 3.09      /* Threshold for cloud, shadow, and snow detection (0.999) */
#define T_CONST_SINGLETAIL_9999 3.71      /* Threshold for cloud, shadow, and snow detection (0.9999) */
#define T_CONST_DOUBLETAIL_99 2.58
#define MAX_NUM_OUTLIERS 90
#define MIN_SAMPLE 5

//#define PENALTY_INTERCEPT 0.028 /* 2 variogram = 1/100; 3 variogram = 1/1000*/
//#define PENALTY_INTERCEPT 1.99 /* variogram = 1; 2 variogram = 1/100*/
#define PENALTY_INTERCEPT 1.99 /* 1.5variogram = 1; 3 variogram = 1/100*/

#define penalty_scale_1 0.05
#define penalty_scale_2 0.005
#define penalty_scale_3 0.0005


#define DRY_INTERVAL 45
#define RAINY_INTERVAL 75

#define DEFAULT_COMPOSITING_METHOD 6
/* from 2darray.c */
/* Define a unique (i.e. random) value that can be used to verify a pointer
   points to an LSRD_2D_ARRAY. This is used to verify the operation succeeds to
   get an LSRD_2D_ARRAY pointer from a row pointer. */
#define SIGNATURE 0x326589ab

/* Given an address returned by the allocate routine, get a pointer to the
   entire structure. */
#define GET_ARRAY_STRUCTURE_FROM_PTR(ptr) \
    ((LSRD_2D_ARRAY *)((char *)(ptr) - offsetof(LSRD_2D_ARRAY, memory_block)))

#endif // CONST_H
