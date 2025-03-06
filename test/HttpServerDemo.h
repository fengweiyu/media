/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerDemo.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef HTTP_SERVER_DEMO_H
#define HTTP_SERVER_DEMO_H

#include <mutex>
#include <string>
#include <list>
#include <map>
#include "HttpServerIO.h"

using std::map;
using std::string;
using std::list;
using std::mutex;

/*****************************************************************************
-Class			: HttpServerDemo
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class HttpServerDemo : public TcpServer
{
public:
	HttpServerDemo(int i_iServerPort);
	virtual ~HttpServerDemo();
    int Proc();
    
private:
    int CheckMapServerIO();
    int AddMapServerIO(HttpServerIO * i_pHttpServerIO,int i_iClientSocketFd);
    
    map<int, HttpServerIO *>  m_HttpServerIOMap;
    mutex m_MapMtx;
};

#endif
