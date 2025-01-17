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

MediaConvert * MediaConvert::m_pInstance = new MediaConvert();//һ��ʹ�ö���ģʽ,����ģʽ�̲߳���ȫ
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
    m_pbInputBuf = new DataBuf(MEDIA_INPUT_BUF_MAX_LEN);
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
	int iPutFrameLen=0;

#ifdef SUPPORT_PRI
    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    m_streamer.Inputdata(i_pbSrcData,i_iSrcDataLen);
    while(1)
    {
        NSXPTL::FRAME_INFO* pFrame = m_streamer.GetNextFrameNoSafe();
        if(pFrame == NULL)
        {
            printf("pFrame == NULL [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,pFrame->nLength);
            break;
        } 
        memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
        iRet=PriToMedia(pFrame,&tFileFrameInfo);
        if(iRet<0)
        {
            printf("PriToMedia err%d\r\n",pFrame->nEncodeType);
            continue;
        }

        iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {
            pbOutBuf = new DataBuf(iPutFrameLen+MEDIA_FORMAT_MAX_LEN);
        }
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            iPutFrameLen=0;
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//û�л�ȡ����װ����(�������ݲ���)
                delete pbOutBuf;//�����´�������ݻ���󣬻�ȡ��װ������Ҫ����Ļ���
                pbOutBuf = NULL;//������Ҫ�ͷţ����·���
            }//���ÿ�ζ�����һ���㹻��Ļ��棬����Ҫ��ô�鷳
            continue;
        }
        printf("FrameToContainer iPutFrameLen %d iWriteLen %d iFrameProcessedLen[%d]\r\n",iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//û�õ��ķ�װ���泤��
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//������-����������ݳ��Ⱥͷ�װ����=δ������ݳ���+δ�÷�װ���泤���ټ�iRemain=δ������ݳ���
        iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLenҪ���ڷ�װ�����л����δ��������ݳ��ȣ������ڴ�Ź���
    }
    m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);
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
	int iPutFrameLen=0;

    if(NULL == i_pbSrcData || i_iSrcDataLen <= 0)
    {
        printf("Convert NULL == i_pbSrcData err\r\n");
        return iRet;
    } 
    m_pbInputBuf->Copy(i_pbSrcData,i_iSrcDataLen);
    memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf;
    tFileFrameInfo.eStreamType = i_eSrcStreamType;
    tFileFrameInfo.eEncType = i_eSrcEncType;
    tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen;
    tFileFrameInfo.iFrameBufMaxLen = m_pbInputBuf->iBufMaxLen;
    while(1)
    {
        tFileFrameInfo.iFrameLen = 0;
        tFileFrameInfo.pbFrameBuf = m_pbInputBuf->pbBuf+tFileFrameInfo.iFrameProcessedLen;
        tFileFrameInfo.iFrameBufLen = m_pbInputBuf->iBufLen-tFileFrameInfo.iFrameProcessedLen;
        if(STREAM_TYPE_VIDEO_STREAM==i_eSrcStreamType)
        {//���ļ���ʹ��m_oMediaHandle��Ҫ��֤����ֻ��һ֡
            tFileFrameInfo.pbFrameStartPos = NULL;//��˸ɴ�ֱ��ʹ��ParseNaluFromFrame
            tFileFrameInfo.eStreamType = STREAM_TYPE_UNKNOW;//ParseNaluFromFrame ��ֵ��Ҫ������ΪMEDIA_ENCODE_TYPE_UNKNOW
            ParseNaluFromFrame(&tFileFrameInfo);
            tFileFrameInfo.eStreamType = i_eSrcStreamType;
        }
        else//GetFrame ��aac�Ѿ���Ϊʹ��eFrameType�ж��ⲿ�Ƿ��Ѿ�������һ֡����Ϣ
        {//aac��eFrameTypeΪUNKNOW�������ʹ��GetFrame����,�ڲ�����и�ֵ
            m_oMediaHandle.GetFrame(&tFileFrameInfo);//������Ƶ����Ҳ��Ϊʹ��eFrameType�жϣ�������eStreamType����Ҳ���԰���aac�ķ�ʽʹ��GetFrame
        }
        if(tFileFrameInfo.iFrameLen <= 0)
        {
            printf("tFileFrameInfo.iFrameLen <= 0 [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,tFileFrameInfo.iFrameProcessedLen);
            break;
        } 
#ifdef SUPPORT_PRI
        if(STREAM_TYPE_UNKNOW == i_eDstStreamType)
        {
            FRAME_INFO* ptFrameInfo = MediaToPri(&tFileFrameInfo);
            if(NULL == ptFrameInfo)
            {
                printf("MediaToPri err%d\r\n",tFileFrameInfo.iFrameLen);
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
            break;
        }
#endif
        iPutFrameLen+=tFileFrameInfo.iFrameLen;
        if(NULL == pbOutBuf)
        {//���ÿ�ζ�����һ���㹻��Ļ��棬����Ҫͳ�� iPutFrameLen��ô�鷳
            pbOutBuf = new DataBuf(iPutFrameLen+MEDIA_FORMAT_MAX_LEN);//����ÿ��(֡)����ô��Ļ��棬������ܼ�ʱ�ͷ����ڴ����׺ľ�
        }
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,tFileFrameInfo.iFrameProcessedLen);
            iPutFrameLen=0;
            break;
        }
        if(iWriteLen == 0)
        {
            if(NULL != pbOutBuf)
            {//û�л�ȡ����װ����(�������ݲ���)
                delete pbOutBuf;//�����´�������ݻ���󣬻�ȡ��װ������Ҫ����Ļ���
                pbOutBuf = NULL;//������Ҫ�ͷţ����·���
            }//���ÿ�ζ�����һ���㹻��Ļ��棬����Ҫ��ô�鷳
            continue;
        }
        printf("FrameToContainer iPutFrameLen %d iWriteLen %d iFrameProcessedLen[%d]\r\n",iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//û�õ��ķ�װ���泤��
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//������-����������ݳ��Ⱥͷ�װ����=δ������ݳ���+δ�÷�װ���泤���ټ�iRemain=δ������ݳ���
        iPutFrameLen=tFileFrameInfo.iFrameLen;//iPutFrameLenҪ���ڷ�װ�����л����δ��������ݳ��ȣ������ڴ�Ź���
    }//gop����(mp4)�����µ�i֡���ᱻ������´δ�������µ�֡����= ��װ�����л����δ��������ݳ���
    m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);
    if(NULL != pbOutBuf)
    {
        delete pbOutBuf;
    }
    
    return iWriteLen;
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
int MediaConvert::GetData(unsigned char * o_pbData,int i_iMaxDataLen)
{
    int iRet = -1;
    
    if(NULL == o_pbData || i_iMaxDataLen<= 0)
    {
        printf("GetData NULL %d\r\n",i_iMaxDataLen);
        return iRet;
    }
    
    if(!m_pDataBufList.empty())
    {
        DataBuf * it = m_pDataBufList.front();
        if(it->iBufLen > i_iMaxDataLen)
        {
            printf("GetData err %d,%d\r\n",it->iBufLen, i_iMaxDataLen);
        }
        else
        {
            memcpy(o_pbData,it->pbBuf,it->iBufLen);
            iRet =it->iBufLen;
            delete it;
            m_pDataBufList.pop_front();
        }
    }
    return iRet;
}

#ifdef SUPPORT_PRI
/*****************************************************************************
-Fuction        : WebRtcFrameToFrameInfo
-Description    : ������
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
    	//ptFrameInfo->nFrameRate = m_iPullVideoFrameRate==0?m_dwTalkVideoFPS:m_iPullVideoFrameRate;//���ǰ��û�д���֡����ʹ��json����ĸ���ת���Ҫת�ɶ���֡��
	ptFrameInfo->nWidth = i_ptMediaFrame->dwWidth;//��߲���ֵ����߲�һ�¾ͻᴥ��ת��
	ptFrameInfo->nHeight = i_ptMediaFrame->dwHeight;//h265 pc����� b֡���ֻ������֡��Ƭ������Ҫת��

    FRAME_INFO* pFrame = CSTDStream::NewFrame(ptFrameInfo, (const char*)i_ptMediaFrame->pbFrameStartPos, i_ptMediaFrame->iFrameLen);

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
            printf("PriToMedia i_ptFrame->eEncType err %d\r\n",i_pFrame->nEncodeType);
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
    o_ptMediaFrameInfo->eStreamType= STREAM_TYPE_MUX_STREAM;//��Ƶ����Ҫ�����ɶ��nalu����Ƶ���ݿ�ֱ���ⲿ����

    o_ptMediaFrameInfo->eEncType = eEncType;
    o_ptMediaFrameInfo->eFrameType = eFrameType;

    o_ptMediaFrameInfo->dwTimeStamp=i_pFrame->nTimeStamp;
    o_ptMediaFrameInfo->dwWidth= i_pFrame->nWidth;
    o_ptMediaFrameInfo->dwHeight=i_pFrame->nHeight;

    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        o_ptMediaFrameInfo->dwSampleRate= 90000;//h264Ƶ�ʲ��ᴫ������h264Ƶ�ʿ��Թ̶�
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
        printf("GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
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
                pcNaluEndPos = pcFrameData;//��ʱ��һ��nalu�Ľ���
            }
            else
            {
                pcNaluStartPos = pcFrameData;//��ʱ��һ��nalu�Ŀ�ʼ
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH264NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//��һ��nalu�Ľ���Ϊ��һ��nalu�Ŀ�ʼ
                bStartCodeLen = 3;
                bNaluType = pcNaluStartPos[3] & 0x1f;
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
                        break;//������һ֡���˳�
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
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
                        break;//������һ֡���˳�
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
    {//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
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
        printf("GetFrame NULL %d\r\n", m_ptFrame->iFrameBufLen);
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
                pcNaluEndPos = pcFrameData;//��ʱ��һ��nalu�Ľ���
            }
            else
            {
                pcNaluStartPos = pcFrameData;//��ʱ��һ��nalu�Ŀ�ʼ
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//ȡnalu����
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 3)
                {
                    iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);
                }
                pcNaluStartPos = pcNaluEndPos;//��һ��nalu�Ľ���Ϊ��һ��nalu�Ŀ�ʼ
                bStartCodeLen = 3;
                bNaluType = (pcNaluStartPos[bStartCodeLen] & 0x7E)>>1;//ȡnalu����
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
                        break;//������һ֡���˳�
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
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//ȡnalu����
            }
            if(pcNaluEndPos != NULL)
            {
                if(pcNaluEndPos - pcNaluStartPos > 4)
                {
                    iRet = SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcNaluEndPos - pcNaluStartPos,m_ptFrame);//�������ͼ�4//ȥ��00 00 00 01
                }
                pcNaluStartPos = pcNaluEndPos;
                bStartCodeLen = 4;
                bNaluType = (pcNaluStartPos[4] & 0x7E)>>1;//ȡnalu����
                pcNaluEndPos = NULL;
                if(iRet > 0)
                {
                    iFrameType = iRet;
                    if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
                        break;//������һ֡���˳�
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
    {//���ļ��������ⲿ���϶���һ֡��������Ҫȫ�����������˳�,��ֹ֡��Ƭ�ò������������
        iRet=SetH265NaluData(bNaluType,bStartCodeLen,pcNaluStartPos,pcFrameData - pcNaluStartPos,m_ptFrame);//�������ͼ�4��ʼ��
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
    unsigned char * pbNaluData = NULL;//ȥ��00 00 00 01
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

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//�������ͼ���ʼ��
    pbNaluData = i_pbNaluData+i_bStartCodeLen;
    switch(i_bNaluType)//ȡnalu����
    {
        case 0x7:
        {
            memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//��������(��3��ʼ��)
            memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
            break;
        }
        case 0x8:
        {
            memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
            m_ptFrame->tVideoEncodeParam.iSizeOfPPS = iNaluDataLen;//�������ͼ�3��ʼ��
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
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//�ļ���ʱ�����Ҫ��ֵ����������ʱ���ⲿ�ḳֵ���ⲿΪ׼
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H264;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//������һ֡���˳�
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
    unsigned char * pbNaluData = NULL;//ȥ��00 00 00 01
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

    iNaluDataLen = i_iNaluDataLen-i_bStartCodeLen;//�������ͼ���ʼ��
    pbNaluData = i_pbNaluData+i_bStartCodeLen;

    if(i_bNaluType >= 0 && i_bNaluType <= 9)// p slice Ƭ
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
    }
    else if(i_bNaluType >= 16 && i_bNaluType <= 21)// IRAP ��ͬ��i֡
    {
        eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
    }
    else if(i_bNaluType == 32)//VPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abVPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abVPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfVPS= iNaluDataLen;//�������ͼ�4
        memcpy(m_ptFrame->tVideoEncodeParam.abVPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfVPS);
    }
    else if(i_bNaluType == 33)//SPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abSPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abSPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfSPS= iNaluDataLen;//�������ͼ�4
        memcpy(m_ptFrame->tVideoEncodeParam.abSPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfSPS);
    }
    else if(i_bNaluType == 34)//PPS
    {
        memset(m_ptFrame->tVideoEncodeParam.abPPS,0,sizeof(m_ptFrame->tVideoEncodeParam.abPPS));
        m_ptFrame->tVideoEncodeParam.iSizeOfPPS= iNaluDataLen;//�������ͼ�4
        memcpy(m_ptFrame->tVideoEncodeParam.abPPS,pbNaluData,m_ptFrame->tVideoEncodeParam.iSizeOfPPS);
    }
    
    if(MEDIA_FRAME_TYPE_UNKNOW != eFrameType)
    {
        if(STREAM_TYPE_UNKNOW == m_ptFrame->eStreamType)//�ļ���ʱ�����Ҫ��ֵ����������ʱ���ⲿ�ḳֵ���ⲿΪ׼
        {
            m_ptFrame->eFrameType = eFrameType;
            m_ptFrame->eEncType = MEDIA_ENCODE_TYPE_H265;
            m_ptFrame->dwTimeStamp += 40;//VIDEO_H264_FRAME_INTERVAL*VIDEO_H264_SAMPLE_RATE/1000;
            m_ptFrame->dwSampleRate= 90000;
        }
        iRet = 0;//������һ֡���˳�
    }
    return eFrameType;
}


/*****************************************************************************
-Fuction        : InputData
-Description    : InputData
-Input          : ������������� ,�������������
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

    if(NULL != strstr(i_strDstName,".ts"))
    {
        eDstStreamType=STREAM_TYPE_TS_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".mp4"))
    {
        eDstStreamType=STREAM_TYPE_FMP4_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".flv"))
    {
        eDstStreamType=STREAM_TYPE_ENHANCED_FLV_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".VideoRaw"))
    {
        eDstStreamType=STREAM_TYPE_VIDEO_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".AudioRaw"))
    {
        eDstStreamType=STREAM_TYPE_AUDIO_STREAM;
    }
#ifdef SUPPORT_PRI
    else if(NULL != strstr(i_strDstName,".pri"))
    {
        eDstStreamType=STREAM_TYPE_UNKNOW;
    }
#endif    
    else
    {
        printf("i_strDstName %s err\r\n",i_strDstName);
        return -1;
    }
    if(NULL != strstr(i_strSrcName,".pri"))
    {
#ifdef SUPPORT_PRI
        printf("Convert %s to %s\r\n",i_strSrcName,i_strDstName);
        return MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#else
        printf("Convert %s to %s unsupport\r\n",i_strSrcName,i_strDstName);
        return -1;//MediaConvert::Instance()->ConvertFromPri(i_pbSrcData,i_iSrcDataLen,eDstStreamType);
#endif    
    }

    if(NULL != strstr(i_strSrcName,".flv"))
    {
        eSrcStreamType=STREAM_TYPE_FLV_STREAM;
    }
    else if(NULL != strstr(i_strSrcName,".h264"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H264;
    }
    else if(NULL != strstr(i_strSrcName,".h265"))
    {
        eSrcStreamType=STREAM_TYPE_VIDEO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_H265;
    }
    else if(NULL != strstr(i_strSrcName,".aac"))
    {
        eSrcStreamType=STREAM_TYPE_AUDIO_STREAM;
        eSrcEncType=MEDIA_ENCODE_TYPE_AAC;
    }
    else
    {
        printf("i_strSrcName %s err\r\n",i_strSrcName);
        return -1;
    }
    printf("Convert %s to %s\r\n",i_strSrcName,i_strDstName);
    return MediaConvert::Instance()->Convert(i_pbSrcData,i_iSrcDataLen,eSrcEncType,eSrcStreamType,eDstStreamType);
}


/*****************************************************************************
-Fuction        : GetData
-Description    : һ�η����������ݵ���С��λ��
���������ͷ���һ֡
flv����һ֡��Ӧ��tag
mp4����һ��gop
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int GetData(unsigned char * o_pbData,int i_iMaxDataLen)
{
    return MediaConvert::Instance()->GetData(o_pbData,i_iMaxDataLen);
}

