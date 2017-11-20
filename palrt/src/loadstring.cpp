// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
// ===========================================================================
// File: loadstring.cpp
// 
// ===========================================================================

/*============================================================
**
**                                                    
**
** Purpose: Functions for loading strings from satellite files
**
** Date:  July 26, 2001
** 
**
===========================================================*/

#include "rotor_palrt.h"

#define MAX_SAT_STRING_LENGTH 511
#define MAX_INT_CHARS 11

typedef struct _HSATELLITE{
  LPVOID data;
  UINT size;

}SATELLITE, *HSATELLITE;


HSATELLITE PALAPI PAL_LoadSatelliteResourceW(LPCWSTR SatelliteResourceFileName)
{
  HANDLE satelliteFile;
  HANDLE fileMappingObject;
  HSATELLITE Satellite = (HSATELLITE)malloc(sizeof(SATELLITE));
  
  if (Satellite == NULL){
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }
    

  satelliteFile = CreateFileW(SatelliteResourceFileName,
			     GENERIC_READ,
			     FILE_SHARE_READ | FILE_SHARE_WRITE,
			     NULL,
			     OPEN_EXISTING,
			     FILE_ATTRIBUTE_NORMAL,
			     NULL);

  if (satelliteFile == INVALID_HANDLE_VALUE){
    free(Satellite);
    return NULL;
  }

  Satellite->size = GetFileSize(satelliteFile, NULL);

  if (Satellite->size == INVALID_FILE_SIZE){
    CloseHandle(satelliteFile);
    free(Satellite);
    return NULL;
  }

  fileMappingObject = CreateFileMapping(satelliteFile,            
					NULL, //if null, gets default security
					PAGE_READONLY,
					0, //hmaxsize - 0 uses size of file
					0, //lmaxsize - 0 uses size of file
					NULL);  //Name
  if (fileMappingObject == NULL){
    CloseHandle(satelliteFile);
    free(Satellite);
    return NULL;
  }
  
  Satellite->data = MapViewOfFile(fileMappingObject,
				 FILE_MAP_READ,
				 0,
				 0,
				 0);

  CloseHandle(fileMappingObject);
  CloseHandle(satelliteFile);

  if(Satellite->data == NULL){
    free(Satellite);
    return NULL;
  }

  return Satellite;
}

BOOL PALAPI PAL_FreeSatelliteResource(HSATELLITE SatelliteResource)
{
  BOOL freed;

  freed = UnmapViewOfFile((LPVOID)SatelliteResource->data);

  free(SatelliteResource);

  return freed;
}
UINT PALAPI PAL_LoadSatelliteStringW(HSATELLITE SatelliteResource,
			 UINT uID,
			 LPWSTR lpBuffer,
			 UINT nBufferMax)
{
  LPSTR bigString;
  char littleString[MAX_SAT_STRING_LENGTH + 1];
  UINT i;
  UINT length;
  UINT currentInt;

  if (SatelliteResource->data == NULL){
    SetLastError(ERROR_INVALID_DATA);
    return 0;
  }

  // Note: we are indexing only using the low 16 bits
  uID = (UINT16)uID;
  currentInt = uID+1;

  bigString = (LPSTR) SatelliteResource->data;

  for (i=0; i < SatelliteResource->size 
         && bigString[i] != EOF; 
       i++){      //Outer loop - once per id/string pair
    UINT j;
    char numBuf[MAX_INT_CHARS + 1];  


    for (j=0; i < SatelliteResource->size
           && bigString[i] != EOF
           && j < MAX_INT_CHARS 
           && isdigit(bigString[i]);
         j++, i++){
      numBuf[j] = bigString[i];
    } //Get string ID

    if (i >= SatelliteResource->size || bigString[i] == EOF){
      SetLastError(ERROR_INVALID_DATA);
      return 0;
    }//file shouldn't end in the middle of the number
      
    if (j >= MAX_INT_CHARS){
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
    }

    numBuf[j] = '\0';
 
    currentInt = atol(numBuf);

    for(; i > 0
          && i < SatelliteResource->size
          && bigString[i-1] != '\"'
          && bigString[i] != EOF;
        i++);  ///skip white space up to quote 

    for (j=0; i < SatelliteResource->size
           && j < nBufferMax - 1   /// -1 is to leave space for '\0'
           && j < MAX_SAT_STRING_LENGTH 
           && bigString[i] != '\"'
           && bigString[i] != EOF; 
         j++, i++){
      littleString[j] = bigString[i];
    } //Get String Value

    littleString[j] = '\0';
    length = j+1;

    //  If we stopped because the buffer was too small, 
    //  skip the rest of the string up to the next quote.
    for(; i > 0
          && i < SatelliteResource->size
          && bigString[i] != EOF
          && bigString[i-1] != '\"';
        i++);

    if (i > 0 && (i >= SatelliteResource->size || bigString[i] == EOF)){
      if (bigString[i-1] == '\"')
        break;  //case where file ended right after quote
      else{
        SetLastError(ERROR_INVALID_DATA);
        return 0;  //case where file ended before quote
      }
    }
     
    if(bigString[i] == '\r'){
      i++;
    }
    
    if (currentInt == uID){
      break;
    }
  }
  if (currentInt != uID){
    SetLastError(ERROR_NOT_FOUND);
    return 0;    
  }
  else{
    UINT numOfWideChars;

    numOfWideChars = MultiByteToWideChar(CP_UTF8, 
					 0,
					 littleString, 
					 strlen(littleString), 
					 NULL, 
					 0); 
    
    numOfWideChars = min(numOfWideChars, nBufferMax);
    
    if (numOfWideChars == 0){
      return 0;
    }
    
    if(!MultiByteToWideChar(CP_UTF8, 
			    0,
			    littleString, 
			    strlen(littleString), 
			    lpBuffer, 
			    numOfWideChars)){
      return 0;
    }
    
    lpBuffer[numOfWideChars] = '\0';
    return numOfWideChars;
  }
}

UINT PALAPI PAL_LoadSatelliteStringA(HSATELLITE SatelliteResource,
			 UINT uID,
			 LPSTR lpBuffer,
			 UINT nBufferMax)
{
  UINT numOfAChars;
  LPWSTR goodWString = (LPWSTR)malloc(nBufferMax*sizeof(WCHAR));

  if (goodWString == NULL){
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  if(!PAL_LoadSatelliteStringW(SatelliteResource,
			       uID,
			       goodWString,
			       nBufferMax)){
    free(goodWString);
    return 0;
  }
 
  numOfAChars = WideCharToMultiByte(CP_ACP, 
				    0,
				    goodWString, 
				    wcslen(goodWString), 
				    NULL, 
				    0,
				    NULL,
				    NULL); 

  numOfAChars = min(numOfAChars, nBufferMax);

  if (numOfAChars == 0){
    free(goodWString);
    return 0;
  }

  if(!WideCharToMultiByte(CP_ACP, 
			  0,
			  goodWString, 
			  wcslen(goodWString), 
			  lpBuffer, 
			  numOfAChars,
			  NULL,
			  NULL)){
    free(goodWString);
    return 0;
  }
 
  lpBuffer[numOfAChars] = '\0';

  free(goodWString);

  return numOfAChars;
}
