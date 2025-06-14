/*****************************************************************************
* Copyright (C) 2017-2018 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	MediaHandle.cpp
* Description		: 	Media operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include "MediaHandle.h"
#include <string.h>
#include <iostream>
#include "MediaAdapter.h"
#include "RawVideoHandle.h"
#include "RawAudioHandle.h"
#include "FlvHandleInterface.h"
#include "FMP4HandleInterface.h"
#include "TsInterface.h"
#include "WAVInterface.h"

using std::cout;//需要<iostream>
using std::endl;

/*****************************************************************************
-Fuction		: MediaHandle
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
MediaHandle::MediaHandle()
{
	m_pMediaHandle =NULL;
	m_pMediaAudioHandle =NULL;
    m_pMediaFile = NULL;
	m_pMediaPackHandle =NULL;
}

/*****************************************************************************
-Fuction		: ~VideoHandle
-Description	: ~VideoHandle
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
MediaHandle::~MediaHandle()
{
    //MH_LOGW("~MediaHandle ~MediaHandle\r\n");
	if(m_pMediaHandle !=NULL)
	{
        delete m_pMediaHandle;
        m_pMediaHandle = NULL;
	}
	if(m_pMediaAudioHandle !=NULL)
	{
        delete m_pMediaAudioHandle;
        m_pMediaAudioHandle = NULL;
	}
	if(m_pMediaPackHandle !=NULL)
	{
        delete m_pMediaPackHandle;
        m_pMediaPackHandle = NULL;
	}
    if(NULL != m_pMediaFile)
    {
        fclose(m_pMediaFile);
        m_pMediaFile = NULL;
    }
}

/*****************************************************************************
-Fuction		: VideoHandle::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::Init(char *i_strPath)
{
    int iRet=FALSE;
    if(NULL == i_strPath)
    {
        cout<<"Init NULL"<<endl;
        return iRet;
    }
    m_pMediaFile = fopen(i_strPath,"rb");//
    if(NULL == m_pMediaFile)
    {
        cout<<"Init "<<i_strPath<<"failed !"<<endl;
        return iRet;
    } 
    
    if(NULL != strstr(i_strPath,FlvHandleInterface::m_strFormatName))
    {
        m_pMediaHandle=new FlvHandleInterface();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
    if(NULL != strstr(i_strPath,FMP4HandleInterface::m_strFormatName))
    {
        m_pMediaHandle=new FMP4HandleInterface();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
    if(NULL != strstr(i_strPath,H264Handle::m_strVideoFormatName))
    {
        m_pMediaHandle=new H264Handle();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
    if(NULL != strstr(i_strPath,H265Handle::m_strVideoFormatName))
    {
        m_pMediaHandle=new H265Handle();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
    if(NULL != strstr(i_strPath,G711Handle::m_strAudioFormatName))
    {
        m_pMediaHandle=new G711Handle();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
    if(NULL != strstr(i_strPath,AACHandle::m_strAudioFormatName))
    {
        m_pMediaHandle=new AACHandle();
        if(NULL !=m_pMediaHandle)
            iRet=m_pMediaHandle->Init(i_strPath);
        return iRet;
    }
	return iRet;
}

/*****************************************************************************
-Fuction		: VideoHandle::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::Init(E_StreamType i_eStreamType,E_MediaEncodeType i_eVideoEncType,E_MediaEncodeType i_eAudioEncType)
{
    int iRet=FALSE;

    
    if(STREAM_TYPE_VIDEO_STREAM == i_eStreamType ||STREAM_TYPE_MUX_STREAM == i_eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_H264 == i_eVideoEncType)
        {
            if(NULL != m_pMediaHandle)
            {
                delete m_pMediaHandle;
            }
            m_pMediaHandle=new H264Handle();
        } 
        if(MEDIA_ENCODE_TYPE_H265 == i_eVideoEncType)
        {
            if(NULL != m_pMediaHandle)
            {
                delete m_pMediaHandle;
            }
            m_pMediaHandle=new H265Handle();
        } 
        iRet=TRUE;
    } 
    if(STREAM_TYPE_MUX_STREAM == i_eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_G711U == i_eAudioEncType ||MEDIA_ENCODE_TYPE_G711A == i_eAudioEncType)
        {
            if(NULL != m_pMediaAudioHandle)
            {
                delete m_pMediaAudioHandle;
            }
            m_pMediaAudioHandle=new G711Handle();
        } 
        if(MEDIA_ENCODE_TYPE_AAC == i_eAudioEncType)
        {
            if(NULL != m_pMediaAudioHandle)
            {
                delete m_pMediaAudioHandle;
            }
            m_pMediaAudioHandle=new AACHandle();
        } 
        iRet=TRUE;
    }
    if(STREAM_TYPE_AUDIO_STREAM == i_eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_G711U == i_eAudioEncType ||MEDIA_ENCODE_TYPE_G711A == i_eAudioEncType)
        {
            if(NULL != m_pMediaHandle)
            {
                delete m_pMediaHandle;
            }
            m_pMediaHandle=new G711Handle();
        } 
        if(MEDIA_ENCODE_TYPE_AAC == i_eAudioEncType)
        {
            if(NULL != m_pMediaHandle)
            {
                delete m_pMediaHandle;
            }
            m_pMediaHandle=new AACHandle();
        } 
        iRet=TRUE;
    }
	return iRet;
}

/*****************************************************************************
-Fuction		: VideoHandle::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::GetNextFrame(T_MediaFrameParam *m_ptMediaFrameParam)
{
    int iRet=FALSE;
    int iReadLen = 0;
    if(NULL == m_ptMediaFrameParam)
    {
        cout<<"GetNextFrame NULL"<<endl;
        return iRet;
    }
    if(NULL == m_pMediaFile)
    {
        cout<<"GetNextFrame m_pMediaFile NULL"<<endl;
        return iRet;
    }
    iReadLen = fread(m_ptMediaFrameParam->pbFrameBuf, 1, m_ptMediaFrameParam->iFrameBufMaxLen, m_pMediaFile);
    if(iReadLen <= 0)
    {
        cout<<"fread err "<<m_ptMediaFrameParam->iFrameBufMaxLen<<m_pMediaFile<<endl;
        return iRet;
    }
    m_ptMediaFrameParam->iFrameBufLen = iReadLen;
    if(NULL !=m_pMediaHandle)
    {
        iRet = m_pMediaHandle->GetNextFrame(m_ptMediaFrameParam);
        if(TRUE == iRet)
        {
            fseek(m_pMediaFile,m_ptMediaFrameParam->iFrameProcessedLen,SEEK_SET);
            //cout<<"fseek m_pMediaFile "<<m_ptMediaFrameParam->iFrameProcessedLen<<m_pMediaFile<<endl;
        }
        else
        {
            cout<<"fseek err m_pMediaFile "<<m_ptMediaFrameParam->iFrameProcessedLen<<m_pMediaFile<<endl;
        }
    }
	return iRet;
}




/*****************************************************************************
-Fuction		: VideoHandle::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::GetVideoEncParam(T_VideoEncodeParam *o_ptVideoEncodeParam)
{
    int iRet=FALSE;
    if(NULL !=m_pMediaHandle)
    {
        iRet=m_pMediaHandle->GetVideoEncParam(o_ptVideoEncodeParam);
    }
	return iRet;
}




/*****************************************************************************
-Fuction		: VideoHandle::GetNextVideoFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::GetMediaInfo(T_MediaInfo *o_ptMediaInfo)
{
    int iRet=FALSE;
    if(NULL !=m_pMediaHandle)
    {
        iRet=m_pMediaHandle->GetMediaInfo(o_ptMediaInfo);
    }
	return iRet;
}

/*****************************************************************************
-Fuction		: GetFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::GetFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet=FALSE;
    int iReadLen = 0;

    
    if(NULL == m_ptFrame)
    {
        MH_LOGE("GetNextFrame NULL\r\n");
        return iRet;
    }
    
    if(NULL != m_pMediaFile)
    {
        int iFileSeekFlag = 0;
        if(0 == m_ptFrame->iFrameProcessedLen)//兼容iFrameProcessedLen每次会清零的情况,
        {//(为了将接收的部分帧数据重组为完整的一帧,故每次iFrameProcessedLen会清零)
            iFileSeekFlag=1;//iFrameProcessedLen就变成每次处理的长度，则要从当前位置开始偏移(SEEK_CUR)
        }//iFrameProcessedLen一直+=没被清零过，则要从文件开头开始偏移(SEEK_SET)
        long dwPosition = ftell(m_pMediaFile);
        iReadLen = fread(m_ptFrame->pbFrameBuf, 1, m_ptFrame->iFrameBufMaxLen, m_pMediaFile);
        if(iReadLen <= 0)
        {
            MH_LOGE("fread err %d iReadLen%d\r\n",m_ptFrame->iFrameBufMaxLen,iReadLen);
            return iRet;
        }
        m_ptFrame->iFrameBufLen = iReadLen;//(文件流iFrameBufLen每次都会直接赋值,
        iRet = m_pMediaHandle->GetFrame(m_ptFrame);
        if(TRUE == iRet)
        {
            if(0 != iFileSeekFlag)
            {//iFrameProcessedLen就变成每次处理的长度，则要从当前位置开始偏移(SEEK_CUR)
                dwPosition+=m_ptFrame->iFrameProcessedLen;
            }
            else
            {//iFrameProcessedLen一直+=没被清零过，则要从文件开头开始偏移(SEEK_SET)
                dwPosition=m_ptFrame->iFrameProcessedLen;
            }
            fseek(m_pMediaFile,dwPosition,SEEK_SET);
            m_ptFrame->iFrameBufLen = m_ptFrame->iFrameProcessedLen;//兼容外部使用iFrameBufLen iFrameProcessedLen判断处理是否正常的情况
            //(文件流iFrameBufLen每次都会直接赋值,所以可以这么兼容)
        }
        else
        {
            fclose(m_pMediaFile);//fseek(m_pMediaFile,0,SEEK_SET); 
            m_pMediaFile = NULL;
            MH_LOGE("fseek err m_pMediaFile %d\r\n",m_ptFrame->iFrameProcessedLen);
        }
        return iRet;
    }
    if(STREAM_TYPE_VIDEO_STREAM == m_ptFrame->eStreamType ||STREAM_TYPE_MUX_STREAM == m_ptFrame->eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_H264 == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaHandle)
            {
                m_pMediaHandle=new H264Handle();
            }
        } 
        if(MEDIA_ENCODE_TYPE_H265 == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaHandle)
            {
                m_pMediaHandle=new H265Handle();
            }
        } 
    } 
    if(STREAM_TYPE_MUX_STREAM == m_ptFrame->eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_G711U == m_ptFrame->eEncType ||MEDIA_ENCODE_TYPE_G711A == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaAudioHandle)
            {
                m_pMediaAudioHandle=new G711Handle();
            }
        } 
        if(MEDIA_ENCODE_TYPE_AAC == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaAudioHandle)
            {
                m_pMediaAudioHandle=new AACHandle();
            }
        } 
    }
    if(STREAM_TYPE_AUDIO_STREAM == m_ptFrame->eStreamType)
    {
        if(MEDIA_ENCODE_TYPE_G711U == m_ptFrame->eEncType ||MEDIA_ENCODE_TYPE_G711A == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaHandle)
            {
                m_pMediaHandle=new G711Handle();
            }
        } 
        if(MEDIA_ENCODE_TYPE_AAC == m_ptFrame->eEncType)
        {
            if(NULL == m_pMediaHandle)
            {
                m_pMediaHandle=new AACHandle();
            }
        } 
    }

    if(STREAM_TYPE_MUX_STREAM == m_ptFrame->eStreamType && (MEDIA_ENCODE_TYPE_G711U == m_ptFrame->eEncType||
    MEDIA_ENCODE_TYPE_G711A == m_ptFrame->eEncType||MEDIA_ENCODE_TYPE_AAC == m_ptFrame->eEncType))
    {
        if(NULL != m_pMediaAudioHandle)
        {
            iRet = m_pMediaAudioHandle->GetFrame(m_ptFrame);
        }
        return iRet;
    } 
    if(STREAM_TYPE_FLV_STREAM == m_ptFrame->eStreamType)
    {
        if(NULL == m_pMediaHandle)
        {
            m_pMediaHandle=new FlvHandleInterface();
        }
    }
    if(STREAM_TYPE_WAV_STREAM == m_ptFrame->eStreamType)
    {
        if(NULL == m_pMediaHandle)
        {
            m_pMediaHandle=new WAVInterface();
        }
    }

    if(NULL != m_pMediaHandle)
    {
        iRet = m_pMediaHandle->GetFrame(m_ptFrame);
    }

    if(FALSE == iRet)
    {
        MH_LOGE("GetNextFrame FALSE:eStreamType%d,eEncType%d\r\n",m_ptFrame->eStreamType, m_ptFrame->eEncType);
    }
    
	return iRet;
}

/*****************************************************************************
-Fuction		: MediaHandle
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaHandle::FrameToContainer(T_MediaFrameInfo *i_ptFrame,E_StreamType i_eStreamType,unsigned char * o_pbBuf, unsigned int i_dwMaxBufLen,int *o_piHeaderOffset,int i_iForcePack)
{
    int iRet=FALSE;
    int iWriteLen = 0;
    
    if(NULL == i_ptFrame)
    {
        MH_LOGE("FrameToContainer NULL\r\n");
        return iRet;
    }
    
    if(NULL == o_pbBuf)
    {
        MH_LOGE("FrameToContainer o_pbBuf NULL\r\n");
        return iRet;
    }
    
    switch(i_eStreamType)
    {
        case STREAM_TYPE_WAV_STREAM :
        {
            if(NULL == m_pMediaPackHandle)
            {
                m_pMediaPackHandle=new WAVInterface();
            }
            break;
        }
        case STREAM_TYPE_TS_STREAM :
        {
            if(NULL == m_pMediaPackHandle)
            {
                m_pMediaPackHandle=new TsInterface();
            }
            break;
        }
        case STREAM_TYPE_FMP4_STREAM :
        {
            if(NULL == m_pMediaPackHandle)
            {
                m_pMediaPackHandle=new FMP4HandleInterface();
            }
            break;
        }
        case STREAM_TYPE_FLV_STREAM :
        case STREAM_TYPE_ENHANCED_FLV_STREAM :
        {
            if(NULL == m_pMediaPackHandle)
            {
                m_pMediaPackHandle=new FlvHandleInterface();
            }
            break;
        }
        case STREAM_TYPE_VIDEO_STREAM :
        {
            if(i_ptFrame->iFrameLen > i_dwMaxBufLen)
            {
                MH_LOGE("i_ptFrame->iFrameLen > i_dwMaxBufLen err\r\n");
                break;
            }
            if(i_ptFrame->eFrameType != MEDIA_FRAME_TYPE_VIDEO_I_FRAME && 
            i_ptFrame->eFrameType != MEDIA_FRAME_TYPE_VIDEO_P_FRAME && 
            i_ptFrame->eFrameType != MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
            {
                iRet = 0;
                break;
            }
            memcpy(o_pbBuf,i_ptFrame->pbFrameStartPos,i_ptFrame->iFrameLen);
            iRet = i_ptFrame->iFrameLen;
            break;
        }
        case STREAM_TYPE_AUDIO_STREAM :
        {
            if(i_ptFrame->iFrameLen > i_dwMaxBufLen)
            {
                MH_LOGE("i_ptFrame->iFrameLen > i_dwMaxBufLen err\r\n");
                break;
            }
            if(i_ptFrame->eFrameType != MEDIA_FRAME_TYPE_AUDIO_FRAME)
            {
                iRet = 0;
                break;
            }
            memcpy(o_pbBuf,i_ptFrame->pbFrameStartPos,i_ptFrame->iFrameLen);
            iRet = i_ptFrame->iFrameLen;
            break;
        }
        default :
        {
            MH_LOGE("FrameToContainer i_eStreamType err%d\r\n",i_eStreamType);
            break;
        }
    }
    if(NULL != m_pMediaPackHandle)
    {
        iRet = m_pMediaPackHandle->FrameToContainer(i_ptFrame,i_eStreamType,o_pbBuf,i_dwMaxBufLen,o_piHeaderOffset,i_iForcePack);
    }

    if(iRet < 0)
    {
        MH_LOGE("FrameToContainer FALSE:eStreamType%d,iFrameLen %d\r\n",i_eStreamType,i_ptFrame->iFrameLen);
    }
    
	return iRet;
}

