/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerIO.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "HttpServerIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <thread>

using std::thread;

#define HTTP_IO_RECV_MAX_LEN (10240)
#define HTTP_IO_SEND_MAX_LEN (3*1024*1024)

/*****************************************************************************
-Fuction		: HttpServerIO
-Description	: HttpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpServerIO :: HttpServerIO(int i_iClientSocketFd)
{
    m_iClientSocketFd=i_iClientSocketFd;
    
    m_iHttpServerIOFlag = 0;
    m_pHttpServerIOProc = new thread(&HttpServerIO::Proc, this);
    //m_pHttpSessionProc->detach();//注意线程回收
}

/*****************************************************************************
-Fuction		: ~HttpServerIO
-Description	: ~HttpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpServerIO :: ~HttpServerIO()
{
    if(NULL!= m_pHttpServerIOProc)
    {
        TEST_LOGW("~HttpServerIO start exit\r\n");
        m_iHttpServerIOFlag = 0;
        //while(0 == m_iExitProcFlag){usleep(10);};
        m_pHttpServerIOProc->join();//
        delete m_pHttpServerIOProc;
        m_pHttpServerIOProc = NULL;
    }
    TEST_LOGW("~~HttpServerIO exit\r\n");
}

/*****************************************************************************
-Fuction		: Proc
-Description	: 阻塞
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int HttpServerIO :: Proc()
{
    int iRet=-1;
    char *pcRecvBuf=NULL;
    char *pcSendBuf=NULL;
    int iRecvLen=-1;
    timeval tTimeValue;
    
    if(m_iClientSocketFd < 0)
    {
        TEST_LOGE("HttpServerIO m_iClientSocketFd < 0 err\r\n");
        return -1;
    }
    pcRecvBuf = new char[HTTP_IO_RECV_MAX_LEN];
    if(NULL == pcRecvBuf)
    {
        TEST_LOGE("HttpServerIO NULL == pcRecvBuf err\r\n");
        return -1;
    }
    pcSendBuf = new char[HTTP_IO_SEND_MAX_LEN];
    if(NULL == pcSendBuf)
    {
        TEST_LOGE("HttpServerIO NULL == pcSendBuf err\r\n");
        delete[] pcRecvBuf;
        return -1;
    }
    m_iHttpServerIOFlag = 1;
    TEST_LOGW("HlsServerIO start Proc\r\n");
    while(m_iHttpServerIOFlag)
    {
        iRecvLen = 0;
        memset(pcRecvBuf,0,HTTP_IO_RECV_MAX_LEN);
        milliseconds timeMS(10);// 表示10毫秒
        iRet=TcpServer::Recv(pcRecvBuf,&iRecvLen,HTTP_IO_RECV_MAX_LEN,m_iClientSocketFd,&timeMS);
        if(iRet < 0)
        {
            TEST_LOGE("TcpServer::Recv err exit %d\r\n",iRecvLen);
            break;
        }
        if(iRecvLen<=0)
        {
            continue;
        }
        memset(pcSendBuf,0,HTTP_IO_SEND_MAX_LEN);
        iRet=m_HttpServerSession.HandleHttpReq(pcRecvBuf,pcSendBuf,HTTP_IO_SEND_MAX_LEN);
        if(iRet > 0)
        {
            TcpServer::Send(pcSendBuf,iRet,m_iClientSocketFd);
        }
    }
    
    if(m_iClientSocketFd>=0)
    {
        TcpServer::Close(m_iClientSocketFd);//主动退出,
        TEST_LOGW("HttpServerIO::Close m_iClientSocketFd Exit%d\r\n",m_iClientSocketFd);
    }
    if(NULL != pcSendBuf)
    {
        delete[] pcSendBuf;
    }
    if(NULL != pcRecvBuf)
    {
        delete[] pcRecvBuf;
    }
    
    m_iHttpServerIOFlag=0;
    return 0;
}

/*****************************************************************************
-Fuction		: GetProcFlag
-Description	: HttpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int HttpServerIO :: GetProcFlag()
{
    return m_iHttpServerIOFlag;//多线程竞争注意优化
}

