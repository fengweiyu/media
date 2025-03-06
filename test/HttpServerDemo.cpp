/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpServerDemo.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "HttpServerDemo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <utility>

using std::make_pair;

/*****************************************************************************
-Fuction		: HttpServerDemo
-Description	: HttpServerDemo
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpServerDemo :: HttpServerDemo(int i_iServerPort)
{
    TcpServer::Init(NULL,i_iServerPort);
}

/*****************************************************************************
-Fuction		: ~HttpServerDemo
-Description	: ~HttpServerDemo
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
HttpServerDemo :: ~HttpServerDemo()
{
}

/*****************************************************************************
-Fuction		: Proc
-Description	: 阻塞
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int HttpServerDemo :: Proc()
{
    int iClientSocketFd=-1;
    HttpServerIO *pHttpServerIO = NULL;
    while(1)
    {
        iClientSocketFd=TcpServer::Accept();
        if(iClientSocketFd<0)  
        {  
            SleepMs(10);
            CheckMapServerIO();
            continue;
        } 
        pHttpServerIO = new HttpServerIO(iClientSocketFd);
        AddMapServerIO(pHttpServerIO,iClientSocketFd);
        TEST_LOGD("m_HttpServerIOMap size %d\r\n",m_HttpServerIOMap.size());
    }
    return 0;
}

/*****************************************************************************
-Fuction        : CheckMapServerIO
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int HttpServerDemo::CheckMapServerIO()
{
    int iRet = -1;
    HttpServerIO *pHttpServerIO=NULL;

    std::lock_guard<std::mutex> lock(m_MapMtx);//std::lock_guard对象会在其作用域结束时自动释放互斥量
    for (map<int, HttpServerIO *>::iterator iter = m_HttpServerIOMap.begin(); iter != m_HttpServerIOMap.end(); )
    {
        pHttpServerIO=iter->second;
        if(0 == pHttpServerIO->GetProcFlag())
        {
            delete pHttpServerIO;
            iter=m_HttpServerIOMap.erase(iter);// 擦除元素并返回下一个元素的迭代器
        }
        else
        {
            iter++;// 继续遍历下一个元素
        }
    }
    return 0;
}

/*****************************************************************************
-Fuction        : AddMapHttpSession
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int HttpServerDemo::AddMapServerIO(HttpServerIO * i_pHttpServerIO,int i_iClientSocketFd)
{
    int iRet = -1;

    if(NULL == i_pHttpServerIO)
    {
        TEST_LOGE("AddMapServerIO NULL!!!%p\r\n",i_pHttpServerIO);
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_MapMtx);//std::lock_guard对象会在其作用域结束时自动释放互斥量
    m_HttpServerIOMap.insert(make_pair(i_iClientSocketFd,i_pHttpServerIO));
    return 0;
}


