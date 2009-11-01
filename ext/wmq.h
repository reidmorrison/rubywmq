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

#include <ruby.h>

/* New with Ruby 1.9, define for prior Ruby versions */
#ifndef RSTRING_PTR
   #define RSTRING_PTR(str) RSTRING(str)->ptr
#endif
#ifndef RSTRING_LEN
   #define RSTRING_LEN(str) RSTRING(str)->len
#endif
#ifndef RARRAY_PTR
   #define RARRAY_PTR(ary) RARRAY(ary)->ptr
#endif
#ifndef RARRAY_LEN
   #define RARRAY_LEN(ary) RARRAY(ary)->len
#endif
#ifndef HAVE_RB_STR_SET_LEN
   #define rb_str_set_len(str, length) (RSTRING_LEN(str) = (length))
#endif

#include <cmqc.h>
#include <cmqxc.h>

/* Todo: Add a #ifdef here to exclude the following includes when applicable  */
#include <cmqcfc.h>                        /* PCF                             */
#include <cmqbc.h>                         /* MQAI                            */

#ifdef _WIN32
    #define WMQ_EXPORT __declspec(dllexport)
#else
    #define WMQ_EXPORT
#endif

void QueueManager_id_init();
void QueueManager_selector_id_init();
void QueueManager_command_id_init();
VALUE QUEUE_MANAGER_alloc(VALUE klass);
VALUE QueueManager_singleton_connect(int argc, VALUE *argv, VALUE self);
VALUE QueueManager_open_queue(int argc, VALUE *argv, VALUE self);
VALUE QueueManager_initialize(VALUE self, VALUE parms);
VALUE QueueManager_connect(VALUE self);
VALUE QueueManager_disconnect(VALUE self);
VALUE QueueManager_begin(VALUE self);
VALUE QueueManager_commit(VALUE self);
VALUE QueueManager_backout(VALUE self);
VALUE QueueManager_put(VALUE self, VALUE parms);
VALUE QueueManager_reason_code(VALUE self);
VALUE QueueManager_comp_code(VALUE self);
VALUE QueueManager_reason(VALUE self);
VALUE QueueManager_exception_on_error(VALUE self);
VALUE QueueManager_connected_q(VALUE self);
VALUE QueueManager_name(VALUE self);
VALUE QueueManager_execute(VALUE self, VALUE hash);

void  Queue_id_init();
VALUE QUEUE_alloc(VALUE klass);
VALUE Queue_initialize(VALUE self, VALUE parms);
VALUE Queue_singleton_open(int argc, VALUE *argv, VALUE self);
VALUE Queue_open(VALUE self);
VALUE Queue_close(VALUE self);
VALUE Queue_put(VALUE self, VALUE parms);
VALUE Queue_get(VALUE self, VALUE parms);
VALUE Queue_each(int argc, VALUE *argv, VALUE self);
VALUE Queue_name(VALUE self);
VALUE Queue_reason_code(VALUE self);
VALUE Queue_comp_code(VALUE self);
VALUE Queue_reason(VALUE self);
VALUE Queue_open_q(VALUE self);

void Queue_extract_put_message_options(VALUE hash, PMQPMO ppmo);

extern VALUE wmq_queue;
extern VALUE wmq_queue_manager;
extern VALUE wmq_message;
extern VALUE wmq_exception;

#define WMQ_EXEC_STRING_INQ_BUFFER_SIZE 32768           /* Todo: Should we make the mqai string return buffer dynamic? */

/* Internal C Structures for holding MQ data types */
 typedef struct tagQUEUE_MANAGER QUEUE_MANAGER;
 typedef QUEUE_MANAGER MQPOINTER PQUEUE_MANAGER;

 struct tagQUEUE_MANAGER {
    MQHCONN  hcon;                    /* connection handle             */
    MQLONG   comp_code;               /* completion code               */
    MQLONG   reason_code;             /* reason code for MQCONN        */
    MQLONG   exception_on_error;      /* Non-Zero means throw exception*/
    MQLONG   already_connected;       /* Already connected means don't disconnect */
    MQLONG   trace_level;             /* Trace level. 0==None, 1==Info 2==Debug ..*/
    MQCNO    connect_options;         /* MQCONNX Connection Options    */
  #ifdef MQCNO_VERSION_2
    MQCD     client_conn;             /* Client Connection             */
  #endif
  #ifdef MQCNO_VERSION_4
    MQSCO    ssl_config_opts;         /* Security options              */
  #endif
  #ifdef MQCD_VERSION_6
    MQPTR    long_remote_user_id_ptr;
  #endif
  #ifdef MQCD_VERSION_7
    MQPTR    ssl_peer_name_ptr;
  #endif
  #ifdef MQHB_UNUSABLE_HBAG
    MQHBAG   admin_bag;
    MQHBAG   reply_bag;
  #endif
    PMQBYTE  p_buffer;                /* message buffer                */
    MQLONG   buffer_size;             /* Allocated size of buffer      */

    MQLONG   is_client_conn;          /* Is this a Client Connection?  */
    void*    mq_lib_handle;           /* Handle to MQ library          */

    void(*MQCONNX)(PMQCHAR,PMQCNO,PMQHCONN,PMQLONG,PMQLONG);
    void(*MQCONN) (PMQCHAR,PMQHCONN,PMQLONG,PMQLONG);
    void(*MQDISC) (PMQHCONN,PMQLONG,PMQLONG);
    void(*MQBEGIN)(MQHCONN,PMQVOID,PMQLONG,PMQLONG);
    void(*MQBACK) (MQHCONN,PMQLONG,PMQLONG);
    void(*MQCMIT) (MQHCONN,PMQLONG,PMQLONG);
    void(*MQPUT1) (MQHCONN,PMQVOID,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG);

    void(*MQOPEN) (MQHCONN,PMQVOID,MQLONG,PMQHOBJ,PMQLONG,PMQLONG);
    void(*MQCLOSE)(MQHCONN,PMQHOBJ,MQLONG,PMQLONG,PMQLONG);
    void(*MQGET)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG,PMQLONG);
    void(*MQPUT)  (MQHCONN,MQHOBJ,PMQVOID,PMQVOID,MQLONG,PMQVOID,PMQLONG,PMQLONG);

    void(*MQINQ)  (MQHCONN,MQHOBJ,MQLONG,PMQLONG,MQLONG,PMQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG);
    void(*MQSET)  (MQHCONN,MQHOBJ,MQLONG,PMQLONG,MQLONG,PMQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG);

    void(*mqCreateBag)(MQLONG,PMQHBAG,PMQLONG,PMQLONG);
    void(*mqDeleteBag)(PMQHBAG,PMQLONG,PMQLONG);
    void(*mqClearBag)(MQHBAG,PMQLONG,PMQLONG);
    void(*mqExecute)(MQHCONN,MQLONG,MQHBAG,MQHBAG,MQHBAG,MQHOBJ,MQHOBJ,PMQLONG,PMQLONG);
    void(*mqCountItems)(MQHBAG,MQLONG,PMQLONG,PMQLONG,PMQLONG);
    void(*mqInquireBag)(MQHBAG,MQLONG,MQLONG,PMQHBAG,PMQLONG,PMQLONG);
    void(*mqInquireItemInfo)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG,PMQLONG,PMQLONG);
    void(*mqInquireInteger)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG,PMQLONG);
    void(*mqInquireString)(MQHBAG,MQLONG,MQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG,PMQLONG,PMQLONG);
    void(*mqAddInquiry)(MQHBAG,MQLONG,PMQLONG,PMQLONG);
    void(*mqAddInteger)(MQHBAG,MQLONG,MQLONG,PMQLONG,PMQLONG);
    void(*mqAddString)(MQHBAG,MQLONG,MQLONG,PMQCHAR,PMQLONG,PMQLONG);
 };

void Queue_manager_mq_load(PQUEUE_MANAGER pqm);
void Queue_manager_mq_free(PQUEUE_MANAGER pqm);


/*
 * Message
 */
struct Message_build_header_arg {
    PMQBYTE* pp_buffer;                               /* Autosize: Pointer to start of total buffer */
    PMQLONG  p_buffer_size;                           /* Autosize: Size of total buffer */
    MQLONG   data_length;                             /* Autosize: Length of the data being written */
    PMQLONG  p_data_offset;                           /* Current offset of data portion in total buffer */
    MQLONG   trace_level;                             /* Trace level. 0==None, 1==Info 2==Debug ..*/
    ID       next_header_id;                          /* Used for setting MQ Format to next header */
    PMQBYTE  data_format;                             /* Format of data. Used when next_header_id == 0 */
};

void    Message_id_init();
VALUE   Message_initialize(int argc, VALUE *argv, VALUE self);
VALUE   Message_clear(VALUE self);
PMQBYTE Message_autogrow_data_buffer(struct Message_build_header_arg* parg, MQLONG additional_size);
void    Message_build_rf_header (VALUE hash, struct Message_build_header_arg* parg);
MQLONG  Message_deblock_rf_header (VALUE hash, PMQBYTE p_data, MQLONG data_len);
void    Message_build_rf_header_2 (VALUE hash, struct Message_build_header_arg* parg);
MQLONG  Message_deblock_rf_header_2 (VALUE hash, PMQBYTE p_data, MQLONG data_len);

void    Message_build_set_format(ID header_type, PMQBYTE p_format);
void    Message_build(PMQBYTE* pq_pp_buffer, PMQLONG pq_p_buffer_size, MQLONG trace_level,
                      VALUE parms, PPMQVOID pp_buffer, PMQLONG p_total_length, PMQMD pmqmd);
void    Message_build_mqmd(VALUE self, PMQMD pmqmd);
void    Message_deblock(VALUE message, PMQMD pmqmd, PMQBYTE p_buffer, MQLONG total_length, MQLONG trace_level);

int  Message_build_header(VALUE hash, struct Message_build_header_arg* parg);

/* Utility methods */

/* --------------------------------------------------
 * Set internal variable based on value passed in from
 * a hash
 * --------------------------------------------------*/
void setFromHashString(VALUE self, VALUE hash, char* pKey, char* pAttribute, char* pDefault);
void setFromHashValue(VALUE self, VALUE hash, char* pKey, char* pAttribute, VALUE valDefault);

void to_mqmd(VALUE hash, MQMD* pmqmd);
void from_mqmd(VALUE hash, MQMD* pmqmd);

void to_mqdlh(VALUE hash, MQDLH* pmqdlh);
void from_mqdlh(VALUE hash, MQDLH* pmqdlh);

void wmq_structs_id_init();
void Message_to_mqmd(VALUE hash, MQMD* pmqmd);
void Message_to_mqmd1(VALUE hash, MQMD1* pmqmd1);
void Message_to_mqrfh2(VALUE hash, MQRFH2* pmqrfh2);
void Message_to_mqrfh(VALUE hash, MQRFH* pmqrfh);
void Message_to_mqdlh(VALUE hash, MQDLH* pmqdlh);
void Message_to_mqcih(VALUE hash, MQCIH* pmqcih);
void Message_to_mqdh(VALUE hash, MQDH* pmqdh);
void Message_to_mqiih(VALUE hash, MQIIH* pmqiih);
void Message_to_mqrmh(VALUE hash, MQRMH* pmqrmh);
void Message_to_mqtm(VALUE hash, MQTM* pmqtm);
void Message_to_mqtmc2(VALUE hash, MQTMC2* pmqtmc2);
void Message_to_mqwih(VALUE hash, MQWIH* pmqwih);
void Message_to_mqxqh(VALUE hash, MQXQH* pmqxqh);
void Message_from_mqmd(VALUE hash, MQMD* pmqmd);
void Message_from_mqmd1(VALUE hash, MQMD1* pmqmd1);
void Message_from_mqrfh2(VALUE hash, MQRFH2* pmqrfh2);
void Message_from_mqrfh(VALUE hash, MQRFH* pmqrfh);
void Message_from_mqdlh(VALUE hash, MQDLH* pmqdlh);
void Message_from_mqcih(VALUE hash, MQCIH* pmqcih);
void Message_from_mqdh(VALUE hash, MQDH* pmqdh);
void Message_from_mqiih(VALUE hash, MQIIH* pmqiih);
void Message_from_mqrmh(VALUE hash, MQRMH* pmqrmh);
void Message_from_mqtm(VALUE hash, MQTM* pmqtm);
void Message_from_mqtmc2(VALUE hash, MQTMC2* pmqtmc2);
void Message_from_mqwih(VALUE hash, MQWIH* pmqwih);
void Message_from_mqxqh(VALUE hash, MQXQH* pmqxqh);

char*  wmq_reason(MQLONG reason_code);
ID     wmq_selector_id(MQLONG selector);
MQLONG wmq_command_lookup(ID command_id);
void   wmq_selector(ID selector_id, PMQLONG selector_type, PMQLONG selector);

/* --------------------------------------------------
 * MACROS for moving data between Ruby and MQ
 * --------------------------------------------------*/
#define WMQ_STR2MQLONG(STR,ELEMENT) \
    ELEMENT = NUM2LONG(STR);

#define WMQ_STR2MQCHAR(STR,ELEMENT) \
    ELEMENT = NUM2LONG(STR);

#define WMQ_STR2MQCHARS(STR,ELEMENT)            \
    str = StringValue(STR);                     \
    length = RSTRING_LEN(STR);                 \
    size = sizeof(ELEMENT);                     \
    strncpy(ELEMENT, RSTRING_PTR(STR), length > size ? size : length);

#define WMQ_STR2MQBYTES(STR,ELEMENT)                \
    str = StringValue(STR);                         \
    length = RSTRING_LEN(str);                     \
    size = sizeof(ELEMENT);                         \
    if (length >= size)                             \
    {                                               \
        memcpy(ELEMENT, RSTRING_PTR(str), size);   \
    }                                               \
    else                                            \
    {                                               \
        memcpy(ELEMENT, RSTRING_PTR(str), length); \
        memset(ELEMENT+length, 0, size-length);     \
    }

#define WMQ_HASH2MQLONG(HASH,KEY,ELEMENT)           \
    val = rb_hash_aref(HASH, ID2SYM(ID_##KEY));     \
    if (!NIL_P(val)) { WMQ_STR2MQLONG(val,ELEMENT) }

#define WMQ_HASH2MQCHARS(HASH,KEY,ELEMENT)      \
    val = rb_hash_aref(HASH, ID2SYM(ID_##KEY)); \
    if (!NIL_P(val))                            \
    {                                           \
         WMQ_STR2MQCHARS(val,ELEMENT)           \
    }

#define WMQ_HASH2MQCHAR(HASH,KEY,ELEMENT)             \
    val = rb_hash_aref(HASH, ID2SYM(ID_##KEY));       \
    if (!NIL_P(val)) { WMQ_STR2MQCHAR(val,ELEMENT); } \

#define WMQ_HASH2MQBYTES(HASH,KEY,ELEMENT)      \
    val = rb_hash_aref(HASH, ID2SYM(ID_##KEY)); \
    if (!NIL_P(val))                            \
    {                                           \
         WMQ_STR2MQBYTES(val,ELEMENT)           \
    }

#define WMQ_HASH2BOOL(HASH,KEY,ELEMENT)             \
    val = rb_hash_aref(hash, ID2SYM(ID_##KEY));     \
    if (!NIL_P(val))                                \
    {                                               \
        if(TYPE(val) == T_TRUE)       ELEMENT = 1;  \
        else if(TYPE(val) == T_FALSE) ELEMENT = 0;  \
        else                                        \
             rb_raise(rb_eTypeError, ":" #KEY       \
                         " must be true or false"); \
    }

#define IF_TRUE(KEY,DEFAULT)                        \
    val = rb_hash_aref(hash, ID2SYM(ID_##KEY));     \
    if (NIL_P(val))                                 \
    {                                               \
        flag = DEFAULT;                             \
    }                                               \
    else                                            \
    {                                               \
        if(TYPE(val) == T_TRUE)       flag = 1;     \
        else if(TYPE(val) == T_FALSE) flag = 0;     \
        else                                        \
             rb_raise(rb_eTypeError, ":" #KEY       \
                         " must be true or false"); \
    }                                               \
    if (flag)

/* --------------------------------------------------
 * Strip trailing nulls and spaces
 * --------------------------------------------------*/
#define WMQ_MQCHARS2STR(ELEMENT, TARGET) \
    size = sizeof(ELEMENT);           \
    length = 0;                       \
    pChar = ELEMENT + size-1;         \
    for (i = size; i > 0; i--)        \
    {                                 \
        if (*pChar != ' ' && *pChar != 0) \
        {                             \
            length = i;               \
            break;                    \
        }                             \
        pChar--;                      \
    }                                 \
    TARGET = rb_str_new(ELEMENT,length);

#define WMQ_MQCHARS2HASH(HASH,KEY,ELEMENT)        \
    WMQ_MQCHARS2STR(ELEMENT, str)                 \
    rb_hash_aset(HASH, ID2SYM(ID_##KEY), str);

#define WMQ_MQLONG2HASH(HASH,KEY,ELEMENT) \
    rb_hash_aset(HASH, ID2SYM(ID_##KEY), LONG2NUM(ELEMENT));

#define WMQ_MQCHAR2HASH(HASH,KEY,ELEMENT) \
    rb_hash_aset(HASH, ID2SYM(ID_##KEY), LONG2NUM(ELEMENT));

/* --------------------------------------------------
 * Trailing Spaces are important with binary fields
 * --------------------------------------------------*/
#define WMQ_MQBYTES2STR(ELEMENT,TARGET)  \
    size = sizeof(ELEMENT);           \
    length = 0;                       \
    pChar = ELEMENT + size-1;         \
    for (i = size; i > 0; i--)        \
    {                                 \
        if (*pChar != 0)              \
        {                             \
            length = i;               \
            break;                    \
        }                             \
        pChar--;                      \
    }                                 \
    TARGET = rb_str_new(ELEMENT,length);

#define WMQ_MQBYTES2HASH(HASH,KEY,ELEMENT)  \
    WMQ_MQBYTES2STR(ELEMENT, str)           \
    rb_hash_aset(HASH, ID2SYM(ID_##KEY), str);

