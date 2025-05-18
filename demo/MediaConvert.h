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
#ifndef MEDIA_CONVERT_H
#define MEDIA_CONVERT_H

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include "AudioCodec.h"
#include "MediaHandle.h"
#ifdef SUPPORT_CODEC
#include "MediaTranscodeInf.h"
#endif
#ifdef SUPPORT_PRI
#include "XStreamParser.h"
#include "STDStream.h"
#endif
using std::string;
using std::list;


//EMSCRIPTEN_KEEPALIVE 同样能导出一个函数， 它跟将函数加到EXPORTED_FUNCTIONS中效果一样。
//为了防止name mangling， 在导出函数一定要使用extern “C” 来修饰

#ifndef EM_EXPORT_API
    #if defined(__EMSCRIPTEN__)
        #include <emscripten.h>
        #if defined(__cplusplus)
            #define EM_EXPORT_API(rettype) extern "C" rettype EMSCRIPTEN_KEEPALIVE
        #else
            #define EM_EXPORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE
        #endif
    #else
        #if defined(__cplusplus)
            #define EM_EXPORT_API(rettype) extern "C" rettype
        #else
            #define EM_EXPORT_API(rettype) rettype
        #endif
    #endif
#endif

typedef struct SegInfo
{
    int iHaveKeyFrameFlag;//0 否，1是
    int iSegStartTime;//段开始时间ms
    int iSegDurationTime;//段持续时间ms
    int iVideoFrameCnt;//
    int iAudioFrameCnt;//
    unsigned int dwStartTimeHigh;//
    unsigned int dwStartTimeLow;//
    unsigned int dwStartAbsTime;//
    int iEncType;//当xxxFrameCnt=1是可以用(此时该字段表示原始编码类型不是解码后的,解码后的用GetEncodeType方法)，E_MediaEncodeType 
    int iFrameType;//当xxxFrameCnt=1是可以用，E_MediaFrameType
    unsigned int dwFrameTimeStamp;//
    unsigned int dwWidth;//
    unsigned int dwHeight;//
    unsigned int dwSampleRate;//
}T_SegInfo;

/*****************************************************************************
-Class			: DataBuf
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class DataBuf
{
public:
    DataBuf(int i_iBufMaxLen)
    {
        pbBuf = new unsigned char [i_iBufMaxLen];
        iBufLen = 0;
        iBufMaxLen = i_iBufMaxLen;
        memset(&tSegInfo,0,sizeof(T_SegInfo));
    }
    virtual ~DataBuf()
    {
        delete [] pbBuf;
    }
    void Copy(unsigned char * i_pbSrcData,int i_iSrcDataLen)
    {
        if(i_iSrcDataLen+iBufLen<=iBufMaxLen)
        {
            memcpy(pbBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
            iBufLen += i_iSrcDataLen;
            return;
        }
        unsigned char * pBuf = new unsigned char [i_iSrcDataLen+iBufLen];
        memcpy(pBuf,pbBuf,iBufLen);
        memcpy(pBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
        delete [] pbBuf;
        pbBuf=pBuf;
        iBufLen = i_iSrcDataLen+iBufLen;
        iBufMaxLen = i_iSrcDataLen+iBufLen;
    }
    void Delete(int i_iProcessedLen)
    {
        if(i_iProcessedLen>=iBufLen)
        {
            iBufLen=0;
            return;
        }
        memmove(pbBuf,pbBuf+i_iProcessedLen,iBufLen-i_iProcessedLen);
        iBufLen = iBufLen-i_iProcessedLen;
    }
    unsigned char * pbBuf;//只允许只读操作
    int iBufLen;//只允许只读操作
    int iBufMaxLen;
    
    T_SegInfo tSegInfo;//
};

/*****************************************************************************
-Class			: MediaConvert
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class MediaConvert
{
public:
	MediaConvert();
	virtual ~MediaConvert();
    static MediaConvert * Instance();
    int ConvertFromPri(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_StreamType i_eDstStreamType);
    int Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType);
    int GetData(unsigned char * o_pbData,int i_iMaxDataLen,T_SegInfo *o_ptSegInfo);
    int GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen);
    int SetWaterMark(int i_iEnable,unsigned char * i_pbTextBuf,int i_iMaxTextBufLen,unsigned char * i_pbFontFileBuf,int i_iMaxFontFileBufLen);
    int SetTransCodec(int i_iVideoEnable,unsigned char * i_pbDstVideoEncBuf,int i_iMaxDstVideoEncBufLen,int i_iAudioEnable,unsigned char * i_pbDstAudioEncBuf,int i_iMaxDstAudioEncBufLen,unsigned char * i_pbDstAudioSampleRateBuf,int i_iMaxDstAudioSampleRateBufLen);
    static MediaConvert *m_pInstance;
private:
    int SynchronizerAudioVideo(T_MediaFrameInfo * i_ptFrameInfo);
    int AudioTranscode(T_MediaFrameInfo * i_pbAudioFrame,E_AudioCodecType i_eDstCodecType,unsigned int i_dwDstSampleRate,T_MediaFrameInfo * o_pbAudioFrame);
    int MediaTranscode(T_MediaFrameInfo * m_pbFrame);
    int GetCodecData(T_MediaFrameInfo * o_pbFrame);
    int CodecDataToOriginalData(T_MediaFrameInfo * i_ptFrameInfo);
    int DecodeToOriginalData(T_MediaFrameInfo * i_pbFrame,T_MediaFrameInfo * o_pbFrame);

    int SetH264NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame);
    int SetH265NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame);
    int ParseH264NaluFromFrame(T_MediaFrameInfo *m_ptFrame);
    int ParseH265NaluFromFrame(T_MediaFrameInfo *m_ptFrame);
    int ParseNaluFromFrame(T_MediaFrameInfo * o_ptFrameInfo);
#ifdef SUPPORT_CODEC
    int CodecFrameToMediaFrame(T_CodecFrame * i_ptCodecFrame,T_MediaFrameInfo * m_ptMediaFrame);
    int MediaFrameToCodecFrame(T_MediaFrameInfo * i_ptMediaFrame,T_CodecFrame * i_ptCodecFrame);
    MediaTranscodeInf m_oMediaTranscodeInf;
#endif
#ifdef SUPPORT_PRI
    FRAME_INFO* MediaToPri(T_MediaFrameInfo * i_ptMediaFrame);
    int PriToMedia(FRAME_INFO * i_pFrame,T_MediaFrameInfo *o_ptMediaFrameInfo);
	CSTDStream m_streamer;
#endif
    MediaHandle m_oMediaHandle;
    DataBuf * m_pbInputBuf;
	int m_iPutFrameLen;
	int m_iPutVideoFrameCnt;
    unsigned int m_dwPutVideoFrameTime;//ms
    T_SegInfo m_tSegInfo;//
	int m_iFindKeyFrame;//0 否 ，1是
    T_MediaFrameInfo m_tFileFrameInfo;//video 的sps等参数集会被tag分段，故要保存
    E_MediaEncodeType m_eDstVideoEncType;//转换后输出的视频编码格式
    E_MediaEncodeType m_eDstAudioEncType;//转换后输出的音频编码格式
    list<DataBuf *> m_pDataBufList;
    AudioCodec * m_pAudioCodec;
    unsigned char * m_pAudioTranscodeBuf;
    int m_iTransAudioCodecFlag;//0 否 ，1是
    
    int m_iSetWaterMarkFlag;//0 否 ，1是,
    list<T_CodecFrame> m_tOutCodecFrameList;
    unsigned char * m_pOutCodecFrameBuf;
	int m_iOutCodecFrameBufLen;
    unsigned char * m_pMediaTranscodeBuf;
    int m_iTransCodecFlag;//0 否 ，1是,目前只用来视频转码
    E_CodecType m_eDstTransVideoCodecType;
    E_AudioCodecType m_eDstTransAudioCodecType;
    unsigned int m_dwAudioCodecSampleRate;//

    unsigned int m_dwLastAudioTimeStamp;
    unsigned int m_dwLastVideoTimeStamp;
    unsigned int m_dwVideoTimeStamp;
    unsigned int m_dwAudioTimeStamp;
};
EM_EXPORT_API(int) InputData(unsigned char * i_pbSrcData,int i_iSrcDataLen,const char *i_strSrcName,const char *i_strDstName);
EM_EXPORT_API(int) GetData(unsigned char * o_pbData,int i_iMaxDataLen,unsigned char * o_pbDataInfo,int i_iMaxInfoLen);
EM_EXPORT_API(int) GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen);
EM_EXPORT_API(int) SetWaterMark(int i_iEnable,unsigned char * i_pbTextBuf,int i_iMaxTextBufLen,unsigned char * i_pbFontFileBuf,int i_iMaxFontFileBufLen);
EM_EXPORT_API(int) SetTransCodec(int i_iVideoEnable,unsigned char * i_pbDstVideoEncBuf,int i_iMaxDstVideoEncBufLen,int i_iAudioEnable,unsigned char * i_pbDstAudioEncBuf,int i_iMaxDstAudioEncBufLen,unsigned char * i_pbDstAudioSampleRateBuf,int i_iMaxDstAudioSampleRateBufLen);
EM_EXPORT_API(int)  Clean();


#endif
