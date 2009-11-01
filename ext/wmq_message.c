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
#include "decode_rfh.h"

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
static ID ID_xml;
static ID ID_gsub;
static ID ID_header_type;

void Message_id_init()
{
    ID_data            = rb_intern("data");
    ID_size            = rb_intern("size");
    ID_data_set        = rb_intern("data=");
    ID_descriptor      = rb_intern("descriptor");
    ID_headers         = rb_intern("headers");
    ID_message         = rb_intern("message");
    ID_name_value      = rb_intern("name_value");
    ID_xml             = rb_intern("xml");
    ID_gsub            = rb_intern("gsub");
    ID_header_type     = rb_intern("header_type");
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
        *p_total_length = RSTRING_LEN(data);
        *pp_buffer      = RSTRING_PTR(data);
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
        Message_to_mqmd(descriptor, pmqmd);

        headers = rb_funcall(message, ID_headers, 0);
        Check_Type(headers, T_ARRAY);

        Check_Type(data, T_STRING);

        /* Headers present? */
        if ( !NIL_P(headers) &&
             (NUM2LONG(rb_funcall(headers, ID_size, 0))>0) )
        {
            MQLONG  data_offset = 0;
            struct  Message_build_header_arg arg;
            VALUE   next_header;
            VALUE   first_header;
            VALUE   header_type;
            VALUE   ind_val;
            size_t  index;

            if(trace_level>2)
                printf ("WMQ::Queue#put %ld Header(s) supplied\n", NUM2LONG(rb_funcall(headers, ID_size, 0)));

            /* First sanity check: Do we even have enough space for the data being written and a small header */
            if(RSTRING_LEN(data) + 128 >= *pq_p_buffer_size)
            {
                MQLONG new_size = RSTRING_LEN(data) + 512; /* Add space for data and a header */
                if(trace_level>2)
                    printf ("WMQ::Queue#reallocate Resizing buffer from %ld to %ld bytes\n", *pq_p_buffer_size, (long)new_size);

                *pq_p_buffer_size = new_size;
                free(*pq_pp_buffer);
                *pq_pp_buffer = ALLOC_N(unsigned char, new_size);
            }

            arg.pp_buffer      = pq_pp_buffer;
            arg.p_buffer_size  = pq_p_buffer_size;
            arg.data_length    = RSTRING_LEN(data);
            arg.p_data_offset  = &data_offset;
            arg.trace_level    = trace_level;
            arg.next_header_id = 0;
            arg.data_format    = pmqmd->Format;

            if(trace_level>2)
                printf ("WMQ::Queue#put Building %ld headers.\n", RARRAY_LEN(headers));

            for(index = 0; index < RARRAY_LEN(headers); index++)
            {
                /*
                 * Look at the next Header so that this header can set it's format
                 * to that required for the next header.
                 */
                ind_val = LONG2FIX(index+1);
                next_header = rb_ary_aref(1, &ind_val, headers);

                if(NIL_P(next_header))
                {
                    arg.next_header_id = 0;
                }
                else
                {
                    header_type = rb_hash_aref(next_header, ID2SYM(ID_header_type));
                    if (!NIL_P(header_type))
                    {
                        arg.next_header_id = rb_to_id(header_type);
                    }
                }

                ind_val = LONG2FIX(index);
                Message_build_header(rb_ary_aref(1, &ind_val, headers), &arg);
            }

            /* Obtain Format of first header and copy in MQMD.Format */
            ind_val      = LONG2FIX(0);
            first_header = rb_ary_aref(1, &ind_val, headers);
            header_type  = rb_hash_aref(first_header, ID2SYM(ID_header_type));
            if (!NIL_P(header_type))
            {
                Message_build_set_format(rb_to_id(header_type), pmqmd->Format);
            }

            if(trace_level>2)
                printf ("WMQ::Queue#put done building headers. Offset is now %ld\n", *arg.p_data_offset);

            memcpy((*pq_pp_buffer) + data_offset, RSTRING_PTR(data), RSTRING_LEN(data));
            *p_total_length = data_offset + RSTRING_LEN(data);
            *pp_buffer      = *pq_pp_buffer;
        }
        else
        {
            *p_total_length = RSTRING_LEN(data);
            *pp_buffer      = RSTRING_PTR(data);
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
    Message_to_mqmd(rb_funcall(self, ID_descriptor, 0), pmqmd);
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
            printf ("WMQ::Message Reallocating buffer from %ld to %ld\n", *(parg->p_buffer_size), (long)size);

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
    if (RSTRING_LEN(str) == 0)                       /* Empty String: "" */
    {
        rb_str_concat(string, rb_str_new2("\"\""));
    }
    else
    {
        void*  contains_spaces = memchr(RSTRING_PTR(str),' ',RSTRING_LEN(str));
        void*  contains_dbl_quotes = memchr(RSTRING_PTR(str),'"',RSTRING_LEN(str));

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

static VALUE Message_build_rf_header_each_value(VALUE value, struct Message_build_rf_header_each_value_arg* parg)
{
    Message_name_value_concat(parg->string, parg->key);
    rb_str_concat(parg->string, parg->space);
    Message_name_value_concat(parg->string, value);
    rb_str_concat(parg->string, parg->space);

    return Qnil;
}

static int Message_build_rf_header_each (VALUE key, VALUE value, VALUE string)
{
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

    MQRFH_DEF.CodedCharSetId = MQCCSI_INHERIT;

    if(parg->trace_level>2)
        printf ("WMQ::Message#build_rf_header Found rf_header\n");

    if (!NIL_P(name_value))
    {
        if (TYPE(name_value) == T_HASH)
        {
            VALUE name_value_str = rb_str_buf_new(512);         /* Allocate 512 char buffer, will grow as needed */
            rb_hash_foreach(name_value, Message_build_rf_header_each, name_value_str);
            name_value = name_value_str;
        }
        else if(TYPE(name_value) != T_STRING)
        {
            rb_raise(rb_eArgError, ":name_value supplied in rf_header to WMQ::Message#headers must be either a String or a Hash");
        }

        name_value_len = RSTRING_LEN(name_value);
        if (name_value_len % 4)                           /* Not on 4 byte boundary ? */
        {
            rb_str_concat(name_value, rb_str_new("    ", 4 - (name_value_len % 4)));
            name_value_len = RSTRING_LEN(name_value);
        }
        p_name_value   = RSTRING_PTR(name_value);
    }

    p_data = Message_autogrow_data_buffer(parg, sizeof(MQRFH)+name_value_len);

    memcpy(p_data, &MQRFH_DEF, sizeof(MQRFH));
    Message_to_mqrfh(hash, (PMQRFH)p_data);
    ((PMQRFH)p_data)->StrucLength = sizeof(MQRFH) + name_value_len;
    if(parg->next_header_id)
    {
        Message_build_set_format(parg->next_header_id, ((PMQRFH)p_data)->Format);
    }
    else
    {
        strncpy(((PMQRFH)p_data)->Format, parg->data_format, MQ_FORMAT_LENGTH);
    }

    *(parg->p_data_offset) += sizeof(MQRFH);
    p_data += sizeof(MQRFH);

    if(name_value_len)
    {
        memcpy(p_data, p_name_value, name_value_len);
        *(parg->p_data_offset) += name_value_len;
    }

    if(parg->trace_level>3)
        printf ("WMQ::Message#build_rf_header Sizeof namevalue string:%ld\n", (long)name_value_len);

    if(parg->trace_level>2)
        printf ("WMQ::Message#build_rf_header data offset:%ld\n", *(parg->p_data_offset));
}

static void Message_deblock_rf_header_each_pair(const char *p_name, const char *p_value, void* p_name_value_hash)
{
    VALUE name_value_hash = (VALUE)p_name_value_hash;
    VALUE key             = rb_str_new2(p_name);
    VALUE value           = rb_str_new2(p_value);

    /*
     * If multiple values arrive for the same name (key) need to put values in an array
     */
    VALUE existing = rb_hash_aref(name_value_hash, key);
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
}

/*
 * Deblock Any custom data following RF Header
 *   The RF Header has already been deblocked into hash
 *   p_data points to the beginning of the RF Header structure
 *
 *   msg_len is the length of the remainder of the message from this header onwards
 *           It includes the length of any data that may follow
 *
 * Returns the length of RF Header plus the size of any custom data following it
 */
MQLONG Message_deblock_rf_header (VALUE hash, PMQBYTE p_data, MQLONG data_len)
{
    MQLONG size = ((PMQRFH)p_data)->StrucLength;
    VALUE  name_value_hash = rb_hash_new();

    rfh_toktype_t toktype;

    if(size < 0 || size > data_len) /* Poison Message */
    {
        printf("WMQ::Message_deblock_rf_header StrucLength supplied in MQRFH exceeds total message length\n");
        return 0;
    }

    toktype = rfh_decode_name_val_str(p_data + sizeof(MQRFH),
                                      size - sizeof(MQRFH),
                                      Message_deblock_rf_header_each_pair,
                                      (void*)name_value_hash);

    if (toktype != TT_END)
    {
        printf("Could not parse rfh name value string, reason %s\n",rfh_toktype_to_s(toktype));
    }

    rb_hash_aset(hash, ID2SYM(ID_name_value), name_value_hash);

    return size;
}

/*
 * RFH2 Header can contain multiple XML-like strings
 *   Message consists of:
 *       MQRFH2
 *       xml-string1-length  (MQLONG)
 *       xml-string1         (Padded with spaces to match 4 byte boundary)
 *       xml-string2-length  (MQLONG)
 *       xml-string2         (Padded with spaces to match 4 byte boundary)
 *       ....
 */
MQLONG Message_deblock_rf_header_2 (VALUE hash, PMQBYTE p_buffer, MQLONG data_len)
{
    MQLONG  size = ((PMQRFH2)p_buffer)->StrucLength;
    PMQBYTE p_data  = p_buffer + sizeof(MQRFH2);
    PMQBYTE p_end   = p_buffer + size;                /* Points to byte after last character */
    MQLONG  xml_len = 0;
    VALUE   xml_ary = rb_ary_new();

    PMQBYTE pChar;
    size_t  length;
    size_t  i;

    if(size < 0 || size > data_len) /* Poison Message */
    {
        printf("WMQ::Message_deblock_rf_header_2 StrucLength supplied in MQRFH exceeds total message length\n");
        return 0;
    }

    rb_hash_aset(hash, ID2SYM(ID_xml), xml_ary);

    while(p_data < p_end)
    {
        xml_len = *(PMQLONG)p_data;
        p_data += sizeof(MQLONG);

        if (xml_len < 0 || p_data+xml_len > p_end)
        {
            printf("WMQ::Message#deblock_rf_header_2 Poison Message received, stopping further processing\n");
            p_data = p_end;
        }
        else
        {
            /*
             * Strip trailing spaces added as pad characters during put
             */
            length = 0;
            pChar = p_data + xml_len - 1;
            for (i = xml_len; i > 0; i--)
            {
                if (*pChar != ' ')
                {
                    length = i;
                    break;
                }
                pChar--;
            }
            rb_ary_push(xml_ary, rb_str_new(p_data, length));
            p_data += xml_len;
        }
    }

    return size;
}

static VALUE Message_build_rf_header_2_each(VALUE element, struct Message_build_header_arg* parg)
{
    VALUE  str = StringValue(element);
    MQLONG length = RSTRING_LEN(str);

    if (length % 4)                           /* Not on 4 byte boundary ? */
    {
        static const char * blanks = "    ";
        MQLONG pad     = 4 - (length % 4);
        MQLONG len     = length + pad;
        PMQBYTE p_data = Message_autogrow_data_buffer(parg, sizeof(len) + length + pad);

        memcpy(p_data, (void*) &len, sizeof(len));    /* Start with MQLONG length indicator */
        p_data += sizeof(len);
        memcpy(p_data, RSTRING_PTR(str), length);
        p_data += length;
        memcpy(p_data, blanks, pad);
        p_data += pad;
        *(parg->p_data_offset) += sizeof(len) + length + pad;
    }
    else
    {
        PMQBYTE p_data = Message_autogrow_data_buffer(parg, sizeof(length) + length);

        memcpy(p_data, (void*) &length, sizeof(length));  /* Start with MQLONG length indicator */
        p_data += sizeof(length);
        memcpy(p_data, RSTRING_PTR(str), length);
        p_data += length;
        *(parg->p_data_offset) += sizeof(length) + length;
    }

    return Qnil;
}

/*
 * RFH2 Header can contain multiple XML-like strings
 *   Message consists of:
 *       MQRFH2
 *       xml-string1-length  (MQLONG)
 *       xml-string1         (Padded with spaces to match 4 byte boundary)
 *       xml-string2-length  (MQLONG)
 *       xml-string2         (Padded with spaces to match 4 byte boundary)
 *       ....
 *
 *  The input data is either a String or an Array of Strings
 *  E.g.
 *
 *  {
 *    :header_type =>:rf_header_2,
 *    :xml => '<hello>to the world</hello>'
 *  }
 *           OR
 *  {
 *    :header_type =>:rf_header_2,
 *    :xml => ['<hello>to the world</hello>', '<another>xml like string</another>'],
 *  }
 *
 */
void Message_build_rf_header_2(VALUE hash, struct Message_build_header_arg* parg)
{
    static  MQRFH2 MQRFH2_DEF = {MQRFH2_DEFAULT};
    MQLONG  rfh2_offset = *(parg->p_data_offset);
    PMQBYTE p_data;
    VALUE   xml = rb_hash_aref(hash, ID2SYM(ID_xml));

    if(parg->trace_level>2)
        printf ("WMQ::Message#build_rf_header_2 Found rf_header_2\n");

    /* Add MQRFH2 Header, ensure that their is enough space */
    p_data = Message_autogrow_data_buffer(parg, sizeof(MQRFH2));
    memcpy(p_data, &MQRFH2_DEF, sizeof(MQRFH2));
    Message_to_mqrfh2(hash, (PMQRFH2)p_data);
    if(parg->next_header_id)
    {
        Message_build_set_format(parg->next_header_id, ((PMQRFH2)p_data)->Format);
    }
    else
    {
        memcpy(((PMQRFH2)p_data)->Format, parg->data_format, MQ_FORMAT_LENGTH);
    }
    *(parg->p_data_offset) += sizeof(MQRFH2);

    if (!NIL_P(xml))
    {
        if(TYPE(xml) == T_ARRAY)
        {
            rb_iterate (rb_each, xml, Message_build_rf_header_2_each, (VALUE)parg);
        }
        else if(TYPE(xml) != T_STRING)
        {
            Message_build_rf_header_2_each(xml, parg);
        }
        else
        {
            rb_raise(rb_eArgError, ":name_value supplied in rf_header_2 to WMQ::Message#headers must be a String or an Array");
        }
    }

    /* Set total length in MQRFH2 */
    ((PMQRFH)(*(parg->pp_buffer) + rfh2_offset))->StrucLength = *(parg->p_data_offset) - rfh2_offset;

    if(parg->trace_level>2)
        printf ("WMQ::Message#build_rf_header_2 data offset:%ld\n", *(parg->p_data_offset));
}

