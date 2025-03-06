/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerSession.cpp
* Description           : 	    Ϊ��Ӧ�Զ����ӣ�server��Ƴɳ�פ����
ע�⣬��Ա����Ҫ�����߳̾�������
* Created               :       2022.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "HttpServerSession.h"
#include <regex>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

using std::string;
using std::smatch;
using std::regex;


#define HTTP_CLIENT_VLC         "VLC"
#define HTTP_CLIENT_FFPLAY      "Lavf"
#define HTTP_CLIENT_CHROME      "Chrome"

#define HTTP_MAX_MATCH_NUM       8



#define HTTP_UN_SUPPORT_FUN (-2)
#ifndef _WIN32
#include <regex.h> //C++ regexҪ��gcc 4.9���ϰ汾������linux������c��
/*****************************************************************************
-Fuction		: Regex
-Description	: ������ʽ
.��				ƥ�����\r\n��֮����κε����ַ�
*				ƥ��ǰ����ӱ��ʽ����Ρ����磬zo*��ƥ�䡰z����Ҳ��ƥ�䡰zo���Լ���zoo����*�ȼ���o{0,}
				����.*��ƥ��������洢�����������
(pattern)		ƥ��ģʽ��pattern����ȡ��һƥ�䡣����ȡ��ƥ����ԴӲ�����Matches���ϵõ�
[xyz]			�ַ����ϡ�ƥ��������������һ���ַ������磬��[abc]������ƥ�䡰plain���еġ�a����
+				ƥ��ǰ����ӱ��ʽһ�λ���(���ڵ���1�Σ������磬��zo+����ƥ�䡰zo���Լ���zoo����������ƥ�䡰z����+�ȼ���{1,}��
				//���������в���+��Ĭ����һ�Σ���ֻ��ƥ�䵽һ������6
				
[A-Za-z0-9] 	26����д��ĸ��26��Сд��ĸ��0��9����
[A-Za-z0-9+/=]	26����д��ĸ��26��Сд��ĸ0��9�����Լ�+/= �����ַ�


-Input			: i_strPattern ģʽ��,i_strBuf��ƥ���ַ���,
-Output 		: o_ptMatch �洢ƥ�䴮λ�õ�����,���ڴ洢ƥ�����ڴ�ƥ�䴮�е��±귶Χ
//����0��Ԫ�����������ʽƥ������λ��,�������������������ƥ��������ߵĵ�Ԫ���δ����������ʽƥ������λ��
-Return 		: -1 err,other cnt
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/11/01	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CRegex(const char *i_strPattern,char *i_strBuf,regmatch_t *o_ptMatch,int i_iMatchMaxNum)
{
    char acErrBuf[256];
    int iRet=-1;
    regex_t tReg;    //����һ������ʵ��
    //const size_t dwMatch = 6;    //����ƥ�������������       //��ʾ������ƥ��


    //REG_ICASE ƥ����ĸʱ���Դ�Сд��
    iRet =regcomp(&tReg, i_strPattern, REG_EXTENDED);    //��������ģʽ��
    if(iRet != 0) 
    {
        regerror(iRet, &tReg, acErrBuf, sizeof(acErrBuf));
        TEST_LOGE("Regex Error:\r\n");
        return -1;
    }
    
    iRet = regexec(&tReg, i_strBuf, i_iMatchMaxNum, o_ptMatch, 0); //ƥ����
    if (iRet == REG_NOMATCH)
    { //���ûƥ����
        TEST_LOGE("Regex No Match!\r\n");
        iRet = 0;
    }
    else if (iRet == REG_NOERROR)
    { //���ƥ������
        TEST_LOGD("Match\r\n");
        int i=0,j=0;
        for(i=0;i<i_iMatchMaxNum && o_ptMatch[i].rm_so != -1;i++)
        {
            for (j= o_ptMatch[i].rm_so; j < o_ptMatch[i].rm_eo;j++)
            { //�������ƥ�䷶Χ���ַ���
                //printf("%c", i_strBuf[j]);
            }
            //printf("\n");
        }
        iRet = i;
    }
    else
    {
        TEST_LOGE("Regex Unknow err%d:\r\n",iRet);
        iRet = -1;
    }
    regfree(&tReg);  //�ͷ�������ʽ
    
    return iRet;
}
/*****************************************************************************
-Fuction		: HttpParseReqHeader
-Description	: Send
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int LinuxCRegex(const char *i_strPattern,char *i_strBuf,string * o_aMatch,int i_iMatchMaxCnt)
{
    int iRet = -1;
	string strBuf;
	regmatch_t atMatch[HTTP_MAX_MATCH_NUM];
	
    if(NULL == i_strPattern || NULL == i_strBuf || NULL == o_aMatch || i_iMatchMaxCnt <= 0 || i_iMatchMaxCnt <= 0)
    {
        TEST_LOGE("LinuxCRegex NULL \r\n");
        return iRet;
    }
    strBuf.assign(i_strBuf);
    
    memset(atMatch,0,sizeof(atMatch));
    iRet= CRegex(i_strPattern,i_strBuf,atMatch,HTTP_MAX_MATCH_NUM);
    if(iRet <= 0 || i_iMatchMaxCnt < iRet)//ȥ������������-1
    {
        TEST_LOGE("LinuxCRegex %d,%d, iRet <= 0 || i_iMatchMaxCnt < iRet err \r\n",i_iMatchMaxCnt,iRet);
        return iRet;
    }
    // �������ƥ�䵽���Ӵ�
    for (int i = 0; i < iRet; ++i) 
    {
        o_aMatch[i].assign(strBuf,atMatch[i].rm_so,atMatch[i].rm_eo-atMatch[i].rm_so);//0������
    }
    return iRet;
} 
#else
/*****************************************************************************
-Fuction		: HttpParseReqHeader
-Description	: Send
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int LinuxCRegex(const char *i_strPattern,char *i_strBuf,string * o_aMatch,int i_iMatchMaxCnt)
{
    return HTTP_UN_SUPPORT_FUN;
} 
#endif







#define HTTP_MAX_LEN       (1*1024*1024)

/*****************************************************************************
-Fuction        : HlsServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/13      V1.0.0              Yu Weifeng       Created
******************************************************************************/
HttpServerSession::HttpServerSession()
{
}
/*****************************************************************************
-Fuction        : ~HlsServer
-Description    : ~HlsServer
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/13      V1.0.0              Yu Weifeng       Created
******************************************************************************/
HttpServerSession::~HttpServerSession()
{
}
/*****************************************************************************
-Fuction        : HandleHttpReq
-Description    : //return ResLen,<0 err
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/13      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int HttpServerSession::HandleHttpReq(const char * i_strReq,char *o_strRes,int i_iResMaxLen)
{
    int iRet = -1;
    T_HttpReqPacket tHttpReqPacket;
    char * pRes=NULL;
    int iResLen = -1;

    if(NULL == i_strReq || NULL == o_strRes|| i_iResMaxLen <= 0)
    {
        TEST_LOGW("HandleHttpReq NULL \r\n");
        return iRet;
    }
    memset(&tHttpReqPacket,0,sizeof(T_HttpReqPacket));
    TEST_LOGW("HandleHttpReq %s\r\n",i_strReq);
    iRet=HttpServer::ParseRequest((char *)i_strReq,strlen(i_strReq),&tHttpReqPacket);
    if(iRet < 0)
    {
        TEST_LOGE("HttpServer::ParseRequest err%d\r\n",iRet);
        return iRet;
    }
    iRet = -1;
    if(0 == strcmp(tHttpReqPacket.strMethod,HTTP_METHOD_OPTIONS))
    {
        TEST_LOGW("HandleHttpReq HTTP_METHOD_OPTIONS\r\n");
        HttpServer *pHttpServer=new HttpServer();
        iRet=pHttpServer->CreateResponse();
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Method", "POST, GET, OPTIONS, DELETE, PUT");
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Max-Age", "600");
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Headers", "access-control-allow-headers,accessol-allow-origin,content-type");//����������������
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Origin","*");
        iRet|=pHttpServer->SetResHeaderValue("Connection", "Keep-Alive");
        iRet=pHttpServer->FormatResToStream(NULL,0,o_strRes,i_iResMaxLen);
        delete pHttpServer;
        return iRet;
    }
    if(0 == strcmp(tHttpReqPacket.strMethod,HTTP_METHOD_GET) || 0 == strcmp(tHttpReqPacket.strMethod,HTTP_METHOD_POST))
    {
        iRet=HandleReq(tHttpReqPacket.strURL,tHttpReqPacket.pcBody,&pRes);
    }
    else
    {
        TEST_LOGE("unsupport HTTP_METHOD_ %s,url %s\r\n",tHttpReqPacket.strMethod,tHttpReqPacket.strURL);
    }
    if(iRet < 0)
    {
        TEST_LOGE("HandleReq err \r\n");
        HttpServer *pHttpServer=new HttpServer();
        iRet=pHttpServer->CreateResponse(400,"bad request");
        iRet|=pHttpServer->SetResHeaderValue("Connection", "Keep-Alive");
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Origin", "*");
        iRet=pHttpServer->FormatResToStream(NULL,0,o_strRes,i_iResMaxLen);
        delete pHttpServer;
    }
    else if(iRet == 0)
    {
        HttpServer *pHttpServer=new HttpServer();
        iRet=pHttpServer->CreateResponse();
        iRet|=pHttpServer->SetResHeaderValue("Connection", "Keep-Alive");
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Origin", "*");
        iRet=pHttpServer->FormatResToStream(NULL,0,o_strRes,i_iResMaxLen);
        delete pHttpServer;
        return iRet;
        TEST_LOGD("HandleReq  OK : %s\r\n",o_strRes);
    }
    else
    {
        HttpServer *pHttpServer=new HttpServer(); 
        iResLen=iRet;
        iRet=pHttpServer->CreateResponse();
        //iRet=pHttpServer->CreateResponse(206,"Partial Content",HTTP_VERSION);
        iRet|=pHttpServer->SetResHeaderValue("Connection", "Keep-Alive");
        iRet|=pHttpServer->SetResHeaderValue("Content-Type", "application/json; charset=utf-8");//x-flv video/mp4
        iRet|=pHttpServer->SetResHeaderValue("Content-Length", iResLen);
        //snprintf(strRange,sizeof(strRange),"bytes 0-%d/%d",HTTP_MAX_LEN*100-1,HTTP_MAX_LEN*100);
        //iRet|=pHttpServer->SetResHeaderValue("Content-Range", (const char *)strRange);
        iRet|=pHttpServer->SetResHeaderValue("Access-Control-Allow-Origin", "*");
        iRet=pHttpServer->FormatResToStream(pRes,iResLen,o_strRes,i_iResMaxLen);
        delete pHttpServer;
        TEST_LOGD("HandleReq RETURN OK : %s\r\n",o_strRes);
    }
    if(!pRes)
    {
        delete [] pRes;
    }
    return iRet;
}
/*****************************************************************************
-Fuction        : HandleReqGet
-Description    : //return ResLen,<0 err
http://139.9.149.150:9150/test/media/convert
{
  "input": "test2.pri",
  "output": "test2.mp4",
  "serverPath": "/etc/nginx/html",
  "serverURL": "https://139.9.149.150:9123"
}
200 OK
{
    "input": "https://139.9.149.150:9123/test2.pri",
    "output": "https://139.9.149.150:9123/test2.mp4"
}

-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/13      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int HttpServerSession::HandleReq(char * i_strURL,char * i_strBody,char **o_ppRes)
{
    int iRet = -1;
    int iLen = -1;
    string strInFileName("");//
    string strOutFileName("");//
    char strRange[64];
    char strInFilePath[128];
    char strOutFilePath[128];
    string strServerPath(".");///etc/nginx/html/
    string strServerURL(".");//https://139.9.149.150:9123

    if(NULL == i_strURL || NULL == i_strBody || NULL == o_ppRes)
    {
        TEST_LOGE("HandleReqURL NULL \r\n");
        return iRet;
    }
    
    string astrRegex[HTTP_MAX_MATCH_NUM];
    const char * strPattern = "/([A-Za-z_]+)/([A-Za-z0-9_.]+)/([A-Za-z0-9_.]+)";//http://localhost:9214/test/media/convert
    iRet=this->Regex(strPattern,i_strURL,astrRegex,HTTP_MAX_MATCH_NUM);//
    if (iRet<=3) //0������
    {
        TEST_LOGE("HandleReqURL Regex err : %s \r\n",i_strURL);
        return iRet;
    } 
    string strStreamType(astrRegex[1].c_str());//test
    string strMedia(astrRegex[2].c_str());
    string strConvert(astrRegex[3].c_str());
    if(0 != strcmp(strMedia.c_str(),"media") ||0 != strcmp(strConvert.c_str(),"convert"))
    {
        TEST_LOGE("HandleReqURL url err : %s \r\n",i_strURL);
        return iRet;
    } 
    cJSON * ptBodyJson = NULL;
    cJSON * ptNode = NULL;
    ptBodyJson = cJSON_Parse(i_strBody);
    if(NULL != ptBodyJson)
    {
        ptNode = cJSON_GetObjectItem(ptBodyJson,"input");
        if(NULL != ptNode && NULL != ptNode->valuestring)
        {
            strInFileName.assign(ptNode->valuestring);
            ptNode = NULL;
        }
        ptNode = cJSON_GetObjectItem(ptBodyJson,"output");
        if(NULL != ptNode && NULL != ptNode->valuestring)
        {
            strOutFileName.assign(ptNode->valuestring);
            ptNode = NULL;
        }
        ptNode = cJSON_GetObjectItem(ptBodyJson,"serverPath");
        if(NULL != ptNode && NULL != ptNode->valuestring)
        {
            strServerPath.assign(ptNode->valuestring);
            ptNode = NULL;
        }
        ptNode = cJSON_GetObjectItem(ptBodyJson,"serverURL");
        if(NULL != ptNode && NULL != ptNode->valuestring)
        {
            strServerURL.assign(ptNode->valuestring);
            ptNode = NULL;
        }
        cJSON_Delete(ptBodyJson);
    }
    if(strInFileName.empty()||strOutFileName.empty())
    {
        TEST_LOGE("HandleReqURL body err : %s \r\n",i_strBody);
        return iRet;
    } 
    
    TEST_LOGW("%d,strStreamType %s,strInFileName %s,strOutFileName %s\r\n",iRet,strStreamType.c_str(),strInFileName.c_str(),strOutFileName.c_str());
    snprintf(strInFilePath,sizeof(strInFilePath),"%s/%s",strServerPath.c_str(),strInFileName.c_str());
    snprintf(strOutFilePath,sizeof(strOutFilePath),"%s/%s",strServerPath.c_str(),strOutFileName.c_str());
    iRet = m_Test.Test(strInFilePath,strOutFilePath);
    if(iRet<0)
        return iRet;
    snprintf(strInFilePath,sizeof(strInFilePath),"%s/%s",strServerURL.c_str(),strInFileName.c_str());
    snprintf(strOutFilePath,sizeof(strOutFilePath),"%s/%s",strServerURL.c_str(),strOutFileName.c_str());
    *o_ppRes = new char [HTTP_MAX_LEN];//�����Ż�
    cJSON * root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"input",strInFilePath);
    cJSON_AddStringToObject(root,"output",strOutFilePath);
    char * buf = cJSON_PrintUnformatted(root);
    if(buf)
    {
        iRet = snprintf(*o_ppRes,HTTP_MAX_LEN,"%s",buf);
        free(buf);
    }
    cJSON_Delete(root);
    return iRet;
}

/*****************************************************************************
-Fuction        : Regex
-Description    : 
-Input          : 
-Output         : 
-Return         : -1 err,>0 cnt
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int HttpServerSession::Regex(const char *i_strPattern,char *i_strBuf,string * o_aMatch,int i_iMatchMaxCnt)
{
    int iRet = -1;

    if(NULL == i_strPattern || NULL == i_strBuf || NULL == o_aMatch || i_iMatchMaxCnt <= 0)
    {
        TEST_LOGE("HttpServer Regex NULL \r\n");
        return iRet;
    }
    iRet = LinuxCRegex(i_strPattern,i_strBuf,o_aMatch,i_iMatchMaxCnt);
    if(HTTP_UN_SUPPORT_FUN!=iRet)
    {
        return iRet;
    }
    string strBuf(i_strBuf);
    regex Pattern(i_strPattern);//http://localhost:9212/file/H264AAC.mp4/test.m3u8
    smatch Match;
    if (std::regex_search(strBuf,Match,Pattern)) 
    {
        if((int)Match.size()<=0 || i_iMatchMaxCnt < (int)Match.size())
        {
            TEST_LOGE("HlsRegex err i_iMatchMaxCnt %d<%d Match.size()-1 \r\n",i_iMatchMaxCnt,Match.size()-1);
            return iRet;
        }
        iRet = (int)Match.size();
        // �������ƥ�䵽���Ӵ�
        for (int i = 0; i < iRet; ++i) 
        {
            o_aMatch[i].assign( Match[i].str());
        }
    } 
    return iRet;
}


