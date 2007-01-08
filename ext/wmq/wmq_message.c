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
static ID ID_name_value;
static ID ID_gsub;

void Message_id_init()
{
    ID_data            = rb_intern("data");
    ID_size            = rb_intern("size");
    ID_data_set        = rb_intern("data=");
    ID_descriptor      = rb_intern("descriptor");
    ID_headers         = rb_intern("headers");
    ID_message         = rb_intern("message");
    ID_name_value      = rb_intern("name_value");
    ID_gsub            = rb_intern("gsub");
}

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

/*
 * Automatically grow size of buffer to meet required size
 *    Existing data upto offset is preserved by copying to the new buffer
 *
 * additional_size: Size of any additional data to be written to this buffer
 *                  EXCLUDING current offset and size of data to be written
 *
 * Returns pointer to new buffer incremented by offset within buffer
 */
PMQBYTE Message_autogrow_data_buffer(struct Message_build_header_arg* parg, MQLONG additional_size)
{
    MQLONG size = *(parg->p_data_offset) + parg->data_length + additional_size;
    /* Is buffer large enough for headers */
    if(size >= *(parg->p_buffer_size))
    {
        PMQBYTE old_buffer = *(parg->pp_buffer);
        size += 512;                   /* Additional space for subsequent headers */

        if(parg->trace_level>2)
            printf ("WMQ::Message Reallocating buffer from %ld to %ld\n", *(parg->p_buffer_size), size);

        *(parg->p_buffer_size) = size;
        *(parg->pp_buffer) = ALLOC_N(char, size);
        memcpy(*(parg->pp_buffer), old_buffer, *(parg->p_data_offset));
        free(old_buffer);
    }
    return *(parg->pp_buffer) + *(parg->p_data_offset);
}

/*
 * Concatenate the passed name or value element to the existing string
 */
static void Message_name_value_concat(VALUE string, VALUE element)
{
    VALUE str = StringValue(element);
    if (RSTRING(str)->len == 0)                       /* Empty String: "" */
    {
        rb_str_concat(string, rb_str_new2("\"\""));
    }
    else
    {
        void*  contains_spaces = memchr(RSTRING(str)->ptr,' ',RSTRING(str)->len);
        void*  contains_dbl_quotes = memchr(RSTRING(str)->ptr,'"',RSTRING(str)->len);

        if(contains_spaces == NULL && contains_dbl_quotes == NULL)
        {
            rb_str_concat(string, str);
        }
        else
        {
            VALUE quote = rb_str_new2("\"");
            rb_str_concat(string, quote);
            if(contains_dbl_quotes)
            {
                rb_str_concat(string, rb_funcall(str, ID_gsub, 2, quote, rb_str_new2("\"\"")));
            }
            else
            {
                rb_str_concat(string, str);
            }
            rb_str_concat(string, quote);
        }
    }
}

struct Message_build_rf_header_each_value_arg {
    VALUE    string;
    VALUE    key;
    VALUE    space;
};

static int Message_build_rf_header_each_value(VALUE value, struct Message_build_rf_header_each_value_arg* parg)
{
    Message_name_value_concat(parg->string, parg->key);
    rb_str_concat(parg->string, parg->space);
    Message_name_value_concat(parg->string, value);
    rb_str_concat(parg->string, parg->space);

    return 0;
}

#if RUBY_VERSION_CODE > 183
static int Message_build_rf_header_each (VALUE key, VALUE value, VALUE string)
{
#else
static int Message_build_rf_header_each (VALUE array, VALUE string)
{
    VALUE key   = rb_ary_shift(array);
    VALUE value = rb_ary_shift(array);
#endif
    VALUE space = rb_str_new2(" ");
    /* If Value is an Array, need to repeat name for each value */
    if (TYPE(value) == T_ARRAY)
    {
        struct Message_build_rf_header_each_value_arg arg;
        arg.key    = key;
        arg.string = string;
        arg.space  = space;
        rb_iterate (rb_each, value, Message_build_rf_header_each_value, (VALUE)&arg);
    }
    else
    {
        Message_name_value_concat(string, key);
        rb_str_concat(string, space);
        Message_name_value_concat(string, value);
        rb_str_concat(string, space);
    }
    return 0;
}

void Message_build_rf_header (VALUE hash, struct Message_build_header_arg* parg)
{
    PMQBYTE p_data = *(parg->pp_buffer) + *(parg->p_data_offset);

    static  MQRFH MQRFH_DEF = {MQRFH_DEFAULT};
    MQLONG  name_value_len = 0;
    PMQCHAR p_name_value = 0;
    VALUE   name_value = rb_hash_aref(hash, ID2SYM(ID_name_value));

    if(parg->trace_level>2)
        printf ("WMQ::Queue#put Found rf_header\n");

    if (!NIL_P(name_value))
    {
        if (TYPE(name_value) == T_HASH)
        {
            VALUE name_value_str = rb_str_buf_new(512);         /* Allocate 512 char buffer, will grow as needed */
#if RUBY_VERSION_CODE > 183
            rb_hash_foreach(name_value, Message_build_rf_header_each, name_value_str);
#else
            rb_iterate (rb_each, name_value, Message_build_rf_header_each, name_value_str);
#endif
            name_value = name_value_str;
        }
        else if(TYPE(name_value) != T_STRING)
        {
            rb_raise(rb_eArgError, ":name_value supplied in rf_header to WMQ::Message#headers must be either a String or a Hash");
        }
        name_value_len = RSTRING(name_value)->len;
        p_name_value   = RSTRING(name_value)->ptr;
    }

    p_data = Message_autogrow_data_buffer(parg, sizeof(MQRFH)+name_value_len);

    memcpy(p_data, &MQRFH_DEF, sizeof(MQRFH));
    to_mqrfh(hash, (PMQRFH)p_data);

    *(parg->p_data_offset) += sizeof(MQRFH);

    if(name_value_len)
    {
        strncpy(p_data + *(parg->p_data_offset), p_name_value, name_value_len);
        *(parg->p_data_offset) += name_value_len;
/*--------------------------------------------------
 * TODO:
 *   To avoid problems with data conversion of the user data in some environments,
 *   it is recommended that StrucLength should be a multiple of four.
 * --------------------------------------------------*/
    }

    if(parg->trace_level>3)
        printf ("WMQ::Queue#put Sizeof namevalue string:%ld\n", name_value_len);

    ((PMQRFH)p_data)->StrucLength = sizeof(MQRFH) + name_value_len;

    if(parg->trace_level>2)
        printf ("WMQ::Queue#put data offset:%ld\n", *(parg->p_data_offset));

}

/*
 * Replace a pair of double quotes with a single occurence
 */
static VALUE Message_remove_doubled_quotes(VALUE str)
{
    if(memchr(RSTRING(str)->ptr,'"',RSTRING(str)->len) != NULL)
    {
        return rb_funcall(str, ID_gsub, 2, rb_str_new2("\"\""), rb_str_new2("\""));
    }
    else
    {
        return str;
    }
}

VALUE Message_test (VALUE self, VALUE hash, VALUE string)
{
    VALUE str = StringValue(string);
    MQCHAR buffer[65535];
    PMQBYTE p_buffer = buffer;
    MQRFH rfh;
    rfh.StrucLength = sizeof(MQRFH) + RSTRING(str)->len;
    memcpy(p_buffer, &rfh, sizeof(MQRFH));
    memcpy((p_buffer)+sizeof(MQRFH), RSTRING(str)->ptr, (RSTRING(str)->len)+1);  // Include null
    Message_deblock_rf_header (hash, p_buffer);
    return Qnil;
}

/*
 * Deblock Any custom data following RF Header
 *   The RF Header has already been deblocked into hash
 *   p_data points to the beginning of the RF Header structure
 *
 * Returns the length of RF Header plus the size of any custom data following it
 */
MQLONG Message_deblock_rf_header (VALUE hash, PMQBYTE p_data)
{
    MQLONG  size = ((PMQRFH)p_data)->StrucLength;
    PMQBYTE p_start = p_data + sizeof(MQRFH);
    PMQBYTE p_end   = p_data + size;
    PMQBYTE p_current = p_start;
    PMQBYTE p_name = 0;
    PMQBYTE p_name_end = 0;
    PMQBYTE p_value = 0;
    PMQBYTE p_last = 0;
    MQBYTE  current_delimiter = ' ';
    VALUE   name_value_hash = rb_hash_new();
    VALUE   existing;
    VALUE   key;
    VALUE   value;

    /* Skip leading spaces */
    while (p_current < p_end && *p_current == ' ')
    {
        p_current++;
    }

    if(*p_current!='"')
    {
        p_name = p_current;
    }

    /* printf("Name Value:'%s'\n", p_start);  */

    while (p_current <= p_end)
    {
        if (p_current == p_end || *p_current == ' ' || *p_current == '"')
        {
            if(current_delimiter == ' ')
            {
                /* Skip multiple spaces until next character */
                while (p_current < p_end && *p_current == ' ')
                {
                    p_current++;
                }
            }

            if (p_current < p_end)
            {
                if (current_delimiter == '"')
                {
                    if (*p_current == ' ')
                    {
                        p_last = p_current;
                        p_current++;
                        continue;
                    }

                    if (*p_current == '"')
                    {
                        p_current++;
                        if (*p_current == '"')        /* Detect Double Quotes */
                        {
                            p_current++;
                        }
                        else
                        {
                            current_delimiter = ' ';
                        }
                        continue;
                    }
                }
                else if (*p_current == '"')
                {
                    p_current++;

                    /*
                     * Check for empty string
                     * -. We already have start "
                     * 1. Is current char the end " ?
                     * 2. Is the next character a ' ' ?
                     *    Or, end of data
                     */
                    if (*p_current == '"' &&          /* Check for empty string */
                        (p_end == p_current + 1 ||
                         *(p_current + 1) == ' ') )
                    {
                        if(!p_name)
                        {
                            /*
                             * TODO: Does not yet support empty name/key
                             *       (Extremely unlikely, but still possible)
                             */
                            p_name     = p_current;
                            p_current++;
                        }
                        else
                        {
                            p_value    = p_current+1;
                            p_name_end = p_last;
                            p_last     = p_current;
                            p_current++;
                            continue;
                        }
                    }
                    else
                    {
                        current_delimiter = '"';
                    }
                }
            }

/*          printf("p_current :%ld\n", p_current);
            printf("*p_current:%c\n", *p_current);
            printf("p_last    :%ld\n", p_last);
            printf("p_name    :%ld\n", p_name);
            printf("p_name_end:%ld\n", p_name_end);
            printf("p_value   :%ld\n", p_value);
            printf("p_end     :%ld\n", p_end);
            printf("delimiter :'%c'\n", current_delimiter);
            printf("---------------------\n"); */

            if(!p_name)
            {
                p_name = p_current;
            }
            else if(p_value)                          /* End of Value */
            {
                key   = Message_remove_doubled_quotes(rb_str_new(p_name, p_name_end - p_name + 1));
                value = Message_remove_doubled_quotes(rb_str_new(p_value, p_last - p_value + 1));

          /*      printf("Found Name:'%s' Value:'%s'\n", RSTRING(key)->ptr, RSTRING(value)->ptr);   */

                /*
                 * If multiple values arrive for the same name (key) need to put values in an array
                 */
                existing = rb_hash_aref(name_value_hash, key);
                if(NIL_P(existing))
                {
                    rb_hash_aset(name_value_hash, key, value);
                }
                else
                {
                    if(TYPE(existing) == T_ARRAY)     /* Add to existing Array */
                    {
                        rb_ary_push(existing, value);
                    }
                    else                              /* Convert existing entry into an array */
                    {
                        VALUE array = rb_ary_new();
                        rb_ary_push(array, existing);
                        rb_ary_push(array, value);
                        rb_hash_aset(name_value_hash, key, array);
                    }
                }

                p_name = p_current;
                p_name_end = 0;
                p_value = 0;
            }
            else
            {
                p_value = p_current;
                p_name_end = p_last;
            }
        }
        p_last = p_current;
        p_current++;
    }

    rb_hash_aset(hash, ID2SYM(ID_name_value), name_value_hash);

    return size;
}
