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
 * Initialize Ruby ID's for Message Class
 *
 * This function is called when the library is loaded
 * by ruby
 * --------------------------------------------------*/

static ID ID_data;
static ID ID_message;
static ID ID_descriptor;
static ID ID_headers;
static ID ID_data_set;
static ID ID_size;

void Message_id_init()
{
    ID_data            = rb_intern("data");
    ID_size            = rb_intern("size");
    ID_data_set        = rb_intern("data=");
    ID_descriptor      = rb_intern("descriptor");
    ID_headers         = rb_intern("headers");
    ID_message         = rb_intern("message");
}

#if 0 /* Leave auto-grow buffer in Queue for now */

 typedef struct tagWMQ_MESSAGE WMQ_MESSAGE;
 typedef WMQ_MESSAGE MQPOINTER P_WMQ_MESSAGE;

 struct tagQUEUE {
    MQLONG   trace_level;             /* Trace level.                  */
    PMQBYTE  p_buffer;                /* message buffer                */
    MQLONG   buffer_size;             /* Allocated size of buffer      */
 };

/* --------------------------------------------------
 * C Structure to store MQ data types and other
 *   C internal values
 * --------------------------------------------------*/
void WMQ_MESSAGE_free(void* p)
{
    P_WMQ_MESSAGE pm = (P_WMQ_MESSAGE)p;
    if(pm->trace_level) printf("WMQ::Message Freeing WMQ_MESSAGE structure\n");

    free(pm->p_buffer);
    free(p);
}

VALUE WMQ_MESSAGE_alloc(VALUE klass)
{
    P_WMQ_MESSAGE pm = ALLOC(WMQ_MESSAGE);
    pm->trace_level = 0;
    pm->buffer_size = 16384;
    pm->p_buffer = ALLOC_N(char, pm->buffer_size);

    return Data_Wrap_Struct(klass, 0, QUEUE_free, pq);
}
#endif

void Message_build(PMQBYTE* pq_pp_buffer, PMQLONG pq_p_buffer_size, MQLONG trace_level,
                   VALUE parms, PPMQVOID pp_buffer, PMQLONG p_total_length, PMQMD pmqmd)
{
    VALUE    data;
    VALUE    descriptor;
    VALUE    headers;
    VALUE    message;

    /* :data is an optional parameter, that if supplied overrides message.data */
    data = rb_hash_aref(parms, ID2SYM(ID_data));
    if(!NIL_P(data))
    {
        Check_Type(data, T_STRING);
        *p_total_length = RSTRING(data)->len;
        *pp_buffer      = RSTRING(data)->ptr;
    }

    /* :message is optional */
    message = rb_hash_aref(parms, ID2SYM(ID_message));
    if(!NIL_P(message))
    {
        if (NIL_P(data))                              /* Obtain data from message.data */
        {
            data = rb_funcall(message, ID_data, 0);
        }

        descriptor = rb_funcall(message, ID_descriptor, 0);
        Check_Type(descriptor, T_HASH);
        to_mqmd(descriptor, pmqmd);

        headers = rb_funcall(message, ID_headers, 0);
        Check_Type(headers, T_ARRAY);

        Check_Type(data, T_STRING);

        /* Headers present? */
        if ( !NIL_P(headers) &&
             (NUM2LONG(rb_funcall(headers, ID_size, 0))>0) )
        {
            PMQBYTE p_data = 0;
            MQLONG  data_offset = 0;
            struct Message_build_header_arg arg;

            if(trace_level>2)
                printf ("WMQ::Queue#put %ld Header(s) supplied\n", NUM2LONG(rb_funcall(headers, ID_size, 0)));

            /* First sanity check: Do we even have enough space for the data being written and a small header */
            if(RSTRING(data)->len + 128 >= *pq_p_buffer_size)
            {
                MQLONG new_size = RSTRING(data)->len + 512; /* Add space for data and a header */
                if(trace_level>2)
                    printf ("WMQ::Queue#reallocate Resizing buffer from %ld to %ld bytes\n", *pq_p_buffer_size, new_size);

                *pq_p_buffer_size = new_size;
                free(*pq_pp_buffer);
                *pq_pp_buffer = ALLOC_N(char, new_size);
            }

            arg.pp_buffer      = pq_pp_buffer;
            arg.p_buffer_size  = pq_p_buffer_size;
            arg.data_length    = RSTRING(data)->len;
            arg.p_data_offset  = &data_offset;
            arg.trace_level    = trace_level;

            rb_iterate (rb_each, headers, Message_build_header, (VALUE)&arg);  /* For each hash in headers array */

            if(trace_level>2)
                printf ("WMQ::Queue#put done building headers. Offset is now %ld\n", arg.p_data_offset);

            memcpy((*pq_pp_buffer) + data_offset, RSTRING(data)->ptr, RSTRING(data)->len);
            *p_total_length = data_offset + RSTRING(data)->len;
            *pp_buffer      = *pq_pp_buffer;
        }
        else
        {
            *p_total_length = RSTRING(data)->len;
            *pp_buffer      = RSTRING(data)->ptr;
        }
    }
    /* If :message is not supplied, then :data must be supplied in the parameter list */
    else if(NIL_P(data))
    {
        rb_raise(rb_eArgError, "At least one of :message or :data is required.");
    }
    return;
}

/*
 * Extract MQMD from descriptor hash
 */
void Message_build_mqmd(VALUE self, PMQMD pmqmd)
{
    to_mqmd(rb_funcall(self, ID_descriptor, 0), pmqmd);
}

/*
 * call-seq:
 *   new(...)
 *
 * Optional Named Parameters (as a single hash):
 * * :data
 *   * Data to be written, or was read from the queue
 * * :descriptor
 *   * Desciptor
 *
 * Example:
 *   message = WMQ::Message.new
 *
 * Example:
 *   message = WMQ::Message.new(:data=>'Hello World',
 *                              :descriptor=> {
 *                                :format => WMQ::MQFMT_STRING
 *                              })
 */
VALUE Message_initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE parms = Qnil;
    VALUE proc = Qnil;

    /* Extract optional parameter */
    rb_scan_args(argc, argv, "01", &parms);

    if (NIL_P(parms))
    {
        rb_iv_set(self, "@data", Qnil);
        rb_iv_set(self, "@headers", rb_ary_new());
        rb_iv_set(self, "@descriptor", rb_hash_new());
    }
    else
    {
        VALUE val;
        Check_Type(parms, T_HASH);

        rb_iv_set(self, "@data", rb_hash_aref(parms, ID2SYM(ID_data)));

        val = rb_hash_aref(parms, ID2SYM(ID_headers));
        if (NIL_P(val))
        {
            rb_iv_set(self, "@headers", rb_ary_new());
        }
        else
        {
            rb_iv_set(self, "@headers", val);
        }

        val = rb_hash_aref(parms, ID2SYM(ID_descriptor));
        if (NIL_P(val))
        {
            rb_iv_set(self, "@headers", rb_hash_new());
        }
        else
        {
            rb_iv_set(self, "@descriptor", val);
        }
    }

    return Qnil;
}

/*
 * Clear out the message data and headers
 *
 * Note:
 * * The descriptor is not affected in any way
 */
VALUE Message_clear(VALUE self)
{
    rb_iv_set(self, "@data", Qnil);
    rb_iv_set(self, "@headers", rb_ary_new());

    return self;
}
