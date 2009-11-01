/*
 *  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
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

#include "wmq.h"

#if defined _WIN32 && !defined __CYGWIN__
    /*
     * WINDOWS 32 BIT
     */

    #define MQ_LOAD(LIBRARY)                                                              \
        HINSTANCE handle = LoadLibrary(LIBRARY);                                          \
        if (!handle)                                                                      \
        {                                                                                 \
            rb_raise(wmq_exception,                                                       \
                     "WMQ::QueueManager#connect(). Failed to load MQ Library:%s, rc=%ld", \
                     LIBRARY,                                                             \
                     GetLastError());                                                     \
        }

    #define MQ_RELEASE FreeLibrary((HINSTANCE)pqm->mq_lib_handle);

    #define MQ_FUNCTION(FUNC, CAST) \
        pqm->FUNC = (CAST)GetProcAddress(handle, #FUNC);                         \
        if (!pqm->FUNC)                                                          \
        {                                                                        \
            rb_raise(wmq_exception, "Failed to find API "#FUNC" in MQ Library"); \
        }

    #define MQ_LIBRARY_SERVER "mqm"
    #define MQ_LIBRARY_CLIENT "mqic32"
#elif defined(SOLARIS) || defined(__SVR4) || defined(__linux__) || defined(LINUX)
    /*
     * SOLARIS, LINUX
     */

    #ifndef RTLD_LAZY
        #define RTLD_LAZY 1
    #endif
    #ifndef RTLD_GLOBAL
        #define RTLD_GLOBAL 0
    #endif

    #if defined(__linux__) || defined(LINUX)
        #include <dlfcn.h>
    #endif

    #define MQ_LOAD(LIBRARY)                                                             \
        void* handle = (void*)dlopen(LIBRARY, RTLD_LAZY|RTLD_GLOBAL);                    \
        if (!handle)                                                                     \
        {                                                                                \
            rb_raise(wmq_exception,                                                      \
                     "WMQ::QueueManager#connect(). Failed to load MQ Library:%s, rc=%s", \
                     LIBRARY,                                                            \
                     dlerror());                                                         \
        }

    #define MQ_RELEASE dlclose(pqm->mq_lib_handle);

    #define MQ_FUNCTION(FUNC, CAST) \
        pqm->FUNC = (CAST)dlsym(handle, #FUNC);                                  \
        if (!pqm->FUNC)                                                          \
        {                                                                        \
            rb_raise(wmq_exception, "Failed to find API "#FUNC" in MQ Library"); \
        }

    #if defined(SOLARIS) || defined(__SVR4)
        #define MQ_LIBRARY_SERVER "libmqm.so"
        #define MQ_LIBRARY_CLIENT "libmqic.so"
    #else
        #define MQ_LIBRARY_SERVER "libmqm_r.so"
        #define MQ_LIBRARY_CLIENT "libmqic_r.so"
    #endif
#elif defined(__hpux)
    /*
     * HP-UX
     */

    #define MQ_LOAD(LIBRARY)                                                             \
        shl_t handle = shl_load(file, BIND_DEFERRED, 0);                                 \
        if (!handle)                                                                     \
        {                                                                                \
            rb_raise(wmq_exception,                                                      \
                     "WMQ::QueueManager#connect(). Failed to load MQ Library:%s, rc=%s", \
                     LIBRARY,                                                            \
                     strerror(errno));                                                   \
        }

    #define MQ_RELEASE shl_unload((shl_t)pqm->mq_lib_handle);

    #define MQ_FUNCTION(FUNC, CAST)                                                  \
        pqm->FUNC = NULL;                                                            \
        shl_findsym(&handle,FUNC,TYPE_PROCEDURE,(void*)&(pqm->FUNC));                \
        if(pqm->FUNC == NULL)                                                        \
        {                                                                            \
            shl_findsym(&handle,FUNC,TYPE_UNDEFINED,(void*)&(pqm->FUNC));            \
            if(pqm->FUNC == NULL)                                                    \
            {                                                                        \
                rb_raise(wmq_exception, "Failed to find API "#FUNC" in MQ Library"); \
            }                                                                        \
        }

    #define MQ_LIBRARY_SERVER "libmqm_r.sl"
    #define MQ_LIBRARY_CLIENT "libmqic_r.sl"
#endif

void Queue_manager_mq_load(PQUEUE_MANAGER pqm)
{
#if defined MQ_FUNCTION
    PMQCHAR library;
    if(pqm->is_client_conn)
    {
        library = MQ_LIBRARY_CLIENT;
        if(pqm->trace_level) printf("WMQ::QueueManager#connect() Loading MQ Client Library:%s\n", library);
    }
    else
    {
        library = MQ_LIBRARY_SERVER;
        if(pqm->trace_level) printf("WMQ::QueueManager#connect() Loading MQ Server Library:%s\n", library);
    }

    {
        MQ_LOAD(library)

        if(pqm->trace_level>1) printf("WMQ::QueueManager#connect() MQ Library:%s Loaded successfully\n", library);

        MQ_FUNCTION(MQCONNX,void(*)(PMQCHAR,PMQCNO,PMQHCONN,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQCONN, void(*)(PMQCHAR,PMQHCONN,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQDISC,void(*) (PMQHCONN,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQBEGIN,void(*)(MQHCONN,PMQVOID,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQBACK,void(*) (MQHCONN,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQCMIT,void(*) (MQHCONN,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQPUT1,void(*) (MQHCONN,PMQVOID,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG))

        MQ_FUNCTION(MQOPEN,void(*) (MQHCONN,PMQVOID,MQLONG,PMQHOBJ,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQCLOSE,void(*)(MQHCONN,PMQHOBJ,MQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQGET,void(*)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQPUT,void(*)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG))

        MQ_FUNCTION(MQINQ,void(*)  (MQHCONN,MQHOBJ,MQLONG,PMQLONG,MQLONG,PMQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG))
        MQ_FUNCTION(MQSET,void(*)  (MQHCONN,MQHOBJ,MQLONG,PMQLONG,MQLONG,PMQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG))

        MQ_FUNCTION(mqCreateBag,void(*)(MQLONG,PMQHBAG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqDeleteBag,void(*)(PMQHBAG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqClearBag,void(*)(MQHBAG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqExecute,void(*)(MQHCONN,MQLONG,MQHBAG,MQHBAG,MQHBAG,MQHOBJ,MQHOBJ,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqCountItems,void(*)(MQHBAG,MQLONG,PMQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqInquireBag,void(*)(MQHBAG,MQLONG,MQLONG,PMQHBAG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqInquireItemInfo,void(*)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqInquireInteger,void(*)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqInquireString,void(*)(MQHBAG,MQLONG,MQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqAddInquiry,void(*)(MQHBAG,MQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqAddInteger,void(*)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG))
        MQ_FUNCTION(mqAddString,void(*)(MQHBAG,MQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG))

        pqm->mq_lib_handle = (void*)handle;

        if(pqm->trace_level>1) printf("WMQ::QueueManager#connect() MQ API's loaded successfully\n");
    }
#else
    /*
     * For all the other platforms were we have to have two versions of Ruby WMQ
     *  1: Linked with MQ Server Library
     *  2: Linked with MQ Client Library
     *
     * As a result to use the client library:
     *    require 'wmq/wmq_client'
     */
    pqm->MQCONNX = &MQCONNX;
    pqm->MQCONN  = &MQCONN;
    pqm->MQDISC  = &MQDISC;
    pqm->MQBEGIN = &MQBEGIN;
    pqm->MQBACK  = &MQBACK;
    pqm->MQCMIT  = &MQCMIT;
    pqm->MQPUT1  = &MQPUT1;

    pqm->MQOPEN  = &MQOPEN;
    pqm->MQCLOSE = &MQCLOSE;
    pqm->MQGET   = &MQGET;
    pqm->MQPUT   = &MQPUT;

    pqm->MQINQ   = &MQINQ;
    pqm->MQSET   = &MQSET;

    pqm->mqCreateBag      = &mqCreateBag;
    pqm->mqClearBag       = &mqClearBag;
    pqm->mqExecute        = &mqExecute;
    pqm->mqCountItems     = &mqCountItems;
    pqm->mqInquireBag     = &mqInquireBag;
    pqm->mqInquireItemInfo= &mqInquireItemInfo;
    pqm->mqInquireInteger = &mqInquireInteger;
    pqm->mqInquireString  = &mqInquireString;
#endif
}

void Queue_manager_mq_free(PQUEUE_MANAGER pqm)
{
    if(pqm->mq_lib_handle)
    {
        if(pqm->trace_level>1) printf("WMQ::QueueManager#gc() Releasing MQ Library\n");
        MQ_RELEASE
        pqm->mq_lib_handle = 0;
    }
}
