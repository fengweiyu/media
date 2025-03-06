/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpTestCase.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "HttpTestCase.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <thread>

#include "MediaConvert.h"




using std::thread;


/*****************************************************************************
-Fuction		: HttpTestCase
-Description	: HttpTestCase
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpTestCase :: HttpTestCase()
{

}

/*****************************************************************************
-Fuction		: ~HttpTestCase
-Description	: ~HttpTestCase
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpTestCase :: ~HttpTestCase()
{
}
/*****************************************************************************
-Fuction        : proc
-Description    : proc
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int HttpTestCase :: Test(const char * i_strSrcFilePath,const char *i_strDstFilePath)
{
    int iRet = -1;

    if(NULL == i_strSrcFilePath || NULL == i_strDstFilePath)
    {
        TEST_LOGE("Test NULL \r\n");
        return iRet;
    }
    if(Proc(i_strSrcFilePath,i_strDstFilePath)<=0)
    {
        return iRet;
    }
    return 0;
}
/*****************************************************************************
-Fuction        : ReadFile
-Description    : ReadFile
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int HttpTestCase :: ReadFile(const char * i_strSrcFilePath,unsigned char **o_ppBuffer) 
{  
    FILE *pFile;  
    int iBytesRead;  
    int iFileSize;  
    unsigned char *pBuffer=NULL;
    int iRet = -1;

    
    // 打开二进制文件  
    pFile= fopen(i_strSrcFilePath, "rb");  
    if (pFile == NULL) 
    {  
        printf("ReadFile err");  
        return iRet;  
    }  

    do
    {
        // 获取文件大小  
        fseek(pFile, 0, SEEK_END);  
        iFileSize = ftell(pFile);  
        fseek(pFile, 0, SEEK_SET); // 重置文件指针到开头  
        
        // 分配内存以存储文件内容  
        pBuffer = new unsigned char [iFileSize];
        if (pBuffer == NULL) 
        {  
            printf("malloc err %d",iFileSize);  
            break;  
        }  
        
        // 读取文件内容  
        iBytesRead = fread(pBuffer, sizeof(unsigned char), iFileSize, pFile);  
        if (iBytesRead != iFileSize && !feof(pFile)) 
        {  
            printf("fread err %d",iFileSize);  
            break;  
        }  
        iRet = iBytesRead;
    }while(0);
    fclose(pFile);  
    if (iRet<= 0 && pBuffer != NULL) 
    {  
        delete [] pBuffer;//free(pBuffer); 
        return iRet;  
    }  
    *o_ppBuffer=pBuffer;
    return iRet;  
} 

/*****************************************************************************
-Fuction        : proc
-Description    : proc
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int HttpTestCase :: Proc(const char * i_strSrcFilePath,const char *i_strDstFilePath)
{
    unsigned char * pbSrcFileBuf=NULL;
    unsigned char * pbFileBuf=NULL;
	int iRet = -1,iReadLen = -1,iWriteLen=0;
	int iMaxLen=0;
    FILE *pDstFile=NULL;  


    iReadLen=ReadFile(i_strSrcFilePath,&pbSrcFileBuf);
    if(iReadLen <= 0)
    {
        TEST_LOGE("ReadFile err %s\r\n",i_strSrcFilePath);
        return iRet;
    } 
    do
    {
        iRet=InputData(pbSrcFileBuf,iReadLen,i_strSrcFilePath,i_strDstFilePath);
        if(iRet <= 0)
        {
            TEST_LOGE("InputData err %s %s\r\n",i_strSrcFilePath,i_strDstFilePath);
            //break;
        } 
        iMaxLen=iReadLen+(10*1024*1024) ;
        pbFileBuf = new unsigned char [iMaxLen];
        if(NULL == pbFileBuf)
        {
            TEST_LOGE("NULL == pbFileBuf err\r\n");
            break;
        } 
        iWriteLen=0;
        iRet=0;
        do
        {
            iWriteLen+=iRet;
            iRet=GetData(pbFileBuf+iWriteLen,iMaxLen-iWriteLen);
        } while(iRet>0);

        
        pDstFile = fopen(i_strDstFilePath,"wb");//
        if(NULL == pDstFile)
        {
            TEST_LOGE("fopen %s err\r\n",i_strDstFilePath);
            break;
        } 
        iRet = fwrite(pbFileBuf,1,iWriteLen, pDstFile);
        if(iRet != iWriteLen)
        {
            printf("fwrite err %d iWriteLen%d\r\n",iRet,iWriteLen);
            break;
        }
    }while(0);
    
    if(NULL != pbFileBuf)
    {
        delete [] pbFileBuf;
    } 
    if(NULL != pbSrcFileBuf)
    {
        delete [] pbSrcFileBuf;
    } 
    if(NULL != pDstFile)
    {
        fclose(pDstFile);//fseek(m_pMediaFile,0,SEEK_SET); 
    } 
    return iWriteLen;
}

