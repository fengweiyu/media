/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecPCM.cpp
* Description		: 	CodecPCM operation center
                        ����ѹ�������ݴ�����Ҫ���ز���
����Դ���ݺ�Ŀ�����ݲ����ʣ�����λ����ͨ����һ�µ�����
1.�����ʲ�һ�£�
    Ҫ������(��ֵ�㷨,�����Բ�ֵ��������ֵ������Ҷ�任��)
    Ҫ������(�����㷨,��ͨ�˲�����������˲�)
2.����λ����һ�£�
    һ�����������֧�ֲ�ͬ�Ĳ���λ����������ﴦ��
    ���ڸ�λ��ת��λ����������λ���˵�λ
        (�� 24 λ����ת��Ϊ 16 λ������ͨ��������ֱ�ӱ����� 16 λ����)
    ���ڵ�λ��ת��λ������ͨ���򵥵���������չ��Ƶ������ֵ��ʵ��
        (��ÿ�� 16 λ�����ĸ� 16 λ��չΪ 24 λ������ͨ���򵥵����ƺ������)
3.ͨ����һ�£�
    ����������������
        ���Խ���������Ƶ���������Ƶ����������У�����һ����ͬ�������������
    ����������������
        ��������������������ϳ�һ��������ͨ��ͨ����������������ƽ��ֵ��ʵ�֡�
    ����������������
        ����и�������������� 5.1 �������������ѡ���ض����������л�ϻ�ֱ��ѡ����������������Ϊ��������
    ����������������
        �����Ҫ��չ���������������������ͨ������������ʹ��Ч������ģ������������������
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>

#include "CodecPCM.h"



/*****************************************************************************
-Fuction		: CodecPCM
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecPCM::CodecPCM()
{
    m_pbTransBuf=NULL;
    memset(&m_tSrcCodecParam,0,sizeof(T_AudioCodecParam));
    memset(&m_tDstCodecParam,0,sizeof(T_AudioCodecParam));
}
/*****************************************************************************
-Fuction		: ~CodecPCM
-Description	: ~CodecPCM
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecPCM::~CodecPCM()
{
    if(NULL != m_pbTransBuf)
        delete m_pbTransBuf;
}

/*****************************************************************************
-Fuction		: CodecPCM::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::Init(T_AudioCodecParam i_tSrcCodecParam,T_AudioCodecParam i_tDstCodecParam)
{
    memcpy(&m_tSrcCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    memcpy(&m_tDstCodecParam,&i_tDstCodecParam,sizeof(T_AudioCodecParam));
    AC_LOGW("CodecPCM dwSampleRate %d %d ,dwChannels %d %d ,dwBitsPerSample %d %d\r\n",m_tSrcCodecParam.dwSampleRate,m_tDstCodecParam.dwSampleRate,
    m_tSrcCodecParam.dwChannels,m_tDstCodecParam.dwChannels,m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
	return 0;
}

/*****************************************************************************
-Fuction		: TransSampleRate
-Description	: 
-Input			: Դ��������i_abBuf �ͳ���i_iBufLen
-Output 		: ת�������������i_abBuf
-Return 		: ת�������������
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::TransSampleRate(unsigned char * i_abBuf,int i_iBufLen,int i_iBufMaxLen)
{
    int iRet = -1;
    
    if(NULL == i_abBuf || i_iBufLen<=0)
    {
        AC_LOGE("TransSampleRate err NULL %d\r\n",i_iBufLen);
        return iRet;
    }
    if(m_tSrcCodecParam.dwSampleRate==m_tDstCodecParam.dwSampleRate)
    {
        return i_iBufLen;
    }
    if(NULL == m_pbTransBuf)
    {
        m_pbTransBuf = new Buffer(i_iBufLen);
        m_pbTransBuf->Copy(i_abBuf,i_iBufLen);
    }
    else
    {
        m_pbTransBuf->Copy(i_abBuf,i_iBufLen);
    }
    if(m_tSrcCodecParam.dwSampleRate>m_tDstCodecParam.dwSampleRate)
    {
        iRet=DownSampleRate(m_pbTransBuf->pbBuf,m_pbTransBuf->iBufLen,i_abBuf,i_iBufMaxLen);
        m_pbTransBuf->Delete(m_pbTransBuf->iBufLen);
        return iRet;
    }
    if(m_tSrcCodecParam.dwSampleRate<m_tDstCodecParam.dwSampleRate)
    {
        iRet=UpSampleRate(m_pbTransBuf->pbBuf,m_pbTransBuf->iBufLen,i_abBuf,i_iBufMaxLen);
        m_pbTransBuf->Delete(m_pbTransBuf->iBufLen);
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: UpSampleRate
-Description	: ������
���Բ�ֵ��
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::UpSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet = -1;
    short * pwSrcData=NULL;
    short * pwDstData=NULL;
    int i=0,j=0;
    int iDiffSampleRate=0;
    int iDstSampleCnt=0;
    int iCurSampleCnt=0;

    if(m_tSrcCodecParam.dwBitsPerSample!=m_tDstCodecParam.dwBitsPerSample)
    {//�ݲ�֧�ֲ���λ��ת��
        AC_LOGE("UpSampleRate.dwBitsPerSample %d!=m_tDstCodecParam.dwBitsPerSample %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    if(m_tSrcCodecParam.dwBitsPerSample!=16)//��ʱֻ֧��pcm16�����ݴ���
    {
        AC_LOGE("UpSampleRate dwBitsPerSample err %d %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    iDstSampleCnt=(float)i_iSrcBufLen*m_tDstCodecParam.dwSampleRate/(sizeof(short)*m_tSrcCodecParam.dwSampleRate);//�Ż������
    iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;
    pwSrcData=(short *)i_abSrcBuf;
    pwDstData=(short *)o_abDstBuf;
    j=0;
    for (i = 0; i < i_iSrcBufLen/sizeof(short); i ++) // ����ÿ������������Ӧ���������Ƶ������  
    {  
        if(i*iDiffSampleRate*sizeof(short)>i_iDstBufMaxLen)
        {
            AC_LOGE("UpSampleRate err DstBufMaxLen %d ,%d %d\r\n",i_iSrcBufLen,i*iDiffSampleRate*sizeof(short),i_iDstBufMaxLen);
            return iRet;
        }
        pwDstData[iCurSampleCnt] = pwSrcData[i]; // ��ÿ�������������Ƶ��������Ӧλ�ã�ÿ��5������һ��  ,����ŵ�һ��
        // �������Բ�ֵ�ļ�ʵ��  
        if (i < i_iSrcBufLen/sizeof(short) - 1) //������ڼ�������������������ʹ�ü��������һ������ֵͳһ��ֵ���߲�������0������
        {  
            for (j = 1; j < iDiffSampleRate; j++) 
            { //��һ����������������
                pwDstData[iCurSampleCnt + j] = (pwSrcData[i] * (iDiffSampleRate - j) + pwSrcData[i + 1] * j) / iDiffSampleRate;//�����һ����ǰһ�����֮�ļӺ�һ�����֮һ  
            } //��һ������ǰһ���Ӻ�һ��ֵ��ǰһ����ֵ�İٷ�֮��ʮ���ڶ����ǰٷ�֮��ʮ��ʵ�����Բ�ֵ
        }
        else //i = i_iSrcBufLen/2 - 1 ,���һ��Դ����
        {
            for (j = 1; j < iDiffSampleRate; j++) 
            { 
                pwDstData[iCurSampleCnt + j] = pwSrcData[i];//���һ������ֵͳһ��ֵ���߲�������0������
            } 
        }
        iCurSampleCnt+=iDiffSampleRate;
        
        //��̬�����㷨
        iDstSampleCnt=(float)(i+1)*m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//����Ŀ�������ͬ��ʱ���Ӧ��������(Ŀ��������)
        if(iCurSampleCnt>iDstSampleCnt)//
        {//���������ˣ���ɼ���Щ
            iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//iDiffSampleRate--;//���ڼ����������ᵼ�²�����϶����
        }
        else if(iCurSampleCnt<iDstSampleCnt)
        {//���������ˣ���ɼ���Щ
            iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate+1;//iDiffSampleRate++;//���ڼ����������ᵼ�²�����϶����
        }
        //AC_LOGD("%d ",iCurSampleCnt);
    }  
    iRet=iCurSampleCnt*sizeof(short);
    return iRet;
}

/*****************************************************************************
-Fuction		: DownSampleRate
-Description	: ������
����������
�˲���Ӧ�õ�ͨ�˲�����
ȡ������ÿ�� 5.51 ��������ѡ��һ��������44.1kHz �� 8kHz �ı�����44100/8000 = 5.5125��
����ȡ��Ϊÿ 5 ������ȡһ��
(�ɼ�������࣬�ᵼ�²���ʱ��䳤��ʱ���Ӧ�û᲻׼
������ͳһ�������ᵼ�¶����ݣ����Լ��һ��ʱ�䶪����������100ms ,
������ʱ����Ҫ���Եõ�
��׼ȷ�ķ����ǣ����Դ������Ӧ��ʱ��õ�Ŀ������������ǰ���������������
����Ŀ�������������õ����������������������ٶ��������ͻ�׼һЩ(1.1���Ѵ�����ǰ����չ���ľ���û����)��
����Ҫһ��������ʱ�����������)
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::DownSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet = -1;
    short * pwSrcData=NULL;
    short * pwDstData=NULL;
    int i=0,j=0;
    int iDiffSampleRate=0;
    float fSrcSampleTime=0;
    int iDstSampleCnt=0;

    if(m_tSrcCodecParam.dwBitsPerSample!=m_tDstCodecParam.dwBitsPerSample)
    {//�ݲ�֧�ֲ���λ��ת��
        AC_LOGE("m_tSrcCodecParam.dwBitsPerSample %d!=m_tDstCodecParam.dwBitsPerSample %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    if(m_tSrcCodecParam.dwBitsPerSample!=16)//��ʱֻ֧��pcm16�����ݴ���
    {
        AC_LOGE("dwBitsPerSample err %d %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    fSrcSampleTime=(float)i_iSrcBufLen/sizeof(short)*1000/m_tSrcCodecParam.dwSampleRate;//�����������Բ����ʾ���ʱ�䣬������ms��λ
    iDstSampleCnt=fSrcSampleTime*m_tDstCodecParam.dwSampleRate/1000;//����ʱ����Բ����ʾ��ǲ�������(������)
    iDstSampleCnt=(float)i_iSrcBufLen*m_tDstCodecParam.dwSampleRate/(sizeof(short)*m_tSrcCodecParam.dwSampleRate);//�Ż������
    
    iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate;
    pwSrcData=(short *)i_abSrcBuf;
    pwDstData=(short *)o_abDstBuf;
    j=0;
    for (i = 0; i < i_iSrcBufLen/sizeof(short); i += iDiffSampleRate) 
    {  
        if(j*sizeof(short)>i_iDstBufMaxLen)
        {
            AC_LOGE("DownSampleRate err DstBufMaxLen %d %d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
        pwDstData[j] = pwSrcData[i]; // ÿ5������ȡһ��  
        j++;

        //��̬�����㷨
        iDstSampleCnt=(float)i*m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//����Ŀ�������ͬ��ʱ���Ӧ��������(Ŀ��������)
        if(j>iDstSampleCnt)//
        {//���������ˣ���ɼ���Щ
            iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate+1;//iDiffSampleRate++;//���ڼ����������ᵼ�²�����϶����
        }
        else if(j<iDstSampleCnt)
        {//���������ˣ���ɼ���Щ
            iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate;//iDiffSampleRate--;//���ڼ����������ᵼ�²�����϶����
        }
        //AC_LOGD("%d ",i);
    }  
    iRet=j*sizeof(short);
    return iRet;
}

/*
// ����������  
//ȡ������ÿ�� 551 ��������ѡ��һ��������44.1kHz �� 8kHz �ı�����44100/8000 = 5.5125��
//����ȡ��Ϊÿ 551 ������ȡһ��
void downsample(int16_t *input, int16_t *output) {  
    for (int i = 0, j = 0; j < OUTPUT_SAMPLES; i += 551, j++) {  
        output[j] = input[i]; // ÿ551������ȡһ��  
    }  
} 
#define INPUT_SAMPLES 8000  // ���� 8kHz��������������1�룩  
#define OUTPUT_SAMPLES 44100 // ��Ӧ 44.1kHz�����������  

void upsample(int16_t *input, int16_t *output) 
{  
    // ����ÿ������������Ӧ���������Ƶ������  
    for (int i = 0; i < INPUT_SAMPLES; i++) 
    {  
        output[i * 5] = input[i]; // ��ÿ�������������Ƶ��������Ӧλ�ã�ÿ��5������һ��  
        // �������Բ�ֵ�ļ�ʵ��  
        if (i < INPUT_SAMPLES - 1) //������ڼ�������������������ʹ�ü��������һ������ֵͳһ��ֵ���߲�������0������
        {  
            for (int j = 1; j < 5; j++) {  
                output[i * 5 + j] = (input[i] * (5 - j) + input[i + 1] * j) / 5;//�����һ����ǰһ�����֮�ļӺ�һ�����֮һ  
            }  //��һ������ǰһ���Ӻ�һ��ֵ��ǰһ����ֵ�İٷ�֮��ʮ���ڶ����ǰٷ�֮��ʮ��ʵ�����Բ�ֵ
        }  
    }  
}
*/
