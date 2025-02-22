/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecWAV.cpp
* Description		: 	CodecWAV operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>
#include "CodecWAV.h"

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
-Fuction		: CodecWAV
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecWAV::CodecWAV()
{
    m_pAudioEncoder = NULL;
    memset(&m_tCodecEncParam,0,sizeof(T_AudioCodecParam));
}
/*****************************************************************************
-Fuction		: ~CodecWAV
-Description	: ~CodecWAV
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecWAV::~CodecWAV()
{
    if(NULL != m_pAudioEncoder)
    {
        delete m_pAudioEncoder;
        m_pAudioEncoder = NULL;
    }
}

/*****************************************************************************
-Fuction		: CodecWAV::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecWAV::Init()
{

	return 0;
}

/*****************************************************************************
-Fuction		: CodecWAV::InitEncode
-Description	: 
-Input			: i_tSrcCodecParam �������ݵı������
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecWAV::InitEncode(T_AudioCodecParam i_tSrcCodecParam)
{
    memcpy(&m_tCodecEncParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
	return 0;
}

/*****************************************************************************
-Fuction		: CodecWAV::Encode
-Description	: ���벻�ı�����ʣ�����λ��������(ͨ��)
-Input			: ����Ϊδѹ��������Ƶ����
-Output 		: 
-Return 		: <0 err,=0 need more data,>0 DstBufLen
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecWAV::Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet=-1;
    T_WavHeader tWavHeader;
    unsigned char bAudioFormat=0;
    
    if(NULL == i_abSrcBuf || NULL == o_abDstBuf || i_iDstBufMaxLen < sizeof(T_WavHeader)+i_iSrcBufLen)
    {
        AC_LOGE("CodecWAV Encode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    switch(m_tCodecEncParam.eAudioCodecType)
    {
        case AUDIO_CODEC_TYPE_WAV_PCM:
        {
            bAudioFormat=1;
            break;
        }
        case AUDIO_CODEC_TYPE_WAV_PCMA:
        {
            bAudioFormat=6;
            break;
        }
        case AUDIO_CODEC_TYPE_WAV_PCMU:
        {
            bAudioFormat=7;
            break;
        }
        default:
        {
            AC_LOGE("i_tCodecParam.eAudioCodecType err %d.only support pcm\r\n",m_tCodecEncParam.eAudioCodecType);
            return iRet;
        }
    }
    
    memset(&tWavHeader,0,sizeof(T_WavHeader));
	// RIFF chunk
	strncpy((char *)tWavHeader.abChunkId,"RIFF",sizeof(tWavHeader.abChunkId));
	tWavHeader.dwChunkSize = 36 + i_iSrcBufLen;

	// fmt sub-chunk (to be optimized)
	strncpy((char *)tWavHeader.abSubChunk1Id,"WAVEfmt ",sizeof(tWavHeader.abSubChunk1Id));
	tWavHeader.dwSubChunk1Size= 16;
	tWavHeader.wAudioFormat= bAudioFormat;
	tWavHeader.wNumChannels= (unsigned short)m_tCodecEncParam.dwChannels;//1;
	tWavHeader.dwSampleRate= (unsigned short)m_tCodecEncParam.dwSampleRate;//8000;
	tWavHeader.wBitsPerSample= (unsigned short)m_tCodecEncParam.dwBitsPerSample;//16;
	tWavHeader.wBlockAlign= tWavHeader.wNumChannels* tWavHeader.wBitsPerSample / 8;
	tWavHeader.dwByteRate= tWavHeader.dwSampleRate* tWavHeader.wNumChannels* tWavHeader.wBitsPerSample / 8;

	// data sub-chunk
	strncpy((char *)tWavHeader.abSubChunk2Id,"data",sizeof(tWavHeader.abSubChunk2Id));
	tWavHeader.dwSubChunk2Size= i_iSrcBufLen;


    memcpy(o_abDstBuf,&tWavHeader,sizeof(T_WavHeader));
    memcpy(o_abDstBuf+sizeof(T_WavHeader),i_abSrcBuf,i_iSrcBufLen);
    iRet=sizeof(T_WavHeader)+i_iSrcBufLen;
    
	return iRet;
}

/*****************************************************************************
-Fuction		: CodecWAV::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecWAV::InitDecode()
{

	return 0;
}

/*****************************************************************************
-Fuction		: CodecWAV::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: <0 err,=0 need more data,>0 DstBufLen
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecWAV::Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)
{
    int iRet=-1;
    T_WavHeader tWavHeader;
	E_AudioCodecType eAudioCodecType;         // 

    if(NULL == i_abSrcBuf || NULL == o_abDstBuf || i_iSrcBufLen < sizeof(T_WavHeader))
    {
        AC_LOGE("CodecWAV Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    
	memset(&tWavHeader, 0, sizeof(T_WavHeader));
	memcpy(&tWavHeader, i_abSrcBuf, sizeof(T_WavHeader));
	if (strncmp((char*)tWavHeader.abChunkId, "RIFF", 4) != 0)
	{
        AC_LOGE("tWavHeader.abChunkId err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
	}
    switch(tWavHeader.wAudioFormat)
    {
        case 1:
        {
            eAudioCodecType=AUDIO_CODEC_TYPE_PCM;
            break;
        }
        case 6:
        {
            eAudioCodecType=AUDIO_CODEC_TYPE_PCMA;
            break;
        }
        case 7:
        {
            eAudioCodecType=AUDIO_CODEC_TYPE_PCMU;
            break;
        }
        default:
        {
            AC_LOGE("i_tCodecParam.eAudioCodecType err %d.only support pcm\r\n",tWavHeader.wAudioFormat);
            return iRet;
        }
    }
    o_ptCodecParam->eAudioCodecType=eAudioCodecType;
    o_ptCodecParam->dwChannels=tWavHeader.wNumChannels;
    o_ptCodecParam->dwBitsPerSample=tWavHeader.wBitsPerSample;
    o_ptCodecParam->dwSampleRate=tWavHeader.dwSampleRate;
	memcpy(o_abDstBuf, i_abSrcBuf+sizeof(T_WavHeader), tWavHeader.dwSubChunk2Size);
    iRet = tWavHeader.dwSubChunk2Size;
	return iRet;
}

