/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecG711U.cpp
* Description		: 	CodecG711U operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>

#include "CodecG711U.h"

#define         SIGN_BIT        (0x80)      /* Sign bit for a A-law byte. */
#define         QUANT_MASK      (0xf)       /* Quantization field mask. */
#define         NSEGS           (8)         /* Number of A-law segments. */
#define         SEG_SHIFT       (4)         /* Left shift for segment number. */
#define         SEG_MASK        (0x70)      /* Segment field mask. */
#define         BIAS            (0x84)      /* Bias for linear code. */
#define		    CLIP            8159
 
#define		    G711_A_LAW	(0)
#define		    G711_MU_LAW	(1)
#define		    DATA_LEN	(16)

static short seg_aend[8] = {
	0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF
};
static short seg_uend[8] = {
	0x3F, 0x7F, 0xFF, 0x1FF,
	0x3FF, 0x7FF, 0xFFF, 0x1FFF
};
unsigned char _u2a[128] = {
	/* u- to A-law conversions */
	1,1,2,2,3,3,4,4,
	5,5,6,6,7,7,8,8,
	9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,
	25,27,29,31,33,34,35,36,
	37,38,39,40,41,42,43,44,
	46,48,49,50,51,52,53,54,
	55,56,57,58,59,60,61,62,
	64,65,66,67,68,69,70,71,
	72,73,74,75,76,77,78,79,
	81,82,83,84,85,86,87,88, 
	89,90,91,92,93,94,95,96,
	97,98,99,100,101,102,103,104,
	105,106,107,108,109,110,111,112,
	113,114,115,116,117,118,119,120,
	121,122,123,124,125,126,127,128
};
unsigned char _a2u[128] = {
	/* A- to u-law conversions */
	1,3,5,7,9,11,13,15,
	16,17,18,19,20,21,22,23,
	24,25,26,27,28,29,30,31,
	32,32,33,33,34,34,35,35,
	36,37,38,39,40,41,42,43,
	44,45,46,47,48,48,49,49,
	50,51,52,53,54,55,56,57,
	58,59,60,61,62,63,64,64,
	65,66,67,68,69,70,71,72,
	73,74,75,76,77,78,79,79,
	80,81,82,83,84,85,86,87,
	88,89,90,91,92,93,94,95,
	96,97,98,99,100,101,102,103,
	104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,
	120,121,122,123,124,125,126,127
};

static short search(int val, short *table, int size)
{
	int i;
	for (i = 0; i < size; i++) 
	{
		if (val <= *table++)
			return (i);
	}
	return (size);
}
/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *Linear Input CodeCompressed Code
 *---------------------------------------
 *0000000wxyza000wxyz
 *0000001wxyza001wxyz
 *000001wxyzab010wxyz
 *00001wxyzabc011wxyz
 *0001wxyzabcd100wxyz
 *001wxyzabcde101wxyz
 *01wxyzabcdef110wxyz
 *1wxyzabcdefg111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static unsigned char linear2alaw(int pcm_val)/* 2's complement (16-bit range) */
{
	int mask;
	int seg;
	unsigned char aval;
 
	pcm_val = pcm_val >> 3;
	if (pcm_val >= 0) 
	{
		mask = 0xD5;/* sign (7th) bit = 1 */
	} 
	else 
	{
		mask = 0x55;/* sign bit = 0 */
		pcm_val = -pcm_val - 1;
	}
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_aend, 8);
	/* Combine the sign, segment, and quantization bits. */
	if (seg >= 8)/* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else 
	{
		aval = (unsigned char) seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 1) & QUANT_MASK;
		else
			aval |= (pcm_val >> seg) & QUANT_MASK;
		return (aval ^ mask);
	}
}

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static int alaw2linear(unsigned char a_val)
{
	int t;
	int seg;
 
	a_val ^= 0x55;
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}
/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *Biased Linear Input CodeCompressed Code
 *---------------------------------------
 *00000001wxyza000wxyz
 *0000001wxyzab001wxyz
 *000001wxyzabc010wxyz
 *00001wxyzabcd011wxyz
 *0001wxyzabcde100wxyz
 *001wxyzabcdef101wxyz
 *01wxyzabcdefg110wxyz
 *1wxyzabcdefgh111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static unsigned char linear2ulaw(short pcm_val)/* 2's complement (16-bit range) */
{
	short mask;
	short seg;
	unsigned char uval;
 
	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0) 
	{
		pcm_val = -pcm_val;
		mask = 0x7F;
	} 
	else 
	{
		mask = 0xFF;
	}
    if (pcm_val > CLIP)
		pcm_val = CLIP;/* clip the magnitude */
	pcm_val += (BIAS >> 2);
 
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_uend, 8);
 
	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)/* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else 
	{
		uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}
/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static short ulaw2linear(unsigned char u_val)
{
	short t;
 
	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;
 
	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}
/* A-law to u-law conversion */
static unsigned char alaw2ulaw(unsigned char aval)
{
	aval &= 0xff;
	return (unsigned char) ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
	    (0x7F ^ _a2u[aval ^ 0x55]));
}
 
/* u-law to A-law conversion */
static unsigned char ulaw2alaw(unsigned char uval)
{
	uval &= 0xff;
	return (unsigned char) ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) :
	    (unsigned char) (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}

static int encode(char *a_psrc, char *a_pdst, int in_data_len, unsigned char type)
{
	int i;
	short *psrc = (short *)a_psrc;
	int out_data_len = in_data_len / sizeof(short);
 
	if (a_psrc == NULL || a_pdst == NULL) 
	{
		return (-1);
	}
 
	if (in_data_len <= 0) 
	{
		return (-1);
	}
 
	if (type == G711_A_LAW) 
	{
		for (i = 0; i < out_data_len; i++) 
		{
			a_pdst[i] = (char)linear2alaw(psrc[i]);
		}
	} 
	else 
	{
		for (i = 0; i < out_data_len; i++) 
		{
			a_pdst[i] = (char)linear2ulaw(psrc[i]);
		}
	}
	return (i);
}
 
static int decode(char *a_psrc, char *a_pdst, int in_data_len, unsigned char type)
{
	int i;
	short *pdst = (short *)a_pdst;
	int out_data_len = in_data_len / sizeof(char);
 
	if (a_psrc == NULL || a_pdst == NULL) 
	{
		return (-1);
	}
 
	if (type == G711_A_LAW) 
	{
		for (i = 0; i < out_data_len; i++) 
		{
			pdst[i] = (short)alaw2linear((unsigned char)a_psrc[i]);
		}
	} 
	else 
	{
		for (i = 0; i < out_data_len; i++) 
		{
			pdst[i] = (short)ulaw2linear((unsigned char)a_psrc[i]);
		}
	}
	return (i * sizeof(short));
}


/*****************************************************************************
-Fuction		: CodecG711U
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecG711U::CodecG711U()
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
CodecG711U::~CodecG711U()
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
int CodecG711U::Init()
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
int CodecG711U::InitEncode(T_AudioCodecParam i_tSrcCodecParam)
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
int CodecG711U::Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet=-1;
	int i=0,iDstLen=0;
	unsigned short * pwSrcBuf = (unsigned short *)i_abSrcBuf;//只支持16位的采样位数，如需支持其他位数则需修改代码保留最高16(13)位
	
    if(NULL == i_abSrcBuf || 0 == i_iSrcBufLen  || NULL == o_abDstBuf || i_iDstBufMaxLen < i_iSrcBufLen / 2)
    {
        AC_LOGE("CodecG711U Encode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    //encode(pcm_buf, g711_buf, sizeof(pcm_buf), G711_A_LAW));
    return encode((char *)i_abSrcBuf, (char *)o_abDstBuf,i_iSrcBufLen, G711_MU_LAW);
}

/*****************************************************************************
-Fuction		: CodecG711U::InitEncode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711U::InitDecode()
{

	return 0;
}

/*****************************************************************************
-Fuction		: CodecG711U::Decode
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecG711U::Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam)
{
    int iRet=-1;
	int i=0,iDstLen=0;
	unsigned short * pwDstBuf = (unsigned short*)o_abDstBuf;//解码出采样位数为16(13)位的pcm

    if(NULL == i_abSrcBuf || NULL == o_abDstBuf)
    {
        AC_LOGE("CodecG711U Decode err %d,%d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
        return iRet;
    }
    //decode(g711_buf, pcm_buf2, sizeof(g711_buf), G711_A_LAW));
    iDstLen=decode((char *)i_abSrcBuf, (char *)o_abDstBuf, i_iSrcBufLen, G711_MU_LAW);

    o_ptCodecParam->eAudioCodecType=AUDIO_CODEC_TYPE_PCM;
    o_ptCodecParam->dwBitsPerSample=sizeof(unsigned short)*8;
    //o_ptCodecParam->dwChannels=tWavHeader.wNumChannels;
    //o_ptCodecParam->dwSampleRate=tWavHeader.dwSampleRate;
	
    iRet=iDstLen;
	return iRet;
}







