/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	WAVInterface.h
* Description		: 	WAVInterface operation center
* Created			: 	2020.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef WAV_INTERFACE_H
#define WAV_INTERFACE_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "MediaHandle.h"
#include "WAV.h"

using std::string;


#define WAV_MUX_NAME        ".wav"


/*****************************************************************************
-Class			: WAVInterface
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class WAVInterface : public MediaHandle
{
public:
    WAVInterface();
    ~WAVInterface();
    virtual int Init(char *i_strPath);
    virtual int GetNextFrame(T_MediaFrameParam *m_ptMediaFrameParam);
    virtual int GetVideoEncParam(T_VideoEncodeParam *o_ptVideoEncodeParam);
    virtual int GetMediaInfo(T_MediaInfo *o_ptMediaInfo);
    
    virtual int GetFrame(T_MediaFrameInfo *m_ptFrame);//
    virtual int FrameToContainer(T_MediaFrameInfo *i_ptFrame,E_StreamType i_eStreamType,unsigned char * o_pbBuf, unsigned int i_dwMaxBufLen,int *o_piHeaderOffset=NULL,int i_iForcePack=0);//

    static char *m_strFormatName;
private:
    WAV *m_pWAV;
};












#endif
