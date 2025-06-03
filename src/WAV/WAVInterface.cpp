/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	WAVInterface.cpp
* Description		: 	WAVInterface operation center
* Created			: 	2020.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include "MediaAdapter.h"
#include "WAVInterface.h"
#include <string.h>
#include <iostream>


using std::cout;//需要<iostream>
using std::endl;



char * WAVInterface::m_strFormatName=(char *)WAV_MUX_NAME;
/*****************************************************************************
-Fuction		: WAVInterface
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
WAVInterface::WAVInterface()
{
    m_pWAV=NULL;
}
/*****************************************************************************
-Fuction		: ~WAVInterface
-Description	: ~WAVInterface
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
WAVInterface::~WAVInterface()
{
    if(NULL!= m_pWAV)
    {
        delete m_pWAV;
    }
}


/*****************************************************************************
-Fuction		: WAVInterface::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int WAVInterface::Init(char *i_strPath)
{
    int iRet=FALSE;
	return TRUE;
}

/*****************************************************************************
-Fuction		: WAVInterface::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int WAVInterface::GetNextFrame(T_MediaFrameParam *m_ptMediaFrameParam)
{
    int iRet=FALSE;

	return TRUE;
}
/*****************************************************************************
-Fuction		: WAVInterface::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int WAVInterface::GetVideoEncParam(T_VideoEncodeParam *o_ptVideoEncodeParam)
{
    int iRet=FALSE;

	return TRUE;
}
/*****************************************************************************
-Fuction		: WAVInterface::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int WAVInterface::GetMediaInfo(T_MediaInfo *o_ptMediaInfo)
{
    int iRet=FALSE;

	return TRUE;
}

/*****************************************************************************
-Fuction        : GetFrame
-Description    : m_ptFrame->iFrameBufLen 必须大于等于一帧数据大小否则会失败
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int WAVInterface::GetFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    
    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf )
    {
        MH_LOGE("GetFrame NULL %p\r\n", m_ptFrame);
        return iRet;
    }
    if(NULL== m_pWAV)
    {
        m_pWAV=new WAV();
    }
	if(m_pWAV->GetFrameData(m_ptFrame)<=0)
	{
        MH_LOGE("m_pWAV->GetFrameData err\r\n");
        return iRet;
	}
	return TRUE;
}

/*****************************************************************************
-Fuction        : FrameToContainer
-Description    : m_ptFrame->iFrameBufLen 必须大于等于一帧数据大小否则会失败
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int WAVInterface::FrameToContainer(T_MediaFrameInfo *i_ptFrame,E_StreamType i_eStreamType,unsigned char * o_pbBuf, unsigned int i_dwMaxBufLen,int *o_piHeaderOffset,int i_iForcePack)
{
    int iRet=FALSE;
    
    if(NULL == i_ptFrame ||NULL == o_pbBuf ||NULL == i_ptFrame->pbFrameStartPos ||i_ptFrame->iFrameLen <= 0)
    {
        MH_LOGE("FrameToContainer err NULL\r\n");
        return iRet;
    }

    if(NULL== m_pWAV)
    {
        m_pWAV=new WAV();
    }
    return m_pWAV->GetMuxData(i_ptFrame,o_pbBuf,i_dwMaxBufLen);
}



