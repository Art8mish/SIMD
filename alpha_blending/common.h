#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

#include <sstream>
#include <iostream>

#include <omp.h>

#define SOFT_ASSERT(condition)                                                    \
            do                                                                    \
            {                                                                     \
                if (condition)                                                    \
                    printf("Error in %s = %d; file: %s; num of line: %d \n",      \
                           #condition, condition, __FILE__, __LINE__);            \
            } while(false)

#define ERR_CHCK(cond, error)                               \
            do                                              \
            {                                               \
                SOFT_ASSERT(cond);                          \
                if (cond)                                   \
                    return error;                           \
            } while(false)

#define FILE_ERR_CHCK(cond, error, closing_file)            \
            do                                              \
            {                                               \
                SOFT_ASSERT(cond);                          \
                if (cond)                                   \
                {                                           \
                    fclose(closing_file);                   \
                    return error;                           \
                }                                           \
            } while(false)

#define THREADS_NB omp_get_max_threads()

enum HackToolError
{
    SUCCESS                 = 0,
    ERROR_FILE_OPENING      = 1,
    ERROR_STAT              = 2,
    ERROR_CALLOC            = 3,
    ERROR_MAKE_WINDOW       = 5,
    ERROR_CALC_MANDELBROT   = 6,
    ERROR_PRINT_MANDELBROT  = 7,
    ERROR_FILE_CLOSING      = 8,
    ERROR_GET_RGB_BUF       = 9,
};

#endif //COMMON_H_INCLUDED