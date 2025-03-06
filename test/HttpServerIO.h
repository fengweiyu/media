/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerIO.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef HTTP_SERVER_IO_H
#define HTTP_SERVER_IO_H

#include "TcpSocket.h"
#include "HttpServerSession.h"
#include <thread>
#include <mutex>

using std::thread;
using std::mutex;

/*****************************************************************************
-Class			: HttpServerIO
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class HttpServerIO : TcpServer
{
public:
	HttpServerIO(int i_iClientSocketFd);
	virtual ~HttpServerIO();
    int Proc();
    int GetProcFlag();
private:
	int m_iClientSocketFd;
	
    HttpServerSession m_HttpServerSession;

    thread * m_pHttpServerIOProc;
	int m_iHttpServerIOFlag;
};

#endif
