/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerSession.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef HTTP_SERVER_SESSION_H
#define HTTP_SERVER_SESSION_H

#include "HttpServer.h"
#include "HttpTestCase.h"


/*****************************************************************************
-Class          : HttpServer
-Description    : HttpServer
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/11      V1.0.0              Yu Weifeng       Created
******************************************************************************/
class HttpServerSession : public HttpServer
{
public:
	HttpServerSession();
	virtual ~HttpServerSession();
    int HandleHttpReq(const char * i_strReq,char *o_strRes,int i_iResMaxLen);//return ResLen,<0 err
private:
    int HandleReq(char * i_strURL,char * i_strBody,char **o_ppRes);
    int Regex(const char *i_strPattern,char *i_strBuf,string * o_aMatch,int i_iMatchMaxCnt);
    
    HttpTestCase m_Test;
};













#endif
