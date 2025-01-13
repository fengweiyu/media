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
#include <list>

#include "MediaHandle.h"

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



#define MEDIA_OUTPUT_BUF_MAX_LEN	(2*1024*1024) 
#define MEDIA_INPUT_BUF_MAX_LEN	(6*1024*1024) 



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
    int Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType);
    int GetData(unsigned char * o_pbData,int i_iMaxDataLen);
private:

    MediaHandle m_oMediaHandle;
    DataBuf * m_pbInputBuf;
    list<DataBuf *> m_pDataBufList;
    static MediaConvert *m_pInstance;
};
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
int MediaConvert::Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType)
{
    T_MediaFrameInfo tFileFrameInfo;
	DataBuf * pbOutBuf =NULL;
	int iRet = -1,iWriteLen=0;
	int iHeaderLen=0;


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
        m_oMediaHandle.GetFrame(&tFileFrameInfo);
        if(tFileFrameInfo.iFrameLen <= 0)
        {
            printf("tFileFrameInfo.iFrameLen <= 0 %d\r\n",i_iSrcDataLen);
            break;
        } 
        if(NULL == pbOutBuf)
        {
            pbOutBuf = new DataBuf(MEDIA_OUTPUT_BUF_MAX_LEN);
        }
        iWriteLen = m_oMediaHandle.FrameToContainer(&tFileFrameInfo,i_eDstStreamType,pbOutBuf->pbBuf,pbOutBuf->iBufMaxLen,&iHeaderLen);
        if(iWriteLen < 0)
        {
            printf("FrameToContainer err iWriteLen %d\r\n",iWriteLen);
            break;
        }
        if(iWriteLen == 0)
        {
            continue;
        }
        m_pDataBufList.push_back(pbOutBuf);
        pbOutBuf = NULL;
    }
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
EM_EXPORT_API(int) InputData(unsigned char * i_pbSrcData,int i_iSrcDataLen,const char *i_strSrcName,const char *i_strDstName)
{
    E_StreamType eSrcStreamType=STREAM_TYPE_UNKNOW;
    E_StreamType eDstStreamType=STREAM_TYPE_UNKNOW;
    E_MediaEncodeType eSrcEncType=MEDIA_ENCODE_TYPE_UNKNOW;

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
    else if(NULL != strstr(i_strDstName,".h264")||NULL != strstr(i_strDstName,".h265"))
    {
        eDstStreamType=STREAM_TYPE_VIDEO_STREAM;
    }
    else if(NULL != strstr(i_strDstName,".aac"))
    {
        eDstStreamType=STREAM_TYPE_AUDIO_STREAM;
    }
    else
    {
        printf("i_strDstName %s err\r\n",i_strDstName);
        return -1;
    }

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
EM_EXPORT_API(int) GetData(unsigned char * o_pbData,int i_iMaxDataLen)
{
    return MediaConvert::Instance()->GetData(o_pbData,i_iMaxDataLen);
}

