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

//(��ʱ����������ȥ��)
// WAVE file header format �ַ�����Ϊ��ˣ�����ΪС��
/*typedef struct WavHeader
{
	unsigned char   abChunkId[4];        // RIFF string
	unsigned int    dwChunkSize;         // overall size of file in bytes (36 + data_size)
	unsigned char   abSubChunk1Id[8];   // WAVEfmt string with trailing null char
	unsigned int    dwSubChunk1Size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
	unsigned short  wAudioFormat;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned short  wNumChannels;       // Mono = 1, Stereo = 2
	unsigned int    dwSampleRate;        // 8000, 16000, 44100, etc. (blocks per second)
	unsigned int    dwByteRate;          // SampleRate * NumChannels * BitsPerSample/8
	unsigned short  wBlockAlign;        // NumChannels * BitsPerSample/8
	unsigned short  wBitsPerSample;    // bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char   abSubChunk2Id[4];   // Contains the letters "data"
	unsigned int    dwSubChunk2Size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} T_WavHeader;//һ��ֻ���ļ�ͷ�У�����ʼ��һ�μ��ɣ�����Ҫÿ֡ǰ���*/


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
    m_eDstVideoEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_eDstAudioEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    m_pbInputBuf = new DataBuf(MEDIA_INPUT_BUF_MAX_LEN);
    m_pAudioCodec = NULL;
    m_pAudioCodec = new AudioCodec();
    m_pAudioTranscodeBuf = new unsigned char [MEDIA_FORMAT_MAX_LEN];
    m_iTransAudioCodecFlag=0;
    m_iPutFrameLen=0;
    m_tOutCodecFrameList.clear();
    m_pOutCodecFrameBuf = new unsigned char [MEDIA_OUTPUT_BUF_MAX_LEN];
    m_iOutCodecFrameBufLen=0;
    m_pMediaTranscodeBuf = new unsigned char [MEDIA_OUTPUT_BUF_MAX_LEN];
    m_eDstTransVideoCodecType=CODEC_TYPE_H264;
    m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_AAC;
    m_dwAudioCodecSampleRate=0;
    m_iTransCodecFlag=0;
    m_iSetWaterMarkFlag=0;
    
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
    if(NULL != m_pOutCodecFrameBuf)
    {
        delete [] m_pOutCodecFrameBuf;
    }
    if(NULL != m_pMediaTranscodeBuf)
    {
        delete [] m_pMediaTranscodeBuf;
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
    T_MediaFrameInfo * ptFrameInfo =NULL;
    T_MediaFrameInfo tTranscodeFrameInfo;

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
        
        if(STREAM_TYPE_FMP4_STREAM==i_eDstStreamType)
        {//mse(mp4)����Ҫת��
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_AAC;
            m_dwAudioCodecSampleRate=44100;
            m_iTransAudioCodecFlag=1;
        }
        else if(STREAM_TYPE_ORIGINAL_STREAM==i_eDstStreamType)
        {//
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_WAV_PCM;
            //m_dwAudioCodecSampleRate=8000;//���ⲿ����
            CodecDataToOriginalData(&tFileFrameInfo);
            m_eDstVideoEncType = MEDIA_ENCODE_TYPE_RGBA;
            m_eDstAudioEncType = MEDIA_ENCODE_TYPE_WAV;//MEDIA_ENCODE_TYPE_LPCM;
            continue;
        }
        ptFrameInfo =&tFileFrameInfo;
        if(0!=m_iSetWaterMarkFlag)
            m_iTransCodecFlag=1;
        if(ptFrameInfo->eFrameType != MEDIA_FRAME_TYPE_AUDIO_FRAME && 0 != m_iTransCodecFlag)
        {
            MediaTranscode(ptFrameInfo);
            memcpy(&tTranscodeFrameInfo,ptFrameInfo,sizeof(T_MediaFrameInfo));
            ptFrameInfo=&tTranscodeFrameInfo;
            ptFrameInfo->pbFrameBuf=m_pMediaTranscodeBuf;
            ptFrameInfo->iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN;
            GetCodecData(ptFrameInfo);
        }

        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
        {
            m_iFindKeyFrame=1;
            m_tSegInfo.iHaveKeyFrameFlag=1;
        }
        else
        {
            if(m_iFindKeyFrame==0)
                continue;
        }
        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_tSegInfo.iVideoFrameCnt++;
            m_eDstVideoEncType = ptFrameInfo->eEncType;
            m_iPutVideoFrameCnt++;//tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp Ҫ��SynchronizerAudioVideoǰ
            if(m_dwLastVideoTimeStamp != 0)//����Ƶ���ܷ�gop�ֶΣ��������(web���Ż�һֱ���ٲ���������ֻ��i֡)
                m_dwPutVideoFrameTime+=m_iPutVideoFrameCnt>1?(ptFrameInfo->dwTimeStamp-m_dwLastVideoTimeStamp):0;
            //m_dwLastVideoTimeStamp = tFileFrameInfo.dwTimeStamp;//���� SynchronizerAudioVideo��Ҫע������
        }
        //if(i_eDstStreamType != STREAM_TYPE_FMP4_STREAM)//
            SynchronizerAudioVideo(ptFrameInfo);//mp4����Ƶ�����Ƶ���Ƿֱ𵥶��ģ����Բ���ҪͬԴ����,������Ҳ��Ӱ��
        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)//ͬʱweb�˶�ʱ���Ҫ���ϸ����ͬԴ�����᲻����֡��/�����ʣ��ᵼ�¿�������
        {
            m_tSegInfo.iAudioFrameCnt++;
            if(0 != m_iTransAudioCodecFlag)
            {
                if(AudioTranscode(ptFrameInfo,m_eDstTransAudioCodecType,m_dwAudioCodecSampleRate,&tTranscodeFrameInfo)<=0)
                {
                    continue;//ת��ʧ�ܻ����ݲ�������������һ֡
                }
                ptFrameInfo=&tTranscodeFrameInfo;
            }
            m_eDstAudioEncType = ptFrameInfo->eEncType;
        }
        //printf("FrameType %d TimeStamp %u PutFrameTime %u FrameLen %d\r\n", tFileFrameInfo.eFrameType, tFileFrameInfo.dwTimeStamp,m_dwPutVideoFrameTime, tFileFrameInfo.iFrameLen);
        if(0 == m_iPutFrameLen)
            printf("start PutFrameLen %d iFrameLen %d eFrameType[%d] eEncType[%d]\r\n",m_iPutFrameLen,ptFrameInfo->iFrameLen,ptFrameInfo->eFrameType,ptFrameInfo->eEncType);
        m_iPutFrameLen+=ptFrameInfo->iFrameLen;
        if(NULL == pbOutBuf)
        {
            pbOutBuf = new DataBuf(m_iPutFrameLen+MEDIA_FORMAT_MAX_LEN);
        }//����Ƶ���ܷ�gop�ֶΣ��������(web���Ż�һֱ���ٲ�����)
        iWriteLen = m_oMediaHandle.FrameToContainer(ptFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen,m_dwPutVideoFrameTime>=600?1:0);
        if(iWriteLen < 0)
        {
            printf("!!!FrameToContainer err ,iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,ptFrameInfo->iFrameProcessedLen);
            m_iPutFrameLen=0;
            m_dwPutVideoFrameTime=0;
            m_iPutVideoFrameCnt=0;
            memset(&m_tSegInfo,0,sizeof(T_SegInfo));
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
        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
            printf("FrameToContainer iPutFrame %d Len %d iWriteLen %d ProcessedLen[%d] iBufMaxLen[%d]\r\n",m_iPutVideoFrameCnt,m_iPutFrameLen,iWriteLen,ptFrameInfo->iFrameProcessedLen,pbOutBuf->iBufMaxLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_tSegInfo.iSegDurationTime=ptFrameInfo->dwTimeStamp-m_tSegInfo.iSegStartTime;
        memcpy(&pbOutBuf->tSegInfo,&m_tSegInfo,sizeof(T_SegInfo));
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        memset(&m_tSegInfo,0,sizeof(T_SegInfo));
        m_tSegInfo.iSegStartTime=ptFrameInfo->dwTimeStamp;
        m_tSegInfo.dwStartTimeHigh=(pFrame->nTimeStamp>>32)&0xffffffff;
        m_tSegInfo.dwStartTimeLow=(pFrame->nTimeStamp)&0xffffffff;
        m_tSegInfo.dwStartAbsTime=(pFrame->nTimeStamp)/1000;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//û�õ��ķ�װ���泤��
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//������-����������ݳ��Ⱥͷ�װ����=δ������ݳ���+δ�÷�װ���泤���ټ�iRemain=δ������ݳ���
        m_iPutFrameLen=ptFrameInfo->iFrameLen;//iPutFrameLenҪ���ڷ�װ�����л����δ��������ݳ��ȣ������ڴ�Ź���
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
    T_MediaFrameInfo * ptFrameInfo =NULL;
    T_MediaFrameInfo tTranscodeFrameInfo;


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
        memcpy(&m_tFileFrameInfo,&tFileFrameInfo,sizeof(T_MediaFrameInfo));//sps�Ȳ������ȱ�����������ֹ�´ν��붪ʧ
        m_pbInputBuf->Delete(tFileFrameInfo.iFrameProcessedLen);//�����˵Ķ�Ҫɾȥ��flvÿ�δ���һ��tag,����һ֡
        if(tFileFrameInfo.iFrameLen <= 0)
        {
            //printf("tFileFrameInfo.iFrameLen <= 0 [%x,%d,%d]\r\n",i_pbSrcData[0],i_iSrcDataLen,tFileFrameInfo.iFrameProcessedLen);
            break;
        } 

        if(STREAM_TYPE_FMP4_STREAM==i_eDstStreamType)
        {//mse(mp4)����Ҫת��
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_AAC;
            m_dwAudioCodecSampleRate=44100;
            m_iTransAudioCodecFlag=1;
        }
        else if(STREAM_TYPE_ORIGINAL_STREAM==i_eDstStreamType)
        {//
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_WAV_PCM;
            //m_dwAudioCodecSampleRate=8000;//���ⲿ����
            CodecDataToOriginalData(&tFileFrameInfo);
            m_eDstVideoEncType = MEDIA_ENCODE_TYPE_RGBA;
            m_eDstAudioEncType = MEDIA_ENCODE_TYPE_WAV;//MEDIA_ENCODE_TYPE_LPCM;
            continue;
        }
        ptFrameInfo =&tFileFrameInfo;
        if(0!=m_iSetWaterMarkFlag)
            m_iTransCodecFlag=1;
        if(ptFrameInfo->eFrameType != MEDIA_FRAME_TYPE_AUDIO_FRAME && 0 != m_iTransCodecFlag)
        {
            MediaTranscode(ptFrameInfo);
            memcpy(&tTranscodeFrameInfo,ptFrameInfo,sizeof(T_MediaFrameInfo));
            ptFrameInfo=&tTranscodeFrameInfo;
            ptFrameInfo->pbFrameBuf=m_pMediaTranscodeBuf;
            ptFrameInfo->iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN;
            GetCodecData(ptFrameInfo);
        }

        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
        {
            m_iFindKeyFrame=1;
            m_tSegInfo.iHaveKeyFrameFlag=1;
        }
        else
        {
            if(m_iFindKeyFrame==0)
                continue;
        }
        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
        ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
        ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
        {
            m_tSegInfo.iVideoFrameCnt++;
            m_eDstVideoEncType = ptFrameInfo->eEncType;
            m_iPutVideoFrameCnt++;//tFileFrameInfo.dwTimeStamp-m_dwLastVideoTimeStamp Ҫ��SynchronizerAudioVideoǰ
            if(m_dwLastVideoTimeStamp != 0)//����Ƶ���ܷ�gop�ֶΣ��������(web���Ż�һֱ���ٲ���������ֻ��i֡)
                m_dwPutVideoFrameTime+=m_iPutVideoFrameCnt>1?(ptFrameInfo->dwTimeStamp-m_dwLastVideoTimeStamp):0;
            //m_dwLastVideoTimeStamp = tFileFrameInfo.dwTimeStamp;//���� SynchronizerAudioVideo��Ҫע������
        }
        //if(i_eDstStreamType != STREAM_TYPE_FMP4_STREAM)//
            SynchronizerAudioVideo(ptFrameInfo);//mp4����Ƶ�����Ƶ���Ƿֱ𵥶��ģ����Բ���ҪͬԴ����,������Ҳ��Ӱ��
        if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
        {
            m_tSegInfo.iAudioFrameCnt++;
            if(0 != m_iTransAudioCodecFlag)
            {
                if(AudioTranscode(ptFrameInfo,m_eDstTransAudioCodecType,m_dwAudioCodecSampleRate,&tTranscodeFrameInfo)<=0)
                {
                    continue;//ת��ʧ�ܻ����ݲ�������������һ֡
                }
                ptFrameInfo=&tTranscodeFrameInfo;
            }
            m_eDstAudioEncType = ptFrameInfo->eEncType;
        }

#ifdef SUPPORT_PRI
        if(STREAM_TYPE_UNKNOW == i_eDstStreamType)
        {
            FRAME_INFO* ptPriFrameInfo = MediaToPri(ptFrameInfo);
            if(NULL == ptPriFrameInfo || ptPriFrameInfo->nLength <= 0)
            {
                printf("MediaToPri err %d %d\r\n",ptFrameInfo->iFrameLen,ptPriFrameInfo->nLength);
                continue;
            }
            if(NULL == pbOutBuf)
            {
                pbOutBuf = new DataBuf(ptPriFrameInfo->nLength);
            }
            pbOutBuf->Copy(ptPriFrameInfo->pHeader,ptPriFrameInfo->nLength);
            m_pDataBufList.push_back(pbOutBuf);
            pbOutBuf = NULL;
            ptPriFrameInfo->Release();
            continue;
        }
#endif
        //printf("FrameType %d TimeStamp %u PutFrameTime %u FrameLen %d\r\n", tFileFrameInfo.eFrameType, tFileFrameInfo.dwTimeStamp,m_dwPutVideoFrameTime, tFileFrameInfo.iFrameLen);
        if(0 == m_iPutFrameLen)
            printf("start m_dwPutVideoFrameTime %u ,%d iFrameLen %d eFrameType[%d] eEncType[%d]\r\n",m_dwPutVideoFrameTime,m_pbInputBuf->iBufLen,ptFrameInfo->iFrameLen,ptFrameInfo->eFrameType,ptFrameInfo->eEncType);
        m_iPutFrameLen+=ptFrameInfo->iFrameLen;
        if(NULL == pbOutBuf)
        {//���ÿ�ζ�����һ���㹻��Ļ��棬����Ҫͳ�� iPutFrameLen��ô�鷳
            pbOutBuf = new DataBuf(m_iPutFrameLen+MEDIA_FORMAT_MAX_LEN);//����ÿ��(֡)����ô��Ļ��棬������ܼ�ʱ�ͷ����ڴ����׺ľ�
        }//����Ƶ���ܷ�gop�ֶΣ��������(web���Ż�һֱ���ٲ�����)
        iWriteLen = m_oMediaHandle.FrameToContainer(ptFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen,m_dwPutVideoFrameTime>=600?1:0);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d iFrameProcessedLen[%d]\r\n",iWriteLen,ptFrameInfo->iFrameProcessedLen);
            m_iPutFrameLen=0;
            m_dwPutVideoFrameTime=0;
            m_iPutVideoFrameCnt=0;
            memset(&m_tSegInfo,0,sizeof(T_SegInfo));
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
        //if(tFileFrameInfo.eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
            printf("FrameToContainer%d m_dwPutVideoFrameTime %u Len %d iWriteLen %d ProcessedLen[%d] iBufMaxLen[%d]\r\n",m_iPutVideoFrameCnt,m_dwPutVideoFrameTime,m_iPutFrameLen,iWriteLen,tFileFrameInfo.iFrameProcessedLen,pbOutBuf->iBufMaxLen);
        pbOutBuf->iBufLen=iWriteLen;
        m_tSegInfo.iSegDurationTime=ptFrameInfo->dwTimeStamp-m_tSegInfo.iSegStartTime;
        memcpy(&pbOutBuf->tSegInfo,&m_tSegInfo,sizeof(T_SegInfo));
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
        memset(&m_tSegInfo,0,sizeof(T_SegInfo));
        m_tSegInfo.iSegStartTime=ptFrameInfo->dwTimeStamp;
        //int iRemain = MEDIA_FORMAT_MAX_LEN - (iWriteLen-iPutFrameLen);//û�õ��ķ�װ���泤��
        //iPutFrameLen=pbOutBuf->iBufMaxLen-iWriteLen-iRemain;//������-����������ݳ��Ⱥͷ�װ����=δ������ݳ���+δ�÷�װ���泤���ټ�iRemain=δ������ݳ���
        m_iPutFrameLen=ptFrameInfo->iFrameLen;//iPutFrameLenҪ���ڷ�װ�����л����δ��������ݳ��ȣ������ڴ�Ź���
        m_dwPutVideoFrameTime=0;
        m_iPutVideoFrameCnt=0;
    }//gop����(mp4)�����µ�i֡���ᱻ������´δ�������µ�֡����= ��װ�����л����δ��������ݳ���
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
            case MEDIA_ENCODE_TYPE_RGBA:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","rgba");
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
            case MEDIA_ENCODE_TYPE_LPCM:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","pcm");
                iRet=0;
                break;
            }
            case MEDIA_ENCODE_TYPE_WAV:
            {
                snprintf((char *)o_pbVideoEncBuf,i_iMaxVideoEncBufLen,"%s","wav");
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
-Description    : �������Ƶ��ͬ�������ܻᵼ�²�������֡
����ffplay���������Ƶ֡�ܶ�ʱ�����Ƶ֡ ��ֱ�Ӷ����������֡(ֻ��i֡����)
// ����Ƶͬ������
// ����Ƶʱ�䳬����Ƶʱ��ʱ��������Ƶ����(���ͬ����Ƶʱ��������Ƶ���������(ż������))
// 1.����Ƶͬ������
// ����Ƶʱ�����(500ms)��ͬ����Ƶʱ�䣨����Ƶʱ�䳬����Ƶʱ��ʱ�ᶪ�������Բ�������Ƶʱ�䳬ǰ���⣩
// 2.����Ƶͬ������
// ����Ƶʱ�䳬����Ƶʱ�䣬ͬ����Ƶʱ��(��ʮms)
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
    
    dwTimeStamp = i_ptFrameInfo->dwTimeStamp;    // ʹ�õ���ʱ�����������ʵʱ��(ʹ��֡ʱ���������)
    if (i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
    i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
    i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
    {// i_pFrame->nTimeStamp;//����Ƶʱ�Ӳ�ͬԴ96                                           
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

        if (m_dwVideoTimeStamp < m_dwAudioTimeStamp) // 2.����Ƶͬ������
        {// ��Ƶʱ�䳬����Ƶʱ�䣬ͬ����Ƶʱ��(��Ƶ��������� ��ʮms)
            //printf("m_dwVideoTimeStamp %d < m_dwAudioTimeStamp %d \r\n", m_dwVideoTimeStamp, m_dwAudioTimeStamp);
            m_dwVideoTimeStamp = m_dwAudioTimeStamp; // ffplay ����Ч������
        }// ���û��������ԣ��������Ƶ����Ƶ�����Ӷ����²�������(ffplay������)

        m_dwLastVideoTimeStamp = dwTimeStamp;
        dwTimeStamp = m_dwVideoTimeStamp;
    }
    else if (i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        //i_ptFrameInfo->dwTimeStamp = m_dwVideoTimeStamp; //ֱ��ʹ�ã�web�˻Ῠ
        //return 0;
    
        if (0 == m_dwLastAudioTimeStamp)
        {
            m_dwLastAudioTimeStamp = dwTimeStamp;
            m_dwAudioTimeStamp = m_dwVideoTimeStamp; // ����Ƶͬ��������ͬԴ�����ڲ�MP4�������ͬԴʱ���
        }
        m_dwAudioTimeStamp += (unsigned int)(dwTimeStamp - m_dwLastAudioTimeStamp);

        if (m_dwAudioTimeStamp < m_dwVideoTimeStamp) // 1.����Ƶͬ������
        {// ����Ƶʱ�����ͬ����Ƶʱ��
            m_dwAudioTimeStamp = m_dwVideoTimeStamp; // 0.���ֻ����Ƶʱ���ֱ��ʹ����Ƶʱ����Ĳ��ԣ�(ffplay)����Ч��������
        }// ������Ƶʱ�䳬����Ƶʱ��ʱ�ᶪ�������Բ�������Ƶʱ�䳬ǰ���⣩
        /*if (m_dwAudioTimeStamp > m_dwVideoTimeStamp)// 2.����Ƶͬ������
        {// ����Ƶʱ�䳬����Ƶʱ��ʱ��������Ƶ����
            m_ddwLastAudioTimeStamp =ddwTimeStamp;//(���ͬ����Ƶʱ��������Ƶ���������(��ʮms))
            HTTP_LOGD(this,"m_dwAudioTimeStamp %d > m_dwVideoTimeStamp %d \r\n",m_dwAudioTimeStamp,m_dwVideoTimeStamp);
            //return iRet;//�����ʱ���������������Ƶʱ���,���return�Ͷ���������,����m_dwAudioTimeStamp =
        m_dwVideoTimeStamp;
        }//�����1��2���ͬʱ���ã�����Ƶʱ���ֱ��ʹ����Ƶʱ���û������*/

        m_dwLastAudioTimeStamp = dwTimeStamp;
        dwTimeStamp = m_dwAudioTimeStamp;//m_dwVideoTimeStamp;m_dwAudioTimeStamp
    }

    i_ptFrameInfo->dwTimeStamp = dwTimeStamp; //
    return 0;
}

/*****************************************************************************
-Fuction		: AudioTranscode
-Description	: g711תaac(mseֻ֧��aac)
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::AudioTranscode(T_MediaFrameInfo * i_pbAudioFrame,E_AudioCodecType i_eDstCodecType,unsigned int i_dwDstSampleRate,T_MediaFrameInfo * o_pbAudioFrame)
{
    int iRet = -1;
    T_AudioCodecParam i_tSrcCodecParam;
    T_AudioCodecParam i_tDstCodecParam;
    E_AudioCodecType eSrcAudioCodecType=AUDIO_CODEC_TYPE_UNKNOW;
    int iWriteLen = 0;


    if(NULL == i_pbAudioFrame || i_pbAudioFrame->iFrameLen<= 0)
    {
        printf("AudioTranscode NULL %p\r\n",i_pbAudioFrame);
        return iRet;
    }
    if(AUDIO_CODEC_TYPE_AAC != i_eDstCodecType && AUDIO_CODEC_TYPE_WAV_PCM != i_eDstCodecType)
    {
        printf("i_iDstCodecType err %d\r\n",i_eDstCodecType);
        return iRet;
    }
    if(m_eDstAudioEncType == MEDIA_ENCODE_TYPE_UNKNOW)
        printf("AudioTranscode,eEncType %d,dwSampleRate %d,dst eEncType %d,dwSampleRate %d\r\n",i_pbAudioFrame->eEncType,i_pbAudioFrame->dwSampleRate,i_eDstCodecType,i_dwDstSampleRate);
    switch(i_pbAudioFrame->eEncType)
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
            if(m_eDstAudioEncType == MEDIA_ENCODE_TYPE_UNKNOW&& AUDIO_CODEC_TYPE_AAC == i_eDstCodecType)
                printf("already aac,dwSampleRate %d,iFrameLen %d\r\n",i_pbAudioFrame->dwSampleRate,i_pbAudioFrame->iFrameLen);
            if(i_eDstCodecType == AUDIO_CODEC_TYPE_AAC)
            {
                memset(o_pbAudioFrame,0,sizeof(T_MediaFrameInfo));
                memcpy(o_pbAudioFrame,i_pbAudioFrame,sizeof(T_MediaFrameInfo));
                return o_pbAudioFrame->iFrameLen;
            }
            eSrcAudioCodecType=AUDIO_CODEC_TYPE_AAC;
            break;
        }
        default :
        {
            printf("AudioTranscode eEncType err %d\r\n",i_pbAudioFrame->eEncType);
            return iRet;
        }
    }
    if(eSrcAudioCodecType == i_eDstCodecType && i_pbAudioFrame->dwSampleRate == i_dwDstSampleRate)
    {
        //printf("AudioTranscode,CodecType%d %d && dwSampleRate%d SampleRate%d\r\n",eSrcAudioCodecType, i_eDstCodecType, i_pbAudioFrame->dwSampleRate, i_dwDstSampleRate);
        return i_pbAudioFrame->iFrameLen;
    }

    
    memset(&i_tSrcCodecParam,0,sizeof(T_AudioCodecParam));
    i_tSrcCodecParam.dwBitsPerSample=i_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample;
    i_tSrcCodecParam.dwChannels=i_pbAudioFrame->tAudioEncodeParam.dwChannels;
    i_tSrcCodecParam.dwSampleRate=i_pbAudioFrame->dwSampleRate;
    i_tSrcCodecParam.eAudioCodecType=eSrcAudioCodecType;
    memcpy(&i_tDstCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    i_tDstCodecParam.eAudioCodecType=i_eDstCodecType;
    if(i_dwDstSampleRate>0)
        i_tDstCodecParam.dwSampleRate=i_dwDstSampleRate;//������Ҫת��������web���޷�֧��

    iRet = m_pAudioCodec->Transcode(i_pbAudioFrame->pbFrameStartPos,i_pbAudioFrame->iFrameLen,i_tSrcCodecParam,m_pAudioTranscodeBuf,MEDIA_FORMAT_MAX_LEN,i_tDstCodecParam);
    if(iRet <= 0)
    {
        printf("Transcode iRet %d iFrameLen%d\r\n",iRet,i_pbAudioFrame->iFrameLen);
        return iRet;
    } 
    //printf("Transcode iRet %d iFrameLen%d %p\r\n",iRet,i_pbAudioFrame->iFrameLen,i_pbAudioFrame->pbFrameStartPos);
    iWriteLen=iRet;
    iRet=0;
    
    if(AUDIO_CODEC_TYPE_WAV_PCM != i_eDstCodecType)
    {
        do
        {
            iWriteLen+=iRet;
            iRet=m_pAudioCodec->Transcode(NULL,0,i_tSrcCodecParam,m_pAudioTranscodeBuf+iWriteLen,MEDIA_FORMAT_MAX_LEN-iWriteLen,i_tDstCodecParam);
        } while(iRet>0);
    }

    memset(o_pbAudioFrame,0,sizeof(T_MediaFrameInfo));
    memcpy(o_pbAudioFrame,i_pbAudioFrame,sizeof(T_MediaFrameInfo));
    switch(i_tDstCodecParam.eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_AAC:
        {
            o_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMU:
        {
            o_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711U;
            break;
        }
        case AUDIO_CODEC_TYPE_PCMA:
        {
            o_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        case AUDIO_CODEC_TYPE_WAV_PCM:
        {
            o_pbAudioFrame->eEncType=MEDIA_ENCODE_TYPE_WAV;
            break;
        }
        default :
        {
            printf("AudioTranscode dst eEncType err %d\r\n",i_tDstCodecParam.eAudioCodecType);
            return iRet;
        }
    }
    o_pbAudioFrame->dwSampleRate=i_tDstCodecParam.dwSampleRate;//�Ḳ��ԭ���Ĳ����ʣ����Ա���ʹ�õ������������
    o_pbAudioFrame->tAudioEncodeParam.dwBitsPerSample=i_tDstCodecParam.dwBitsPerSample;
    o_pbAudioFrame->tAudioEncodeParam.dwChannels=i_tDstCodecParam.dwChannels;
    o_pbAudioFrame->pbFrameBuf=m_pAudioTranscodeBuf;//
    o_pbAudioFrame->iFrameBufLen=iWriteLen;//
    o_pbAudioFrame->iFrameBufMaxLen=MEDIA_FORMAT_MAX_LEN;//
    o_pbAudioFrame->iFrameLen=iWriteLen;
    o_pbAudioFrame->pbFrameStartPos=m_pAudioTranscodeBuf;
    
    return iWriteLen;
}
/*****************************************************************************
-Fuction        : MediaTranscode
-Description    : MediaTranscode
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert :: MediaTranscode(T_MediaFrameInfo * m_pbFrame)
{
	int iRet = -1;
#ifdef SUPPORT_CODEC
    T_CodecFrame tSrcFrame,tDstFrame;
    
    if(NULL== m_pbFrame)
    {
        printf("NULL== m_pbFrameerr \r\n");
        return iRet;
    }
    if(m_pbFrame->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        return 0;
    }
    memset(&tSrcFrame,0,sizeof(T_CodecFrame));
    MediaFrameToCodecFrame(m_pbFrame,&tSrcFrame);
    
    memset(&tDstFrame,0,sizeof(T_CodecFrame));
    memcpy(&tDstFrame,&tSrcFrame,sizeof(T_CodecFrame));
    if(tSrcFrame.eEncType==CODEC_TYPE_H264)
        tDstFrame.eEncType=m_eDstTransVideoCodecType;
    else
        tDstFrame.eEncType=m_eDstTransVideoCodecType;//CODEC_TYPE_H264
    tDstFrame.pbFrameBuf=m_pOutCodecFrameBuf+m_iOutCodecFrameBufLen;
    tDstFrame.iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN-m_iOutCodecFrameBufLen;
    printf("Transform dwTimeStamp%d,iFrameRate %d,iFrameBufLen%d,iFrameLen %d,dwNaluCount%d,iFrameRate%d\r\n",m_pbFrame->dwTimeStamp,
    tDstFrame.iFrameRate,tSrcFrame.iFrameBufLen,m_pbFrame->iFrameLen,m_pbFrame->dwNaluCount,m_pbFrame->iFrameRate);
    iRet = m_oMediaTranscodeInf.Transform(&tSrcFrame, &tDstFrame);
    if(iRet < 0)
    {
        printf("oMediaTranscodeInf.Transform err %d\r\n",tSrcFrame.iFrameBufLen);
        return iRet;
    } 
    do
    {
        if(tDstFrame.iFrameBufLen == 0)
        {
            printf("tDstFrame.iFrameBufLen == 0 \r\n");
            break;
        } 
        //����ת�����֡
        m_tOutCodecFrameList.push_back(tDstFrame);
        m_iOutCodecFrameBufLen += tDstFrame.iFrameBufLen;
        tDstFrame.iFrameBufLen = 0;
        tDstFrame.pbFrameBuf=m_pOutCodecFrameBuf+m_iOutCodecFrameBufLen;
        tDstFrame.iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN-m_iOutCodecFrameBufLen;
        if(tDstFrame.iFrameBufMaxLen <= 0)
        {
            printf("tDstFrame.iFrameBufMaxLen <= 0 \r\n");
            break;
        } 
        iRet =m_oMediaTranscodeInf.GetDstFrame(&tSrcFrame, &tDstFrame);
        if(iRet < 0)
        {
            printf("oMediaTranscodeInf.GetDstFrame err \r\n");
            break;
        } 
    }while(iRet>0);
    iRet=0;
    if(m_tOutCodecFrameList.size() <= 0)
    {
        printf("MediaTranscodeInf.Transform err %d\r\n",tSrcFrame.eEncType);
        iRet=-1;
    } 
#endif
    return iRet;
}

/*****************************************************************************
-Fuction		: GetCodecData
-Description	: GetCodecData
-Input			:
-Output 		:
-Return 		:
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2024/09/26	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert::GetCodecData(T_MediaFrameInfo * o_pbFrame)
{
    int iRet = -1;
#ifdef SUPPORT_CODEC
    T_CodecFrame tCodecFrame;
    T_MediaFrameInfo tMediaTmpFrame;


    if(NULL == o_pbFrame)
    {
        printf("GetCodecData NULL \r\n");
        return iRet;
    }
    
    if(!m_tOutCodecFrameList.empty())
    {
        T_CodecFrame & tFrontCodecFrame = m_tOutCodecFrameList.front();
        memcpy(&tCodecFrame,&tFrontCodecFrame,sizeof(T_CodecFrame));

        memcpy(&tMediaTmpFrame,o_pbFrame,sizeof(T_MediaFrameInfo));
        CodecFrameToMediaFrame(&tCodecFrame,o_pbFrame);
        if(o_pbFrame->iFrameBufLen > tMediaTmpFrame.iFrameBufMaxLen)
        {
            printf("GetCodecData err %d,%d\r\n",o_pbFrame->iFrameBufMaxLen,tMediaTmpFrame.iFrameBufLen);
            memcpy(o_pbFrame,&tMediaTmpFrame,sizeof(T_MediaFrameInfo));
            iRet = -1;
        }
        else
        {
            memcpy(tMediaTmpFrame.pbFrameBuf,o_pbFrame->pbFrameBuf,o_pbFrame->iFrameBufLen);
            o_pbFrame->pbFrameBuf=tMediaTmpFrame.pbFrameBuf;
            o_pbFrame->iFrameBufMaxLen=tMediaTmpFrame.iFrameBufMaxLen;
            o_pbFrame->eStreamType=tMediaTmpFrame.eStreamType;
            o_pbFrame->iFrameProcessedLen=0;
            iRet=ParseNaluFromFrame(o_pbFrame);
            if(iRet<0)
            {
                printf("GetCodecData ParseNaluFromFrame err %d\r\n",o_pbFrame->eEncType);
                return iRet;
            }
            iRet = o_pbFrame->iFrameBufLen;
            memmove(m_pOutCodecFrameBuf,m_pOutCodecFrameBuf+iRet,iRet);//
            m_iOutCodecFrameBufLen-=iRet;
            m_tOutCodecFrameList.pop_front();
        }
    }
    else
    {
        iRet = 0;
    }
#endif
    return iRet;
}



/*****************************************************************************
-Fuction        : SetTransCodec
-Description    : �����Ƿ�����ת��
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert :: SetWaterMark(int i_iEnable,unsigned char * i_pbTextBuf,int i_iMaxTextBufLen,unsigned char * i_pbFontFileBuf,int i_iMaxFontFileBufLen)
{
    int iRet = -1;
    char strText[256];
    char strFontFile[128];
    memset(strText,0,sizeof(strText));
    memset(strFontFile,0,sizeof(strFontFile));


    if(0 == i_iEnable)
    {
        m_iSetWaterMarkFlag=0;//
    }
    else
    {
        if(NULL== i_pbTextBuf || i_iMaxTextBufLen<=0|| i_iMaxTextBufLen>=sizeof(strText))
        {
            printf("SetWaterMark NULL err %d\r\n",i_iMaxTextBufLen);
            return iRet;
        }
        if(NULL== i_pbFontFileBuf || i_iMaxFontFileBufLen<=0|| i_iMaxFontFileBufLen>=sizeof(strFontFile))
        {
            printf("SetWaterMark Font NULL err %d\r\n",i_iMaxFontFileBufLen);
            return iRet;
        }
        m_iSetWaterMarkFlag=1;//��ˮӡ��Ҫ����ת��
        memcpy(strText,i_pbTextBuf,i_iMaxTextBufLen);
        memcpy(strFontFile,i_pbFontFileBuf,i_iMaxFontFileBufLen);
    }

    return m_oMediaTranscodeInf.SetWaterMarkParam(i_iEnable,(const char *)strText,(const char *)strFontFile);
}

/*****************************************************************************
-Fuction        : SetTransCodec
-Description    : �����Ƿ�����ת��
//mp4Ӳ���Ȼ��������Ƶ����Ϊaac 44100����web���޷�����,����ʱ��ֻ�����Ƶ�����ʻ��д���
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert :: SetTransCodec(int i_iVideoEnable,unsigned char * i_pbDstVideoEncBuf,int i_iMaxDstVideoEncBufLen,int i_iAudioEnable,unsigned char * i_pbDstAudioEncBuf,int i_iMaxDstAudioEncBufLen,unsigned char * i_pbDstAudioSampleRateBuf,int i_iMaxDstAudioSampleRateBufLen)
{
    int iRet = -1;
    char strDstVideoEnc[16];
    char strDstAudioEnc[16];
    char strDstAudioSampleRate[16];
    memset(strDstVideoEnc,0,sizeof(strDstVideoEnc));
    memset(strDstAudioEnc,0,sizeof(strDstAudioEnc));
    memset(strDstAudioSampleRate,0,sizeof(strDstAudioSampleRate));

    if(0 != i_iVideoEnable && (NULL== i_pbDstVideoEncBuf || i_iMaxDstVideoEncBufLen<=0|| i_iMaxDstVideoEncBufLen>=sizeof(strDstVideoEnc)))
    {
        printf("NULL== i_pbDstVideoEncBuf || i_iMaxDstVideoEncBufLen<=0 err %d\r\n",i_iMaxDstVideoEncBufLen);
        return iRet;
    }
    if(0 != i_iAudioEnable && (NULL== i_pbDstAudioEncBuf || i_iMaxDstAudioEncBufLen<=0|| i_iMaxDstAudioEncBufLen>=sizeof(strDstAudioEnc)))
    {
        printf("NULL== i_pbDstAudioEncBuf || i_iMaxDstAudioEncBufLen<=0 err %d\r\n",i_iMaxDstAudioEncBufLen);
        return iRet;
    }

    if(NULL== i_pbDstAudioSampleRateBuf || i_iMaxDstAudioSampleRateBufLen<=0|| i_iMaxDstAudioSampleRateBufLen>=sizeof(strDstAudioSampleRate))
    {
        printf("NULL== i_pbDstAudioSampleRateBuf use default %d\r\n",i_iMaxDstAudioSampleRateBufLen);
    }
    else
    {
        memcpy(strDstAudioSampleRate,i_pbDstAudioSampleRateBuf,i_iMaxDstAudioSampleRateBufLen);
        printf("m_iTransAudioCodecFlag %d strDstAudioSampleRate %s\r\n",m_iTransAudioCodecFlag,strDstAudioSampleRate);
    }
    
    if(0 == i_iVideoEnable)
    {
        m_iTransCodecFlag=i_iVideoEnable;
        printf("SetTransCodec %d\r\n",m_iTransCodecFlag);
    }
    else
    {
        m_iTransCodecFlag=i_iVideoEnable;
        memcpy(strDstVideoEnc,i_pbDstVideoEncBuf,i_iMaxDstVideoEncBufLen);
        printf("m_iTransCodecFlag %d strDstVideoEnc %s\r\n",m_iTransCodecFlag,strDstVideoEnc);
        if(NULL != strstr(strDstVideoEnc,"h264"))
        {
            m_eDstTransVideoCodecType=CODEC_TYPE_H264;
        }
        else if(NULL != strstr(strDstVideoEnc,"h265"))
        {
            m_eDstTransVideoCodecType=CODEC_TYPE_H265;
        }
    }

    if(0 == i_iAudioEnable)
    {
        m_iTransAudioCodecFlag=i_iAudioEnable;
        printf("SetAudioTransCodec %d\r\n",m_iTransAudioCodecFlag);
    }
    else
    {
        m_iTransAudioCodecFlag=i_iAudioEnable;
        memcpy(strDstAudioEnc,i_pbDstAudioEncBuf,i_iMaxDstAudioEncBufLen);
        printf("m_iTransAudioCodecFlag %d strDstAudioEnc %s\r\n",m_iTransAudioCodecFlag,strDstAudioEnc);
        if(NULL != strstr(strDstAudioEnc,"aac"))
        {
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_AAC;
            m_dwAudioCodecSampleRate=44100;
        }
        else if(NULL != strstr(strDstAudioEnc,"g711a"))
        {
            m_eDstTransAudioCodecType=AUDIO_CODEC_TYPE_PCMA;
            m_dwAudioCodecSampleRate=8000;
        }

        if(NULL != strstr(strDstAudioSampleRate,"44100"))
        {
            m_dwAudioCodecSampleRate=44100;
        }
        else if(NULL != strstr(strDstAudioSampleRate,"8000"))
        {
            m_dwAudioCodecSampleRate=8000;
        }
    }
    return 0;
}

/*****************************************************************************
-Fuction        : MediaTranscode
-Description    : MediaTranscode
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert :: DecodeToOriginalData(T_MediaFrameInfo * i_pbFrame,T_MediaFrameInfo * o_pbFrame)
{
	int iRet = -1;
#ifdef SUPPORT_CODEC
    T_CodecFrame tSrcFrame,tDstFrame;
    
    if(NULL== i_pbFrame||NULL== o_pbFrame)
    {
        printf("NULL== i_pbFrame||NULL== o_pbFrame err \r\n");
        return iRet;
    }
    if(i_pbFrame->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        printf("UNSUPPORT AUDIO_FRAME \r\n");
        return 0;
    }
    memset(&tSrcFrame,0,sizeof(T_CodecFrame));
    MediaFrameToCodecFrame(i_pbFrame,&tSrcFrame);
    
    memset(&tDstFrame,0,sizeof(T_CodecFrame));
    memcpy(&tDstFrame,&tSrcFrame,sizeof(T_CodecFrame));
    tDstFrame.pbFrameBuf=m_pOutCodecFrameBuf+m_iOutCodecFrameBufLen;
    tDstFrame.iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN-m_iOutCodecFrameBufLen;
    
    iRet = m_oMediaTranscodeInf.DecodeToRGB(&tSrcFrame, &tDstFrame);
    if(iRet < 0)
    {
        printf("oMediaTranscodeInf.DecodeToRGB err %d\r\n",tSrcFrame.iFrameBufLen);
        return iRet;
    } 
    do
    {
        if(tDstFrame.iFrameBufLen == 0)
        {
            printf("DecodeToRGB DstFrame.iFrameBufLen == 0 \r\n");
            break;
        } 
        m_iOutCodecFrameBufLen += tDstFrame.iFrameBufLen;
        memcpy(o_pbFrame->pbFrameBuf,tDstFrame.pbFrameBuf,tDstFrame.iFrameBufLen);
        o_pbFrame->iFrameBufLen=tDstFrame.iFrameBufLen;
        o_pbFrame->iFrameLen=tDstFrame.iFrameBufLen;
        
        memmove(m_pOutCodecFrameBuf,m_pOutCodecFrameBuf+tDstFrame.iFrameBufLen,tDstFrame.iFrameBufLen);//
        m_iOutCodecFrameBufLen-=tDstFrame.iFrameBufLen;
    }while(0);
    //printf("DecodeToOriginalData dwTimeStamp%d,iFrameRate %d,iFrameBufLen%d,iFrameLen %d,dwNaluCount%d,iFrameRate%d\r\n",i_pbFrame->dwTimeStamp,
    //tDstFrame.iFrameRate,o_pbFrame->iFrameBufLen,i_pbFrame->iFrameLen,i_pbFrame->dwNaluCount,i_pbFrame->iFrameRate);
    iRet=0;
#endif
    return iRet;
}

/*****************************************************************************
-Fuction        : CodecDataToOriginalData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int MediaConvert :: CodecDataToOriginalData(T_MediaFrameInfo * i_ptFrameInfo)
{
    int iRet = -1;
    T_MediaFrameInfo * ptFrameInfo =NULL;
    T_MediaFrameInfo tTranscodeFrameInfo;
    T_MediaFrameInfo tAudioTranscodeFrameInfo;
	DataBuf * pbOutBuf =NULL;

    if(NULL== i_ptFrameInfo)
    {
        printf("CodecDataToOriginalData NULL err \r\n");
        return iRet;
    }

    ptFrameInfo=i_ptFrameInfo;
    memset(&m_tSegInfo,0,sizeof(T_SegInfo));
    if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME)
    {
        m_tSegInfo.iHaveKeyFrameFlag=1;
    }
    if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_I_FRAME ||
    ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_P_FRAME ||
    ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_VIDEO_B_FRAME)
    {
        m_tSegInfo.iVideoFrameCnt++;
    }
    else if(ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        m_tSegInfo.iAudioFrameCnt++;
    }
    m_tSegInfo.dwFrameTimeStamp=ptFrameInfo->dwTimeStamp;
    m_tSegInfo.iEncType=ptFrameInfo->eEncType;//MEDIA_ENCODE_TYPE_RGBA
    m_tSegInfo.iFrameType=ptFrameInfo->eFrameType;
    m_tSegInfo.dwWidth=ptFrameInfo->dwWidth;
    m_tSegInfo.dwHeight=ptFrameInfo->dwHeight;
    m_tSegInfo.dwSampleRate=ptFrameInfo->dwSampleRate;

    if(i_ptFrameInfo->eFrameType == MEDIA_FRAME_TYPE_AUDIO_FRAME)
    {
        iRet = AudioTranscode(ptFrameInfo,m_eDstTransAudioCodecType,m_dwAudioCodecSampleRate,&tAudioTranscodeFrameInfo);
        if(iRet <= 0)
        {
            printf("AudioTranscode iRet <= 0 \r\n");
            return iRet;
        }
        memcpy(&tTranscodeFrameInfo,&tAudioTranscodeFrameInfo,sizeof(T_MediaFrameInfo));
        ptFrameInfo=&tTranscodeFrameInfo;
        ptFrameInfo->pbFrameBuf=m_pMediaTranscodeBuf;
        ptFrameInfo->iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN;
        memcpy(ptFrameInfo->pbFrameBuf,tAudioTranscodeFrameInfo.pbFrameBuf,tAudioTranscodeFrameInfo.iFrameBufLen);
        ptFrameInfo->iFrameBufLen=tAudioTranscodeFrameInfo.iFrameBufLen;//ǰ�˴���44�ֽ�wavͷ
        //memcpy(ptFrameInfo->pbFrameBuf,tAudioTranscodeFrameInfo.pbFrameBuf+sizeof(T_WavHeader),tAudioTranscodeFrameInfo.iFrameBufLen-sizeof(T_WavHeader));
        //ptFrameInfo->iFrameBufLen=tAudioTranscodeFrameInfo.iFrameBufLen-sizeof(T_WavHeader);
        m_tSegInfo.dwSampleRate=ptFrameInfo->dwSampleRate;
    }
    else
    {
        memset(&tTranscodeFrameInfo,0,sizeof(T_MediaFrameInfo));
        ptFrameInfo=&tTranscodeFrameInfo;
        ptFrameInfo->pbFrameBuf=m_pMediaTranscodeBuf;
        ptFrameInfo->iFrameBufMaxLen=MEDIA_OUTPUT_BUF_MAX_LEN;
        iRet=DecodeToOriginalData(i_ptFrameInfo,ptFrameInfo);
    }
    
    if(NULL == pbOutBuf)
    {
        pbOutBuf = new DataBuf(tTranscodeFrameInfo.iFrameBufLen);
    }
    pbOutBuf->Copy(ptFrameInfo->pbFrameBuf,ptFrameInfo->iFrameBufLen);
    memcpy(&pbOutBuf->tSegInfo,&m_tSegInfo,sizeof(T_SegInfo));
    m_pDataBufList.push_back(pbOutBuf);
    pbOutBuf = NULL;

    return iRet;
}

#ifdef SUPPORT_CODEC
/*****************************************************************************
-Fuction		: CodecFrameToMediaFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert :: CodecFrameToMediaFrame(T_CodecFrame * i_ptCodecFrame,T_MediaFrameInfo * m_ptMediaFrame)
{
    int iRet = -1;

    if(NULL== i_ptCodecFrame || NULL== m_ptMediaFrame)
    {
        printf("NULL== i_ptCodecFrame || NULL== m_ptMediaFrame err \r\n");
        return iRet;
    }
    m_ptMediaFrame->pbFrameBuf=i_ptCodecFrame->pbFrameBuf;
    m_ptMediaFrame->iFrameBufMaxLen=i_ptCodecFrame->iFrameBufLen;//
    m_ptMediaFrame->iFrameBufLen=i_ptCodecFrame->iFrameBufLen;

    switch (i_ptCodecFrame->eFrameType)
    {
        case CODEC_FRAME_TYPE_VIDEO_I_FRAME:
        {
            m_ptMediaFrame->eFrameType=MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case CODEC_FRAME_TYPE_VIDEO_P_FRAME:
        {
            m_ptMediaFrame->eFrameType=MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case CODEC_FRAME_TYPE_VIDEO_B_FRAME:
        {
            m_ptMediaFrame->eFrameType=MEDIA_FRAME_TYPE_VIDEO_B_FRAME;
            break;
        }
        case CODEC_FRAME_TYPE_AUDIO_FRAME:
        {
            m_ptMediaFrame->eFrameType=MEDIA_FRAME_TYPE_AUDIO_FRAME;
            break;
        }
        default:
        {
            printf("m_ptMediaFrame->eFrameType err%d %d\r\n",m_ptMediaFrame->eFrameType,i_ptCodecFrame->eFrameType);
            return iRet;
        }
    }
    switch (i_ptCodecFrame->eEncType)
    {
        case CODEC_TYPE_H264:
        {
            m_ptMediaFrame->eEncType=MEDIA_ENCODE_TYPE_H264;
            break;
        }
        case CODEC_TYPE_H265:
        {
            m_ptMediaFrame->eEncType=MEDIA_ENCODE_TYPE_H265;
            break;
        }
        case CODEC_TYPE_AAC:
        {
            m_ptMediaFrame->eEncType=MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case CODEC_TYPE_G711A:
        {
            m_ptMediaFrame->eEncType=MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        case CODEC_TYPE_G711U:
        {
            m_ptMediaFrame->eEncType=MEDIA_ENCODE_TYPE_G711U;
            break;
        }
        default:
        {
            printf("m_ptMediaFrame->eEncType err%d %d\r\n",m_ptMediaFrame->eEncType,i_ptCodecFrame->eEncType);
            return iRet;
        }
    }
    m_ptMediaFrame->dwTimeStamp=(unsigned int)i_ptCodecFrame->ddwPTS;
    m_ptMediaFrame->dwSampleRate=i_ptCodecFrame->dwSampleRate;
    m_ptMediaFrame->dwWidth=i_ptCodecFrame->dwWidth;
    m_ptMediaFrame->dwHeight=i_ptCodecFrame->dwHeight;
    m_ptMediaFrame->tAudioEncodeParam.dwChannels=i_ptCodecFrame->dwChannels;
    
    return 0;
}

/*****************************************************************************
-Fuction		: MediaFrameToCodecFrame
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int MediaConvert :: MediaFrameToCodecFrame(T_MediaFrameInfo * i_ptMediaFrame,T_CodecFrame * i_ptCodecFrame)
{
    int iRet = -1;

    if(NULL== i_ptMediaFrame || NULL== i_ptCodecFrame)
    {
        printf("NULL== i_pMediaFrame || NULL== i_pCodecFrame err \r\n");
        return iRet;
    }
    i_ptCodecFrame->pbFrameBuf=i_ptMediaFrame->pbFrameStartPos;
    i_ptCodecFrame->iFrameBufMaxLen=i_ptMediaFrame->iFrameLen;//
    i_ptCodecFrame->iFrameBufLen=i_ptMediaFrame->iFrameLen;

    switch (i_ptMediaFrame->eFrameType)
    {
        case MEDIA_FRAME_TYPE_VIDEO_I_FRAME:
        {
            i_ptCodecFrame->eFrameType=CODEC_FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_P_FRAME:
        {
            i_ptCodecFrame->eFrameType=CODEC_FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_B_FRAME:
        {
            i_ptCodecFrame->eFrameType=CODEC_FRAME_TYPE_VIDEO_B_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_AUDIO_FRAME:
        {
            i_ptCodecFrame->eFrameType=CODEC_FRAME_TYPE_AUDIO_FRAME;
            break;
        }
        default:
        {
            printf("ptFrameInfo->eFrameType err%d %d\r\n",i_ptMediaFrame->eFrameType,i_ptCodecFrame->eFrameType);
            return iRet;
        }
    }
    switch (i_ptMediaFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_H264:
        {
            i_ptCodecFrame->eEncType=CODEC_TYPE_H264;
            break;
        }
        case MEDIA_ENCODE_TYPE_H265:
        {
            i_ptCodecFrame->eEncType=CODEC_TYPE_H265;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            i_ptCodecFrame->eEncType=CODEC_TYPE_AAC;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711A:
        {
            i_ptCodecFrame->eEncType=CODEC_TYPE_G711A;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            i_ptCodecFrame->eEncType=CODEC_TYPE_G711U;
            break;
        }
        default:
        {
            printf("ptFrameInfo->eEncType err%d %d\r\n",i_ptMediaFrame->eEncType,i_ptCodecFrame->eEncType);
            return iRet;
        }
    }
    i_ptCodecFrame->ddwDTS=(int64_t)i_ptMediaFrame->dwTimeStamp;
    i_ptCodecFrame->ddwPTS=(int64_t)i_ptMediaFrame->dwTimeStamp;
    i_ptCodecFrame->iFrameRate=i_ptMediaFrame->iFrameRate>0?i_ptMediaFrame->iFrameRate:25;//���費�У�����ת���Ĭ��֡���Ǹ���ֵ����Ϊ0֡��Ҳ���ԣ�ͬʱgop����֡����Ҳ���ܲ�����
    i_ptCodecFrame->dwSampleRate=i_ptMediaFrame->dwSampleRate>0?i_ptMediaFrame->dwSampleRate : i_ptCodecFrame->dwSampleRate;//�ļ����ܽ����������ļ�Ϊ׼
    i_ptCodecFrame->dwWidth=i_ptMediaFrame->dwWidth;
    i_ptCodecFrame->dwHeight=i_ptMediaFrame->dwHeight;
    i_ptCodecFrame->dwChannels=i_ptMediaFrame->tAudioEncodeParam.dwChannels;
    //CODEC_LOGD("ptFrameInfo->dwTimeStamp %d ,%lld %lld\r\n",i_ptMediaFrame->dwTimeStamp,i_ptCodecFrame->ddwPTS,i_ptCodecFrame->ddwDTS);
    return 0;
}
#endif
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
    o_ptMediaFrameInfo->eStreamType= STREAM_TYPE_MUX_STREAM;//��Ƶ����Ҫ�����ɶ��nalu����Ƶ���ݿ�ֱ���ⲿ����

    o_ptMediaFrameInfo->eEncType = eEncType;
    o_ptMediaFrameInfo->eFrameType = eFrameType;

    o_ptMediaFrameInfo->dwTimeStamp=i_pFrame->nTimeStamp;
    o_ptMediaFrameInfo->dwWidth= i_pFrame->nWidth;
    o_ptMediaFrameInfo->dwHeight=i_pFrame->nHeight;
    o_ptMediaFrameInfo->iFrameRate=i_pFrame->nFrameRate;

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
                else if(iRet < 0)
                {
                    printf("SetH264NaluData err %d\r\n",m_ptFrame->dwNaluCount);
                    break;//err
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
                else if(iRet < 0)
                {
                    printf("SetH264NaluData err %d\r\n",m_ptFrame->dwNaluCount);
                    break;//err
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
                else if(iRet < 0)
                {
                    printf("SetH265NaluData err %d\r\n",m_ptFrame->dwNaluCount);
                    break;//err
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
                else if(iRet < 0)
                {
                    printf("SetH265NaluData err %d\r\n",m_ptFrame->dwNaluCount);
                    break;//err
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
-Return         : <0 err,=0 need more,>0 eFrameType
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
    if(m_ptFrame->dwNaluCount >= sizeof(m_ptFrame->atNaluInfo)/sizeof(T_MediaNaluInfo))
    {
        printf("m_ptFrame->dwNaluCount %d >= MAX_NALU_CNT_ONE_FRAME %d\r\n",m_ptFrame->dwNaluCount, sizeof(m_ptFrame->atNaluInfo)/sizeof(T_MediaNaluInfo));
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
        iRet = (int)eFrameType;//������һ֡���˳�
    }
    else
    {
        iRet = 0;
    }
    return eFrameType;
}
/*****************************************************************************
-Fuction        : SetH265NaluData
-Description    : 
-Input          : 
-Output         : 
-Return         : <0 err,=0 need more,>0 eFrameType
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
    if(m_ptFrame->dwNaluCount >= sizeof(m_ptFrame->atNaluInfo)/sizeof(T_MediaNaluInfo))
    {
        printf("m_ptFrame->dwNaluCount %d >= MAX_NALU_CNT_ONE_FRAME %d\r\n",m_ptFrame->dwNaluCount, sizeof(m_ptFrame->atNaluInfo)/sizeof(T_MediaNaluInfo));
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
        iRet = (int)eFrameType;//������һ֡���˳�
    }
    else
    {
        iRet = 0;
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
    else if(NULL != strstr(i_strDstName,"OriginalData"))
    {
        eDstStreamType=STREAM_TYPE_ORIGINAL_STREAM;
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
    else if(NULL != strstr(i_strSrcName,"wav"))
    {
        eSrcStreamType=STREAM_TYPE_WAV_STREAM;
    }
    else
    {
        printf("i_strSrcName %s err\r\n",i_strSrcName);
        return -1;
    }
    //printf("Convert %d ,%s to %s\r\n",i_iSrcDataLen,i_strSrcName,i_strDstName);
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
int GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen)
{
    return MediaConvert::Instance()->GetEncodeType(o_pbVideoEncBuf,i_iMaxVideoEncBufLen,o_pbAudioEncBuf,i_iMaxAudioEncBufLen);
}


/*****************************************************************************
-Fuction        : SetWaterMark
-Description    : ����ˮӡ����
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int SetWaterMark(int i_iEnable,unsigned char * i_pbTextBuf,int i_iMaxTextBufLen,unsigned char * i_pbFontFileBuf,int i_iMaxFontFileBufLen)
{
    return MediaConvert::Instance()->SetWaterMark(i_iEnable,i_pbTextBuf,i_iMaxTextBufLen,i_pbFontFileBuf,i_iMaxFontFileBufLen);
}


/*****************************************************************************
-Fuction        : SetTransCodec
-Description    : �����Ƿ�����ת��
//mp4Ӳ���Ȼ��������Ƶ����Ϊaac 44100����web���޷�����,����ʱ��ֻ�����Ƶ�����ʻ��д���
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int SetTransCodec(int i_iVideoEnable,unsigned char * i_pbDstVideoEncBuf,int i_iMaxDstVideoEncBufLen,int i_iAudioEnable,unsigned char * i_pbDstAudioEncBuf,int i_iMaxDstAudioEncBufLen,unsigned char * i_pbDstAudioSampleRateBuf,int i_iMaxDstAudioSampleRateBufLen)
{
    return MediaConvert::Instance()->SetTransCodec(i_iVideoEnable,i_pbDstVideoEncBuf,i_iMaxDstVideoEncBufLen,i_iAudioEnable,i_pbDstAudioEncBuf,i_iMaxDstAudioEncBufLen,i_pbDstAudioSampleRateBuf,i_iMaxDstAudioSampleRateBufLen);
}


/*****************************************************************************
-Fuction        : Clean
-Description    : ����������ݣ����¿�ʼת��
������Ϊ��������ര�ڶ�·����
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

