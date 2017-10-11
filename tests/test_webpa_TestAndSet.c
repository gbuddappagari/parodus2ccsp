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
#include <string.h>

#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include <ccsp_base_api.h>
#include "mock_stack.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern BOOL applySettingsFlag;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

parameterValStruct_t ** getCmc(char *strCmc)
{
    parameterValStruct_t **cmc = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmc[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmc[0]->parameterName = strdup(PARAM_CMC);
    cmc[0]->parameterValue = strdup(strCmc);
    cmc[0]->type = ccsp_int;
    return cmc;
}

parameterValStruct_t ** getCid(char *strCid)
{
    parameterValStruct_t **cid = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cid[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cid[0]->parameterName = strdup(PARAM_CID);
    cid[0]->parameterValue = strdup(strCid);
    cid[0]->type = ccsp_string;
    return cid;
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_testAndSetWithSingleParameterSet()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.Count\",\"value\":\"3\",\"dataType\":2}],\"new-cid\":\"1234\",\"old-cid\":\"0\",\"sync-cmc\":\"123\",\"command\":\"TEST_AND_SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1;
    int totalCount = 1;

    getCompDetails();

    will_return(get_global_values, getCmc("123"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, getCid("0"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, getCid("0"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    will_return(get_global_values, getCmc("512"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    valueList[0]->parameterName = strdup("Device.DeviceInfo.Webpa.Count");
    valueList[0]->parameterValue = strdup("3");
    valueList[0]->type = ccsp_int;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(2, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal(PARAM_CID,cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("1234",cJSON_GetObjectItem(resParamObj, "value")->valuestring);
    resParamObj = cJSON_GetArrayItem(paramArray, 1);
    assert_string_equal(PARAM_CMC,cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_int_equal(512,cJSON_GetObjectItem(resParamObj, "value")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

void test_testAndSetWithoutCmcSingleParameterSet()
{
    char *reqPayload = "{\"parameters\":[{\"name\":\"Device.DeviceInfo.Webpa.Count\",\"value\":\"3\",\"dataType\":2}],\"new-cid\":\"1234\",\"old-cid\":\"0\",\"command\":\"TEST_AND_SET\"}";
    char *resPayload = NULL;
    cJSON *response = NULL, *paramArray = NULL, *resParamObj = NULL;
    int count = 1;
    int totalCount = 1;

    getCompDetails();

    will_return(get_global_values, getCmc("512"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, getCid("0"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_values, getCid("0"));
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    parameterValStruct_t **valueList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    valueList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    valueList[0]->parameterName = strdup("Device.DeviceInfo.Webpa.Count");
    valueList[0]->parameterValue = strdup("3");
    valueList[0]->type = ccsp_int;

    will_return(get_global_values, valueList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    will_return(get_global_faultParam, NULL);
    will_return(CcspBaseIf_setParameterValues, CCSP_SUCCESS);
    expect_function_call(CcspBaseIf_setParameterValues);
    expect_value(CcspBaseIf_setParameterValues, size, 1);

    processRequest(reqPayload, NULL, &resPayload);
    WalInfo("resPayload : %s\n",resPayload);

    assert_non_null(resPayload);
    response = cJSON_Parse(resPayload);
    assert_non_null(response);
    paramArray = cJSON_GetObjectItem(response, "parameters");
    assert_int_equal(2, cJSON_GetArraySize(paramArray));
    resParamObj = cJSON_GetArrayItem(paramArray, 0);
    assert_string_equal(PARAM_CID,cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_string_equal("1234",cJSON_GetObjectItem(resParamObj, "value")->valuestring);
    resParamObj = cJSON_GetArrayItem(paramArray, 1);
    assert_string_equal(PARAM_CMC,cJSON_GetObjectItem(resParamObj, "name")->valuestring);
    assert_int_equal(512,cJSON_GetObjectItem(resParamObj, "value")->valueint);
    assert_string_equal("Success",cJSON_GetObjectItem(response, "message")->valuestring );
    assert_int_equal(200, cJSON_GetObjectItem(response, "statusCode")->valueint);
    cJSON_Delete(response);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_testAndSetWithSingleParameterSet),
        cmocka_unit_test(test_testAndSetWithoutCmcSingleParameterSet)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
