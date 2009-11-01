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

static ID ID_open;
static ID ID_call;
static ID ID_new;
static ID ID_backout;
static ID ID_connect_options;
static ID ID_q_mgr_name;
static ID ID_queue_manager;
static ID ID_exception_on_error;
static ID ID_descriptor;
static ID ID_message;
static ID ID_trace_level;

/* MQCD ID's */
static ID ID_channel_name;
static ID ID_transport_type;
static ID ID_mode_name;
static ID ID_tp_name;
static ID ID_security_exit;
static ID ID_send_exit;
static ID ID_receive_exit;
static ID ID_max_msg_length;
static ID ID_security_user_data;
static ID ID_send_user_data;
static ID ID_receive_user_data;
static ID ID_user_identifier;
static ID ID_password;
static ID ID_connection_name;
static ID ID_heartbeat_interval;
static ID ID_long_remote_user_id;
static ID ID_remote_security_id;
static ID ID_ssl_cipher_spec;
static ID ID_ssl_peer_name;
static ID ID_keep_alive_interval;
static ID ID_crypto_hardware;

/* MQSCO ID's */
static ID ID_key_repository;

/* Admin ID's */
static ID ID_create_queue;
static ID ID_q_name;
static ID ID_command;

void QueueManager_id_init(void)
{
    ID_open                 = rb_intern("open");
    ID_call                 = rb_intern("call");
    ID_new                  = rb_intern("new");
    ID_backout              = rb_intern("backout");
    ID_q_mgr_name           = rb_intern("q_mgr_name");
    ID_queue_manager        = rb_intern("queue_manager");
    ID_exception_on_error   = rb_intern("exception_on_error");
    ID_connect_options      = rb_intern("connect_options");
    ID_trace_level          = rb_intern("trace_level");
    ID_descriptor           = rb_intern("descriptor");
    ID_message              = rb_intern("message");

    /* MQCD ID's */
    ID_channel_name         = rb_intern("channel_name");
    ID_transport_type       = rb_intern("transport_type");
    ID_mode_name            = rb_intern("mode_name");
    ID_tp_name              = rb_intern("tp_name");
    ID_security_exit        = rb_intern("security_exit");
    ID_send_exit            = rb_intern("send_exit");
    ID_receive_exit         = rb_intern("receive_exit");
    ID_max_msg_length       = rb_intern("max_msg_length");
    ID_security_user_data   = rb_intern("security_user_data");
    ID_send_user_data       = rb_intern("send_user_data");
    ID_receive_user_data    = rb_intern("receive_user_data");
    ID_user_identifier      = rb_intern("user_identifier");
    ID_password             = rb_intern("password");
    ID_connection_name      = rb_intern("connection_name");
    ID_heartbeat_interval   = rb_intern("heartbeat_interval");
    ID_long_remote_user_id  = rb_intern("long_remote_user_id");
    ID_remote_security_id   = rb_intern("remote_security_id");
    ID_ssl_cipher_spec      = rb_intern("ssl_cipher_spec");
    ID_ssl_peer_name        = rb_intern("ssl_peer_name");
    ID_keep_alive_interval  = rb_intern("keep_alive_interval");

    /* MQSCO ID's */
    ID_key_repository       = rb_intern("key_repository");
    ID_crypto_hardware      = rb_intern("crypto_hardware");

    /* Admin ID's */
    ID_create_queue         = rb_intern("create_queue");
    ID_q_name               = rb_intern("q_name");
    ID_command              = rb_intern("command");
}

/* --------------------------------------------------
 * C Structure to store MQ data types and other
 *   C internal values
 * --------------------------------------------------*/
void QUEUE_MANAGER_free(void* p)
{
    PQUEUE_MANAGER pqm = (PQUEUE_MANAGER)p;

    if(pqm->trace_level>1) printf("WMQ::QueueManager Freeing QUEUE_MANAGER structure\n");

    if (pqm->hcon && !pqm->already_connected)  /* Valid MQ handle means MQDISC was not called */
    {
        printf("WMQ::QueueManager#free disconnect() was not called for Queue Manager instance!!\n");
        printf("WMQ::QueueManager#free Automatically calling back() and disconnect()\n");
        pqm->MQBACK(pqm->hcon, &pqm->comp_code, &pqm->reason_code);
        pqm->MQDISC(&pqm->hcon, &pqm->comp_code, &pqm->reason_code);
    }
  #ifdef MQCD_VERSION_6
    free(pqm->long_remote_user_id_ptr);
  #endif
  #ifdef MQCD_VERSION_7
    free(pqm->ssl_peer_name_ptr);
  #endif
  #ifdef MQHB_UNUSABLE_HBAG
    if (pqm->admin_bag != MQHB_UNUSABLE_HBAG)
    {
        pqm->mqDeleteBag(&pqm->admin_bag, &pqm->comp_code, &pqm->reason_code);
    }

    if (pqm->reply_bag != MQHB_UNUSABLE_HBAG)
    {
        pqm->mqDeleteBag(&pqm->reply_bag, &pqm->comp_code, &pqm->reason_code);
    }
  #endif
    Queue_manager_mq_free(pqm);
    free(pqm->p_buffer);
    free(p);
}

VALUE QUEUE_MANAGER_alloc(VALUE klass)
{
    static MQCNO default_MQCNO = {MQCNO_DEFAULT};       /* MQCONNX Connection Options    */
  #ifdef MQCNO_VERSION_2
    static MQCD  default_MQCD  = {MQCD_CLIENT_CONN_DEFAULT}; /* Client Connection             */
  #endif
  #ifdef MQCNO_VERSION_4
    static MQSCO default_MQSCO = {MQSCO_DEFAULT};
  #endif

    PQUEUE_MANAGER pqm = ALLOC(QUEUE_MANAGER);

    pqm->hcon = 0;
    pqm->comp_code = 0;
    pqm->reason_code = 0;
    pqm->exception_on_error = 1;
    pqm->already_connected = 0;
    pqm->trace_level = 0;
    memcpy(&pqm->connect_options, &default_MQCNO, sizeof(MQCNO));
  #ifdef MQCNO_VERSION_2
    memcpy(&pqm->client_conn, &default_MQCD, sizeof(MQCD));

    /* Tell MQ to use Client Conn structures, etc. */
    pqm->connect_options.Version = MQCNO_CURRENT_VERSION;
    pqm->connect_options.ClientConnPtr = &pqm->client_conn;
  #endif
  #ifdef MQCNO_VERSION_4
    memcpy(&pqm->ssl_config_opts, &default_MQSCO, sizeof(MQSCO));
  #endif
  #ifdef MQCD_VERSION_6
    pqm->long_remote_user_id_ptr = 0;
  #endif
  #ifdef MQCD_VERSION_7
    pqm->ssl_peer_name_ptr = 0;
  #endif
  #ifdef MQHB_UNUSABLE_HBAG
    pqm->admin_bag = MQHB_UNUSABLE_HBAG;
    pqm->reply_bag = MQHB_UNUSABLE_HBAG;
  #endif
    pqm->buffer_size = 0;
    pqm->p_buffer    = 0;

    pqm->is_client_conn = 0;
    pqm->mq_lib_handle  = 0;

    return Data_Wrap_Struct(klass, 0, QUEUE_MANAGER_free, pqm);
}

/*
 * call-seq:
 *   new(...)
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * See QueueManager.new for details on all the parameters
 *
 * Note:
 * * It is not recommended to use this method, rather use QueueManager.connect, since
 *   it will automatically disconnect from the queue manager. It also deals with backing out
 *   the current unit of work in the event of an unhandled exception. E.g. Syntax Error
 * * RuntimeError and ArgumentError exceptions are always thrown regardless of the
 *   value of :exception_on_error
 *
 * Todo:
 * * Support multiple send and receive exits
 */
VALUE QueueManager_initialize(VALUE self, VALUE hash)
{
    VALUE          val;
    VALUE          str;
    size_t         size;
    size_t         length;
    PQUEUE_MANAGER pqm;

    Check_Type(hash, T_HASH);

    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    WMQ_HASH2MQLONG(hash,trace_level, pqm->trace_level)

    /* @name = options[:q_mgr_name] || ''     # QMGR Name optional with Client Connection */
    val = rb_hash_aref(hash, ID2SYM(ID_q_mgr_name));
    if (NIL_P(val))
    {
        rb_iv_set(self, "@name", rb_str_new2(""));
        if(pqm->trace_level > 1) printf("WMQ::QueueManager#initialize() Queue Manager:[Not specified, use Default QMGR]\n");
    }
    else
    {
        rb_iv_set(self, "@name", val);
        if(pqm->trace_level > 1) printf("WMQ::QueueManager#initialize() Queue Manager:%s\n", RSTRING_PTR(val));
    }

    WMQ_HASH2BOOL(hash,exception_on_error, pqm->exception_on_error)

    /*
     * All Client connection parameters are ignored if connection_name is missing
     */
#ifdef MQCNO_VERSION_2
    if(!NIL_P(rb_hash_aref(hash, ID2SYM(ID_connection_name))))
    {
        PMQCD pmqcd = &pqm->client_conn;              /* Process MQCD */
        pqm->is_client_conn = 1;                      /* Set to Client connection */

        WMQ_HASH2MQCHARS(hash,connection_name,             pmqcd->ConnectionName)
        WMQ_HASH2MQLONG (hash,transport_type,              pmqcd->TransportType)
        WMQ_HASH2MQCHARS(hash,mode_name,                   pmqcd->ModeName)
        WMQ_HASH2MQCHARS(hash,tp_name,                     pmqcd->TpName)
        WMQ_HASH2MQCHARS(hash,security_exit,               pmqcd->SecurityExit)
        WMQ_HASH2MQCHARS(hash,send_exit,                   pmqcd->SendExit)
        WMQ_HASH2MQCHARS(hash,receive_exit,                pmqcd->ReceiveExit)
        WMQ_HASH2MQLONG (hash,max_msg_length,              pmqcd->MaxMsgLength)
        WMQ_HASH2MQCHARS(hash,security_user_data,          pmqcd->SecurityUserData)
        WMQ_HASH2MQCHARS(hash,send_user_data,              pmqcd->SendUserData)
        WMQ_HASH2MQCHARS(hash,receive_user_data,           pmqcd->ReceiveUserData)
        WMQ_HASH2MQCHARS(hash,user_identifier,             pmqcd->UserIdentifier)
        WMQ_HASH2MQCHARS(hash,password,                    pmqcd->Password)

        /* Default channel name to system default */
        val = rb_hash_aref(hash, ID2SYM(ID_channel_name));
        if (NIL_P(val))
        {
            strncpy(pmqcd->ChannelName, "SYSTEM.DEF.SVRCONN", sizeof(pmqcd->ChannelName));
        }
        else
        {
            WMQ_HASH2MQCHARS(hash,channel_name,            pmqcd->ChannelName)
        }

    #ifdef MQCD_VERSION_4
        WMQ_HASH2MQLONG(hash,heartbeat_interval,          pmqcd->HeartbeatInterval)
        /* TODO:
        WMQ_HASH2MQLONG(hash,exit_name_length,            pmqcd->ExitNameLength)
        WMQ_HASH2MQLONG(hash,exit_data_length,            pmqcd->ExitDataLength)
        WMQ_HASH2MQLONG(hash,send_exits_defined,          pmqcd->SendExitsDefined)
        WMQ_HASH2MQLONG(hash,receive_exits_defined,       pmqcd->ReceiveExitsDefined)
        TO_PTR (send_exit_ptr,               pmqcd->SendExitPtr)
        TO_PTR (send_user_data_ptr,          pmqcd->SendUserDataPtr)
        TO_PTR (receive_exit_ptr,            pmqcd->ReceiveExitPtr)
        TO_PTR (receive_user_data_ptr,       pmqcd->ReceiveUserDataPtr)
        */
    #endif
    #ifdef MQCD_VERSION_6
        val = rb_hash_aref(hash, ID2SYM(ID_long_remote_user_id));
        if (!NIL_P(val))
        {
            str = StringValue(val);
            length = RSTRING_LEN(str);

            if (length > 0)
            {
                MQPTR pBuffer;
                if(pqm->trace_level > 1)
                    printf("WMQ::QueueManager#initialize() Setting long_remote_user_id:%s\n",
                        RSTRING_PTR(str));

                /* Include null at end of string */
                pBuffer = ALLOC_N(char, length+1);
                memcpy(pBuffer, RSTRING_PTR(str), length+1);

                pmqcd->LongRemoteUserIdLength = length;
                pmqcd->LongRemoteUserIdPtr    = pBuffer;
                pqm->long_remote_user_id_ptr  = pBuffer;
            }
        }
        WMQ_HASH2MQBYTES(hash,remote_security_id,          pmqcd->RemoteSecurityId)
        WMQ_HASH2MQCHARS(hash,ssl_cipher_spec,             pmqcd->SSLCipherSpec)
    #endif
    #ifdef MQCD_VERSION_7
        val = rb_hash_aref(hash, ID2SYM(ID_ssl_peer_name));
        if (!NIL_P(val))
        {
            str = StringValue(val);
            length = RSTRING_LEN(str);

            if (length > 0)
            {
                MQPTR pBuffer;
                if(pqm->trace_level > 1)
                    printf("WMQ::QueueManager#initialize() Setting ssl_peer_name:%s\n",
                        RSTRING_PTR(str));

                /* Include null at end of string */
                pBuffer = ALLOC_N(char, length+1);
                memcpy(pBuffer, RSTRING_PTR(str), length+1);

                pmqcd->SSLPeerNameLength = length;
                pmqcd->SSLPeerNamePtr    = pBuffer;
                pqm->ssl_peer_name_ptr   = pBuffer;
            }
        }
        WMQ_HASH2MQLONG(hash,keep_alive_interval,         pmqcd->KeepAliveInterval)

        /* Only set if SSL options are supplied, otherwise
        * environment variables are ignored: MQSSLKEYR and MQSSLCRYP
        * Any SSL info in the client channel definition tables is also ignored
        */
        if (!NIL_P(rb_hash_aref(hash, ID2SYM(ID_key_repository))) ||
            !NIL_P(rb_hash_aref(hash, ID2SYM(ID_crypto_hardware))))
        {
            /* Process MQSCO */
            WMQ_HASH2MQCHARS(hash,key_repository,              pqm->ssl_config_opts.KeyRepository)
            WMQ_HASH2MQCHARS(hash,crypto_hardware,             pqm->ssl_config_opts.CryptoHardware)

            pqm->connect_options.SSLConfigPtr = &pqm->ssl_config_opts;
        }
    #endif

    }
    else
    {
        pqm->is_client_conn = 0;                       /* Set to Server connection */
    }
#endif

#ifdef MQCNO_VERSION_4
    /* Process MQCNO */
    WMQ_HASH2MQLONG(hash,connect_options,             pqm->connect_options.Options)
#endif

  /* --------------------------------------------------
   * TODO:   MQAIR Structure - LDAP Security
   * --------------------------------------------------*/

    return Qnil;
}

/*
 * Before working with any queues, it is necessary to connect
 * to the queue manager.
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code != MQCC_OK
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_connect(VALUE self)
{
    VALUE    name;

    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    pqm->already_connected = 0;

    Queue_manager_mq_load(pqm);                       /* Load MQ Library */

    name = rb_iv_get(self,"@name");
    name = StringValue(name);

    if(pqm->trace_level)
        printf("WMQ::QueueManager#connect() Connect to Queue Manager:%s\n", RSTRING_PTR(name));

    if (pqm->hcon)                                    /* Disconnect from qmgr if already connected, ignore errors */
    {
        if(pqm->trace_level)
            printf("WMQ::QueueManager#connect() Already connected to Queue Manager:%s, Disconnecting first!\n", RSTRING_PTR(name));

        pqm->MQDISC(&pqm->hcon, &pqm->comp_code, &pqm->reason_code);
    }

    pqm->MQCONNX(
            RSTRING_PTR(name),      /* queue manager                  */
            &pqm->connect_options,   /* Connection Options             */
            &pqm->hcon,              /* connection handle              */
            &pqm->comp_code,         /* completion code                */
            &pqm->reason_code);      /* connect reason code            */

    if(pqm->trace_level)
        printf("WMQ::QueueManager#connect() MQCONNX completed with reason:%s, Handle:%ld\n",
               wmq_reason(pqm->reason_code),
               (long)pqm->hcon);

    if (pqm->comp_code == MQCC_FAILED)
    {
        pqm->hcon = 0;

        if (pqm->exception_on_error)
        {
            rb_raise(wmq_exception,
                     "WMQ::QueueManager#connect(). Error connecting to Queue Manager:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pqm->reason_code));
        }

        return Qfalse;
    }

    if (pqm->reason_code == MQRC_ALREADY_CONNECTED)
    {
        if(pqm->trace_level) printf("WMQ::QueueManager#connect() Already connected\n");
        pqm->already_connected = 1;
    }

    return Qtrue;
}

/*
 * Disconnect from this QueueManager instance
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code != MQCC_OK
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_disconnect(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    if(pqm->trace_level) printf ("WMQ::QueueManager#disconnect() Queue Manager Handle:%ld\n", (long)pqm->hcon);

    if (!pqm->already_connected)
    {
        pqm->MQDISC(&pqm->hcon, &pqm->comp_code, &pqm->reason_code);

        if(pqm->trace_level) printf("WMQ::QueueManager#disconnect() MQDISC completed with reason:%s\n", wmq_reason(pqm->reason_code));

        if (pqm->comp_code != MQCC_OK)
        {
            if (pqm->exception_on_error)
            {
                VALUE name = rb_iv_get(self,"@name");
                name = StringValue(name);

                rb_raise(wmq_exception,
                         "WMQ::QueueManager#disconnect(). Error disconnecting from Queue Manager:%s, reason:%s",
                         RSTRING_PTR(name),
                         wmq_reason(pqm->reason_code));
            }

            return Qfalse;
        }
    }
    else
    {
        pqm->comp_code = 0;
        pqm->reason_code = 0;

        if(pqm->trace_level) printf ("WMQ::QueueManager#disconnect() Not calling MQDISC, since already connected on connect\n");
    }

    pqm->hcon = 0;

    return Qtrue;
}

/*
 * Commit the current unit of work for this QueueManager instance
 *
 * Note:
 * * commit will have no effect if all put and get operations were performed
 *   without specifying :sync => true
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code != MQCC_OK
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_commit(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    if(pqm->trace_level) printf ("WMQ::QueueManager#commit() Queue Manager Handle:%ld\n", (long)pqm->hcon);

    pqm->MQCMIT(pqm->hcon, &pqm->comp_code, &pqm->reason_code);

    if(pqm->trace_level) printf("WMQ::QueueManager#commit() MQCMIT completed with reason:%s\n", wmq_reason(pqm->reason_code));

    if (pqm->comp_code != MQCC_OK)
    {
        if (pqm->exception_on_error)
        {
            VALUE name = rb_iv_get(self,"@name");
            name = StringValue(name);

            rb_raise(wmq_exception,
                     "WMQ::QueueManager#commit(). Error commiting changes to Queue Manager:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pqm->reason_code));
        }
        return Qfalse;
    }

    return Qtrue;
}

/*
 * Backout the current unit of work for this QueueManager instance
 *
 * Since the last commit or rollback any messages put to a queue
 * under synchpoint will be removed and any messages retrieved
 * under synchpoint from any queues will be returned
 *
 * Note:
 * * backout will have no effect if all put and get operations were performed
 *   without specifying :sync => true
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code != MQCC_OK
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_backout(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    if(pqm->trace_level) printf ("WMQ::QueueManager#backout() Queue Manager Handle:%ld\n", (long)pqm->hcon);

    pqm->MQBACK(pqm->hcon, &pqm->comp_code, &pqm->reason_code);

    if(pqm->trace_level) printf("WMQ::QueueManager#backout() MQBACK completed with reason:%s\n", wmq_reason(pqm->reason_code));

    if (pqm->comp_code != MQCC_OK)
    {
        if (pqm->exception_on_error)
        {
            VALUE name = rb_iv_get(self,"@name");
            name = StringValue(name);

            rb_raise(wmq_exception,
                     "WMQ::QueueManager#backout(). Error backing out changes to Queue Manager:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pqm->reason_code));
        }
        return Qfalse;
    }

    return Qtrue;
}

/*
 * Advanced WebSphere MQ Use:
 *
 * Begin a unit of work between this QueueManager instance and another
 * resource such as a Database
 *
 * Starts a new unit of work under which put and get can be called with
 * with the parameter :sync => true
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code != MQCC_OK
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_begin(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    if(pqm->trace_level) printf ("WMQ::QueueManager#begin() Queue Manager Handle:%ld\n", (long)pqm->hcon);

    pqm->MQBEGIN(pqm->hcon, 0, &pqm->comp_code, &pqm->reason_code);

    if(pqm->trace_level) printf("WMQ::QueueManager#begin() MQBEGIN completed with reason:%s\n", wmq_reason(pqm->reason_code));

    if (pqm->comp_code != MQCC_OK)
    {
        if (pqm->exception_on_error)
        {
            VALUE name = rb_iv_get(self,"@name");
            name = StringValue(name);

            rb_raise(wmq_exception,
                     "WMQ::QueueManager#begin(). Error starting unit of work on Queue Manager:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pqm->reason_code));
        }
        return Qfalse;
    }

    return Qtrue;
}

/*
 * call-seq:
 *   put(parameters)
 *
 * Put a message to the queue without having to first open the queue
 * Recommended for reply queues that change frequently
 *
 * * parameters: a Hash consisting of one or more of the following parameters
 *
 * Summary of parameters and their WebSphere MQ equivalents
 *  queue.get(                                             # WebSphere MQ Equivalents:
 *   :q_name             => 'Queue Name',                  # MQOD.ObjectName
 *   :q_name             => { queue_manager=>'QMGR_name',  # MQOD.ObjectQMgrName
 *                            q_name       =>'q_name'}
 *   :message            => my_message,                    # n/a : Instance of Message
 *   :data               => "Hello World",                 # n/a : Data to send
 *   :sync               => false,                         # MQGMO_SYNCPOINT
 *   :new_id             => true,                          # MQPMO_NEW_MSG_ID & MQPMO_NEW_CORREL_ID
 *   :new_msg_id         => true,                          # MQPMO_NEW_MSG_ID
 *   :new_correl_id      => true,                          # MQPMO_NEW_CORREL_ID
 *   :fail_if_quiescing  => true,                          # MQOO_FAIL_IF_QUIESCING
 *   :options            => WMQ::MQPMO_FAIL_IF_QUIESCING   # MQPMO_*
 *   )
 *
 * Mandatory Parameters
 * * :q_name => String
 *   * Name of the existing WebSphere MQ local queue, model queue or remote queue to open
 *   * To open remote queues for which a local remote queue definition is not available
 *     pass a Hash as q_name (see q_name => Hash)
 *       OR
 * * :q_name => Hash
 *   * q_name => String
 *     * Name of the existing WebSphere MQ local queue, model queue or remote queue to open
 *   * :q_mgr_name => String
 *     * Name of the remote WebSphere MQ queue manager to send the message to.
 *     * This allows a message to be written to a queue on a remote queue manager
 *       where a remote queue definition is not defined locally
 *     * Commonly used to reply to messages from remote systems
 *     * If q_mgr_name is the same as the local queue manager name then the message
 *       is merely written to the local queue.
 *     * Note: q_mgr_name should only be supplied when putting messages to the queue.
 *         It is not possible to get messages from a queue on a queue manager other
 *         than the currently connected queue manager
 *
 * * Either :message or :data must be supplied
 *   * If both are supplied, then :data will be written to the queue. The data in :message
 *     will be ignored
 *
 * Optional Parameters
 * * :data => String
 *   * Data to be written to the queue. Can be binary or text data
 *
 * * :message => Message
 *   * An instance of the WMQ::Message
 *   * The Message descriptor, headers and data is retrieved from :message
 *     * message.data is ignored if :data is supplied
 *
 * * :sync => true or false
 *   * Determines whether the get is performed under synchpoint.
 *     I.e. Under the current unit of work
 *      Default: false
 *
 * * :new_id => true or false
 *   * Generate a new message id and correlation id for this
 *     message. :new_msg_id and :new_correl_id will be ignored
 *     if this parameter is true
 *      Default: false
 *
 * * :new_msg_id => true or false
 *   * Generate a new message id for this message
 *   * Note: A blank message id will result in a new message id anyway.
 *     However, for subsequent puts using the same message descriptor, the same
 *     message id will be used.
 *      Default: false
 *
 * * :new_correl_id => true or false
 *   * Generate a new correlation id for this message
 *      Default: false
 *
 * * :fail_if_quiescing => true or false
 *   * Determines whether the WMQ::Queue#put call will fail if the queue manager is
 *     in the process of being quiesced.
 *   * Note: This interface differs from other WebSphere MQ interfaces,
 *     they do not default to true.
 *      Default: true
 *      Equivalent to: MQGMO_FAIL_IF_QUIESCING
 *
 *   * Note: As part of the application design, carefull consideration
 *     should be given as to when to allow a transaction or
 *     unit of work to complete or fail under this condition.
 *     As such it is important to include this option where
 *     appropriate so that MQ Administrators can shutdown the
 *     queue managers without having to resort to the 'immediate'
 *     shutdown option.
 *
 * * :options => Fixnum (Advanced MQ Use only)
 *   * Numeric field containing any of the MQ Put message options or'd together
 *     * E.g. :options => WMQ::MQPMO_PASS_IDENTITY_CONTEXT | WMQ::MQPMO_ALTERNATE_USER_AUTHORITY
 *   * Note: If :options is supplied, it is applied first, then the above parameters are
 *     applied afterwards.
 *   * One or more of the following values:
 *       WMQ::MQPMO_NO_SYNCPOINT
 *       WMQ::MQPMO_LOGICAL_ORDER
 *       WMQ::MQPMO_NO_CONTEXT
 *       WMQ::MQPMO_DEFAULT_CONTEXT
 *       WMQ::MQPMO_PASS_IDENTITY_CONTEXT
 *       WMQ::MQPMO_PASS_ALL_CONTEXT
 *       WMQ::MQPMO_SET_IDENTITY_CONTEXT
 *       WMQ::MQPMO_SET_ALL_CONTEXT
 *       WMQ::MQPMO_ALTERNATE_USER_AUTHORITY
 *       WMQ::MQPMO_RESOLVE_LOCAL_Q
 *       WMQ::MQPMO_NONE
 *   * Please see the WebSphere MQ documentation for more details on the above options
 *      Default: WMQ::MQPMO_NONE
 *
 * Returns:
 * * true : On Success
 * * false: On Failure
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code == MQCC_FAILED
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 */
VALUE QueueManager_put(VALUE self, VALUE hash)
{
    MQLONG   BufferLength;           /* Length of the message in Buffer */
    PMQVOID  pBuffer;                /* Message data                  */
    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    MQPMO    pmo = {MQPMO_DEFAULT};  /* put message options           */
    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */
    VALUE    q_name;
    VALUE    str;
    size_t   size;
    size_t   length;
    VALUE    val;

    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    Check_Type(hash, T_HASH);

    q_name = rb_hash_aref(hash, ID2SYM(ID_q_name));

    if (NIL_P(q_name))
    {
        rb_raise(rb_eArgError,
                 "Mandatory parameter :q_name is missing from WMQ::QueueManager::put1()");
    }

    /* --------------------------------------------------
     * If :q_name is a hash, extract :q_name and :q_mgr_name
     * --------------------------------------------------*/
    if(TYPE(q_name) == T_HASH)
    {
        WMQ_HASH2MQCHARS(q_name, q_mgr_name, od.ObjectQMgrName)

        q_name = rb_hash_aref(val, ID2SYM(ID_q_name));
        if (NIL_P(q_name))
        {
            rb_raise(rb_eArgError,
                     "Mandatory parameter :q_name missing from :q_name hash passed to WMQ::QueueManager#put");
        }
    }

    WMQ_STR2MQCHARS(q_name,od.ObjectName)

    Queue_extract_put_message_options(hash, &pmo);
    Message_build(&pqm->p_buffer, &pqm->buffer_size, pqm->trace_level,
                  hash, &pBuffer, &BufferLength,    &md);

    if(pqm->trace_level) printf("WMQ::QueueManager#put Queue Manager Handle:%ld\n", (long)pqm->hcon);

    pqm->MQPUT1(
           pqm->hcon,           /* connection handle               */
           &od,                 /* object descriptor               */
           &md,                 /* message descriptor              */
           &pmo,                /* put message options             */
           BufferLength,        /* message length                  */
           pBuffer,             /* message buffer                  */
           &pqm->comp_code,     /* completion code                 */
           &pqm->reason_code);  /* reason code                     */

    if(pqm->trace_level) printf("WMQ::QueueManager#put MQPUT1 ended with reason:%s\n", wmq_reason(pqm->reason_code));

    if (pqm->reason_code != MQRC_NONE)
    {
        if (pqm->exception_on_error)
        {
            VALUE qmgr_name = QueueManager_name(self);

            rb_raise(wmq_exception,
                     "WMQ::QueueManager.put(). Error writing a message to Queue:%s on Queue Manager:%s reason:%s",
                     RSTRING_PTR(q_name),
                     RSTRING_PTR(qmgr_name),
                     wmq_reason(pqm->reason_code));
        }
        return Qfalse;
    }
    else
    {
        VALUE message = rb_hash_aref(hash, ID2SYM(ID_message));
        if(!NIL_P(message))
        {
            VALUE descriptor = rb_funcall(message, ID_descriptor, 0);
            Message_from_mqmd(descriptor, &md);           /* This could be optimized to output only fields */
        }
    }

    return Qtrue;
}

/*
 * Return the completion code for the last MQ operation
 *
 * Returns => FixNum
 * * WMQ::MQCC_OK       0
 * * WMQ::MQCC_WARNING  1
 * * WMQ::MQCC_FAILED   2
 * * WMQ::MQCC_UNKNOWN  -1
 *
 */
VALUE QueueManager_comp_code(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    return LONG2NUM(pqm->comp_code);
}

/*
 * Return the reason code for the last MQ operation
 *
 * Returns => FixNum
 * * For a complete list of reason codes, please see WMQ Constants or
 *   the WebSphere MQ documentation for Reason Codes
 *
 * Note
 * * The list of Reason Codes varies depending on the version of WebSphere MQ
 *   and the operating system on which Ruby WMQ was compiled
 */
VALUE QueueManager_reason_code(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    return LONG2NUM(pqm->reason_code);
}

/*
 * Returns a textual representation of the reason_code for the last MQ operation
 *
 * Returns => String
 * * For a complete list of reasons, please see WMQ Constants or
 *   the WebSphere MQ documentation for Reason Codes
 *
 * Note
 * * The list of Reason Codes varies depending on the version of WebSphere MQ
 *   and the operating system on which Ruby WMQ was compiled
 */
VALUE QueueManager_reason(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    return rb_str_new2(wmq_reason(pqm->reason_code));
}

/*
 * Returns whether this QueueManager instance is set
 * to throw a WMQ::WMQException whenever an MQ operation fails
 *
 * Returns:
 * * true : This QueueManager instance will throw a WMQ::WMQException whenever
 *   an MQ operation fails. I.e. if comp_code != WMQ::OK.
 * * false: WMQ::WMQException will not be thrown
 *
 * Note:
 * * RuntimeError and ArgumentError exceptions are always thrown regardless of the
 *   value of exception_on_error
 */
VALUE QueueManager_exception_on_error(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    if (pqm->exception_on_error)
    {
        return Qtrue;
    }

    return Qfalse;
}

/*
 * Returns whether this QueueManager instance is currently
 * connected to a WebSphere MQ queue manager
 *
 * Returns:
 * * true : This QueueManager instance is connected to a local or remote queue manager
 * * false: This QueueManager instance is not currently connected to a local or
 *   remote queue manager
 */
VALUE QueueManager_connected_q(VALUE self)
{
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);
    if (pqm->hcon)
    {
        return Qtrue;
    }
    return Qfalse;
}

/*
 * Returns the QueueManager name => String
 */
VALUE QueueManager_name(VALUE self)
{
    return rb_iv_get(self,"@name");
}

static VALUE QueueManager_open_queue_block(VALUE message, VALUE proc)
{
    return rb_funcall(proc, ID_call, 1, message);
}

static VALUE QueueManager_open_queue_each(VALUE parameters)
{
    return Queue_singleton_open(1, &parameters, wmq_queue);
}

/*
 * call-seq:
 *   open_queue(...)
 *   access_queue(...)
 *
 * Open the specified queue, then close it once the
 * supplied code block has completed
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * See Queue.open for the complete list of parameters, except that :queue_manager is *not* required
 *   since it is supplied automatically by this method
 *
 * Example:
 *   require 'wmq/wmq_client'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)') do |qmgr|
 *     qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
 *       queue.put(:data => 'Hello World')
 *     end
 *   end
 */
VALUE QueueManager_open_queue(int argc, VALUE *argv, VALUE self)
{
    VALUE parameters;
    VALUE proc;

    /* Extract parameters and code block (Proc) */
    rb_scan_args(argc, argv, "1&", &parameters, &proc);

    Check_Type(parameters, T_HASH);
    rb_hash_aset(parameters, ID2SYM(ID_queue_manager), self);

    return rb_iterate(QueueManager_open_queue_each, parameters, QueueManager_open_queue_block, proc);
}

struct QueueManager_singleton_connect_arg {
    VALUE self;
    VALUE proc;
};

static VALUE QueueManager_singleton_connect_body2(struct QueueManager_singleton_connect_arg* arg)
{
    return rb_funcall(arg->proc, ID_call, 1, arg->self);
}

static VALUE QueueManager_singleton_connect_rescue(VALUE self)
{
    PQUEUE_MANAGER pqm;
    VALUE          exception;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    if(pqm->trace_level) printf("WMQ::QueueManager.connect() Backing out due to unhandled exception\n");
    //exception = rb_gvar_get("$!");
    exception = rb_eval_string("$!");           /* $! holds the current exception */
    if(pqm->trace_level > 1) printf("WMQ::QueueManager.connect() Exception $! extracted\n");
    QueueManager_backout(self);                       /* Backout Unit of work           */
    if(pqm->trace_level > 1) printf("WMQ::QueueManager.connect() Rethrowing Exception\n");
    rb_exc_raise(exception);                          /* Re-Raise Exception             */
    return Qnil;
}

static VALUE QueueManager_singleton_connect_body(struct QueueManager_singleton_connect_arg* arg)
{
    return rb_rescue2(QueueManager_singleton_connect_body2, (VALUE)arg,
                      QueueManager_singleton_connect_rescue, arg->self,
                      rb_eException, 0);
}

static VALUE QueueManager_singleton_connect_ensure(VALUE self)
{
    return QueueManager_disconnect(self);
}

/*
 * call-seq:
 *   connect(...)
 *
 * Connect to the queue manager, then disconnect once the supplied code block completes
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * Summary of parameters and their WebSphere MQ equivalents:
 *  WMQ::QueueManager.connect(                             # WebSphere MQ Equivalents:
 *   :q_mgr_name         => 'queue_manager name',
 *   :exception_on_error => true,                          # n/a
 *   :connect_options    => WMQ::MQCNO_FASTBATH_BINDING    # MQCNO.Options
 *
 *   :trace_level        => 0,                             # n/a
 *
 *   # Common client connection parameters
 *   :channel_name       => 'svrconn channel name',        # MQCD.ChannelName
 *   :connection_name    => 'localhost(1414)',             # MQCD.ConnectionName
 *   :transport_type     => WMQ::MQXPT_TCP,                # MQCD.TransportType
 *
 *   # Advanced client connections parameters
 *   :max_msg_length     => 65535,                         # MQCD.MaxMsgLength
 *   :security_exit      => 'Name of security exit',       # MQCD.SecurityExit
 *   :send_exit          => 'Name of send exit',           # MQCD.SendExit
 *   :receive_exit       => 'Name of receive exit',        # MQCD.ReceiveExit
 *   :security_user_data => 'Security exit User data',     # MQCD.SecurityUserData
 *   :send_user_data     => 'Send exit user data',         # MQCD.SendUserData
 *   :receive_user_data  => 'Receive exit user data',      # MQCD.ReceiveUserData
 *   :heartbeat_interval =>  1,                            # MQCD.HeartbeatInterval
 *   :remote_security_id => 'Remote Security id',          # MQCD.RemoteSecurityId
 *   :ssl_cipher_spec    => 'SSL Cipher Spec',             # MQCD.SSLCipherSpec
 *   :keep_alive_interval=> -1,                            # MQCD.KeepAliveInterval
 *   :mode_name          => 'LU6.2 Mode Name',             # MQCD.ModeName
 *   :tp_name            => 'LU6.2 Transaction pgm name',  # MQCD.TpName
 *   :user_identifier    => 'LU 6.2 Userid',               # MQCD.UserIdentifier
 *   :password           => 'LU6.2 Password',              # MQCD.Password
 *   :long_remote_user_id=> 'Long remote user identifier', # MQCD.LongRemoteUserId (Ptr, Length)
 *   :ssl_peer_name      => 'SSL Peer name',               # MQCD.SSLPeerName (Ptr, Length)
 *
 *   # SSL Options
 *   :key_repository     => '/var/mqm/qmgrs/.../key',        # MQSCO.KeyRepository
 *   :crypto_hardware    => 'GSK_ACCELERATOR_NCIPHER_NF_ON', # MQSCO.CryptoHardware
 *   )
 *
 * Optional Parameters
 * * :q_mgr_name => String
 *   * Name of the existing WebSphere MQ Queue Manager to connect to
 *
 *   * Default:
 *      - Server connections will connect to the default queue manager
 *      - Client connections will connect to whatever queue
 *        manager is found at the host and port number as specified
 *        by the connection_name
 *
 * * :exception_on_error => true or false
 *      Determines whether WMQ::WMQExceptions are thrown whenever
 *      an error occurs during a WebSphere MQ operation (connect, put, get, etc..)
 *
 *      Default: true
 *
 * * :connect_options => FixNum
 *   * One or more of the following values:
 *       WMQ::MQCNO_STANDARD_BINDING
 *       WMQ::MQCNO_FASTPATH_BINDING
 *       WMQ::MQCNO_SHARED_BINDING
 *       WMQ::MQCNO_ISOLATED_BINDING
 *       WMQ::MQCNO_ACCOUNTING_MQI_ENABLED
 *       WMQ::MQCNO_ACCOUNTING_MQI_DISABLED
 *       WMQ::MQCNO_ACCOUNTING_Q_ENABLED
 *       WMQ::MQCNO_ACCOUNTING_Q_DISABLED
 *       WMQ::MQCNO_NONE
 *
 *   * Multiple values can be or'd together. E.g.
 *       :connect_options=>WMQ::MQCNO_FASTPATH_BINDING | WMQ::MQCNO_ACCOUNTING_MQI_ENABLED
 *
 *   * Please see the WebSphere MQ MQCNO data type documentation for more details
 *      Default: WMQ::MQCNO_NONE
 *
 * * :trace_level => FixNum
 *   * Turns on low-level tracing of the WebSphere MQ API calls to stdout.
 *     * 0: No tracing
 *     * 1: MQ API tracing only (MQCONNX, MQOPEN, MQPUT, etc..)
 *     * 2: Include Ruby WMQ tracing
 *     * 3: Verbose logging (Recommended for when reporting problems in Ruby WMQ)
 *      Default: 0
 *
 * Common Client Connection Parameters (Client connections only)
 * * :connection_name => String (Mandatory for client connections)
 *   * Connection name, made up of the host name (or ip address) and the port number
 *   * E.g.
 *       'mymachine.domain.com(1414)'
 *       '192.168.0.1(1417)'
 *
 * * :channel_name => String
 *   * Name of SVRCONN channel defined on the QueueManager for Client Connections
 *   * Default Value:
 *       'SYSTEM.DEF.SVRCONN'
 *
 * * :transport_type     => WMQ::MQXPT_TCP,                # MQCD.TransportType
 *   * Valid Values:
 *       WMQ::MQXPT_LOCAL
 *       WMQ::MQXPT_LU62
 *       WMQ::MQXPT_TCP
 *       WMQ::MQXPT_NETBIOS
 *       WMQ::MQXPT_SPX
 *       WMQ::MQXPT_DECNET
 *       WMQ::MQXPT_UDP
 *
 *   * Default Value:
 *       WMQ::MQXPT_TCP
 *
 * For the Advanced Client Connection parameters, please see the WebSphere MQ documentation
 *
 * Note:
 * * If an exception is not caught in the code block, the current unit of work is
 *   automatically backed out, before disconnecting from the queue manager.
 *
 * Local Server Connection Example:
 *   require 'wmq/wmq'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
 *     qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
 *   end
 *
 * Client Connection Example:
 *   require 'wmq/wmq_client'
 *
 *   WMQ::QueueManager.connect(
 *               :channel_name    => 'SYSTEM.DEF.SVRCONN',
 *               :transport_type  => WMQ::MQXPT_TCP,
 *               :connection_name => 'localhost(1414)' ) do |qmgr|
 *     qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
 *
 *       message = WMQ::Message.new
 *       if queue.get(:message => message)
 *         puts "Data Received: #{message.data}"
 *       else
 *         puts 'No message available'
 *       end
 *     end
 *   end
 */
VALUE QueueManager_singleton_connect(int argc, VALUE *argv, VALUE self)
{
    VALUE proc, parameters, queue_manager;

    /* Extract parameters and code block (Proc) */
    rb_scan_args(argc, argv, "1&", &parameters, &proc);

    queue_manager = rb_funcall(wmq_queue_manager, ID_new, 1, parameters);
    if(!NIL_P(proc))
    {
        if(Qtrue == QueueManager_connect(queue_manager))
        {
            struct QueueManager_singleton_connect_arg arg;
            arg.self = queue_manager;
            arg.proc = proc;
            rb_ensure(QueueManager_singleton_connect_body, (VALUE)&arg,
                      QueueManager_singleton_connect_ensure, queue_manager);
        }
        else
        {
            return Qfalse;
        }
    }
    return queue_manager;
}

/* What can I say, Ruby blocks have spoilt me :)  */
#define CHECK_COMPLETION_CODE(ACTION)                                                     \
if(pqm->trace_level > 1)                                                                  \
    printf ("WMQ::QueueManager#execute() %s:%s\n", ACTION, wmq_reason(pqm->reason_code)); \
if(pqm->comp_code != MQCC_OK)                                                             \
{                                                                                         \
    if (pqm->exception_on_error)                                                          \
    {                                                                                     \
        if(pqm->trace_level)                                                              \
            printf ("WMQ::QueueManager#execute() raise WMQ::WMQException\n");             \
                                                                                          \
        rb_raise(wmq_exception,                                                           \
                 "WMQ::QueueManager#execute(). Failed:%s, reason:%s",                     \
                 ACTION,                                                                  \
                 wmq_reason(pqm->reason_code));                                           \
    }                                                                                     \
    return Qfalse;                                                                        \
}

static int QueueManager_execute_each (VALUE key, VALUE value, PQUEUE_MANAGER pqm)
{
    MQLONG selector_type, selector;
    VALUE  str;
    ID selector_id = rb_to_id(key);

    if(pqm->trace_level > 1)
    {
        str = rb_funcall(key, rb_intern("to_s"), 0);
        printf ("WMQ::QueueManager#execute_each Key:%s\n", RSTRING_PTR(str));
    }

    if (ID_command == selector_id)                    // Skip :command
    {
        return 0;
    }

    wmq_selector(selector_id, &selector_type, &selector);
    if(NIL_P(value))
    {
        pqm->mqAddInquiry(pqm->admin_bag, selector, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Adding Inquiry to the admin bag")
        return 0;
    }

    switch (selector_type)
    {
        case MQIT_INTEGER:
            if(TYPE(value) == T_SYMBOL)               /* Translate symbol to MQ selector value */
            {
                MQLONG val_selector, val_selector_type;
                wmq_selector(rb_to_id(value), &val_selector_type, &val_selector);

                pqm->mqAddInteger(pqm->admin_bag, selector, val_selector, &pqm->comp_code, &pqm->reason_code);
            }
            else
            {
                pqm->mqAddInteger(pqm->admin_bag, selector, NUM2LONG(value), &pqm->comp_code, &pqm->reason_code);
            }
            CHECK_COMPLETION_CODE("Adding Queue Type to the admin bag")
        break;

        case MQIT_STRING:
            str = StringValue(value);
            pqm->mqAddString(pqm->admin_bag, selector, MQBL_NULL_TERMINATED, RSTRING_PTR(str), &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Adding Queue name to the admin bag")
        break;

        default:
            rb_raise(rb_eArgError, "WMQ::QueueManager#execute_each Unknown selector type returned by wmq_selector()");
        break;
    }
    return 0;
}

/*
 * call-seq:
 *   execute(...)
 *
 * Execute an Administration command against the local queue manager
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * The entire MQ Administration interface has been implemented.
 *   Rather than re-documentation the hundreds of options, a standard
 *   convention has been used to map the MQ constants to Symbols in Ruby.
 *
 * For all MQ Admin commands, just drop the MQAI_ off the front and
 * convert the command name to lower case.
 * * E.g. MQAI_INQUIRE_Q becomes inquire_q
 *
 * For the hundreds of parameters, a similiar technique is followed.
 * Remove the prefixes: MQCA_, MQIA_, etc.. and convert to lowercase
 * * E.g. MQCA_Q_NAME becomes :q_name
 *
 * Example
 *   WMQ::QueueManager.connect do |qmgr|
 *     result = qmgr.execute(
 *                :command         => :inquire_q,
 *                :q_name          => 'MY.LOCAL.QUEUE',
 *                :q_type          => WMQ::MQQT_LOCAL,
 *                :current_q_depth => nil
 *                )
 *     # OR, we can replace the method name execute with the MQAI command:
 *     result = qmgr.inquire_q(
 *                :q_name          => 'MY.LOCAL.QUEUE',
 *                :q_type          => WMQ::MQQT_LOCAL,
 *                :current_q_depth => nil
 *                )
 *
 * Complete Example:
 *   require 'wmq/wmq'
 *   require 'wmq/wmq_const_admin'
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)') do |qmgr|
 *     qmgr.reset_q_stats(:q_name=>'*').each {|item| p item }
 *   end
 *
 * Some one line examples
 *   qmgr.inquire_q(:q_name=>'TEST*').each {|item| p item }
 *
 *   qmgr.inquire_q(:q_name=>'TEST*', :q_type=>WMQ::MQQT_LOCAL, :current_q_depth=>nil).each {|item| p item }
 *
 *   qmgr.inquire_process(:process_name=>'*').each {|item| p item }
 *
 *   qmgr.ping_q_mgr.each {|item| p item }
 *
 *   qmgr.refresh_security.each {|item| p item }
 *
 *   qmgr.inquire_q_status(:q_name=>'TEST*', :q_status_type=>:q_status, :q_status_attrs=>:process_id).each {|item| p item }
 *
 *   qmgr.start_channel_listener.each {|item| p item }
 *
 *   qmgr.inquire_channel_status(:channel_name=>'*').each {|item| p item }
 */
VALUE QueueManager_execute(VALUE self, VALUE hash)
{
#ifdef MQHB_UNUSABLE_HBAG
    VALUE          val;
    PQUEUE_MANAGER pqm;
    Data_Get_Struct(self, QUEUE_MANAGER, pqm);

    Check_Type(hash, T_HASH);

    if (pqm->admin_bag == MQHB_UNUSABLE_HBAG)         /* Lazy create admin bag */
    {
        pqm->mqCreateBag(MQCBO_ADMIN_BAG, &pqm->admin_bag, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Creating the admin bag")
    }
    else
    {
        pqm->mqClearBag(pqm->admin_bag, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Clearing the admin bag")
    }

    if (pqm->reply_bag == MQHB_UNUSABLE_HBAG)         /* Lazy create reply bag */
    {
        pqm->mqCreateBag(MQCBO_ADMIN_BAG, &pqm->reply_bag, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Creating the reply bag")
    }
    else
    {
        pqm->mqClearBag(pqm->reply_bag, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Clearing the reply bag")
    }

    val = rb_hash_aref(hash, ID2SYM(ID_command));     /* :command */
    if (NIL_P(val))
    {
        rb_raise(rb_eArgError, "WMQ::QueueManager#execute Mandatory parameter :command missing");
    }
    rb_hash_foreach(hash, QueueManager_execute_each, (VALUE)pqm);
    if(pqm->trace_level) printf ("WMQ::QueueManager#execute() Queue Manager Handle:%ld\n", (long)pqm->hcon);

    pqm->mqExecute(
              pqm->hcon,                              /* MQ connection handle                 */
              wmq_command_lookup(rb_to_id(val)),      /* Command to be executed               */
              MQHB_NONE,                              /* No options bag                       */
              pqm->admin_bag,                         /* Handle to bag containing commands    */
              pqm->reply_bag,                         /* Handle to bag to receive the response*/
              MQHO_NONE,                              /* Put msg on SYSTEM.ADMIN.COMMAND.QUEUE*/
              MQHO_NONE,                              /* Create a dynamic q for the response  */
              &pqm->comp_code,                        /* Completion code from the mqexecute   */
              &pqm->reason_code);                     /* Reason code from mqexecute call      */

    if(pqm->trace_level) printf("WMQ::QueueManager#execute() completed with reason:%s\n", wmq_reason(pqm->reason_code));

    if (pqm->comp_code == MQCC_OK)
    {
        MQLONG numberOfBags;                          /* number of bags in response bag  */
        MQHBAG qAttrsBag;                             /* bag containing q attributes     */
        VALUE  array;
        MQLONG size;
        MQLONG length;
        MQCHAR inquiry_buffer[WMQ_EXEC_STRING_INQ_BUFFER_SIZE];
        PMQCHAR pChar;

        MQLONG qDepth;                          /* depth of queue                  */
        MQLONG item_type;
        MQLONG selector;
        MQLONG number_of_items;
        int    bag_index, items, k;

        pqm->mqCountItems(pqm->reply_bag, MQHA_BAG_HANDLE, &numberOfBags, &pqm->comp_code, &pqm->reason_code);
        CHECK_COMPLETION_CODE("Counting number of bags returned from the command server")

        if(pqm->trace_level > 1) printf("WMQ::QueueManager#execute() %ld bags returned\n", (long)numberOfBags);
        array = rb_ary_new2(numberOfBags);

        for ( bag_index=0; bag_index<numberOfBags; bag_index++)               /* For each bag, extract the queue depth */
        {
            hash = rb_hash_new();

            pqm->mqInquireBag(pqm->reply_bag, MQHA_BAG_HANDLE, bag_index, &qAttrsBag, &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Inquiring for the attribute bag handle")

            pqm->mqCountItems(qAttrsBag, MQSEL_ALL_SELECTORS, &number_of_items, &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Counting number of items in this bag")

            if(pqm->trace_level > 1) printf("WMQ::QueueManager#execute() Bag %d contains %ld items\n", bag_index, (long)number_of_items);

            for (items=0; items<number_of_items; items++) /* For each item, extract it's value */
            {
                pqm->mqInquireItemInfo(
                                  qAttrsBag,               /* I: Bag handle */
                                  MQSEL_ANY_SELECTOR,      /* I: Item selector */
                                  items,                   /* I: Item index */
                                  &selector,               /* O: Selector of item */
                                  &item_type,              /* O: Data type of item */
                                  &pqm->comp_code,
                                  &pqm->reason_code);
                CHECK_COMPLETION_CODE("Inquiring Item details")

                if (selector > 0)                     /* Skip system selectors */
                {
                    switch (item_type)
                    {
                        case MQIT_INTEGER:
                            pqm->mqInquireInteger(qAttrsBag, MQSEL_ALL_SELECTORS, items, &qDepth, &pqm->comp_code, &pqm->reason_code);
                            CHECK_COMPLETION_CODE("Inquiring Integer item")

                            if(pqm->trace_level > 1)
                                printf("WMQ::QueueManager#execute() Item %d: Integer:%ld, selector:%ld\n", items, (long)qDepth, (long)selector);

                            rb_hash_aset(hash, ID2SYM(wmq_selector_id(selector)), LONG2NUM(qDepth));
                        break;

                        case MQIT_STRING:
                            pqm->mqInquireString(qAttrsBag, MQSEL_ALL_SELECTORS, items, WMQ_EXEC_STRING_INQ_BUFFER_SIZE-1, inquiry_buffer,
                                            &size, NULL, &pqm->comp_code, &pqm->reason_code);
                            if(pqm->trace_level > 2)
                                printf("WMQ::QueueManager#execute() mqInquireString buffer size: %d, string size:%ld\n",
                                    WMQ_EXEC_STRING_INQ_BUFFER_SIZE,(long)size);
                            CHECK_COMPLETION_CODE("Inquiring String item")

                            length = 0;
                            pChar = inquiry_buffer + size-1;
                            for (k = size; k > 0; k--)
                            {
                                if (*pChar != ' ' && *pChar != 0)
                                {
                                    length = k;
                                    break;
                                }
                                pChar--;
                            }
                            rb_hash_aset(hash, ID2SYM(wmq_selector_id(selector)), rb_str_new(inquiry_buffer, length));

                            if(pqm->trace_level > 1)
                            {
                                inquiry_buffer[length] = '\0';
                                printf("WMQ::QueueManager#execute() Item %d: String:'%s', selector:%ld\n",
                                        items, inquiry_buffer, (long)selector);
                            }
                        break;

                        case MQIT_BAG:
                            printf("Ignoring Bag at this level\n");
                        break;

                        default:
                            printf("Ignoring Unknown type:%ld\n", (long)item_type);
                        break;
                    }
                }
            }
            rb_ary_push(array, hash);
        }
        return array;
    }
    else
    {
        VALUE name = rb_iv_get(self,"@name");
        name = StringValue(name);

        if (pqm->reason_code == MQRCCF_COMMAND_FAILED)
        {
            /* Find out why admin command failed */
            MQLONG result_comp_code, result_reason_code;
            MQHBAG result_bag;

            pqm->mqInquireBag(pqm->reply_bag, MQHA_BAG_HANDLE, 0, &result_bag, &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Getting the result bag handle")

            pqm->mqInquireInteger(result_bag, MQIASY_COMP_CODE, MQIND_NONE, &result_comp_code, &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Getting the completion code from the result bag")

            pqm->mqInquireInteger(result_bag, MQIASY_REASON, MQIND_NONE, &result_reason_code, &pqm->comp_code, &pqm->reason_code);
            CHECK_COMPLETION_CODE("Getting the reason code from the result bag")

            pqm->comp_code   = result_comp_code;
            pqm->reason_code = result_reason_code;

            if(pqm->trace_level)
                printf("WMQ::QueueManager#execute() Error returned by command server:%s\n", wmq_reason(pqm->reason_code));
        }

        if (pqm->exception_on_error)
        {
            if (pqm->reason_code == MQRC_CMD_SERVER_NOT_AVAILABLE)
            {
                rb_raise(wmq_exception,
                        "WMQ::QueueManager#execute(). Please start the WebSphere MQ Command Server : 'strmqcsv %s', reason:%s",
                        RSTRING_PTR(name),
                        wmq_reason(pqm->reason_code));
            }
            else
            {
                rb_raise(wmq_exception,
                        "WMQ::QueueManager#execute(). Error executing admin command on Queue Manager:%s, reason:%s",
                        RSTRING_PTR(name),
                        wmq_reason(pqm->reason_code));
            }
        }
        return Qfalse;
    }
    return Qnil;
#else
    rb_notimplement();
    return Qfalse;
#endif
}
