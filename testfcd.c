/***************************************************************************
 *  This file is part of libfcd
 * 
 *  Copyright (C) 2010  Howard Long, G6LVB
 *  Copyright (C) 2011  Alexandru Csete, OZ9AEC
 *                      Mario Lorenz, DL5MLO
 *  Copyright (C) 2012  John Brzustowski
 * 
 *  libfcd is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libfcd is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libfcd.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

/* 

   test of simple fcd control


   For fcd pro +, the windows gui sends out these commands when loaded and
   a simple freq setting is performed.

   GET_RF_FILTER
   GET_IF_FILTER
   GET_BIAS_TEE
   GET_LNA_GAIN
   GET_MIXER_GAIN
   GET_IF_GAIN
   GET_FREQUENCY_HZ

*/
#define FCD_QUERY               1
#define FCD_SET_FREQUENCY_HZ    101
#define FCD_GET_RF_FILTER       153
#define FCD_GET_IF_FILTER       162
#define FCD_GET_BIAS_TEE        166
#define FCD_GET_LNA_GAIN        150
#define FCD_GET_MIXER_GAIN      154
#define FCD_GET_IF_GAIN	        157
#define FCD_GET_FREQUENCY_HZ    102




#include "libfcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    fcdDesc fcd;
    uint32_t parval;

    if (FCD_RETCODE_OKAY != fcdOpen(&fcd, 0, 0, "/dev/hidraw1")) {
      puts("couldn't open");
      exit(1);
    }
    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_RF_FILTER, &parval, 1)) {
      puts("Failed: FCD_GET_RF_FILTER\n");
    }
    printf("FCD_GET_RF_FILTER Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_IF_FILTER, &parval, 1)) {
      puts("Failed: FCD_GET_IF_FILTER\n");
    }
    printf("FCD_GET_IF_FILTER Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_BIAS_TEE, &parval, 1)) {
      puts("Failed: FCD_GET_BIAS_TEE\n");
    }
    printf("FCD_GET_BIAS_TEE Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_LNA_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_LNA_GAIN\n");
    }
    printf("FCD_GET_LNA_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_MIXER_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_MIXER_GAIN\n");
    }
    printf("FCD_GET_MIXER_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_IF_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_IF_GAIN\n");
    }
    printf("FCD_GET_IF_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_FREQUENCY_HZ, &parval, 4)) {
      puts("Failed: FCD_GET_FREQUENCY_HZ\n");
    }
    printf("FCD_GET_FREQUENCY_HZ Got %d\n", parval);

    uint8_t version[60];
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_QUERY, &version, 60)) {
      puts("Failed: FCD_QUERY\n");
    }
    version[59] = 0;
    printf("FCD_QUERY Got %s\n", version);

    fcdClose(&fcd);
    if (FCD_RETCODE_OKAY != fcdOpen(&fcd, 0, 0, "/dev/hidraw0")) {
      puts("couldn't open");
      exit(1);
    }
    parval = 166380000;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppSetParam(&fcd, FCD_SET_FREQUENCY_HZ, &parval, 4)) {
      puts("Failed: FCD_SET_FREQUENCY_HZ\n");
    }
    printf("FCD_SET_FREQUENCY_HZ Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_FREQUENCY_HZ, (uint8_t * ) &parval, 4)) {
      puts("Failed: FCD_GET_FREQUENCY_HZ\n");
    }
    printf("FCD_GET_FREQUENCY_HZ Got %d\n", parval);

    parval = 0;
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_RF_FILTER, &parval, 1)) {
      puts("Failed: FCD_GET_RF_FILTER\n");
    }
    printf("FCD_GET_RF_FILTER Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_IF_FILTER, &parval, 1)) {
      puts("Failed: FCD_GET_IF_FILTER\n");
    }
    printf("FCD_GET_IF_FILTER Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_BIAS_TEE, &parval, 1)) {
      puts("Failed: FCD_GET_BIAS_TEE\n");
    }
    printf("FCD_GET_BIAS_TEE Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_LNA_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_LNA_GAIN\n");
    }
    printf("FCD_GET_LNA_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_MIXER_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_MIXER_GAIN\n");
    }
    printf("FCD_GET_MIXER_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_IF_GAIN, &parval, 1)) {
      puts("Failed: FCD_GET_IF_GAIN\n");
    }
    printf("FCD_GET_IF_GAIN Got %d\n", parval);

    parval = 0;
    usleep(10000);
    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_GET_FREQUENCY_HZ, &parval, 4)) {
      puts("Failed: FCD_GET_FREQUENCY_HZ\n");
    }
    printf("FCD_GET_FREQUENCY_HZ Got %d\n", parval);

    fcdClose(&fcd);
    exit(0);
}
