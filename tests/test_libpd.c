/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <CUnit/Basic.h>

#include "../source/include/webpa_adapter.h"
#include "../source/app/libpd.h"
#include <cimplog/cimplog.h>
#include <libparodus.h>

#define URL_SIZE 	    64
#define UNUSED(x) (void )(x)
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern libpd_instance_t current_instance;
extern char parodus_url[URL_SIZE];
extern char client_url[URL_SIZE];
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

void _WEBPA_LOG(unsigned int level, const char *msg, ...)
{
    int ret = 0;
    va_list arg;
    char *pTempChar = (char *)malloc(4096);
    if(pTempChar)
    {
        va_start(arg, msg);
        ret = vsnprintf(pTempChar, 4096, msg,arg);
        if(ret < 0)
        {
            perror(pTempChar);
        }
        va_end(arg);
    }

    switch(level)
    {
        case 0:
            cimplog_error("WEBPA", pTempChar);
        break;

        case 1:
            cimplog_info("WEBPA", pTempChar);
        break;

        case 2:
            cimplog_debug("WEBPA", pTempChar);
        break;
    }
    free(pTempChar);
}

void getCurrentTime(struct timespec *timer)
{
    clock_gettime(CLOCK_REALTIME, timer); 
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    long msec;
    msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
    msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
    return msec;
}

void processRequest(char *reqPayload, char *transactionId, char **resPayload)
{
    UNUSED(reqPayload);
    UNUSED(transactionId);
    UNUSED(resPayload);
}

int libparodus_init (libpd_instance_t *instance, libpd_cfg_t *libpd_cfg)
{
    UNUSED(instance);
    UNUSED(libpd_cfg);
    function_called();
    return (int) mock();
}
int libparodus_send (libpd_instance_t instance, wrp_msg_t *msg)
{
    UNUSED(instance);
    UNUSED(msg);
    function_called();
    return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_libpd_client_mgr()
{
    strcpy(parodus_url, "tcp://localhost:6666");
    strcpy(client_url ,"tcp://localhost:6667");
    
    will_return(libparodus_init, (intptr_t)0);
    expect_function_call(libparodus_init);
    libpd_client_mgr();
}

void test_sendNotification()
{
    char *source =  (char *) malloc(sizeof(char)*16);
    char *payload =  (char *) malloc(sizeof(char)*100);
    char destination[16] = "destination";
    strcpy(source, "source");
    strcpy(payload, "Hello Webpa!");

    will_return(libparodus_send, (intptr_t)0);
    expect_function_call(libparodus_send);
    sendNotification(payload, source, destination);
}

void err_sendNotification()
{
    char *source =  (char *) malloc(sizeof(char)*16);
    char *payload =  (char *) malloc(sizeof(char)*100);
    char destination[16] = "destination";
    strcpy(source, "source");
    strcpy(payload, "Hello Webpa!");

    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    will_return(libparodus_send, (intptr_t)-404);
    expect_function_calls(libparodus_send, 4);
    sendNotification(payload, source, destination);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_libpd_client_mgr),
        cmocka_unit_test(test_sendNotification),
        cmocka_unit_test(err_sendNotification)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
