/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	WAV.cpp
* Description		: 	WAV operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include "MediaAdapter.h"
#include "WAV.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>


using std::cout;//��Ҫ<iostream>
using std::endl;


//����ͷ����
struct T_RecorderWavHeader
{
    //RIFF chunk descriptor 12byte
    char            riff[4];            // = "RIFF"
    unsigned int        size_8;             // = FileSize - 8
    char            wave[4];            // = "WAVE"
 
    //fmt sub-chunk  24byte
    char            fmt[4];             // = "fmt "
    unsigned int        format_size;        // = �����ֽ�(һ��Ϊ00000010H����Ϊ00000012H˵������ͷЯ��������Ϣ)
    unsigned short        format_tag;         // = ������ WAV �ļ�ʹ�� PCM ���������Ƹ�ʽ,����ֵͨ��Ϊ 1
    unsigned short        channels;           // = ��������: ������Ϊ 1,��������˫����Ϊ 2
    unsigned int        samples_per_sec;    // = ����Ƶ�� : 8000 | 6000 | 11025 | 16000 | 22050  | 44100
    unsigned int        avg_bytes_per_sec;  // = ���ݴ�������(ÿ��ƽ���ֽ���)��������������Ƶ�ʡ�ÿ����������λ��/8������������ô�ֵ���Թ��ƻ������Ĵ�С��
                                        // = samples_per_sec * channels * bits_per_sample / 8
    unsigned short        block_align;        // = ÿ�������ֽ��� : ��������λ��/8�����������Ҫһ�δ�������ֵ��С���ֽ�����,�ø���ֵ������������
                                        // = channels * bits_per_sample / 8
    unsigned short        bits_per_sample;    // = ����λ��: �洢ÿ������ֵ���õĶ�������λ��, 8 | 16
 
    //data sub-chunk  8byte
    char            data[4];            // = "data";
    unsigned int        data_size;          // = �����ݳ��� : FileSize - 44
};

// WAVE file header format �ַ�����Ϊ��ˣ�����ΪС��
typedef struct WavHeader
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
} T_WavHeader;//һ��ֻ���ļ�ͷ�У�����ʼ��һ�μ��ɣ�����Ҫÿ֡ǰ���


/*****************************************************************************
-Fuction		: WAV
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
WAV::WAV()
{
    
}
/*****************************************************************************
-Fuction		: ~WAV
-Description	: ~WAV
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
WAV::~WAV()
{
}


/*****************************************************************************
-Fuction        : GetMuxData
-Description    : 
-Input          : 
-Output         : 
-Return         : <0 err,0 need more data,>0 datalen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int WAV::GetMuxData(T_MediaFrameInfo * i_ptFrameInfo,unsigned char * o_pbBuf,unsigned int i_dwMaxBufLen)
{
    int iRet=-1;
    T_WavHeader tWavHeader;
    unsigned char bAudioFormat=0;
    
    if(NULL == i_ptFrameInfo || NULL == o_pbBuf || i_dwMaxBufLen < sizeof(T_WavHeader)+i_ptFrameInfo->iFrameLen)
    {
        MH_LOGE("WAV GetMuxData err %d,%p\r\n",i_dwMaxBufLen,o_pbBuf);
        return iRet;
    }
    switch(i_ptFrameInfo->eEncType)
    {
        case MEDIA_ENCODE_TYPE_LPCM:
        {
            bAudioFormat=1;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711A:
        {
            bAudioFormat=6;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711U:
        {
            bAudioFormat=7;
            break;
        }
        default:
        {
            MH_LOGE("WAV.eAudioCodecType err %d\r\n",i_ptFrameInfo->eEncType);
            return iRet;
        }
    }
    
    memset(&tWavHeader,0,sizeof(T_WavHeader));
	// RIFF chunk
	strncpy((char *)tWavHeader.abChunkId,"RIFF",sizeof(tWavHeader.abChunkId));
	tWavHeader.dwChunkSize = 36 + i_ptFrameInfo->iFrameLen;

	// fmt sub-chunk (to be optimized)
	strncpy((char *)tWavHeader.abSubChunk1Id,"WAVEfmt ",sizeof(tWavHeader.abSubChunk1Id));
	tWavHeader.dwSubChunk1Size= 16;
	tWavHeader.wAudioFormat= bAudioFormat;
	tWavHeader.wNumChannels= (unsigned short)i_ptFrameInfo->tAudioEncodeParam.dwChannels;//1;
	tWavHeader.dwSampleRate= (unsigned short)i_ptFrameInfo->dwSampleRate;//8000;
	tWavHeader.wBitsPerSample= (unsigned short)i_ptFrameInfo->tAudioEncodeParam.dwBitsPerSample;//16;
	tWavHeader.wBlockAlign= tWavHeader.wNumChannels* tWavHeader.wBitsPerSample / 8;
	tWavHeader.dwByteRate= tWavHeader.dwSampleRate* tWavHeader.wNumChannels* tWavHeader.wBitsPerSample / 8;

	// data sub-chunk
	strncpy((char *)tWavHeader.abSubChunk2Id,"data",sizeof(tWavHeader.abSubChunk2Id));
	tWavHeader.dwSubChunk2Size= i_ptFrameInfo->iFrameLen;


    memcpy(o_pbBuf,&tWavHeader,sizeof(T_WavHeader));
    memcpy(o_pbBuf+sizeof(T_WavHeader),i_ptFrameInfo->pbFrameStartPos,i_ptFrameInfo->iFrameLen);
    iRet=sizeof(T_WavHeader)+i_ptFrameInfo->iFrameLen;
    
	return iRet;
}


/*****************************************************************************
-Fuction        : GetFrameData
-Description    : demux 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int WAV::GetFrameData(T_MediaFrameInfo *m_ptFrame)
{
    int iRet=-1;
    T_WavHeader tWavHeader;
	E_MediaEncodeType eAudioCodecType;         // 

    if(NULL == m_ptFrame || NULL == m_ptFrame->pbFrameBuf || m_ptFrame->iFrameBufLen-m_ptFrame->iFrameProcessedLen< sizeof(T_WavHeader))
    {
        MH_LOGE("WAV GetFrameData err %p\r\n",m_ptFrame);
        return iRet;
    }
    
	memset(&tWavHeader, 0, sizeof(T_WavHeader));
	memcpy(&tWavHeader, m_ptFrame->pbFrameBuf+m_ptFrame->iFrameProcessedLen, sizeof(T_WavHeader));
	if (strncmp((char*)tWavHeader.abChunkId, "RIFF", 4) != 0)
	{
        MH_LOGE("tWavHeader.abChunkId err %d,%d\r\n",m_ptFrame->iFrameBufLen,m_ptFrame->iFrameProcessedLen);
        return iRet;
	}
    switch(tWavHeader.wAudioFormat)
    {
        case 1:
        {
            eAudioCodecType=MEDIA_ENCODE_TYPE_LPCM;
            break;
        }
        case 6:
        {
            eAudioCodecType=MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        case 7:
        {
            eAudioCodecType=MEDIA_ENCODE_TYPE_G711U;
            break;
        }
        default:
        {
            MH_LOGE("tWavHeader.eAudioCodecType err %d.\r\n",tWavHeader.wAudioFormat);
            return iRet;
        }
    }
    m_ptFrame->eEncType=eAudioCodecType;
    m_ptFrame->tAudioEncodeParam.dwChannels=tWavHeader.wNumChannels;
    m_ptFrame->tAudioEncodeParam.dwBitsPerSample=tWavHeader.wBitsPerSample;
    m_ptFrame->dwSampleRate=tWavHeader.dwSampleRate;

	m_ptFrame->pbFrameStartPos=m_ptFrame->pbFrameBuf+m_ptFrame->iFrameProcessedLen+sizeof(T_WavHeader);
    iRet = tWavHeader.dwSubChunk2Size+sizeof(T_WavHeader);
    m_ptFrame->iFrameProcessedLen+=iRet;
    m_ptFrame->iFrameLen=tWavHeader.dwSubChunk2Size;
	return iRet;
}



