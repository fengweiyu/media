/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       MediaConvert.cpp
* Description           : 	    
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <string>
#include "MediaConvert.h"

#define MEDIA_FORMAT_MAX_LEN	(10*1024) 
#define MEDIA_OUTPUT_BUF_MAX_LEN	(2*1024*1024) 
#define MEDIA_INPUT_BUF_MAX_LEN	(6*1024*1024) 





MediaConvert * MediaConvert::m_pInstance = new MediaConvert();//一般使用饿汉模式,懒汉模式线程不安全
/*****************************************************************************
-Fuction        : MediaConvert
-Description    : MediaConvert
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
MediaConvert:: MediaConvert()
{
    m_pDataBufList.clear();
    m_eDstVideoEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_eDstAudioEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_pbInputBuf = new DataBuf(MEDIA_INPUT_BUF_MAX_LEN);
    m_pAudioCodec = NULL;
    m_pAudioCodec = new AudioCodec();
    m_pAudioTranscodeBuf = new unsigned char [MEDIA_FORMAT_MAX_LEN];
    m_iPutFrameLen=0;
    m_dwLastAudioTimeStamp=0;
    m_dwLastVideoTimeStamp=0;
    m_dwVideoTimeStamp = 0;
    m_dwAudioTimeStamp = 0;
    m_dwPutVideoFrameTime=0;
    m_iPutVideoFrameCnt=0;
    m_iFindKeyFrame=0;
    memset(&m_tSegInfo,0,sizeof(T_SegInfo));
    memset(&m_tFileFrameInfo,0,sizeof(T_MediaFrameInfo));//tVideoEncodeParam
}
/*****************************************************************************
-Fuction        : MediaConvert
-Description    : MediaConvert
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
MediaConvert:: ~MediaConvert()
{
    if(!m_pDataBufList.empty())
    {
        DataBuf * it = m_pDataBufList.front();
        delete it;
        m_pDataBufList.pop_front();
    }
    delete m_pbInputBuf;
    if(NULL != m_pAudioCodec)
    {
        delete m_pAudioCodec;
    }
    if(NULL != m_pAudioTranscodeBuf)
    {
        delete [] m_pAudioTranscodeBuf;
    }
}
/*****************************************************************************
-Fuction		: Instance
-Description	: Instance
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
MediaConvert * MediaConvert::Instance()
{
	return m_pInstance;
}

/*****************************************************************************
-Fuction        : Convert
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ConvertFromPri(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_StreamType i_eDstStreamType)
{
    T_MediaFrameInfo tFileFrameInfo;
	DataBuf * pbOutBuf =NULL;
	int iRet = -1,iWriteLen=0;
	int iHeaderLen=0;
	//int iPutFrameLen=0;
#ifdef SUPPORT_PRI
    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    NSXPTL::FRAME_INFO* pFrame =NULL;
    m_streamer.Inputdata(i_pbSrcData,i_iSrcDataLen);
    while(1)
    {
        if(pFrame != NULL)
        {
            pFrame->Release();
            pFrame=NULL;
        } 
        pFrame = m_streamer.GetNextFrameNoSafe();
        if(pFrame == NULL)
        {
            //printf("pFrame = NULL ,input pri data not enough [%x,Len %d]\r\n",i_pbSrcData[0],i_iSrcDataLen);
            break;
        } 
        if(pFrame->nEncodeType == ENCODE_VIDEO_JPG)
        {
            printf("ENCODE_VIDEO_JPG unsupport [%x,Len %d]\r\n",i_pbSrcData[0],i_iSrcDataLen);
            return iRet;
        } 
        memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
        iRet=PriToMedia(pFrame,&tFileFrameInfo);
        if(iRet<0)
        {
            //printf("PriToMedia err%d\r\n",pFrame->nEncodeType);
            continue;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
        {
            m_iFindKeyFrame=1;
            m_tSegInfo.iHaveKeyFrameFlag=1;
        }
        else
        {
            if(m_iFindKeyFrame==0)
                continue;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_tSegInfo.iVideoFrameCnt++;
            m_eDstVideoEncType = tFileFrameInfo.eEncType;
            m_iPutVideoFrameCnt++;//tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp 要在SynchronizerAudioVideo前
            if(m_dwLastVideoTimeStamp != 0)//带音频不能非gop分段，否则出错(web播着会一直卡顿不出流或者只放i帧)
                m_dwPutVideoFrameTime+=m_iPutVideoFrameCnt>1?(tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp):0;
            //m_dwLastVideoTimeStamp = tFileFrameInfo.dwTimeStamp;//调用 SynchronizerAudioVideo则要注释这里
        }
        //if(i_eDstStreamType != STREAM_TYPE_FMP4_STREAM)//
            SynchronizerAudioVideo(&tFileFrameInfo);//mp4是音频轨和视频轨是分别单独的，所以不需要同源处理,处理了也不影响
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)//同时web端对时间戳要求严格，如果同源处理，会不符合帧率/采样率，会导致卡顿跳秒
        {
            m_tSegInfo.iAudioFrameCnt++;
            if(i_eDstStreamType == STREAM_TYPE_FMP4_STREAM)
            {//mse(mp4)才需要转换
                if(AudioTranscode(&tFileFrameInfo)<=0)
                {
                    continue;//转换失败或数据不够，则跳过这一帧
                }
            }
            m_eDstAudioEncType = tFileFrameInfo.eEncType;
        }
        //printf("FrameType %d TimeStamp %u PutFrameTime %u FrameLen %d\r\n", tFileFrameInfo.eFrameType, tFileFrameInfo.dwTimeStamp,m_dwPutVideoFrameTime, tFileFrameInfo.iFrameLen);
        if(0 == m_iPutFrameLen)
            printf("start PutFrameLen %d iFrameLen %d eFrameType[%d] eEncType[%d]\r\n",m_iPutFrameLen,tFileFrameInfo.iFrameLen,tFileFrameInfo.eFrameType,tFileFrameInfo.eEncType);
        m_iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {
            pbOutBuf = new DataBuf(m_iPutFrameLen+MEDIA_FORMAT_MAX_LEN);
        }//带音频不能非gop分段，否则出错(web播着会一直卡顿不出流)
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen,m_dwPutVideoFrameTime>=600?1:0);
        if(iWriteLen < 0)
        {
            printf("!!!FrameToContainer err ,iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            m_iPutFrameLen=0;
            m_dwPutVideoFrameTime=0;
            m_iPutVideoFrameCnt=0;
            memset(&m_tSegInfo,0,sizeof(T_SegInfo));
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//没有获取到封装数据(输入数据不够)
                delete pbOutBuf;//但是下次输出数据会更大，获取封装数据需要更大的缓存
                pbOutBuf = NULL;//所以需要释放，重新分配
            }//如果每次都传入一个足够大的缓存，则不需要这么麻烦
            continue;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
            printf("FrameToContainer iPutFrame %d Len %d iWriteLen %d ProcessedLen[%d] iBufMaxLen[%d]\r\n",m_iPutVideoFrameCnt,m_iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen,pbOutBuf->iBufMaxLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_tSegInfo.iSegDurationTime=tFileFrameInfo.dwTimeStamp-m_tSegInfo.iSegStartTime;
        memcpy(&pbOutBuf->tSegInfo,&m_tSegInfo,sizeof(T_SegInfo));
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        memset(&m_tSegInfo,0,sizeof(T_SegInfo));
        m_tSegInfo.iSegStartTime=tFileFrameInfo.dwTimeStamp;
        m_tSegInfo.dwStartTimeHigh=(pFrame->nTimeStamp>>32)&0xffffffff;
        m_tSegInfo.dwStartTimeLow=(pFrame->nTimeStamp)&0xffffffff;
        m_tSegInfo.dwStartAbsTime=(pFrame->nTimeStamp)/1000;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//没用到的封装缓存长度
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//总数据-被打包的数据长度和封装长度=未打包数据长度+未用封装缓存长度再减iRemain=未打包数据长度
        m_iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLen要等于封装对象中缓存的未打包的数据长度，这样内存才够。
        m_dwPutVideoFrameTime=0;
        m_iPutVideoFrameCnt=0;

        pFrame->Release();
        pFrame=NULL;
    }
    //m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);//
    if(NULL != pbOutBuf)
    {
        delete pbOutBuf;
    }
#endif    
    return iWriteLen;
}

/*****************************************************************************
-Fuction        : Convert
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert::Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType)
{
    T_MediaFrameInfo tFileFrameInfo;
	DataBuf * pbOutBuf =NULL;
	int iRet = -1,iWriteLen=0;
	int iHeaderLen=0;
	//int iPutFrameLen=0;

    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    m_pbInputBuf->Copy(i_pbSrcData,i_iSrcDataLen);
    memcpy(&tFileFrameInfo,&m_tFileFrameInfo,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf;
    tFileFrameInfo.eStreamType = i_eSrcStreamType;
    tFileFrameInfo.eEncType = i_eSrcEncType;
    tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen;
    tFileFrameInfo.iFrameBufMaxLen = m_pbInputBuf->iBufMaxLen;
    while(1)
    {
        tFileFrameInfo.iFrameLen = 0;
        tFileFrameInfo.iFrameProcessedLen = 0;
        tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf;
        tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen;
        tFileFrameInfo.iFrameBufMaxLen = m_pbInputBuf->iBufMaxLen;
        if(STREAM_TYPE_VIDEO_STREAM==i_eSrcStreamType)
        {//非文件流使用m_oMediaHandle则要保证数据只有一帧
            tFileFrameInfo.pbFrameStartPos = NULL;//因此干脆直接使用ParseNaluFromFrame
            tFileFrameInfo.eStreamType = STREAM_TYPE_UNKNOW;//ParseNaluFromFrame 赋值需要先设置为MEDIA_ENCODE_TYPE_UNKNOW
            ParseNaluFromFrame(&tFileFrameInfo);
            tFileFrameInfo.eStreamType = i_eSrcStreamType;
        }
        else//GetFrame 内aac已经改为使用eFrameType判断外部是否已经完善了一帧的信息
        {//aac的eFrameType为UNKNOW，则可以使用GetFrame解析,内部会进行赋值
            m_oMediaHandle.GetFrame(&tFileFrameInfo);//后续视频裸流也改为使用eFrameType判断，而不是eStreamType，则也可以按照aac的方式使用GetFrame
        }
        memcpy(&m_tFileFrameInfo,&tFileFrameInfo,sizeof(T_MediaFrameInfo));
        m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);//处理了的都要删去，flv每次处理一个tag,不是一帧
        if(tFileFrameInfo.iFrameLen <= 0)
        {
            //printf("tFileFrameInfo.iFrameLen <= 0 [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,tFileFrameInfo.iFrameProcessedLen);
            break;
        } 
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
        {
            m_iFindKeyFrame=1;
            m_tSegInfo.iHaveKeyFrameFlag=1;
        }
        else
        {
            if(m_iFindKeyFrame==0)
                continue;
        }
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_tSegInfo.iVideoFrameCnt++;
            m_eDstVideoEncType = tFileFrameInfo.eEncType;
            m_iPutVideoFrameCnt++;//tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp 要在SynchronizerAudioVideo前
            if(m_dwLastVideoTimeStamp != 0)//带音频不能非gop分段，否则出错(web播着会一直卡顿不出流或者只放i帧)
                m_dwPutVideoFrameTime+=m_iPutVideoFrameCnt>1?(tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp):0;
            //m_dwLastVideoTimeStamp = tFileFrameInfo.dwTimeStamp;//调用 SynchronizerAudioVideo则要注释这里
        }
        //if(i_eDstStreamType != STREAM_TYPE_FMP4_STREAM)//
            SynchronizerAudioVideo(&tFileFrameInfo);//mp4是音频轨和视频轨是分别单独的，所以不需要同源处理,处理了也不影响
        if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
        {
            m_tSegInfo.iAudioFrameCnt++;
            if(i_eDstStreamType == STREAM_TYPE_FMP4_STREAM)
            {//mse(mp4)才需要转换
                if(AudioTranscode(&tFileFrameInfo)<=0)
                {
                    continue;//转换失败或数据不够，则跳过这一帧
                }
            }
            m_eDstAudioEncType = tFileFrameInfo.eEncType;
        }

#ifdef SUPPORT_PRI
        if(STREAM_TYPE_UNKNOW == i_eDstStreamType)
        {
            FRAME_INFO* ptFrameInfo = MediaToPri(&tFileFrameInfo);
            if(NULL == ptFrameInfo || ptFrameInfo->nLength <= 0)
            {
                printf("MediaToPri err %d %d\r\n",tFileFrameInfo.iFrameLen,ptFrameInfo->nLength);
                continue;
            }
            if(NULL == pbOutBuf)
            {
                pbOutBuf = new DataBuf(ptFrameInfo->nLength);
            }
            pbOutBuf->Copy(ptFrameInfo->pHeader,ptFrameInfo->nLength);
            m_pDataBufList.push_back(pbOutBuf);
            pbOutBuf = NULL;
            ptFrameInfo->Release();
            continue;
        }
#endif
        //printf("FrameType %d TimeStamp %u PutFrameTime %u FrameLen %d\r\n", tFileFrameInfo.eFrameType, tFileFrameInfo.dwTimeStamp,m_dwPutVideoFrameTime, tFileFrameInfo.iFrameLen);
        if(0 == m_iPutFrameLen)
            printf("start m_dwPutVideoFrameTime %u ,%d iFrameLen %d eFrameType[%d] eEncType[%d]\r\n",m_dwPutVideoFrameTime,m_pbInputBuf->iBufLen,tFileFrameInfo.iFrameLen,tFileFrameInfo.eFrameType,tFileFrameInfo.eEncType);
        m_iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {//如果每次都传入一个足够大的缓存，则不需要统计 iPutFrameLen这么麻烦
            pbOutBuf = new DataBuf(m_iPutFrameLen+MEDIA_FORMAT_MAX_LEN);//但是每次(帧)都这么大的缓存，如果不能及时释放则内存容易耗尽
        }//带音频不能非gop分段，否则出错(web播着会一直卡顿不出流)
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen,m_dwPutVideoFrameTime>=600?1:0);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            m_iPutFrameLen=0;
            m_dwPutVideoFrameTime=0;
            m_iPutVideoFrameCnt=0;
            memset(&m_tSegInfo,0,sizeof(T_SegInfo));
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//没有获取到封装数据(输入数据不够)
                delete pbOutBuf;//但是下次输出数据会更大，获取封装数据需要更大的缓存
                pbOutBuf = NULL;//所以需要释放，重新分配
            }//如果每次都传入一个足够大的缓存，则不需要这么麻烦
            continue;
        }
        //if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
            printf("FrameToContainer m_dwPutVideoFrameTime %u Len %d iWriteLen %d ProcessedLen[%d] iBufMaxLen[%d]\r\n",m_dwPutVideoFrameTime,m_iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen,pbOutBuf->iBufMaxLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_tSegInfo.iSegDurationTime=tFileFrameInfo.dwTimeStamp-m_tSegInfo.iSegStartTime;
        memcpy(&pbOutBuf->tSegInfo,&m_tSegInfo,sizeof(T_SegInfo));
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        memset(&m_tSegInfo,0,sizeof(T_SegInfo));
        m_tSegInfo.iSegStartTime=tFileFrameInfo.dwTimeStamp;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//没用到的封装缓存长度
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//总数据-被打包的数据长度和封装长度=未打包数据长度+未用封装缓存长度再减iRemain=未打包数据长度
        m_iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLen要等于封装对象中缓存的未打包的数据长度，这样内存才够。
        m_dwPutVideoFrameTime=0;
        m_iPutVideoFrameCnt=0;
    }//gop类打包(mp4)，最新的i帧不会被打包是下次打包，最新的帧长度= 封装对象中缓存的未打包的数据长度
    if(NULL != pbOutBuf)
    {
        delete pbOutBuf;
    }
    
    return m_pDataBufList.size();
}

/*****************************************************************************
-Fuction		: GetData
-Description	: GetData
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::GetData(unsigned char * o_pbData,int i_iMaxDataLen,T_SegInfo *o_ptSegInfo)
{
    int iRet = -1;
    
    if(NULL == o_pbData || i_iMaxDataLen<= 0||NULL == o_ptSegInfo)
    {
        printf("MediaConvert GetData NULL %d %p\r\n",i_iMaxDataLen,o_ptSegInfo);
        return iRet;
    }
    
    if(!m_pDataBufList.empty())
    {
        DataBuf * it = m_pDataBufList.front();
        if(it->iBufLen > i_iMaxDataLen)
        {
            printf("GetData err %d,%d\r\n",it->iBufLen, i_iMaxDataLen);
            iRet = -1;
        }
        else
        {
            memcpy(o_pbData,it->pbBuf,it->iBufLen);
            iRet =it->iBufLen;
            memcpy(o_ptSegInfo,&it->tSegInfo,sizeof(T_SegInfo));
            //uint64 ddwTime=it->tSegInfo.dwStartTimeHigh<<32|it->tSegInfo.dwStartTimeLow;
            //printf("GetData iBufLen %d iBufMaxLen[%d] iHaveKeyFrameFlag %d iSegStartTime[%d][%d %d][%lld]\r\n",
            //it->iBufLen,it->iBufMaxLen,it->tSegInfo.iHaveKeyFrameFlag,it->tSegInfo.iSegStartTime,it->tSegInfo.iVideoFrameCnt,it->tSegInfo.iVideoFrameCnt,ddwTime);
            delete it;
            m_pDataBufList.pop_front();
        }
    }
    else
    {
        iRet = 0;
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: GetData
-Description	: GetData
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen)
{
    int iRet = -1;
    
    if(NULL != o_pbVideoEncBuf && i_iMaxVideoEncBufLen>4)
    {
        switch(m_eDstVideoEncType)
        {
            case MEDIA_ENCODE_TYPE_H264:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","h264");
                iRet=0;
                break;
            }
            case MEDIA_ENCODE_TYPE_H265:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","h265");
                iRet=0;
                break;
            }
            default:
            {
                printf("m_eDstVideoEncType err %d %d\r\n",i_iMaxVideoEncBufLen,m_eDstVideoEncType);
                iRet=-1;
                break;
            }
        }
    }

    if(NULL != o_pbAudioEncBuf && i_iMaxAudioEncBufLen>4)
    {
        switch(m_eDstAudioEncType)
        {
            case MEDIA_ENCODE_TYPE_AAC:
            {
                snprintf((char *)o_pbAudioEncBuf,i_iMaxAudioEncBufLen,"%s","aac");
                iRet=0;
                break;
            }
            case MEDIA_ENCODE_TYPE_G711A:
            {
                snprintf((char *)o_pbAudioEncBuf,i_iMaxAudioEncBufLen,"%s","g711a");
                iRet=0;
                break;
            }
            default:
            {
                printf("m_eDstAudioEncType err %d %d\r\n",i_iMaxAudioEncBufLen,m_eDstAudioEncType);
                iRet=-1;
                break;
            }
        }
    }
    if(iRet<0)
    {
        printf("GetEncodeType err %d %d\r\n",i_iMaxVideoEncBufLen,i_iMaxAudioEncBufLen);
        return iRet;
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: SynchronizerAudioAndVideo
-Description    : 如果音视频不同步，可能会导致播放器跳帧
比如ffplay对于落后音频帧很多时间的视频帧 会直接丢掉，造成跳帧(只播i帧现象)
// 音视频同步策略
// 当音频时间超过视频时间时，丢掉音频数据(如果同步音频时间会造成视频间隔不均匀(偶尔跳秒))
// 1.音视频同步策略
// 当音频时间落后(500ms)后，同步视频时间（因音频时间超过视频时间时会丢弃，所以不存在音频时间超前问题）
// 2.音视频同步策略
// 当视频时间超过音频时间，同步音频时间(会差几十ms)
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::SynchronizerAudioVideo(T_MediaFrameInfo * i_ptFrameInfo)
{
    int iRet = -1;
    unsigned int dwTimeStamp = 0;
    unsigned int dwTimeStampDiff = 0;

    if(NULL == i_ptFrameInfo)
    {
        printf("SynchronizerAudioVideo NULL %d\r\n",iRet);
        return iRet;
    }
    
    dwTimeStamp = i_ptFrameInfo->dwTimeStamp;    // 使用到达时间戳流畅且有实时性(使用帧时间戳会跳秒)
    if (i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
    i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
    i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
    {// i_pFrame->nTimeStamp;//音视频时钟不同源96                                           
        if (0 == m_dwLastVideoTimeStamp)
        {
            m_dwLastVideoTimeStamp = dwTimeStamp;
        }
        dwTimeStampDiff = (unsigned int)(dwTimeStamp - m_dwLastVideoTimeStamp);
        if(dwTimeStampDiff>400)
        {
            printf("dwTimeStampDiff %u>400 dwTimeStamp %u m_dwLastVideoTimeStamp %u \r\n",dwTimeStampDiff, dwTimeStamp, m_dwLastVideoTimeStamp);
        }
        else
        {
            m_dwVideoTimeStamp += dwTimeStampDiff;
        }

        if (m_dwVideoTimeStamp < m_dwAudioTimeStamp) // 2.音视频同步策略
        {// 视频时间超过音频时间，同步音频时间(视频间隔不均匀 会差几十ms)
            //printf("m_dwVideoTimeStamp %d < m_dwAudioTimeStamp %d \r\n", m_dwVideoTimeStamp, m_dwAudioTimeStamp);
            m_dwVideoTimeStamp = m_dwAudioTimeStamp; // ffplay 播放效果可以
        }// 如果没有这个策略，会出现视频比音频慢，从而导致播放跳秒(ffplay会跳秒)

        m_dwLastVideoTimeStamp = dwTimeStamp;
        dwTimeStamp = m_dwVideoTimeStamp;
    }
    else if (i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        //i_ptFrameInfo->dwTimeStamp = m_dwVideoTimeStamp; //直接使用，web端会卡
        //return 0;
    
        if (0 == m_dwLastAudioTimeStamp)
        {
            m_dwLastAudioTimeStamp = dwTimeStamp;
            m_dwAudioTimeStamp = m_dwVideoTimeStamp; // 音视频同步，必须同源处理，内部MP4打包依赖同源时间戳
        }
        m_dwAudioTimeStamp += (unsigned int)(dwTimeStamp - m_dwLastAudioTimeStamp);

        if (m_dwAudioTimeStamp < m_dwVideoTimeStamp) // 1.音视频同步策略
        {// 当音频时间落后，同步视频时间
            m_dwAudioTimeStamp = m_dwVideoTimeStamp; // 0.如果只有音频时间戳直接使用视频时间戳的策略，(ffplay)播放效果还可以
        }// （因音频时间超过视频时间时会丢弃，所以不存在音频时间超前问题）
        /*if (m_dwAudioTimeStamp > m_dwVideoTimeStamp)// 2.音视频同步策略
        {// 当音频时间超过视频时间时，丢掉音频数据
            m_ddwLastAudioTimeStamp =ddwTimeStamp;//(如果同步音频时间会造成视频间隔不均匀(会差几十ms))
            HTTP_LOGD(this,"m_dwAudioTimeStamp %d > m_dwVideoTimeStamp %d \r\n",m_dwAudioTimeStamp,m_dwVideoTimeStamp);
            //return iRet;//这里的时间戳基本都大于视频时间戳,如果return就都丢数据了,最多改m_dwAudioTimeStamp =
        m_dwVideoTimeStamp;
        }//这里的1和2如果同时启用，和音频时间戳直接使用视频时间戳没区别了*/

        m_dwLastAudioTimeStamp = dwTimeStamp;
        dwTimeStamp = m_dwAudioTimeStamp;//m_dwVideoTimeStamp;m_dwAudioTimeStamp
    }

    i_ptFrameInfo->dwTimeStamp = dwTimeStamp; //
    return 0;
}

/*****************************************************************************
-Fuction		: AudioTranscode
-Description	: g711转aac(mse只支持aac)
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::AudioTranscode(T_MediaFrameInfo * m_pbAudioFrame)
{
    int iRet = -1;
    T_AudioCodecParam i_tSrcCodecParam;
    T_AudioCodecParam i_tDstCodecParam;
    E_AudioCodecType eSrcAudioCodecType=AUDIO_CODEC_TYPE_UNKNOW;
    int iWriteLen = 0;


    if(NULL == m_pbAudioFrame || m_pbAudioFrame->iFrameLen<= 0)
    {
        printf("AudioTranscode NULL %p\r\n",m_pbAudioFrame);
        return iRet;
    }
    if(m_eDstAudioEncType == MEDIA_ENCODE_TYPE_UNKNOW)
        printf("AudioTranscode,eEncType %d,dwSampleRate %d\r\n",m_pbAudioFrame->eEncType,m_pbAudioFrame->dwSampleRate);
    switch(m_pbAudioFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_G711A:
        {
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_PCMA;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_PCMU;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            if(m_eDstAudioEncType == MEDIA_ENCODE_TYPE_UNKNOW)
                printf("already aac,dwSampleRate %d,iFrameLen %d\r\n",m_pbAudioFrame->dwSampleRate,m_pbAudioFrame->iFrameLen);
            //if(m_pbAudioFrame->dwSampleRate == 44100)
            {
                return m_pbAudioFrame->iFrameLen;
            }
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_AAC;
            break;
        }
        default :
        {
            printf("AudioTranscode eEncType err %d\r\n",m_pbAudioFrame->eEncType);
            return iRet;
        }
    }
    memset(&i_tSrcCodecParam,0,sizeof(T_AudioCodecParam));
    i_tSrcCodecParam.dwBitsPerSample=m_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample;
    i_tSrcCodecParam.dwChannels=m_pbAudioFrame->tAudioEncodeParam.dwChannels;
    i_tSrcCodecParam.dwSampleRate=m_pbAudioFrame->dwSampleRate;
    i_tSrcCodecParam.eAudioCodecType=eSrcAudioCodecType;
    memcpy(&i_tDstCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    i_tDstCodecParam.eAudioCodecType=AUDIO_CODEC_TYPE_AAC;
    i_tDstCodecParam.dwSampleRate=44100;//采样率要转换，否则web端无法支持

    iRet = m_pAudioCodec->Transcode(m_pbAudioFrame->pbFrameStartPos,m_pbAudioFrame->iFrameLen,i_tSrcCodecParam,m_pAudioTranscodeBuf,MEDIA_FORMAT_MAX_LEN,i_tDstCodecParam);
    if(iRet <= 0)
    {
        //printf("Transcode iRet %d iFrameLen%d\r\n",iRet,m_pbAudioFrame->iFrameLen);
        return iRet;
    } 
    //printf("Transcode iRet %d iFrameLen%d\r\n",iRet,m_pbAudioFrame->iFrameLen);
    iWriteLen=iRet;
    iRet=0;
    do
    {
        iWriteLen+=iRet;
        iRet=m_pAudioCodec->Transcode(NULL,0,i_tSrcCodecParam,m_pAudioTranscodeBuf+iWriteLen,MEDIA_FORMAT_MAX_LEN-iWriteLen,i_tDstCodecParam);
    } while(iRet>0);


    switch(i_tDstCodecParam.eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_AAC:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMU:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711U;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMA:
        {
            m_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        default :
        {
            printf("AudioTranscode dst eEncType err %d\r\n",i_tDstCodecParam.eAudioCodecType);
            return iRet;
        }
    }
    m_pbAudioFrame->dwSampleRate=i_tDstCodecParam.dwSampleRate;
    m_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample=i_tDstCodecParam.dwBitsPerSample;
    m_pbAudioFrame->tAudioEncodeParam.dwChannels=i_tDstCodecParam.dwChannels;
    m_pbAudioFrame->pbFrameBuf=m_pAudioTranscodeBuf;//
    m_pbAudioFrame->iFrameBufLen=iWriteLen;//
    m_pbAudioFrame->iFrameBufMaxLen=MEDIA_FORMAT_MAX_LEN;//
    m_pbAudioFrame->iFrameLen=iWriteLen;
    m_pbAudioFrame->pbFrameStartPos=m_pAudioTranscodeBuf;
    
    return iWriteLen;
}

#ifdef SUPPORT_PRI
/*****************************************************************************
-Fuction        : WebRtcFrameToFrameInfo
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
FRAME_INFO* MediaConvert::MediaToPri(T_MediaFrameInfo * i_ptMediaFrame) 
{
	FRAME_INFO* ptFrameInfo = new FRAME_INFO();

    switch (i_ptMediaFrame->eFrameType)
    {
        case MEDIA_FRAME_TYPE_VIDEO_I_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_P_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_B_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_B_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_AUDIO_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_AUDIO;
            break;
        }
        default:
            break;
    }
    switch (i_ptMediaFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_H264:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H264;
            break;
        }
        case MEDIA_ENCODE_TYPE_H265:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H265;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_AAC;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711A:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711A;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711U;
            break;
        }
        default:
        {
            printf("ptFrameInfo->eEncType err%d\r\n",i_ptMediaFrame->eEncType);
            delete ptFrameInfo;
            return NULL;
        }
    }
	ptFrameInfo->nSamplesPerSecond = i_ptMediaFrame->dwSampleRate;
	ptFrameInfo->nTimeStamp = i_ptMediaFrame->dwTimeStamp;
	ptFrameInfo->nCumulativeTimeMS = i_ptMediaFrame->dwTimeStamp;
	ptFrameInfo->nFrameLength = i_ptMediaFrame->iFrameLen;
	ptFrameInfo->nChannels = i_ptMediaFrame->tAudioEncodeParam.dwChannels;
	//if(0 != m_iPullVideoFrameRate ||0!=m_dwTalkVideoFPS)
    	//ptFrameInfo->nFrameRate = m_iPullVideoFrameRate==0?m_dwTalkVideoFPS:m_iPullVideoFrameRate;//如果前端没有传递帧率则使用json里面的告诉转码库要转成多少帧率
	ptFrameInfo->nWidth = i_ptMediaFrame->dwWidth;//宽高不赋值，宽高不一致就会触发转码
	ptFrameInfo->nHeight = i_ptMediaFrame->dwHeight;//h265 pc浏览器 b帧；手机浏览器帧切片，都需要转码

    FRAME_INFO* pFrame = CSTDStream::NewFrame(ptFrameInfo, (const char*)i_ptMediaFrame->pbFrameStartPos, i_ptMediaFrame->iFrameLen);
    pFrame->AddRef();
    delete ptFrameInfo;
	return pFrame;
	return NULL;
}

/*****************************************************************************
-Fuction        : GenerateFlvData
-Description    : 
-Input          : 
-Output         : 
-Return         : len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::PriToMedia(FRAME_INFO * i_pFrame,T_MediaFrameInfo *o_ptMediaFrameInfo)
{
    int iRet=-1;
    E_MediaEncodeType eEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    E_MediaFrameType eFrameType=MEDIA_FRAME_TYPE_UNKNOW;
    E_StreamType eStreamType=STREAM_TYPE_FLV_STREAM;
    
    if(NULL == i_pFrame)
    {
        printf("PriToMedia err NULL\r\n");
        return iRet;
    }
    switch(i_pFrame->nEncodeType)
    {
        case ENCODE_VIDEO_H264 :
        {
            eEncType = MEDIA_ENCODE_TYPE_H264;
            break;
        }
        case ENCODE_VIDEO_H265 :
        {
            eEncType = MEDIA_ENCODE_TYPE_H265;
            break;
        }
        case ENCODE_AUDIO_AAC :
        {
            eEncType = MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case ENCODE_AUDIO_G711A :
        {
            eEncType = MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        default :
        {
            if (i_pFrame->nType != FRAME_TYPE_DATA)
                printf("PriToMedia i_ptFrame->eEncType err %d,nType %d,nSubType %d\r\n",i_pFrame->nEncodeType,i_pFrame->nType,i_pFrame->nSubType);
            return iRet;
        }
    }
    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        switch (i_pFrame->nSubType)
        {
            case FRAME_TYPE_VIDEO_I_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
                break;
            }
            case FRAME_TYPE_VIDEO_P_FRAME:
            case FRAME_TYPE_VIDEO_S_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
                break;
            }
            case FRAME_TYPE_VIDEO_B_FRAME:
            {
                eFrameType = MEDIA_FRAME_TYPE_VIDEO_B_FRAME;
                break;
            }
            default:
            {
                printf("PriToMedia i_ptFrame->eFrameType err %d\r\n",i_pFrame->nSubType);
                return iRet;
            }
        }
    }
    else if (i_pFrame->nType == FRAME_TYPE_AUDIO)
    {
        eFrameType = MEDIA_FRAME_TYPE_AUDIO_FRAME;
    }
    else
    {
        printf("PriToMedia i_ptFrame->nType err%d\r\n",i_pFrame->nType);
        return iRet;
    }

    o_ptMediaFrameInfo->pbFrameBuf = i_pFrame->pContent;
    o_ptMediaFrameInfo->iFrameBufMaxLen= i_pFrame->nFrameLength;
    o_ptMediaFrameInfo->iFrameBufLen = i_pFrame->nFrameLength;
    o_ptMediaFrameInfo->eStreamType= STREAM_TYPE_MUX_STREAM;//视频流需要解析成多个nalu，音频数据可直接外部输入

    o_ptMediaFrameInfo->eEncType = eEncType;
    o_ptMediaFrameInfo->eFrameType = eFrameType;

    o_ptMediaFrameInfo->dwTimeStamp=i_pFrame->nTimeStamp;
    o_ptMediaFrameInfo->dwWidth= i_pFrame->nWidth;
    o_ptMediaFrameInfo->dwHeight=i_pFrame->nHeight;

    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        o_ptMediaFrameInfo->dwSampleRate= 90000;//h264频率不会传过来，h264频率可以固定
        iRet=ParseNaluFromFrame(o_ptMediaFrameInfo);
        if(iRet<0)
        {
            printf("PriToMedia ParseNaluFromFrame err %d\r\n",i_pFrame->nEncodeType);
            return iRet;
        }
    }
    else if (i_pFrame->nType == FRAME_TYPE_AUDIO)
    {
        o_ptMediaFrameInfo->dwSampleRate= i_pFrame->nSamplesPerSecond;
        o_ptMediaFrameInfo->tAudioEncodeParam.dwChannels= i_pFrame->nChannels;
        o_ptMediaFrameInfo->tAudioEncodeParam.dwBitsPerSample= i_pFrame->nBitsPerSample;
        o_ptMediaFrameInfo->pbFrameStartPos = i_pFrame->pContent;
        o_ptMediaFrameInfo->iFrameLen= i_pFrame->nFrameLength;
    }
    return 0;
}
#endif
/*****************************************************************************
-Fuction        : ParseNaluFromFrame
-Description    : ParseNaluFromFrame
-Input          : 
-Output         : 
-Return         : iRet must be 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseNaluFromFrame(T_MediaFrameInfo * o_ptFrameInfo)
{
    int iRet = -1;
    
    if(NULL == o_ptFrameInfo->pbFrameBuf || MEDIA_ENCODE_TYPE_UNKNOW == o_ptFrameInfo->eEncType  ||o_ptFrameInfo->iFrameBufLen <= 4)
    {
        printf("ParseNaluFromFrame NULL %d\r\n", o_ptFrameInfo->iFrameBufLen);
        return iRet;
    }
    if(MEDIA_ENCODE_TYPE_H264 == o_ptFrameInfo->eEncType)
        iRet = ParseH264NaluFromFrame(o_ptFrameInfo);
    else if(MEDIA_ENCODE_TYPE_H265 == o_ptFrameInfo->eEncType)
        iRet = ParseH265NaluFromFrame(o_ptFrameInfo);

    return iRet;
}

/*****************************************************************************
-Fuction        : ParseNaluFromFrame
-Description    : ParseNaluFromFrame
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseH264NaluFromFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    unsigned char bStartCodeLen = 0;
    int iFrameType = MEDIA_FRAME_TYPE_UNKNOW;


    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf ||m_ptFrame->iFrameBufLen <= 4)
    {
        printf("ParseH264NaluFromFrame GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
        return iRet;
    }
	
	pcFrameData = m_ptFrame->pbFrameBuf;
	iRemainDataLen = m_ptFrame->iFrameBufLen;
    m_ptFrame->dwNaluCount = 0;
    m_ptFrame->iFrameLen = 0;
    while(iRemainDataLen > 0)
    {
        if (iRemainDataLen >= 3 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;//此时是一个nalu的结束
            }
            else
            {
                pcNaluStartPos = pcFrameData;//此时是一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//上一个nalu的结束为下一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 3;
            iRemainDataLen -= 3;
        }
        else if (iRemainDataLen >= 4 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 0 && pcFrameData[3] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;
            }
            else
            {
                pcNaluStartPos = pcFrameData;
                bStartCodeLen = 4;
                bNaluType = pcNaluStartPos[4] & 0x1f;
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = pcNaluStartPos[4] & 0x1f;
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 4;
            iRemainDataLen -= 4;
        }
        else
        {
            pcFrameData ++;
            iRemainDataLen --;
        }
    }
    if(pcNaluStartPos != NULL && (iFrameType<=0 ||STREAM_TYPE_UNKNOW != m_ptFrame->eStreamType))
    {//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
        iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcFrameData - pcNaluStartPos,m_ptFrame);
        if(iRet <= 0 && iFrameType<=0)
        {
            printf("SetH264NaluData err %d %d\r\n", m_ptFrame->dwNaluCount,m_ptFrame->iFrameLen);
            return iRet;
        }
        if(iRet>0)
            iFrameType = iRet;
    }
    if(iFrameType>0)
        iRet=0;
	if(NULL != m_ptFrame->pbFrameStartPos)
	{
        m_ptFrame->iFrameProcessedLen += m_ptFrame->pbFrameStartPos - m_ptFrame->pbFrameBuf + m_ptFrame->iFrameLen;
	}
    return iRet;
}

/*****************************************************************************
-Fuction        : GetFrame
-Description    : GetFrame
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::ParseH265NaluFromFrame(T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char *pcNaluStartPos = NULL;
    unsigned char *pcNaluEndPos = NULL;
    unsigned char *pcFrameData = NULL;
    int iRemainDataLen = 0;
    unsigned char bNaluType = 0;
    unsigned char bStartCodeLen = 0;
    int iFrameType = MEDIA_FRAME_TYPE_UNKNOW;


    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf ||m_ptFrame->iFrameBufLen <= 4)
    {
        printf("ParseH264NaluFromFrame GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
        return iRet;
    }
	
	pcFrameData = m_ptFrame->pbFrameBuf;
	iRemainDataLen = m_ptFrame->iFrameBufLen;
    m_ptFrame->dwNaluCount = 0;
    m_ptFrame->iFrameLen = 0;
    while(iRemainDataLen > 0)
    {
        if (iRemainDataLen >= 3 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;//此时是一个nalu的结束
            }
            else
            {
                pcNaluStartPos = pcFrameData;//此时是一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//取nalu类型
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//上一个nalu的结束为下一个nalu的开始
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//取nalu类型
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 3;
            iRemainDataLen -= 3;
        }
        else if (iRemainDataLen >= 4 && pcFrameData[0] == 0 && pcFrameData[1] == 0 && pcFrameData[2] == 0 && pcFrameData[3] == 1)
        {
            if(pcNaluStartPos != NULL)
            {
                pcNaluEndPos = pcFrameData;
            }
            else
            {
                pcNaluStartPos = pcFrameData;
                bStartCodeLen = 4;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet = SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);//包括类型减4//去掉00 00 00 01
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//取nalu类型
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
                        break;//解析出一帧则退出
                }
            }
            pcFrameData += 4;
            iRemainDataLen -= 4;
        }
        else
        {
            pcFrameData ++;
            iRemainDataLen --;
        }
    }
    if(pcNaluStartPos != NULL && (iFrameType<=0 ||STREAM_TYPE_UNKNOW != m_ptFrame->eStreamType))
    {//非文件裸流，外部已认定是一帧，则数据要全部解析完再退出,防止帧切片得不到解析的情况
        iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcFrameData - pcNaluStartPos,m_ptFrame);//包括类型减4开始码
        if(iRet <= 0 && iFrameType<=0)
        {
            printf("SetH265NaluData err bNaluType%d bStartCodeLen%d dwNaluCount%d iFrameLen%d\r\n", bNaluType,bStartCodeLen,m_ptFrame->dwNaluCount,m_ptFrame->iFrameLen);
            return iRet;
        }
        if(iRet>0)
            iFrameType = iRet;
    }
    if(iFrameType>0)
        iRet=0;
	if(NULL != m_ptFrame->pbFrameStartPos)
	{
        m_ptFrame->iFrameProcessedLen += m_ptFrame->pbFrameStartPos - m_ptFrame->pbFrameBuf + m_ptFrame->iFrameLen;
	}
    return iRet;
}


/*****************************************************************************
-Fuction        : SetH264NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : eFrameType
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::SetH264NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char * pbNaluData = NULL;//去掉00 00 00 01
    int iNaluDataLen=0;
    E_MediaFrameType eFrameType = MEDIA_FRAME_TYPE_UNKNOW;
    
    if(NULL == i_pbNaluData || NULL == m_ptFrame)
    {
        printf("SetH264NaluData NULL %d \r\n", i_iNaluDataLen);
        return iRet;
    }
    
    if(m_ptFrame->pbFrameStartPos == NULL)
    {
        m_ptFrame->pbFrameStartPos = i_pbNaluData;
    }
    m_ptFrame->iFrameLen += i_iNaluDataLen;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].pbData= i_pbNaluData;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].dwDataLen= i_iNaluDataLen;
    m_ptFrame->dwNaluCount++;

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//包括类型减开始码
    pbNaluData = i_pbNaluData+i_bStartCodeLen;
    switch(i_bNaluType)//取nalu类型
    {
        case 0x7:
        {
            memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//包括类型(减3开始码)
            memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
            break;
        }
        case 0x8:
        {
            memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfPPS = iNaluDataLen;//包括类型减3开始码
            memcpy(m_ptFrame->tVideoEncodeParam.abPPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfPPS);
            break;
        }
        case 0x1:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case 0x5:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        default:
        {
            break;
        }
    }

    if(MEDIA_FRAME_TYPE_UNKNOW != eFrameType)
    {
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//文件的时候才需要赋值，数据流的时候外部会赋值以外部为准
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H264;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//解析出一帧则退出
    }
    return eFrameType;
}
/*****************************************************************************
-Fuction        : SetH265NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : eFrameType
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int MediaConvert::SetH265NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame)
{
    int iRet = -1;
    unsigned char * pbNaluData = NULL;//去掉00 00 00 01
    int iNaluDataLen=0;
    E_MediaFrameType eFrameType = MEDIA_FRAME_TYPE_UNKNOW;
    
    if(NULL == i_pbNaluData || NULL == m_ptFrame)
    {
        printf("SetH265NaluData NULL %d \r\n", iRet);
        return iRet;
    }
    
    if(m_ptFrame->pbFrameStartPos == NULL)
    {
        m_ptFrame->pbFrameStartPos = i_pbNaluData;
    }
    m_ptFrame->iFrameLen += i_iNaluDataLen;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].pbData= i_pbNaluData;
    m_ptFrame->atNaluInfo[m_ptFrame->dwNaluCount].dwDataLen= i_iNaluDataLen;
    m_ptFrame->dwNaluCount++;

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//包括类型减开始码
    pbNaluData = i_pbNaluData+i_bStartCodeLen;

    if(i_bNaluType >= 0 && i_bNaluType <= 9)// p slice 片
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
    }
    else if(i_bNaluType >= 16 && i_bNaluType <= 21)// IRAP 等同于i帧
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
    }
    else if(i_bNaluType == 32)//VPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abVPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abVPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfVPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abVPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfVPS);
    }
    else if(i_bNaluType == 33)//SPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
    }
    else if(i_bNaluType == 34)//PPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfPPS= iNaluDataLen;//包括类型减4
        memcpy(m_ptFrame->tVideoEncodeParam.abPPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfPPS);
    }
    
    if(MEDIA_FRAME_TYPE_UNKNOW != eFrameType)
    {
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//文件的时候才需要赋值，数据流的时候外部会赋值以外部为准
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H265;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//解析出一帧则退出
    }
    return eFrameType;
}


/*****************************************************************************
-Fuction        : InputData
-Description    : InputData
-Input          : 输入的数据类型 ,输出的数据类型
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int InputData(unsigned char * i_pbSrcData,int i_iSrcDataLen,const char *i_strSrcName,const char *i_strDstName)
{
    E_StreamType eSrcStreamType=STREAM_TYPE_UNKNOW;
    E_StreamType eDstStreamType=STREAM_TYPE_UNKNOW;
    E_MediaEncodeType eSrcEncType=MEDIA_ENCODE_TYPE_UNKNOW;

    if(NULL != strstr(i_strDstName,"ts"))
    {
        eDstStreamType=STREAM_TYPE_TS_STREAM;
    }
    else if(NULL != strstr(i_strDstName,"mp4"))
    {
        eDstStreamType=STREAM_TYPE_FMP4_STREAM;
    }
    else if(NULL != strstr(i_strDstName,"flv"))
    {
        eDstStreamType=STREAM_TYPE_ENHANCED_FLV_STREAM;
    }
    else if(NULL != strstr(i_strDstName,"VideoRaw"))
    {
        eDstStreamType=STREAM_TYPE_VIDEO_STREAM;
    }
    else if(NULL != strstr(i_strDstName,"AudioRaw"))
    {
        eDstStreamType=STREAM_TYPE_AUDIO_STREAM;
    }
#ifdef SUPPORT_PRI
    else if(NULL != strstr(i_strDstName,"pri"))
    {
        eDstStreamType=STREAM_TYPE_UNKNOW;
    }
#endif    
    else
    {
        printf("i_strDstName %s err\r\n",i_strDstName);
        return -1;
    }

    if(NULL != strstr(i_strSrcName,"pri"))
    {
#ifdef SUPPORT_PRI
        //printf("Convert %s to %s\r\n",i_strSrcName,i_strDstName);
        return MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#else
        printf("Convert %s to %s unsupport\r\n",i_strSrcName,i_strDstName);
        return -1;//MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#endif    
    }

    if(NULL != strstr(i_strSrcName,"flv"))
    {
        eSrcStreamType=STREAM_TYPE_FLV_STREAM;
    }
    else if(NULL != strstr(i_strSrcName,"h264"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H264;
    }
    else if(NULL != strstr(i_strSrcName,"h265"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H265;
    }
    else if(NULL != strstr(i_strSrcName,"aac"))
    {
        eSrcStreamType=STREAM_TYPE_AUDIO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_AAC;
    }
    else
    {
        printf("i_strSrcName %s err\r\n",i_strSrcName);
        return -1;
    }
    printf("Convert %d ,%s to %s\r\n",i_iSrcDataLen,i_strSrcName,i_strDstName);
    return MediaConvert::Instance()->Convert(i_pbSrcData,i_iSrcDataLen,eSrcEncType,eSrcStreamType,eDstStreamType);
}


/*****************************************************************************
-Fuction        : GetData
-Description    : 一次返回输入数据的最小单位，
比如裸流就返回一帧
flv返回一帧对应的tag
mp4返回一个gop
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int GetData(unsigned char * o_pbData,int i_iMaxDataLen,unsigned char * o_pbDataInfo,int i_iMaxInfoLen)
{
    if(NULL == o_pbDataInfo || i_iMaxInfoLen<= sizeof(T_SegInfo))
    {
        printf("GetData NULL %d %d\r\n",i_iMaxDataLen,i_iMaxInfoLen);
        return -1;
    }
    T_SegInfo * ptSegInfo=(T_SegInfo *)o_pbDataInfo;
    return MediaConvert::Instance()->GetData(o_pbData,i_iMaxDataLen,ptSegInfo);
}


/*****************************************************************************
-Fuction        : GetData
-Description    : 一次返回输入数据的最小单位，
比如裸流就返回一帧
flv返回一帧对应的tag
mp4返回一个gop
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen)
{
    return MediaConvert::Instance()->GetEncodeType(o_pbVideoEncBuf,i_iMaxVideoEncBufLen,o_pbAudioEncBuf,i_iMaxAudioEncBufLen);
}


/*****************************************************************************
-Fuction        : Clean
-Description    : 清除缓存数据，重新开始转换
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int Clean()
{
    if(NULL != MediaConvert::m_pInstance)
    {
        delete MediaConvert::m_pInstance;
    }
    MediaConvert::m_pInstance = new MediaConvert();
    return 0;
}

