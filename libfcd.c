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

#define FCD
#include <string.h>
#ifdef WIN32
    #include <malloc.h>
#else
    #include <stdlib.h>
#endif
#include "libfcd.h"
#include <stdio.h>
#include <unistd.h>

const unsigned short _usVID[] = {0x04D8, 0x04D8};  /*!< USB vendor ID enumerated by FCD_MODEL_ENUM */
const unsigned short _usPID[] = {0xFB56, 0xFB31};  /*!< USB product ID enumerated by FCD_MODEL_ENUM */

char *FCD_MODEL_NAMES[] = {"FCD Pro", "FCD Pro Plus"};

#define FALSE 0
#define TRUE 1
typedef int BOOL;

#define WRAPPED_HID_READ(dev, data, length) \
  { \
     int _ii_; \
     for (_ii_= WRAPPED_HID_READ_MAX_COUNT; _ii_ > 0; --_ii_) {		\
        int rv = hid_read_timeout(dev, data, length, WRAPPED_HID_READ_SINGLE_TIMEOUT); \
        if (rv > 0) \
          break; \
        if (rv < 0) {				\
  	    fprintf (stderr, "Got error in hid_read\n");  \
            return FCD_RETCODE_ERROR; \
        } \
     } \
     if (_ii_ == 0) {		  \
  	fprintf (stderr, "Got too many timeouts in hid_read\n");  \
        return FCD_RETCODE_ERROR; \
     } \
  }

/** \brief Open FCD device.
  * \param fcd Pointer to a pre-allocated FCD device descriptor, whose fields will be filled by this function.
  * \param serialNum serial number of device to open; 0 means ignore serial number and use enumNum
  * \param enumNum enumeration number of device to open; 0 means first, 1 means 2nd, etc.
  * \param usbPath usb path of device to open
  * \return The parameter fcd, or NULL if the matching FCD was not found.
  *
  * This function looks for FCD devices connected to the computer and
  * opens the first one found that matches the target serial number,
  * or the enumNum'th one if the target serial number is 0.
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdOpen(fcdDesc *fcd, uint16_t serialNum, uint16_t enumNum, const char *usbPath)
{
  struct hid_device_info *phdi=NULL, *phdiList = NULL;
  uint16_t curEnumNum = 0;
  uint16_t curSerialNum;

  if (!fcd)
    return FCD_RETCODE_ERROR;

  fcd->phd = NULL; // assume no matching FCD found
    
  FCD_MODEL_ENUM model;
  for (model = FCD_MODEL_FIRST_MODEL; model <= FCD_MODEL_LAST_MODEL; ++model) {

    phdiList=hid_enumerate(_usVID[model], _usPID[model]);

    if (phdiList==NULL)
      continue;

    for (phdi=phdiList; phdi; phdi = phdi->next, ++curEnumNum) {
      // get curSerialNum for this enumerated device
      curSerialNum = 0;     // CHANGEME: once FCD serial number API exists; parse serial # from device name string
      if ((serialNum  > 0 && curSerialNum == serialNum) ||
	  (usbPath && 0 == strcmp(usbPath, phdi->path)) ||
	  (serialNum == 0 && usbPath == 0 && curEnumNum == enumNum))
	break;
    }
    if (!phdi) {
      hid_free_enumeration(phdiList);
      continue; // try next model
    }
	
    fcd->pszPath = strdup(phdi->path);
    fcd->pszModelName = strdup(FCD_MODEL_NAMES[model]);
    hid_free_enumeration(phdiList);
    phdi=NULL;

    if (fcd->pszPath == NULL)
      {
	return FCD_RETCODE_ERROR;
      }


    if ((fcd->phd=hid_open_path(fcd->pszPath)) == NULL)
      {
	free(fcd->pszPath);
	fcd->pszPath = NULL;
	free(fcd->pszModelName);
	fcd->pszModelName = NULL;

	return FCD_RETCODE_ERROR;
      }
    hid_set_nonblocking(fcd->phd, 0);
    fcd->serialNum = curSerialNum;
    fcd->enumNum = curEnumNum;
    fcd->model = model;
    return FCD_RETCODE_OKAY;
  }
  return FCD_RETCODE_ABSENT;
}


/** \brief Close FCD HID device.
  * \param fcd Pointer to an FCD device descriptor 
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdClose(fcdDesc *fcd)
{
    if (! (fcd && fcd->phd))
	return FCD_RETCODE_ABSENT;

    if (fcd->phd) {
	hid_close(fcd->phd);
	fcd->phd = NULL;
    }
    if (fcd->pszPath) {
	free(fcd->pszPath);
	fcd->pszPath = NULL;
    }
    return FCD_RETCODE_OKAY;

}


/** \brief Get FCD mode.
  * \param fcd Pointer to an FCD device descriptor 
  * \param pMode Pointer to a pre-allocated FCD_MODE_ENUM field. in which the current mode is returned.
  * \return Return code.
  * \sa FCD_RETCODE_ENUM
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdGetMode(fcdDesc *fcd, FCD_MODE_ENUM *pMode)
{
    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];

    if (!(fcd && fcd->phd))
	return FCD_RETCODE_ABSENT;

    if (!pMode)
	return FCD_RETCODE_ERROR;

    /* Send a BL Query Command */
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_QUERY;
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    /* first check status bytes then check which mode */
    if (aucBufIn[0]==FCD_CMD_BL_QUERY && aucBufIn[1]==1) {

        /* In bootloader mode we have the string "FCDBL" starting at acBufIn[2] **/
        if (strncmp((char *)(aucBufIn+2), "FCDBL", 5) == 0) {
            *pMode = FCD_MODE_BL;
        }
        /* In application mode we have "FCDAPP_18.06" where the number is the FW version */
        else if (strncmp((char *)(aucBufIn+2), "FCDAPP", 6) == 0) {
            *pMode = FCD_MODE_APP;
        }
        /* either no FCD or firmware less than 18f */
        else {
            return FCD_RETCODE_ERROR;
        }
    }

    return FCD_RETCODE_OKAY;
}


/** \brief Get FCD firmware version as string.
  * \param fcd Pointer to an FCD device descriptor 
  * \param str The returned vesion number as a 0 terminated string (must be pre-allocated)
  * \return The current FCD mode.
  * \sa FCD_RETCODE_ENUM
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdGetFwVerStr(fcdDesc *fcd, char *str)
{
    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];


    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    if (!str)
	return FCD_RETCODE_ERROR;

    /* Send a BL Query Command */
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_QUERY;
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    /* first check status bytes then check which mode */
    if (aucBufIn[0]==FCD_CMD_BL_QUERY && aucBufIn[1]==1) {

        if (strncmp((char *)(aucBufIn+2), "FCDAPP", 6) == 0) {
            strncpy(str, (char *)(aucBufIn+9), 5);
            str[5] = 0;
            return FCD_RETCODE_OKAY;
        }
    }

    return FCD_RETCODE_ERROR;

}

/** \brief Reset FCD to bootloader mode.
  * \param fcd Pointer to an FCD device descriptor 
  * \return FCD_MODE_NONE
  *
  * This function is used to switch the FCD into bootloader mode in which
  * various firmware operations can be performed.
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppReset(fcdDesc *fcd)
{
    //unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    // Send an App reset command
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_APP_RESET;
    hid_write(fcd->phd, aucBufOut, 65);

    /** FIXME: hid_read() will occasionally hang due to a pthread_cond_wait() never returning.
        It seems that the read_callback() in hid-libusb.c will never receive any
        data during the reconfiguration. Since the same logic works in the native
        windows application, it could be a libusb thing. Anyhow, since the value
        returned by this function is not used, we may as well just skip the hid_read()
        and return FME_NONE.
        Correct switch from APP to BL mode can be observed in /var/log/messages (linux)
        (when in bootloader mode the device version includes 'BL')
    */
    /*
    memset(aucBufIn,0xCC,65); // Clear out the response buffer
    hid_read(fcd->phd,aucBufIn,65);

    if (aucBufIn[0]==FCDCMDAPPRESET && aucBufIn[1]==1)
    {
        return FME_APP;
    }
    return FME_BL;
    */

    return FCD_RETCODE_OKAY;

}


/** \brief Set FCD frequency with kHz resolution.
  * \param fcd Pointer to an FCD device descriptor 
  * \param nFreq The new frequency in kHz.
  * \return the return code.
  *
  * This function sets the frequency of the FCD with 1 kHz resolution. The parameter
  * nFreq must already contain any necessary frequency correction.
  *
  * \sa fcdAppSetFreq
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppSetFreqkHz(fcdDesc *fcd, int nFreq)
{
    return fcdAppSetParam(fcd, FCD_CMD_APP_SET_FREQ_KHZ, (uint8_t *) &nFreq, 3);
}

/** \brief Set FCD frequency with Hz resolution.
  * \param fcd Pointer to an FCD device descriptor 
  * \param nFreq The new frequency in Hz.
  * \return the return code.
  *
  * This function sets the frequency of the FCD with 1 Hz resolution. The parameter
  * nFreq must already contain any necessary frequency correction.
  *
  * \sa fcdAppSetFreqkHz
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppSetFreq(fcdDesc *fcd, uint32_t nFreq)
{
    return fcdAppSetParam(fcd, FCD_CMD_APP_SET_FREQ_HZ, (uint8_t *) &nFreq, 4);

}


/** \brief Get FCD frequency with Hz resolution.
  * \param fcd Pointer to an FCD device descriptor 
  * \param nFreq Pointer to a preallocated loation for storing the current frequency in Hz.
  * \return the return code.
  *
  * This function obtains the current frequency of the FCD with 1 Hz resolution.  The frequency 
  * returned includes any necessary frequency correction, and so is not the "true" frequency. 
  *
  * \sa fcdAppSetFreqHz
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppGetFreq(fcdDesc *fcd, uint32_t *nFreq)
{  
    if (!nFreq) {
        return FCD_RETCODE_ERROR;
    }

    return fcdAppGetParam(fcd, FCD_CMD_APP_GET_FREQ_HZ, (uint8_t *) nFreq, 4);

}


/** \brief Reset FCD to application mode.
  * \param fcd Pointer to an FCD device descriptor 
  * \return FCD_MODE_NONE
  *
  * This function is used to switch the FCD from bootloader mode
  * into application mode.
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdBlReset(fcdDesc *fcd)
{
//    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    // Send an BL reset command
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_RESET;
    hid_write(fcd->phd, aucBufOut, 65);

    /** FIXME: hid_read() will hang due to a pthread_cond_wait() never returning.
        It seems that the read_callback() in hid-libusb.c will never receive any
        data during the reconfiguration. Since the same logic works in the native
        windows application, it could be a libusb thing. Anyhow, since the value
        returned by this function is not used, we may as well jsut skip the hid_read()
        and return FME_NONE.
        Correct switch from BL to APP mode can be observed in /var/log/messages (linux)
        (when in bootloader mode the device version includes 'BL')
    */
    /*
    memset(aucBufIn,0xCC,65); // Clear out the response buffer
    hid_read(fcd->phd,aucBufIn,65);

    if (aucBufIn[0]==FCDCMDBLRESET && aucBufIn[1]==1)
    {
        return FME_BL;
    }
    return FME_APP;
    */

    return FCD_RETCODE_OKAY;

}


/** \brief Erase firmware from FCD.
  * \param fcd Pointer to an FCD device descriptor 
  * \return the return code
  *
  * This function deletes the firmware from the FCD. This is required
  * before writing new firmware into the FCD.
  *
  * \sa fcdBlWriteFirmware
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdBlErase(fcdDesc *fcd)
{
    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    // Send an App reset command
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_ERASE;
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    if (aucBufIn[0]==FCD_CMD_BL_ERASE && aucBufIn[1]==1)
        return FCD_RETCODE_OKAY;

    return FCD_RETCODE_ERROR;
}


/** \brief Write new firmware into the FCD.
  * \param fcd Pointer to an FCD device descriptor 
  * \param pc Pointer to the new firmware data
  * \param n64size The number of bytes in the data
  * \return the return code
  *
  * This function is used to upload new firmware into the FCD flash.
  *
  * \sa fcdBlErase
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdBlWriteFirmware(fcdDesc *fcd, char *pc, int64_t n64Size)
{
    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];
    uint32_t u32AddrStart;
    uint32_t u32AddrEnd;
    uint32_t u32Addr;
    BOOL bFinished=FALSE;

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    if (!pc)
	return FCD_RETCODE_ERROR;

    // Get the valid flash address range
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_GET_BYTE_ADDR_RANGE;
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    if (aucBufIn[0]!=FCD_CMD_BL_GET_BYTE_ADDR_RANGE || aucBufIn[1]!=1)
    {
        return FCD_RETCODE_ERROR;
    }

    u32AddrStart=
        aucBufIn[2]+
        (((uint32_t)aucBufIn[3])<<8)+
        (((uint32_t)aucBufIn[4])<<16)+
        (((uint32_t)aucBufIn[5])<<24);
    u32AddrEnd=
        aucBufIn[6]+
        (((uint32_t)aucBufIn[7])<<8)+
        (((uint32_t)aucBufIn[8])<<16)+
        (((uint32_t)aucBufIn[9])<<24);

    // Set start address for flash
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_SET_BYTE_ADDR;
    aucBufOut[2] = ((unsigned char)u32AddrStart);
    aucBufOut[3] = ((unsigned char)(u32AddrStart>>8));
    aucBufOut[4] = ((unsigned char)(u32AddrStart>>16));
    aucBufOut[5] = ((unsigned char)(u32AddrStart>>24));
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    if (aucBufIn[0]!=FCD_CMD_BL_SET_BYTE_ADDR || aucBufIn[1]!=1)
    {
        return FCD_RETCODE_ERROR;
    }

    // Write blocks
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_WRITE_FLASH_BLOCK;
    for (u32Addr=u32AddrStart; u32Addr+47<u32AddrEnd && u32Addr+47<n64Size && !bFinished; u32Addr+=48)
    {
        memcpy(&aucBufOut[3], &pc[u32Addr], 48);

        hid_write(fcd->phd, aucBufOut, 65);
        memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
        WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

        if (aucBufIn[0]!=FCD_CMD_BL_WRITE_FLASH_BLOCK || aucBufIn[1]!=1)
        {
            bFinished = TRUE;
            return FCD_RETCODE_ERROR;
        }
    }

    return FCD_RETCODE_OKAY;
}


/** \brief Verify firmware in FCd flash.
  * \param fcd Pointer to an FCD device descriptor 
  * \param pc Pointer to firmware data to verify against.
  * \param n64Size Size of the data in pc.
  * \return The FCD_MODE_BL if verification was succesful.
  *
  * This function verifies the firmware currently in the FCd flash against the firmware
  * image pointed to by pc. The function return FCD_MODE_BL if the verification is OK and
  * FCD_MODE_APP otherwise.
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdBlVerifyFirmware(fcdDesc *fcd, char *pc, int64_t n64Size)
{
    unsigned char aucBufIn[65];
    unsigned char aucBufOut[65];
    uint32_t u32AddrStart;
    uint32_t u32AddrEnd;
    uint32_t u32Addr;
    BOOL bFinished=FALSE;

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    if (!pc)
	return FCD_RETCODE_ERROR;

    // Get the valid flash address range
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_GET_BYTE_ADDR_RANGE;
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    if (aucBufIn[0]!=FCD_CMD_BL_GET_BYTE_ADDR_RANGE || aucBufIn[1]!=1)
    {
        return FCD_RETCODE_ERROR;
    }

    u32AddrStart=
        aucBufIn[2]+
        (((uint32_t)aucBufIn[3])<<8)+
        (((uint32_t)aucBufIn[4])<<16)+
        (((uint32_t)aucBufIn[5])<<24);

    u32AddrEnd=
        aucBufIn[6]+
        (((uint32_t)aucBufIn[7])<<8)+
        (((uint32_t)aucBufIn[8])<<16)+
        (((uint32_t)aucBufIn[9])<<24);

    // Set start address for flash
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_SET_BYTE_ADDR;
    aucBufOut[2] = ((unsigned char)u32AddrStart);
    aucBufOut[3] = ((unsigned char)(u32AddrStart>>8));
    aucBufOut[4] = ((unsigned char)(u32AddrStart>>16));
    aucBufOut[5] = ((unsigned char)(u32AddrStart>>24));
    hid_write(fcd->phd, aucBufOut, 65);
    memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
    WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

    if (aucBufIn[0]!=FCD_CMD_BL_SET_BYTE_ADDR || aucBufIn[1]!=1)
    {
        return FCD_RETCODE_ERROR;
    }

    // Read blocks
    aucBufOut[0] = 0; // Report ID, ignored
    aucBufOut[1] = FCD_CMD_BL_READ_FLASH_BLOCK;
    for (u32Addr=u32AddrStart; u32Addr+47<u32AddrEnd && u32Addr+47<n64Size && !bFinished; u32Addr+=48)
    {
        hid_write(fcd->phd, aucBufOut, 65);
        memset(aucBufIn, 0xCC, 65); // Clear out the response buffer
        WRAPPED_HID_READ(fcd->phd, aucBufIn, 65);

        if (aucBufIn[0]!=FCD_CMD_BL_READ_FLASH_BLOCK || aucBufIn[1]!=1)
        {
            bFinished = TRUE;
            return FCD_RETCODE_ERROR;
        }

        if (memcmp(&aucBufIn[2],&pc[u32Addr],48)!=0)
        {
            bFinished = TRUE;
            return FCD_RETCODE_ERROR;
        }
    }
    return FCD_RETCODE_OKAY;

}



/** \brief Write FCD parameter (e.g. gain or filter)
  * \param fcd Pointer to an FCD device descriptor 
  * \param u8Cmd The command byte / parameter ID, see FCD_CMD_APP_SET_*
  * \param pu8Data The parameter value to be written
  * \param u8len Length of pu8Data in bytes
  * \return One of FCD_MODE_NONE, FCD_MODE_APP or FCD_MODE_BL (see description)
  *
  * This function can be used to set the value of a parameter in the FCD for which there is no
  * high level API call. It gives access to the low level API of the FCD and the caller is expected
  * to be aware of the various FCD commands, since they are required to be supplied as parameter
  * to this function.
  *
  * The return value can be used to determine the success or failure of the command execution:
  * - FCD_MODE_APP : Reply from FCD was as expected (nominal case).
  * - FCD_MODE_BL : Reply from FCD was not as expected.
  * - FCD_MODE_NONE : No FCD was found
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppSetParam(fcdDesc *fcd, uint8_t u8Cmd, uint8_t *pu8Data, uint8_t u8len)
{
    unsigned char aucBufOut[65];
    unsigned char aucBufIn[65];

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    aucBufOut[0]=0; // Report ID, ignored
    aucBufOut[1]=u8Cmd;
    memcpy(aucBufOut+2, pu8Data,u8len);
    hid_write(fcd->phd,aucBufOut,65);

    /* we must read after each write in order to empty FCD/HID buffer */
    memset(aucBufIn,0xCC,65); // Clear out the response buffer
    //    usleep(10000);
    //    WRAPPED_HID_READ(fcd->phd,aucBufIn,65);
    hid_read(fcd->phd,aucBufIn,65);

    /* Check the response, if OK return FCD_MODE_APP */
    if (aucBufIn[0]==u8Cmd && aucBufIn[1]==1) {
      if (u8Cmd == FCD_CMD_APP_SET_FREQ_HZ)
	memcpy(pu8Data, &aucBufIn[2], u8len);
      return FCD_RETCODE_OKAY;
    }

    /* Response did not contain the expected bytes */
    fprintf (stderr, "AppSetParam: returned response invalid.\n");
    return FCD_RETCODE_ERROR;

}


/** \brief Read FCD parameter (e.g. gain or filter)
  * \param fcd Pointer to an FCD device descriptor 
  * \param u8Cmd The command byte / parameter ID, see FCD_CMD_APP_GET_*
  * \param pu8Data TPointer to buffer where the parameter value(s) will be written
  * \param u8len Length of pu8Data in bytes
  * \return One of FCD_MODE_NONE, FCD_MODE_APP or FCD_MODE_BL (see description)
  *
  * This function can be used to read the value of a parameter in the FCD for which there is no
  * high level API call. It gives access to the low level API of the FCD and the caller is expected
  * to be aware of the various FCD commands, since they are required to be supplied as parameter
  * to this function.
  *
  * The return value can be used to determine the success or failure of the command execution:
  * - FCD_MODE_APP : Reply from FCD was as expected (nominal case).
  * - FCD_MODE_BL : Reply from FCD was not as expected.
  * - FCD_MODE_NONE : No FCD was found
  */
EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppGetParam(fcdDesc *fcd, uint8_t u8Cmd, uint8_t *pu8Data, uint8_t u8len)
{
    unsigned char aucBufOut[65];
    unsigned char aucBufIn[65];

    if (!(fcd && fcd->phd))
        return FCD_RETCODE_ABSENT;

    if (!pu8Data)
	return FCD_RETCODE_ERROR;

    aucBufOut[0]=0; // Report ID, ignored
    aucBufOut[1]=u8Cmd;
    usleep(10000);
    hid_write(fcd->phd,aucBufOut,65);

    memset(aucBufIn,0xCC,65); // Clear out the response buffer
    //    WRAPPED_HID_READ(fcd->phd,aucBufIn,65);
    hid_read(fcd->phd,aucBufIn,65);

    /* Copy return data to output buffer (even if cmd exec failed) */
    memcpy(pu8Data,aucBufIn+2,u8len);

    /* Check status bytes in returned data */
    if (aucBufIn[0]==u8Cmd && aucBufIn[1]==1) {
        return FCD_RETCODE_OKAY;
    }

    /* Response did not contain the expected bytes */
    return FCD_RETCODE_ERROR;
}

/** \brief Set default gain and filter parameters
  * \param fcd Pointer to an FCD device descriptor 
  */

typedef struct {

  uint8_t parm;
  uint8_t value;

} param_value;

static param_value param_defaults[] = {

  {FCD_CMD_APP_SET_LNA_GAIN    , TLGE_P20_0DB}, 
  {FCD_CMD_APP_SET_LNA_ENHANCE , TLEE_OFF},
  {FCD_CMD_APP_SET_BAND        , TBE_VHF2},
  {FCD_CMD_APP_SET_RF_FILTER   , TRFE_LPF268MHZ},
  {FCD_CMD_APP_SET_MIXER_GAIN  , TMGE_P12_0DB},
  {FCD_CMD_APP_SET_BIAS_CURRENT, TBCE_VUBAND},
  {FCD_CMD_APP_SET_MIXER_FILTER, TMFE_1_9MHZ},
  {FCD_CMD_APP_SET_IF_GAIN1    , TIG1E_P6_0DB},
  {FCD_CMD_APP_SET_IF_GAIN_MODE, TIGME_LINEARITY},
  {FCD_CMD_APP_SET_IF_RC_FILTER, TIRFE_1_0MHZ},
  {FCD_CMD_APP_SET_IF_GAIN2    , TIG2E_P0_0DB},
  {FCD_CMD_APP_SET_IF_GAIN3    , TIG3E_P0_0DB},
  {FCD_CMD_APP_SET_IF_FILTER   , TIFE_2_15MHZ},
  {FCD_CMD_APP_SET_IF_GAIN4    , TIG4E_P0_0DB},
  {FCD_CMD_APP_SET_IF_GAIN5    , TIG5E_P3_0DB},
  {FCD_CMD_APP_SET_IF_GAIN6    , TIG6E_P3_0DB},
   {0                           , 0}

} ;

EXTERN FCD_API_EXPORT FCD_API_CALL FCD_RETCODE_ENUM fcdAppSetParamDefaults(fcdDesc *fcd)
{
  param_value *defs = & param_defaults[0];
  while (defs->parm != 0) {
    if (FCD_RETCODE_OKAY != fcdAppSetParam(fcd, defs->parm, &defs->value, 1))
      return FCD_RETCODE_ERROR;
    ++defs;
  }
  return FCD_RETCODE_OKAY;
}
