/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       HttpTestCase.h
* Description           : 	
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef HTTP_TEST_CASE_H
#define HTTP_TEST_CASE_H

#include <thread>
#include <mutex>

using std::thread;
using std::mutex;

#ifdef _WIN32
#include <Windows.h>
#define SleepMs(val) Sleep(val)
#else
#include <unistd.h>
#define SleepMs(val) usleep(val*1000)
#endif
#define  TEST_LOGW(...)     printf(__VA_ARGS__)
#define  TEST_LOGE(...)     printf(__VA_ARGS__)
#define  TEST_LOGD(...)     printf(__VA_ARGS__)
#define  TEST_LOGI(...)     printf(__VA_ARGS__)

/*****************************************************************************
-Class			: HttpTestCase
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class HttpTestCase
{
public:
	HttpTestCase();
	virtual ~HttpTestCase();
    int Test(const char * i_strSrcFilePath,const char *i_strDstFilePath);
private:
    int ReadFile(const char * i_strSrcFilePath,unsigned char **o_ppBuffer);
    int Proc(const char * i_strSrcFilePath,const char *i_strDstFilePath);
};

#endif
