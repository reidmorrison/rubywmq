/* --------------------------------------------------------------------------
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
 * --------------------------------------------------------------------------*/

#include "wmq.h"
/* --------------------------------------------------
 * Initialize Ruby ID's for Queue Class
 *
 * This function is called when the library is loaded
 * by ruby
 * --------------------------------------------------*/
static ID ID_new;
static ID ID_call;
static ID ID_close;
static ID ID_input;
static ID ID_input_shared;
static ID ID_input_exclusive;
static ID ID_output;
static ID ID_browse;
static ID ID_sync;
static ID ID_new_id;
static ID ID_new_msg_id;
static ID ID_new_correl_id;
static ID ID_convert;
static ID ID_wait;
static ID ID_q_name;
static ID ID_q_mgr_name;
static ID ID_match;
static ID ID_options;
static ID ID_open_options;
static ID ID_mode;
static ID ID_fail_if_quiescing;
static ID ID_queue_manager;
static ID ID_dynamic_q_name;
static ID ID_close_options;
static ID ID_fail_if_exists;
static ID ID_alternate_user_id;
static ID ID_alternate_security_id;
static ID ID_message;
static ID ID_descriptor;

void Queue_id_init()
{
    ID_new             = rb_intern("new");
    ID_call            = rb_intern("call");
    ID_close           = rb_intern("close");

    ID_input           = rb_intern("input");
    ID_input_shared    = rb_intern("input_shared");
    ID_input_exclusive = rb_intern("input_exclusive");
    ID_output          = rb_intern("output");
    ID_browse          = rb_intern("browse");
    ID_q_name          = rb_intern("q_name");
    ID_q_mgr_name      = rb_intern("q_mgr_name");
    ID_queue_manager   = rb_intern("queue_manager");

    ID_sync            = rb_intern("sync");
    ID_new_id          = rb_intern("new_id");
    ID_new_msg_id      = rb_intern("new_msg_id");
    ID_new_correl_id   = rb_intern("new_correl_id");
    ID_convert         = rb_intern("convert");
    ID_wait            = rb_intern("wait");
    ID_match           = rb_intern("match");
    ID_options         = rb_intern("options");
    ID_open_options    = rb_intern("open_options");
    ID_mode            = rb_intern("mode");

    ID_message         = rb_intern("message");
    ID_descriptor      = rb_intern("descriptor");

    ID_fail_if_quiescing     = rb_intern("fail_if_quiescing");
    ID_dynamic_q_name        = rb_intern("dynamic_q_name");
    ID_close_options         = rb_intern("close_options");
    ID_fail_if_exists        = rb_intern("fail_if_exists");
    ID_alternate_security_id = rb_intern("alternate_security_id");
    ID_alternate_user_id     = rb_intern("alternate_user_id");
}

 typedef struct tagQUEUE QUEUE;
 typedef QUEUE MQPOINTER PQUEUE;

 struct tagQUEUE {
    MQHOBJ   hobj;                    /* object handle                 */
    MQHCONN  hcon;                    /* connection handle             */
    MQOD     od;                      /* Object Descriptor             */
    MQLONG   comp_code;               /* completion code               */
    MQLONG   reason_code;             /* reason code for MQCONN        */
    MQLONG   open_options;            /* MQOPEN options                */
    MQLONG   close_options;           /* MQCLOSE options               */
    MQLONG   exception_on_error;      /* Non-Zero means throw exception*/
    MQLONG   fail_if_exists;          /* Non-Zero means open dynamic_q_name directly */
    MQLONG   trace_level;             /* Trace level. 0==None, 1==Info 2==Debug ..*/
    MQCHAR   q_name[MQ_Q_NAME_LENGTH+1]; /* queue name plus null character */
    PMQBYTE  p_buffer;                /* message buffer                */
    MQLONG   buffer_size;             /* Allocated size of buffer      */

    void(*MQCLOSE)(MQHCONN,PMQHOBJ,MQLONG,PMQLONG,PMQLONG);
    void(*MQGET)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG,PMQLONG);
    void(*MQPUT)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG);
 };

/* --------------------------------------------------
 * C Structure to store MQ data types and other
 *   C internal values
 * --------------------------------------------------*/
void QUEUE_free(void* p)
{
    PQUEUE pq = (PQUEUE)p;
    if(pq->trace_level) printf("WMQ::Queue Freeing QUEUE structure\n");

    if (pq->hobj)  /* Valid Q handle means MQCLOSE was not called */
    {
        printf("WMQ::Queue#close was not called. Automatically calling close()\n");
        pq->MQCLOSE(pq->hcon, &pq->hobj, pq->close_options, &pq->comp_code, &pq->reason_code);
    }
    free(pq->p_buffer);
    free(p);
}

VALUE QUEUE_alloc(VALUE klass)
{
    static MQOD default_MQOD = {MQOD_DEFAULT};
    PQUEUE pq = ALLOC(QUEUE);

    pq->hobj = 0;
    pq->hcon = 0;
    memcpy(&pq->od, &default_MQOD, sizeof(MQOD));
    pq->comp_code = 0;
    pq->reason_code = 0;
    pq->open_options = 0;
    pq->close_options = MQCO_NONE;
    pq->exception_on_error = 1;
    pq->trace_level = 0;
    pq->fail_if_exists = 1;
    memset(&pq->q_name, 0, sizeof(pq->q_name));
    pq->buffer_size = 16384;
    pq->p_buffer = ALLOC_N(unsigned char, pq->buffer_size);

    return Data_Wrap_Struct(klass, 0, QUEUE_free, pq);
}

static MQLONG Queue_extract_open_options(VALUE hash, VALUE name)
{
    VALUE          val;
    MQLONG         flag;
    MQLONG         mq_open_options = 0;

    WMQ_HASH2MQLONG(hash,open_options,      mq_open_options)

    val = rb_hash_aref(hash, ID2SYM(ID_mode));             /* :mode */
    if (!NIL_P(val))
    {
        ID mode_id = rb_to_id(val);

        if(mode_id == ID_output)               mq_open_options |= MQOO_OUTPUT;
        else if(mode_id == ID_input)           mq_open_options |= MQOO_INPUT_AS_Q_DEF;
        else if(mode_id == ID_input_shared)    mq_open_options |= MQOO_INPUT_SHARED;
        else if(mode_id == ID_input_exclusive) mq_open_options |= MQOO_INPUT_EXCLUSIVE;
        else if(mode_id == ID_browse)          mq_open_options |= MQOO_BROWSE;
        else
        {
            rb_raise(rb_eArgError,
                     "Unknown mode supplied for Queue:%s",
                     RSTRING_PTR(name));
        }
    }
    else if (!mq_open_options)
    {
        rb_raise(rb_eArgError,
                 "Either :mode or :options is required. Both are missing from hash passed to initialize() for Queue: %s",
                 RSTRING_PTR(name));
    }

    IF_TRUE(fail_if_quiescing, 1)                     /* Defaults to true */
    {
        mq_open_options |= MQOO_FAIL_IF_QUIESCING;
    }

    return mq_open_options;
}

void Queue_extract_put_message_options(VALUE hash, PMQPMO ppmo)
{
    VALUE    val;
    MQLONG   flag;

    WMQ_HASH2MQLONG(hash,options, ppmo->Options)

    IF_TRUE(sync, 0)                                  /* :sync */
    {
        ppmo->Options |= MQPMO_SYNCPOINT;
    }

    IF_TRUE(fail_if_quiescing, 1)                     /* Defaults to true */
    {
        ppmo->Options |= MQPMO_FAIL_IF_QUIESCING;
    }

    IF_TRUE(new_id, 0)                                /* :new_id */
    {
        ppmo->Options |= MQPMO_NEW_MSG_ID;
        ppmo->Options |= MQPMO_NEW_CORREL_ID;
    }
    else
    {
        IF_TRUE(new_msg_id, 0)                        /* :new_msg_id */
        {
            ppmo->Options |= MQPMO_NEW_MSG_ID;
        }

        IF_TRUE(new_correl_id, 0)                     /* new_correl_id */
        {
            ppmo->Options |= MQPMO_NEW_CORREL_ID;
        }
    }
    return;
}

/* Future Use:
 *  *:q_name             => ['q_name1', 'q_name2'] # Not implemented: Future Use!!
 *  *:q_name             => [ {queue_manager=>'QMGR_name',
 *                            queue        =>'queue name1'}, # Future Use!!
 *                            { ... }
 *                          ]
 *  *:resolved           => { queue => 'Resolved queue name',
 *                           queue_manager => 'Resolved queue manager name' }
 */

/*
 * call-seq:
 *   new(...)
 *
 * Note:
 * * It is _not_ recommended to create instances of Queue directly, rather user Queue.open. Which
 *   creates the queue, opens the queue, executes a supplied code block and then ensures the queue
 *   is closed.
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * See Queue.open for details on all parameters
 *
 */
VALUE Queue_initialize(VALUE self, VALUE hash)
{
    VALUE   str;
    size_t  size;
    size_t  length;
    VALUE   val;
    VALUE   q_name;
    PQUEUE  pq;

    Check_Type(hash, T_HASH);

    Data_Get_Struct(self, QUEUE, pq);

    val = rb_hash_aref(hash, ID2SYM(ID_queue_manager));    /* :queue_manager */
    if (NIL_P(val))
    {
        rb_raise(rb_eArgError, "Mandatory parameter :queue_manager missing from WMQ::Queue::new");
    }
    else
    {
        PQUEUE_MANAGER pqm;
        Data_Get_Struct(val, QUEUE_MANAGER, pqm);
        pq->exception_on_error = pqm->exception_on_error;  /* Copy exception_on_error from Queue Manager setting */
        pq->trace_level        = pqm->trace_level;  /* Copy trace_level from Queue Manager setting */

        rb_iv_set(self, "@queue_manager", val);
    }

    q_name = rb_hash_aref(hash, ID2SYM(ID_q_name));            /* :q_name */
    if (NIL_P(q_name))
    {
        rb_raise(rb_eArgError, "Mandatory parameter :q_name missing from WMQ::Queue::new");
    }

    /* --------------------------------------------------
     * If :q_name is a hash, extract :q_name and :q_mgr_name
     * --------------------------------------------------*/
    if(TYPE(q_name) == T_HASH)
    {
        if(pq->trace_level)
            printf ("WMQ::Queue::new q_name is a hash\n");

        WMQ_HASH2MQCHARS(q_name,q_mgr_name, pq->od.ObjectQMgrName)

        q_name = rb_hash_aref(q_name, ID2SYM(ID_q_name));
        if (NIL_P(q_name))
        {
            rb_raise(rb_eArgError,
                        "Mandatory parameter :q_name missing from :q_name hash passed to WMQ::Queue::new");
        }
    }

    str = StringValue(q_name);
    rb_iv_set(self, "@original_name", str);           /* Store original queue name */
    strncpy(pq->q_name, RSTRING_PTR(str), sizeof(pq->q_name));

    pq->open_options = Queue_extract_open_options(hash, q_name);

    if(pq->trace_level > 1) printf("WMQ::Queue::new Queue:%s\n", pq->q_name);

    val = rb_hash_aref(hash, ID2SYM(ID_dynamic_q_name));   /* :dynamic_q_name */
    rb_iv_set(self, "@dynamic_q_name", val);

    WMQ_HASH2MQBYTES(hash,alternate_security_id,         pq->od.AlternateSecurityId)
    WMQ_HASH2MQLONG(hash,close_options,                 pq->close_options)
    WMQ_HASH2BOOL(hash,fail_if_exists,                pq->fail_if_exists)

    val = rb_hash_aref(hash, ID2SYM(ID_alternate_user_id)); /* :alternate_user_id */
    if (!NIL_P(val))
    {
        WMQ_HASH2MQCHARS(hash,alternate_user_id,         pq->od.AlternateUserId)
        pq->open_options |= MQOO_ALTERNATE_USER_AUTHORITY;
    }

    return Qnil;
}

/*
 * call-seq:
 *   open()
 *
 * Open the queue
 *
 * Note:
 * * It is not recommended to use this method to open a queue, since the queue will
 *   have to be closed explicitly.
 * * Rather use WMQ::QueueManager#open_queue
 * * If the queue is already open, it will be closed and re-opened.
 *   Any errors that occur while closing the queue are ignored.
 * * Custom behavior for Dynamic Queues:
 *     When :dynamic_q_name is supplied and MQ fails to
 *     open the queue with MQRC_OBJECT_ALREADY_EXISTS,
 *     this method will automatically open the existing
 *     queue by replacing the queue name with :dynamic_q_name
 *
 *     This technique allows programs to dynamically create
 *     queues, without being concerned with first checking if
 *     the queue is already defined.
 *       I.e. Removes the need to have to explicitly create
 *            required queues in advance
 *     However, in order for this approach to work a
 *     Permanent model queue must be used. A Temporary
 *     model queue is automatically erased by WMQ when the
 *     queue is closed.
 *
 *     Persistent messages cannot be put to a
 *     temporary dynamic queue!
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
 *
 * Example:
 *   require 'wmq/wmq_client'
 *   queue_manager = WMQ::QueueManager.new(:q_mgr_name     =>'REID',
 *                                         :connection_name=>'localhost(1414)')
 *   begin
 *     queue_manager.connect
 *
 *     # Create Queue and clear any messages from the queue
 *     in_queue = WMQ::Queue.new(:queue_manager =>queue_manager,
 *                               :mode          =>:input,
 *                               :dynamic_q_name=>'UNIT.TEST',
 *                               :q_name        =>'SYSTEM.DEFAULT.MODEL.QUEUE',
 *                               :fail_if_exists=>false)
 *     begin
 *       in_queue.open
 *       in_queue.each { |message| p message.data }
 *     ensure
 *       # Note: Very important: Must close the queue explicitly
 *       in_queue.close
 *     end
 *   rescue => exc
 *     queue_manager.backout
 *     raise exc
 *   ensure
 *     # Note: Very important: Must disconnect from the queue manager explicitly
 *     queue_manager.disconnect
 *   end
 */
VALUE Queue_open(VALUE self)
{
    VALUE          name;
    VALUE          val;
    VALUE          dynamic_q_name;
    MQOD           od = {MQOD_DEFAULT};    /* Object Descriptor             */
    VALUE          queue_manager;
    PQUEUE_MANAGER pqm;
    PQUEUE         pq;
    Data_Get_Struct(self, QUEUE, pq);

    name = rb_iv_get(self,"@original_name");          /* Always open original name */
    if (NIL_P(name))
    {
        rb_raise(rb_eRuntimeError, "Fatal: Queue Name not found in Queue instance");
    }
    name = StringValue(name);

    strncpy(od.ObjectName, RSTRING_PTR(name), (size_t)MQ_Q_NAME_LENGTH);

    dynamic_q_name = rb_iv_get(self,"@dynamic_q_name");
    if (!NIL_P(dynamic_q_name))
    {
        val = StringValue(dynamic_q_name);
        strncpy(od.DynamicQName, RSTRING_PTR(dynamic_q_name), (size_t) MQ_Q_NAME_LENGTH);
        if(pq->trace_level>1) printf("WMQ::Queue#open() Using dynamic queue name:%s\n", RSTRING_PTR(dynamic_q_name));
    }

    queue_manager = rb_iv_get(self,"@queue_manager");
    if (NIL_P(queue_manager))
    {
        rb_raise(rb_eRuntimeError, "Fatal: Queue Manager object not found in Queue instance");
    }
    Data_Get_Struct(queue_manager, QUEUE_MANAGER, pqm);
    pq->MQCLOSE= pqm->MQCLOSE;
    pq->MQGET  = pqm->MQGET;
    pq->MQPUT  = pqm->MQPUT;

    pq->hcon = pqm->hcon;                             /* Store Queue Manager handle for subsequent calls */

    if(pq->trace_level)
        printf ("WMQ::Queue#open() Opening Queue:%s, Queue Manager Handle:%ld\n", RSTRING_PTR(name), (long)pq->hcon);

    if(pq->hobj)                                      /* Close queue if already open, ignore errors */
    {
        if(pq->trace_level)
            printf ("WMQ::Queue#open() Queue:%s Already open, closing it!\n", RSTRING_PTR(name));

        pqm->MQCLOSE(pq->hcon, &pq->hobj, pq->close_options, &pq->comp_code, &pq->reason_code);
    }

    pqm->MQOPEN(pq->hcon, &od, pq->open_options, &pq->hobj, &pq->comp_code, &pq->reason_code);

    /* --------------------------------------------------
     * If the Dynamic Queue already exists, just open the
     * dynamic queue name directly
     * --------------------------------------------------*/
    if (pq->reason_code == MQRC_OBJECT_ALREADY_EXISTS &&
        !pq->fail_if_exists &&
        !NIL_P(dynamic_q_name))
    {
        strncpy(od.ObjectName, od.DynamicQName, (size_t) MQ_Q_MGR_NAME_LENGTH);
        od.DynamicQName[0] = 0;

        if(pq->trace_level)
            printf("WMQ::Queue#open() Queue already exists, re-trying with queue name:%s\n",
                   RSTRING_PTR(dynamic_q_name));

        pqm->MQOPEN(pq->hcon, &od, pq->open_options, &pq->hobj, &pq->comp_code, &pq->reason_code);
    }

    if(pq->trace_level)
        printf("WMQ::Queue#open() MQOPEN completed with reason:%s, Handle:%ld\n",
               wmq_reason(pq->reason_code),
               (long)pq->hobj);

    if (pq->comp_code == MQCC_FAILED)
    {
        pq->hobj = 0;
        pq->hcon = 0;

        if (pq->exception_on_error)
        {
            VALUE name = rb_iv_get(self,"@original_name");
            name = StringValue(name);

            rb_raise(wmq_exception,
                     "WMQ::Queue#open(). Error opening Queue:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pq->reason_code));
        }
        return Qfalse;
    }
    else
    {
        size_t size;
        size_t length;
        size_t i;
        char*  pChar;

        WMQ_MQCHARS2STR(od.ObjectName, val)
        rb_iv_set(self, "@name", val);                /* Store actual queue name E.g. Dynamic Queue */

        if(pq->trace_level>1) printf("WMQ::Queue#open() Actual Queue Name opened:%s\n", RSTRING_PTR(val));
    }

    /* Future Use:
    WMQ_MQCHARS2HASH(hash,resolved_q_name,                pmqod->ResolvedQName)
    WMQ_MQCHARS2HASH(hash,resolved_q_mgr_name,            pmqod->ResolvedQMgrName)
    */

    return Qtrue;
}

/*
 * Close the queue
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
VALUE Queue_close(VALUE self)
{
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);

    /* Check if queue is open */
    if (!pq->hcon)
    {
        if(pq->trace_level) printf ("WMQ::Queue#close() Queue not open\n");
        return Qtrue;
    }

    if(pq->trace_level) printf ("WMQ::Queue#close() Queue Handle:%ld, Queue Manager Handle:%ld\n", (long)pq->hobj, (long)pq->hcon);

    pq->MQCLOSE(pq->hcon, &pq->hobj, pq->close_options, &pq->comp_code, &pq->reason_code);

    pq->hcon = 0; /* Every time the queue is opened, the qmgr handle must be fetched again! */
    pq->hobj = 0;

    if(pq->trace_level) printf("WMQ::Queue#close() MQCLOSE ended with reason:%s\n", wmq_reason(pq->reason_code));

    if (pq->comp_code == MQCC_FAILED)
    {
        if (pq->exception_on_error)
        {
            VALUE name = Queue_name(self);

            rb_raise(wmq_exception,
                     "WMQ::Queue#close(). Error closing Queue:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pq->reason_code));
        }
        return Qfalse;
    }

    return Qtrue;
}

/*
 * call-seq:
 *   get(...)
 *
 * Get a message from the opened queue
 *
 * Parameters:
 * * a Hash consisting of one or more of the named parameters
 * * Summary of parameters and their WebSphere MQ equivalents:
 *  queue.get(                                             # WebSphere MQ Equivalents:
 *   :message            => my_message,                    # n/a : Instance of Message
 *   :sync               => false,                         # MQGMO_SYNCPOINT
 *   :wait               => 0,                             # MQGMO_WAIT, duration in ms
 *   :match              => WMQ::MQMO_NONE,                # MQMO_*
 *   :convert            => false,                         # MQGMO_CONVERT
 *   :fail_if_quiescing  => true                           # MQOO_FAIL_IF_QUIESCING
 *   :options            => WMQ::MQGMO_FAIL_IF_QUIESCING   # MQGMO_*
 *   )
 *
 * Mandatory Parameters
 * * :message => Message
 *   * An instance of the WMQ::Message
 *
 * Optional Parameters
 * * :sync => true or false
 *   * Determines whether the get is performed under synchpoint.
 *     I.e. Under the current unit of work
 *      Default: false
 *
 * * :wait => FixNum
 *   * The time in milli-seconds to wait for a message if one is not immediately available
 *     on the queue
 *   * Note: Under the covers the put option MQGMO_WAIT is automatically set when :wait
 *     is supplied
 *      Default: Wait forever
 *
 * * :match => FixNum
 *   * One or more of the following values:
 *       WMQ::MQMO_MATCH_MSG_ID
 *       WMQ::MQMO_MATCH_CORREL_ID
 *       WMQ::MQMO_MATCH_GROUP_ID
 *       WMQ::MQMO_MATCH_MSG_SEQ_NUMBER
 *       WMQ::MQMO_MATCH_OFFSET
 *       WMQ::MQMO_MATCH_MSG_TOKEN
 *       WMQ::MQMO_NONE
 *   * Multiple values can be or'd together. E.g.
 *       :match=>WMQ::MQMO_MATCH_MSG_ID | WMQ::MQMO_MATCH_CORREL_ID
 *   * Please see the WebSphere MQ documentation for more details on the above options
 *      Default: WMQ::MQMO_MATCH_MSG_ID | WMQ::MQMO_MATCH_CORREL_ID
 *
 * * :convert => true or false
 *   * When true, convert results in messages being converted to the local code page.
 *     E.g. When an EBCDIC text message from a mainframe is received, it will be converted
 *     to ASCII before passing the message data to the application.
 *      Default: false
 *
 * * :fail_if_quiescing => true or false
 *   * Determines whether the WMQ::Queue#get call will fail if the queue manager is
 *     in the process of being quiesced.
 *   * Note: This interface differs from other WebSphere MQ interfaces,
 *     they do not default to true.
 *      Default: true
 *
 * * :options => Fixnum (Advanced MQ Use only)
 *   * Numeric field containing any of the MQ Get message options or'd together
 *     * E.g. :options => WMQ::MQGMO_SYNCPOINT_IF_PERSISTENT | WMQ::MQGMO_MARK_SKIP_BACKOUT
 *   * Note: If :options is supplied, it is applied first, then the above parameters are
 *     applied afterwards.
 *   * One or more of the following values:
 *       WMQ::MQGMO_SYNCPOINT_IF_PERSISTENT
 *       WMQ::MQGMO_NO_SYNCPOINT
 *       WMQ::MQGMO_MARK_SKIP_BACKOUT
 *       WMQ::MQGMO_BROWSE_FIRST
 *       WMQ::MQGMO_BROWSE_NEXT
 *       WMQ::MQGMO_BROWSE_MSG_UNDER_CURSOR
 *       WMQ::MQGMO_MSG_UNDER_CURSOR
 *       WMQ::MQGMO_LOCK
 *       WMQ::MQGMO_UNLOCK
 *       WMQ::MQGMO_LOGICAL_ORDER
 *       WMQ::MQGMO_COMPLETE_MSG
 *       WMQ::MQGMO_ALL_MSGS_AVAILABLE
 *       WMQ::MQGMO_ALL_SEGMENTS_AVAILABLE
 *       WMQ::MQGMO_DELETE_MSG
 *       WMQ::MQGMO_NONE
 *   * Please see the WebSphere MQ documentation for more details on the above options
 *      Default: WMQ::MQGMO_NONE
 *
 * Returns:
 * * true : On Success
 * * false: On Failure, or if no message was found on the queue during the wait interval
 *
 *   comp_code and reason_code are also updated.
 *   reason will return a text description of the reason_code
 *
 * Throws:
 * * WMQ::WMQException if comp_code == MQCC_FAILED
 * * Except if :exception_on_error => false was supplied as a parameter
 *   to QueueManager.new
 *
 * Example:
 *   require 'wmq/wmq'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
 *     qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
 *       message = WMQ::Message.new
 *       if queue.get(:message => message)
 *         puts "Data Received: #{message.data}"
 *       else
 *         puts 'No message available'
 *       end
 *     end
 *   end
 */
VALUE Queue_get(VALUE self, VALUE hash)
{
    VALUE    val;
    VALUE    message;
    PQUEUE   pq;
    MQLONG   flag;
    MQLONG   messlen;                /* message length received       */

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */

    md.Version = MQMD_CURRENT_VERSION;   /* Allow Group Options       */
    gmo.Version = MQGMO_CURRENT_VERSION; /* Allow MatchOptions        */

    Check_Type(hash, T_HASH);

    Data_Get_Struct(self, QUEUE, pq);

    /* Automatically open the queue if not already open */
    if (!pq->hcon && (Queue_open(self) == Qfalse))
    {
        return Qfalse;
    }

    message = rb_hash_aref(hash, ID2SYM(ID_message));
    if (NIL_P(message))
    {
        VALUE name = Queue_name(self);

        rb_raise(rb_eArgError,
                 "Mandatory key :message is missing from hash passed to get() for Queue: %s",
                 RSTRING_PTR(name));
    }

    Message_build_mqmd(message, &md);

    WMQ_HASH2MQLONG(hash,options, gmo.Options)          /* :options */

    IF_TRUE(sync, 0)                                  /* :sync defaults to false */
    {
        gmo.Options |= MQGMO_SYNCPOINT;
    }

    IF_TRUE(fail_if_quiescing, 1)                     /* :fail_if_quiescing defaults to true */
    {
        gmo.Options |= MQGMO_FAIL_IF_QUIESCING;
    }

    IF_TRUE(convert, 0)                               /* :convert defaults to false */
    {
        gmo.Options |= MQGMO_CONVERT;
    }

    val = rb_hash_aref(hash, ID2SYM(ID_wait));       /* :wait */
    if (!NIL_P(val))
    {
        gmo.Options |= MQGMO_WAIT;
        gmo.WaitInterval = NUM2LONG(val);
    }

    WMQ_HASH2MQLONG(hash,match, gmo.MatchOptions)                  /* :match */

    if(pq->trace_level > 1) printf("WMQ::Queue#get() Get Message Option: MatchOptions=%ld\n", (long)gmo.MatchOptions);
    if(pq->trace_level) printf("WMQ::Queue#get() Queue Handle:%ld, Queue Manager Handle:%ld\n", (long)pq->hobj, (long)pq->hcon);

    /* If descriptor is re-used

     md.Encoding       = MQENC_NATIVE;
     md.CodedCharSetId = MQCCSI_Q_MGR;
    */

    /*
     * Auto-Grow buffer size
     *
     * Note: If msg size is 70,000, we grow to 70,000, but then another program gets that
     *       message. The next message could be say 80,000 bytes in size, we need to
     *       grow the buffer again.
     */
    do
    {
        pq->MQGET(
              pq->hcon,            /* connection handle                 */
              pq->hobj,            /* object handle                     */
              &md,                 /* message descriptor                */
              &gmo,                /* get message options               */
              pq->buffer_size,     /* message buffer size               */
              pq->p_buffer,        /* message buffer                    */
              &messlen,            /* message length                    */
              &pq->comp_code,      /* completion code                   */
              &pq->reason_code);   /* reason code                       */

        /* report reason, if any     */
        if (pq->reason_code != MQRC_NONE)
        {
            if(pq->trace_level>1) printf("WMQ::Queue#get() Growing buffer size from %ld to %ld\n", (long)pq->buffer_size, (long)messlen);
            /* TODO: Add support for autogrow buffer here */
            if (pq->reason_code == MQRC_TRUNCATED_MSG_FAILED)
            {
                if(pq->trace_level>2)
                    printf ("WMQ::Queue#reallocate Resizing buffer from %ld to %ld bytes\n", (long)pq->buffer_size, (long)messlen);

                free(pq->p_buffer);
                pq->buffer_size = messlen;
                pq->p_buffer = ALLOC_N(unsigned char, messlen);
            }
        }
    }
    while (pq->reason_code == MQRC_TRUNCATED_MSG_FAILED);

    if(pq->trace_level) printf("WMQ::Queue#get() MQGET ended with reason:%s\n", wmq_reason(pq->reason_code));

    if (pq->comp_code != MQCC_FAILED)
    {
        Message_deblock(message, &md, pq->p_buffer, messlen, pq->trace_level);  /* Extract MQMD and any other known MQ headers */
        return Qtrue;
    }
    else
    {
        Message_clear(message);

        /* --------------------------------------------------
         * Do not throw exception when no more messages to be read
         * --------------------------------------------------*/
        if (pq->exception_on_error && (pq->reason_code != MQRC_NO_MSG_AVAILABLE))
        {
            VALUE name = Queue_name(self);

            rb_raise(wmq_exception,
                     "WMQ::Queue#get(). Error reading a message from Queue:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pq->reason_code));
        }
        return Qfalse;
    }
}

/*
 * call-seq:
 *   put(...)
 *
 * Put a message to the WebSphere MQ queue
 *
 * Parameters:
 * * A Hash consisting of one or more of the named parameters
 * * Summary of parameters and their WebSphere MQ equivalents
 *  queue.put(                                             # WebSphere MQ Equivalents:
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
 * Mandatory Parameters:
 *
 * * Either :message or :data must be supplied
 *   * If both are supplied, then :data will be written to the queue. The data in :message
 *     will be ignored
 *
 * Optional Parameters:
 * * :data => String
 *   * Data to be written to the queue. Can be binary or text data
 *   * Takes precendence over the data in :message
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
 *
 * Example:
 *   require 'wmq/wmq_client'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)') do |qmgr|
 *     qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
 *
 *       # First message
 *       queue.put(:data => 'Hello World')
 *
 *       # Set Format of message to string
 *       message = WMQ::Message.new
 *       message.descriptor[:format] = WMQ::MQFMT_STRING
 *       queue.put(:message=>message, :data => 'Hello Again')
 *
 *       # Or, pass the data in the message
 *       message = WMQ::Message.new
 *       message.descriptor[:format] = WMQ::MQFMT_STRING
 *       message.data = 'Hello Again'
 *       queue.put(:message=>message)
 *     end
 *   end
 */
VALUE Queue_put(VALUE self, VALUE hash)
{
    MQPMO    pmo = {MQPMO_DEFAULT};  /* put message options           */
    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    PQUEUE   pq;
    MQLONG   BufferLength = 0;       /* Length of the message in Buffer */
    PMQVOID  pBuffer = 0;            /* Message data                  */

    md.Version = MQMD_CURRENT_VERSION;   /* Allow Group Options       */

    Check_Type(hash, T_HASH);

    Data_Get_Struct(self, QUEUE, pq);

    /* Automatically open the queue if not already open */
    if (!pq->hcon && (Queue_open(self) == Qfalse))
    {
        return Qfalse;
    }

    Queue_extract_put_message_options(hash, &pmo);
    Message_build(&pq->p_buffer,  &pq->buffer_size, pq->trace_level,
                  hash, &pBuffer, &BufferLength,    &md);

    if(pq->trace_level) printf("WMQ::Queue#put() Queue Handle:%ld, Queue Manager Handle:%ld\n", (long)pq->hobj, (long)pq->hcon);

    pq->MQPUT(
          pq->hcon,            /* connection handle               */
          pq->hobj,            /* object handle                   */
          &md,                 /* message descriptor              */
          &pmo,                /* put message options             */
          BufferLength,        /* message length                  */
          pBuffer,             /* message buffer                  */
          &pq->comp_code,      /* completion code                 */
          &pq->reason_code);   /* reason code                     */

    if(pq->trace_level) printf("WMQ::Queue#put() MQPUT ended with reason:%s\n", wmq_reason(pq->reason_code));

    if (pq->reason_code != MQRC_NONE)
    {
        if (pq->exception_on_error)
        {
            VALUE name = Queue_name(self);

            rb_raise(wmq_exception,
                     "WMQ::Queue#put(). Error writing a message to Queue:%s, reason:%s",
                     RSTRING_PTR(name),
                     wmq_reason(pq->reason_code));
        }
        return Qfalse;
    }
    else
    {
        VALUE message = rb_hash_aref(hash, ID2SYM(ID_message));
        if(!NIL_P(message))
        {
            VALUE descriptor = rb_funcall(message, ID_descriptor, 0);
            Message_from_mqmd(descriptor, &md);                /* This should be optimized to output only fields */
        }
    }

    return Qtrue;
}

/*
 * Returns the queue name => String
 */
VALUE Queue_name(VALUE self)
{
    /* If Queue is open, return opened name, otherwise return original name */
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);
    if (pq->hobj)
    {
        return rb_iv_get(self,"@name");
    }
    return rb_iv_get(self,"@original_name");
}

struct Queue_singleton_open_arg {
    VALUE queue;
    VALUE proc;
};

static VALUE Queue_singleton_open_body(struct Queue_singleton_open_arg* arg)
{
    rb_funcall(arg->proc, ID_call, 1, arg->queue);
    return Qnil;
}

static VALUE Queue_singleton_open_ensure(VALUE queue)
{
    return Queue_close(queue);
}

/*
 * call-seq:
 *   open(...)
 *
 * Open a queue, then close the queue once the supplied code block completes
 *
 * Parameters:
 * * Since the number of parameters can vary dramatically, all parameters are passed by name in a hash
 * * Summary of parameters and their WebSphere MQ equivalents:
 *  queue = Queue.new(                                     # WebSphere MQ Equivalents:
 *   :queue_manager      => queue_manager,                 # n/a : Instance of QueueManager
 *   :q_name             => 'Queue Name',                  # MQOD.ObjectName
 *   :q_name             => { queue_manager=>'QMGR_name',  # MQOD.ObjectQMgrName
 *                            q_name       =>'q_name'}
 *   :mode               => :input or :input_shared or :input_exclusive or :output,
 *   :fail_if_quiescing  => true                           # MQOO_FAIL_IF_QUIESCING
 *   :fail_if_exists     => true, # For dynamic queues, fail if it already exists
 *   :open_options       => WMQ::MQOO_BIND_ON_OPEN | ...   # MQOO_*
 *   :close_options      => WMQ::MQCO_DELETE_PURGE         # MQCO_*
 *   :dynamic_q_name     => 'Name of Dynamic Queue'        # MQOD.DynamicQName
 *   :alternate_user_id  => 'userid',                      # MQOD.AlternateUserId
 *   :alternate_security_id => ''                          # MQOD.AlternateSecurityId
 *   )
 *
 * Mandatory Parameters
 * * :queue_manager
 *   * An instance of the WMQ::QueueManager class. E.g. QueueManager.new
 *   * Note: This is _not_ the queue manager name!
 *
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
 * * :mode => Symbol
 *   * Specify how the queue is to be opened
 *     * :output
 *       * Open the queue for output. I.e. WMQ::Queue#put will be called
 *          Equivalent to: MQOO_OUTPUT
 *     * :input
 *       * Open the queue for input. I.e. WMQ::Queue#get will be called.
 *       * Queue sharing for reading from the queue is defined by the queue itself.
 *         By default most queues are set to shared. I.e. Multiple applications
 *         can read and/or write from the queue at the same time
 *          Equivalent to: MQOO_INPUT_AS_Q_DEF
 *     * :input_shared
 *       * Open the queue for input. I.e. WMQ::Queue#get will be called.
 *       * Explicitly open the queue so that other applications can read or write
 *         from the same queue
 *          Equivalent to: MQOO_INPUT_SHARED
 *     * :input_exclusive
 *       * Open the queue for input. I.e. WMQ::Queue#get will be called.
 *       * Explicitly open the queue so that other applications cannot read
 *         from the same queue. Does _not_ affect applications writing to the queue.
 *       * Note: If :input_exclusive is used and connectivity the queue manager is lost.
 *         Upon restart the queue can still be "locked". The application should retry
 *         every minute or so until the queue becomes available. Otherwise, of course,
 *         another application has the queue open exclusively.
 *          Equivalent to: MQOO_INPUT_EXCLUSIVE
 *     * :browse
 *       * Browse the messages on the queue _without_ removing them from the queue
 *       * Open the queue for input. I.e. WMQ::Queue#get will be called.
 *       * Note: It is necessary to specify WMQ::MQGMO_BROWSE_FIRST before the
 *         first get, then set WMQ::MQGMO_BROWSE_NEXT for subsequent calls.
 *       * Note: For now it is also necessary to specify these options when calling
 *         WMQ::Queue#each. A change will be made to each to address this.
 *          Equivalent to: MQOO_BROWSE
 *
 * Optional Parameters
 * * :fail_if_quiescing => true or false
 *   * Determines whether the WMQ::Queue#open call will fail if the queue manager is
 *     in the process of being quiesced.
 *   * Note: If set to false, the MQOO_FAIL_IF_QUIESCING flag will not be removed if
 *     it was also supplied in :open_options. However, if set to true it will override
 *     this value in :open_options
 *   * Note: This interface differs from other WebSphere MQ interfaces,
 *     they do not default to true.
 *      Default: true
 *      Equivalent to: MQOO_FAIL_IF_QUIESCING
 *
 * * :open_options => FixNum
 *   * One or more of the following values:
 *       WMQ::MQOO_INQUIRE
 *       WMQ::MQOO_SET
 *       WMQ::MQOO_BIND_ON_OPEN
 *       WMQ::MQOO_BIND_NOT_FIXED
 *       WMQ::MQOO_BIND_AS_Q_DEF
 *       WMQ::MQOO_SAVE_ALL_CONTEXT
 *       WMQ::MQOO_PASS_IDENTITY_CONTEXT
 *       WMQ::MQOO_PASS_ALL_CONTEXT
 *       WMQ::MQOO_SET_IDENTITY_CONTEXT
 *       WMQ::MQOO_SET_ALL_CONTEXT
 *   * Multiple values can be or'd together. E.g.
 *       :open_options=>WMQ::MQOO_BIND_ON_OPEN | WMQ::MQOO_SAVE_ALL_CONTEXT
 *   * Please see the WebSphere MQ documentation for more details on the above options
 *
 * * :close_options => FixNum
 *   * One of the following values:
 *       WMQ::MQCO_DELETE
 *       WMQ::MQCO_DELETE_PURGE
 *   * Please see the WebSphere MQ documentation for more details on the above options
 *
 * * :dynamic_q_name => String
 *   * If a model queue name is supplied to :q_name then the final queue name that is
 *     created is specified using :dynamic_q_name
 *   * A complete queue name can be specified. E.g. 'MY.LOCAL.QUEUE'
 *   * Or, a partial queue name can be supplied. E.g. 'MY.REPLY.QUEUE.*'
 *     In this case WebSphere MQ will automatically add numbers to the end
 *     of 'MY.REPLY.QUEUE.' to ensure this queue name is unique.
 *   * The most common use of :dynamic_q_name is to create a temporary dynamic queue
 *     to which replies can be posted for this instance of the program
 *   * When opening a model queue, :dynamic_q_name is optional. However it's use is
 *     recommended in order to make it easier to identify which application a
 *     dynamic queue belongs to.
 *
 * * :fail_if_exists => true or false
 *   * Only applicable when opening a model queue
 *   * When opening a queue dynamically, sometimes the :dynamic_q_name already
 *     exists. Under this condition, if :fail_if_exists is false, the queue is
 *     automatically re-opened using the :dynamic_q_name. The :q_name field is ignored.
 *   * This feature is usefull when creating _permanent_ dynamic queues.
 *     (The model queue has the definition type set to Permanent: DEFTYPE(PERMDYN) ).
 *     * In this way it is not necessary to create the queues before running the program.
 *      Default: true
 *
 * * :alternate_user_id [String]
 *   * Sets the alternate userid to use when messages are put to the queue
 *   * Note: It is not necessary to supply WMQ::MQOO_ALTERNATE_USER_AUTHORITY
 *     since it is automatically added to the :open_options when :alternate_user_id
 *     is supplied
 *   * See WebSphere MQ Application Programming Reference: MQOD.AlternateUserId
 *
 * * :alternate_security_id [String]
 *   * Sets the alternate security id to use when messages are put to the queue
 *   * See WebSphere MQ Application Programming Reference: MQOD.AlternateSecurityId
 *
 * Note:
 * * It is more convenient to use WMQ::QueueManager#open_queue, since it automatically supplies
 *   the parameter :queue_manager
 * * That way :queue_manager parameter is _not_ required
 *
 * Example:
 *   # Put 10 Hello World messages onto a queue
 *   require 'wmq/wmq_client'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)') do |qmgr|
 *     WMQ::Queue.open(:queue_manager=>qmgr,
 *                     :q_name       =>'TEST.QUEUE',
 *                     :mode         =>:output) do |queue|
 *       10.times { |counter| queue.put(:data => "Hello World #{counter}") }
 *     end
 *   end
 */
VALUE Queue_singleton_open(int argc, VALUE *argv, VALUE self)
{
    VALUE proc, parameters, queue;

    /* Extract parameters and code block (Proc) */
    rb_scan_args(argc, argv, "1&", &parameters, &proc);

    queue = rb_funcall(wmq_queue, ID_new, 1, parameters);
    if(!NIL_P(proc))
    {
        if(Qtrue == Queue_open(queue))
        {
            struct Queue_singleton_open_arg arg;
            arg.queue = queue;
            arg.proc = proc;
            rb_ensure(Queue_singleton_open_body, (VALUE)&arg, Queue_singleton_open_ensure, queue);
        }
        else
        {
            return Qfalse;
        }
    }
    return queue;
}

/*
 * For each message found on the queue, the supplied block is executed
 *
 * Note:
 * * If no messages are found on the queue during the supplied wait interval,
 *   then the supplied block will not be called at all
 * * If :mode=>:browse is supplied when opening the queue then Queue#each will automatically
 *   set MQGMO_BROWSE_FIRST and MQGMO_BROWSE_NEXT as required
 *
 * Returns:
 * * true: If at least one message was succesfully processed
 * * false: If no messages were retrieved from the queue
 *
 * Example:
 *   require 'wmq/wmq'
 *
 *   WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
 *     qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
 *       queue.each do |message|
 *         puts "Data Received: #{message.data}"
 *       end
 *     end
 *     puts 'Completed.'
 *   end
 */
VALUE Queue_each(int argc, VALUE *argv, VALUE self)
{
    VALUE  message = Qnil;
    VALUE  match   = Qnil;
    VALUE  options = Qnil;
    VALUE  result  = Qfalse;
    VALUE  proc, hash;
    MQLONG browse = 0;

    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);

    /* Extract parameters and code block (Proc) */
    rb_scan_args(argc, argv, "01&", &hash, &proc);

    if(NIL_P(hash))
    {
        hash = rb_hash_new();
    }
    else
    {
        message = rb_hash_aref(hash, ID2SYM(ID_message));
        match   = rb_hash_aref(hash, ID2SYM(ID_match));
        options = rb_hash_aref(hash, ID2SYM(ID_options));
    }

    if (NIL_P(message))
    {
        message = rb_funcall(wmq_message, ID_new, 0);
        rb_hash_aset(hash, ID2SYM(ID_message), message);
    }

    if (NIL_P(match))
    {
        rb_hash_aset(hash, ID2SYM(ID_match), LONG2NUM(MQMO_NONE));
    }

    /* If queue is open for browse, set Borwse first indicator */
    if(pq->open_options & MQOO_BROWSE)
    {
        MQLONG get_options;
        if(NIL_P(options))
        {
            get_options = MQGMO_BROWSE_FIRST;
        }
        else
        {
            get_options = NUM2LONG(options) | MQGMO_BROWSE_FIRST;
        }
        rb_hash_aset(hash, ID2SYM(ID_options), LONG2NUM(get_options));

        if(pq->trace_level>1) printf("WMQ::Queue#each MQGMO_BROWSE_FIRST set, get options:%ld\n", (long)get_options);
        browse = 1;
    }

    while(Queue_get(self, hash))
    {
        result = Qtrue;

        /* Call code block passing in message */
        rb_funcall( proc, ID_call, 1, message );

        if(browse)
        {
            MQLONG get_options;
            if(NIL_P(options))
            {
                get_options = MQGMO_BROWSE_NEXT;
            }
            else
            {
                get_options = (NUM2LONG(options) - MQGMO_BROWSE_FIRST) | MQGMO_BROWSE_NEXT;
            }
            rb_hash_aset(hash, ID2SYM(ID_options), LONG2NUM(get_options));

            if(pq->trace_level>1) printf("WMQ::Queue#each MQGMO_BROWSE_NEXT set, get options:%ld\n", (long)get_options);
        }
    }

    return result;
}

/*
 * Return the completion code for the last MQ operation on this queue instance
 *
 * Returns => FixNum
 * * WMQ::MQCC_OK       0
 * * WMQ::MQCC_WARNING  1
 * * WMQ::MQCC_FAILED   2
 * * WMQ::MQCC_UNKNOWN  -1
 *
 */
VALUE Queue_comp_code(VALUE self)
{
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);
    return LONG2NUM(pq->comp_code);
}

/*
 * Return the reason code for the last MQ operation on this queue instance
 *
 * Returns => FixNum
 * * For a complete list of reason codes, please see WMQ Constants or
 *   the WebSphere MQ documentation for Reason Codes
 *
 * Note
 * * The list of Reason Codes varies depending on the version of WebSphere MQ
 *   and the operating system on which Ruby WMQ was compiled
 */
VALUE Queue_reason_code(VALUE self)
{
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);
    return LONG2NUM(pq->reason_code);
}

/*
 * Returns a textual representation of the reason_code for the last MQ operation on this queue instance
 *
 * Returns => String
 * * For a complete list of reasons, please see WMQ Constants or
 *   the WebSphere MQ documentation for Reason Codes
 *
 * Note
 * * The list of Reason Codes varies depending on the version of WebSphere MQ
 *   and the operating system on which Ruby WMQ was compiled
 */
VALUE Queue_reason(VALUE self)
{
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);
    return rb_str_new2(wmq_reason(pq->reason_code));
}

/*
 * Returns whether this queue is currently open
 *
 * Returns:
 * * true : The queue is open
 * * false: The queue is _not_ open
 */
VALUE Queue_open_q(VALUE self)
{
    PQUEUE pq;
    Data_Get_Struct(self, QUEUE, pq);
    if (pq->hobj)
    {
        return Qtrue;
    }
    return Qfalse;
}

