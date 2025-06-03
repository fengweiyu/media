/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	WAV.h
* Description		: 	WAV operation center
* Created			: 	2020.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef WAV_H
#define WAV_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <stdint.h>
#include "MediaHandle.h"

using std::string;
using std::list;



/*****************************************************************************
-Class			: WAV
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2020/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class WAV 
{
public:
    WAV();
    ~WAV();
    int GetMuxData(T_MediaFrameInfo * i_ptFrameInfo,unsigned char * o_pbBuf,unsigned int i_dwMaxBufLen);
    int GetFrameData(T_MediaFrameInfo *m_ptFrame);
};












#endif
