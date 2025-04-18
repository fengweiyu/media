/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecG711A.cpp
* Description		: 	CodecG711A operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>

#include "CodecG711A.h"

#define G711A_MAX (32635)

/*****************************************************************************
-Fuction		: CodecG711A
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecG711A::CodecG711A()
{
    memset(&m_tCodecEncParam,0,sizeof(T_AudioCodecParam));
}
/*****************************************************************************
-Fuction		: ~CodecG711A
-Description	: ~CodecG711A
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecG711A::~CodecG711A()
{

}

/*****************************************************************************
-Fuction		: CodecG711A::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711A::Init()
{

	return 0;
}

/*****************************************************************************
-Fuction		: CodecG711A::InitEncode
-Description	: 只支持pcm16 , 需要支持其他采样位数可先用CodecPCM类处理
-Input			: i_tSrcCodecParam 输入数据的编码参数
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711A::InitEncode(T_AudioCodecParam i_tSrcCodecParam)
{
    if(i_tSrcCodecParam.dwBitsPerSample != 16)
    {
        AC_LOGE("CodecG711A InitEncode err %d,%d,%d\r\n",i_tSrcCodecParam.dwBitsPerSample,i_tSrcCodecParam.dwSampleRate,i_tSrcCodecParam.dwChannels);
        return -1;
    }

    memcpy(&m_tCodecEncParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
	return 0;
}

/*****************************************************************************
-Fuction		: CodecG711A::Encode
-Description	: 编码不改变采样率，采样位数，声道(通道)
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711A::Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet=-1;
	int i=0,iDstLen=0;
	unsigned short * pwSrcBuf = (unsigned short *)i_abSrcBuf;//只支持16位的采样位数，如需支持其他位数则需修改代码保留最高16(13)位
	
    if(NULL == i_abSrcBuf || 0 == i_iSrcBufLen  || NULL == o_abDstBuf || i_iDstBufMaxLen < i_iSrcBufLen / 2)
    {
        AC_LOGE("CodecG711A Encode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
	iDstLen = i_iSrcBufLen / 2;
	for (i = 0; i < iDstLen; i++)
	{
	    //o_abDstBuf[i] = EncodeFromTable((signed short)pwSrcBuf[i]);
	    o_abDstBuf[i] = PCM13ToAlaw((signed short)pwSrcBuf[i]);
	}
	iRet = iDstLen;
	return iRet;
}

/*****************************************************************************
-Fuction		: CodecWAV::InitEncode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711A::InitDecode()
{

	return 0;
}

/*****************************************************************************
-Fuction		: CodecG711A::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711A::Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)
{
    int iRet=-1;
	int i=0,iDstLen=0;
	unsigned short * pwDstBuf = (unsigned short*)o_abDstBuf;//解码出采样位数为16(13)位的pcm

    if(NULL == i_abSrcBuf || NULL == o_abDstBuf)
    {
        AC_LOGE("CodecG711A Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
	
	for (i = 0; i < i_iSrcBufLen; i++)
	{
		//pwDstBuf[i] = (unsigned short)DecodeFromTable(i_abSrcBuf[i]);//解码出16(13)位pcm
		pwDstBuf[i] = (unsigned short)AlawToPCM13(i_abSrcBuf[i]);//解码出16(13)位pcm
	}
	iDstLen = i_iSrcBufLen << 1;

    o_ptCodecParam->eAudioCodecType=AUDIO_CODEC_TYPE_PCM;
    o_ptCodecParam->dwBitsPerSample=sizeof(unsigned short)*8;
    //o_ptCodecParam->dwChannels=tWavHeader.wNumChannels;
    //o_ptCodecParam->dwSampleRate=tWavHeader.dwSampleRate;
	
    iRet=iDstLen;
	return iRet;
}

/*****************************************************************************
-Fuction		: CodecG711A::Encode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
unsigned char CodecG711A::EncodeFromTable(signed short i_wPCM16)
{
	static unsigned char l2A[2048] =
    {
        0xd5, 0xd4, 0xd7, 0xd6, 0xd1, 0xd0, 0xd3, 0xd2,
        0xdd, 0xdc, 0xdf, 0xde, 0xd9, 0xd8, 0xdb, 0xda,
        0xc5, 0xc4, 0xc7, 0xc6, 0xc1, 0xc0, 0xc3, 0xc2,
        0xcd, 0xcc, 0xcf, 0xce, 0xc9, 0xc8, 0xcb, 0xca,
        0xf5, 0xf5, 0xf4, 0xf4, 0xf7, 0xf7, 0xf6, 0xf6,
        0xf1, 0xf1, 0xf0, 0xf0, 0xf3, 0xf3, 0xf2, 0xf2,
        0xfd, 0xfd, 0xfc, 0xfc, 0xff, 0xff, 0xfe, 0xfe,
        0xf9, 0xf9, 0xf8, 0xf8, 0xfb, 0xfb, 0xfa, 0xfa,
        0xe5, 0xe5, 0xe5, 0xe5, 0xe4, 0xe4, 0xe4, 0xe4,
        0xe7, 0xe7, 0xe7, 0xe7, 0xe6, 0xe6, 0xe6, 0xe6,
        0xe1, 0xe1, 0xe1, 0xe1, 0xe0, 0xe0, 0xe0, 0xe0,
        0xe3, 0xe3, 0xe3, 0xe3, 0xe2, 0xe2, 0xe2, 0xe2,
        0xed, 0xed, 0xed, 0xed, 0xec, 0xec, 0xec, 0xec,
        0xef, 0xef, 0xef, 0xef, 0xee, 0xee, 0xee, 0xee,
        0xe9, 0xe9, 0xe9, 0xe9, 0xe8, 0xe8, 0xe8, 0xe8,
        0xeb, 0xeb, 0xeb, 0xeb, 0xea, 0xea, 0xea, 0xea,
        0x95, 0x95, 0x95, 0x95, 0x95, 0x95, 0x95, 0x95,
        0x94, 0x94, 0x94, 0x94, 0x94, 0x94, 0x94, 0x94,
        0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97,
        0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
        0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91, 0x91,
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93,
        0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92,
        0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d, 0x9d,
        0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c, 0x9c,
        0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f,
        0x9e, 0x9e, 0x9e, 0x9e, 0x9e, 0x9e, 0x9e, 0x9e,
        0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
        0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98,
        0x9b, 0x9b, 0x9b, 0x9b, 0x9b, 0x9b, 0x9b, 0x9b,
        0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a, 0x9a,
        0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85,
        0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85,
        0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84,
        0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84,
        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
        0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,
        0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86,
        0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86,
        0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
        0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
        0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83,
        0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
        0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
        0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d,
        0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d,
        0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c,
        0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c, 0x8c,
        0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f,
        0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f, 0x8f,
        0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
        0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
        0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89,
        0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89, 0x89,
        0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
        0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a,
        0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a, 0x8a,
        0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5,
        0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5,
        0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5,
        0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5, 0xb5,
        0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
        0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
        0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
        0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4, 0xb4,
        0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7,
        0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7,
        0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7,
        0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7, 0xb7,
        0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6,
        0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6,
        0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6,
        0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6,
        0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1,
        0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1,
        0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1,
        0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1, 0xb1,
        0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
        0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
        0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
        0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0,
        0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,
        0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,
        0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,
        0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,
        0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2,
        0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2,
        0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2,
        0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2, 0xb2,
        0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd,
        0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd,
        0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd,
        0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd, 0xbd,
        0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
        0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
        0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
        0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc, 0xbc,
        0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
        0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
        0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
        0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
        0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe,
        0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe,
        0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe,
        0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe, 0xbe,
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9, 0xb9,
        0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,
        0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,
        0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,
        0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,
        0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,
        0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,
        0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,
        0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,
        0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba,
        0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba,
        0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba,
        0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4, 0xa4,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7, 0xa7,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1, 0xa1,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2, 0xa2,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad, 0xad,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac, 0xac,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae, 0xae,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9, 0xa9,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8, 0xa8,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    };
	signed short wSrcData=0;
	unsigned char bMask=0;
	unsigned char bEncodedData=0;

    wSrcData = i_wPCM16;
    bMask = (wSrcData < 0) ? 0x7f : 0xff;//保留符号位
    if (wSrcData < 0)
        wSrcData = -wSrcData;//去掉符号位，等于取了15位
    wSrcData >>= 4;//15位取11位(等于取16位的高12位(包括符号位))
    bEncodedData = l2A[wSrcData] & bMask;//将一个13bit的pcm样本压缩成一个8bit的pcm样本。
	return bEncodedData;
}

/*****************************************************************************
-Fuction		: CodecG711A::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
signed short CodecG711A::DecodeFromTable(unsigned char i_bAlawData)
{
	static signed short A2l[256] =
	{
		-5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
		-7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
		-2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
		-3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
		-22016,-20992,-24064,-23040,-17920,-16896,-19968,-18944,
		-30208,-29184,-32256,-31232,-26112,-25088,-28160,-27136,
		-11008,-10496,-12032,-11520, -8960, -8448, -9984, -9472,
		-15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,
		-344,  -328,  -376,  -360,  -280,  -264,  -312,  -296,
		-472,  -456,  -504,  -488,  -408,  -392,  -440,  -424,
		-88,   -72,  -120,  -104,   -24,    -8,   -56,   -40,
		-216,  -200,  -248,  -232,  -152,  -136,  -184,  -168,
		-1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
		-1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
		-688,  -656,  -752,  -720,  -560,  -528,  -624,  -592,
		-944,  -912, -1008,  -976,  -816,  -784,  -880,  -848,
		5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,
		7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
		2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
		3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
		22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
		30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
		11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472,
		15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
		344,   328,   376,   360,   280,   264,   312,   296,
		472,   456,   504,   488,   408,   392,   440,   424,
		88,    72,   120,   104,    24,     8,    56,    40,
		216,   200,   248,   232,   152,   136,   184,   168,
		1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
		1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
		688,   656,   752,   720,   560,   528,   624,   592,
		944,   912,  1008,   976,   816,   784,   880,   848,
	};
	return A2l[i_bAlawData];//解码出16(13)位pcm
}

/*****************************************************************************
-Fuction		: PCM13ToAlaw
-Description	: 不用数组查表编码
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
unsigned char CodecG711A::PCM13ToAlaw(short pcm)
{
    int sign = (pcm & 0x8000) >> 8;
    if (sign != 0)
        pcm = -pcm;
    if (pcm > G711A_MAX) pcm = G711A_MAX;
    int exponent = 7;
    int expMask;
    for (expMask = 0x4000; (pcm & expMask) == 0 
		&& exponent>0; exponent--, expMask >>= 1) { }
    int mantissa = (pcm >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0f;
    unsigned char alaw = (unsigned char)(sign | exponent << 4 | mantissa);
    return (unsigned char)(alaw^0xD5);
}
/*****************************************************************************
-Fuction		: AlawToPCM13
-Description	: 不用数组查表解码
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
short CodecG711A::AlawToPCM13(unsigned char alaw)
{
    alaw ^= 0xD5;
    int sign = alaw & 0x80;
    int exponent = (alaw & 0x70) >> 4;
    int data = alaw & 0x0f;
    data <<= 4;
    data += 8;
    if (exponent != 0)
        data += 0x100;
    if (exponent > 1)
        data <<= (exponent - 1);
	
    return (short)(sign == 0 ? data : -data);
}







