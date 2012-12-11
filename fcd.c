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

   simple fcd control

*/

#include "libfcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

typedef enum {
  OPT_NONE,
  OPT_SERNUM	   = 'n',
  OPT_USB_PATH     = 'p',
  OPT_ENUMNUM	   = 'e',
  OPT_LIST	   = 'l',
  OPT_SET_DEFAULTS = 'd',
  OPT_GET_FREQ	   = 'g',
  OPT_SET_FREQ	   = 's',
  OPT_SET_FREQ_KHZ = 'k',
  OPT_SET_PARAMS   = 'w',
  OPT_GET_PARAMS   = 'r',
  OPT_QUIET        = 'q'
} cmds_t;

int
main(int argc, char **argv)
{
    int c;
    cmds_t command = OPT_NONE;
    int serialNum = 0;
    int enumNum = 0;
    int quiet = 0;
    uint32_t freq;
    fcdDesc fcd;
    int devOpen = 0;
    char *usbPath = 0;

    static struct option long_options[] = {
	{"serialnum",  1, 0, OPT_SERNUM},
	{"path",       1, 0, OPT_USB_PATH},
	{"enumnum",    1, 0, OPT_ENUMNUM},
	{"list",       0, 0, OPT_LIST},
	{"defaults",   0, 0, OPT_SET_DEFAULTS},
	{"getfreq",    0, 0, OPT_GET_FREQ},
	{"setfreq",    1, 0, OPT_SET_FREQ},
	{"setfreqkHz", 1, 0, OPT_SET_FREQ_KHZ},
	{"setparams",  0, 0, OPT_SET_PARAMS},
	{"getparams",  0, 0, OPT_SET_PARAMS},
	{"quiet",      0, 0, OPT_QUIET},
	{0, 0, 0, 0}
    };

    int have_opt = 0;

    for (;;) {

      c = getopt_long(argc, argv, "e:p:n:ldgs:k:wrq",
		      long_options, NULL);
      if (c == -1 && have_opt)
	break;

      have_opt = 1;
      switch (c) {
      case OPT_LIST:
	command = OPT_LIST;
	break;

      case OPT_SET_DEFAULTS:
	command = OPT_SET_DEFAULTS;
	break;

      case OPT_SERNUM:
	serialNum = atoi(optarg);
	break;

      case OPT_USB_PATH:
	usbPath = optarg;
	break;

      case OPT_ENUMNUM:
	enumNum = atoi(optarg);
	break;

      case OPT_SET_FREQ:
	freq = atoi(optarg);
	command = OPT_SET_FREQ;
	break;

      case OPT_SET_FREQ_KHZ:
	freq = atoi(optarg);
	command = OPT_SET_FREQ_KHZ;
	break;

      case OPT_GET_FREQ:
	command = OPT_GET_FREQ;
	break;

      case OPT_SET_PARAMS:
	command = OPT_SET_PARAMS;
	break;

      case OPT_GET_PARAMS:
	command = OPT_GET_PARAMS;
	break;

      case OPT_QUIET:
	quiet = 1;
	continue;
	break;

      default:
	printf("\nUsage: \n\n"
	       "fcd -l   - list available funcube devices\n"
	       "fcd [-q] [DEVSPEC] -d - set default parameters\n"
	       "fcd [DEVSPEC] -g - get and print current frequency\n"
	       "fcd [-q] [DEVSPEC] -s freq_Hz - set frequency in Hz\n"
	       "fcd [-q] [DEVSPEC] -k freq_kHz - set frequency in kHz\n"
	       "fcd [DEVSPEC] -r [P1 P2 ... Pk] read and print values of parameters P1 ... Pk, or all if none specified\n"
	       "fcd [-q] [DEVSPEC] -w P1 V1 [P2 V2 ... Pk Vk] set values of parameter P1 to V1, P2 to V2, ... Pk to Vk\n"
	       "\n  e.g. fcd -p 0001:0003:02 -k 166380 -g -w 0xf 3\n\n"
	       "where [DEVSPEC] chooses a funcube like so:\n\n"
	       "   <blank>: use the first funcube found\n"
	       "   -e n: use the nth funcube found, with n=0 being the first\n"
	       "   -p <path>: use the usb path as shown by fcd -l; e.g. fcd -p 0001:001c:02\n"
	       "   -n s: use the device with serial number 's'; (NOT YET AVAILABLE IN FCD FIRMWARE)\n"
	       "Option '-q' prevents fcd from printing output.\n"
	       "Parameters P1..Pk and their values V1..Vk  are specified as integers in decimal or hex'\n"
	       "Parameters are numbered from 0 for LNA_GAIN to 15 for IF_GAIN6 - see libfcd.h.\n"
	       "Parameter values are as they appear in the ENUMs in libfcd.h\n"
	       "Values returned by the '-r' option are printed one per line as 0xX 0xY,\n"
	       "where:\n"
	       "   X is the parameter number in hex\n"
	       "   Y is the parameter value in hex\n"
	       );
	exit(1);
      }

      switch(command) {
      case OPT_LIST:
	puts("These FCDs found:");
	for (enumNum=0, serialNum=0;; ++enumNum) {
	  if (FCD_RETCODE_OKAY != fcdOpen(&fcd, serialNum, enumNum, usbPath)) {
	    break;
	  }
	  printf("Model: %12s; enum: %2d; serial: %6d; path: %s\n", fcd.pszModelName, fcd.enumNum, fcd.serialNum, fcd.pszPath);
	  fcdClose(&fcd);
	}	    
	exit(0);
	break;
      default:
	break;
      }
      if (!devOpen && FCD_RETCODE_OKAY != fcdOpen(&fcd, serialNum, enumNum, usbPath)) {
	if (!quiet) puts("Error: unable to open specified FCD.");
	exit(1);
      }
      devOpen = 1;

      switch(command) {
      case OPT_GET_FREQ:
	if (FCD_RETCODE_OKAY != fcdAppGetFreq(&fcd, &freq)) {
	  puts("Error: unable to get frequency for specified FCD.");
	  fcdClose(&fcd);
	  exit(1);
	}
	printf("%d\n", freq);
	break;
      case OPT_SET_FREQ:
	if (FCD_RETCODE_OKAY != fcdAppSetFreq(&fcd, freq)) {
	  if (!quiet) puts("Error: unable to set frequency for specified FCD.");
	  fcdClose(&fcd);
	  exit(1);
	}
	break;
      case OPT_SET_FREQ_KHZ:
	if (FCD_RETCODE_OKAY != fcdAppSetFreqkHz(&fcd, freq)) {
	  if (!quiet) puts("Error: unable to set frequency in kHz for specified FCD.");
	  fcdClose(&fcd);
	  exit(1);
	}
	break;
      case OPT_SET_DEFAULTS:
	if (FCD_RETCODE_OKAY != fcdAppSetParamDefaults(&fcd)) {
	  if (!quiet) puts("Error: unable to set default filter and gain parameters for specified FCD.");
	  fcdClose(&fcd);
	  exit(1);
	}
	if (!quiet) puts("Default gain and filtering parameters set.");
	break;
      case OPT_SET_PARAMS:
	while (optind < argc) {
	  uint8_t parno = (uint8_t) strtol(argv[optind++], 0, 0);
	  if (optind >= argc) {
	    if (!quiet) printf("Error: missing value for parameter 0x%x\n", parno);
	    exit(1);
	  }
	  uint8_t parval = (uint8_t) strtol(argv[optind++], 0, 0);
	  if (FCD_RETCODE_OKAY != fcdAppSetParam(&fcd, FCD_CMD_APP_FIRST_SET_CMD + parno, &parval, sizeof(parval))) {
	    if (!quiet) printf("Error: unable to set parameter 0x%x to value 0x%x\n", parno, parval);
	  }
	}
	fcdClose(&fcd);
	exit(EXIT_SUCCESS);
	break;

      case OPT_GET_PARAMS:
	{
	  int get_all = (optind == argc);
	  int n = get_all ? FCD_CMD_APP_NUM_PARAMS : (argc - optind);
	  int i;
	  for (i = 0; i < n; ++i) {
	    uint8_t parno = get_all ? i : (uint8_t) strtol(argv[optind++], 0, 0);
	    uint8_t parval;
	    if (FCD_RETCODE_OKAY != fcdAppGetParam(&fcd, FCD_CMD_APP_FIRST_GET_CMD + parno, &parval, sizeof(parval))) {
	      printf("Error: unable to get value of parameter 0x%x\n", parno);
	    }
	    printf("0x%x 0x%x\n", parno, parval);
	  }
	  fcdClose(&fcd);
	  exit(EXIT_SUCCESS);
	}
	break;

	  
      default:
	break;
      }
    }
    fcdClose(&fcd);
    exit(EXIT_SUCCESS);
}
