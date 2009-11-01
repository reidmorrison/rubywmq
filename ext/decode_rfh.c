/*
 * Copyright 2007 Edwin M. Fine, Fine Computer Consultants, Inc.
 *
 * Licensed under the Apache License, Version 2.0 
 * (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy 
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, 
 * software distributed under the License is distributed on an 
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
 * either express or implied. See the License for the specific 
 * language governing permissions and limitations under the License.
 */

/*
 * This function decodes an RFH NameValueString into separate
 * name/value strings and invokes a callback function for each
 * name/value pair. The callback function prototype is
 *
 * void cbfunc(const char *name, const char *value);
 *
 * Strings are expected to be null-terminated and not contain any
 * null characters.
 *
 * This is the specification for NameValueString:
 *
 * NameValueString (MQCHARn)
 * 
 *  String containing name/value pairs.
 * 
 *  This is a variable-length character string containing name/value
 *  pairs in the form:
 * 
 *  name1 value1 name2 value2 name3 value3 ...
 * 
 *  Each name or value must be separated from the adjacent name
 *  or value by one or more blank characters; these blanks are
 *  not significant. A name or value can contain significant
 *  blanks by prefixing and suffixing the name or value with the
 *  double-quote character; all characters between the open
 *  double-quote and the matching close double-quote are treated
 *  as significant. In the following example, the name is
 *  FAMOUS_WORDS, and the value is Hello World:
 * 
 *  FAMOUS_WORDS "Hello World"
 * 
 *  A name or value can contain any characters other than the
 *  null character (which acts as a delimiter for
 *  NameValueString). However, to assist interoperability, an
 *  application might prefer to restrict names to the following
 *  characters:
 * 
 *      * First character: upper case or lower case alphabetic (A
 *        through Z, or a through z), or underscore.
 * 
 *      * Second character: upper case or lower case alphabetic,
 *        decimal digit (0 through 9), underscore, hyphen, or 
 *        dot.
 * 
 *  If a name or value contains one or more double-quote
 *  characters, the name or value must be enclosed in double
 *  quotes, and each double quote within the string must be
 *  doubled, for example:
 * 
 *  Famous_Words "The program displayed ""Hello World"""
 * 
 *  Names and values are case sensitive, that is, lowercase
 *  letters are not considered to be the same as uppercase
 *  letters. For example, FAMOUS_WORDS and Famous_Words are two
 *  different names.  / The length in bytes of NameValueString is
 *  equal to StrucLength minus MQRFH_STRUC_LENGTH_FIXED. To avoid
 *  problems with data conversion of the user data in some
 *  environments, make sure that this length is a multiple of
 *  four.  NameValueString must be padded with blanks to this
 *  length, or terminated earlier by placing a null character
 *  following the last value in the string. The null and bytes
 *  following it, up to the specified length of NameValueString,
 *  are ignored.
 */

#include "decode_rfh.h"
#include <stdio.h>

#define MAX_TOK_LEN                     1024 /* This is pretty generous */
#define EOS                             -1 /* End of input string */
#define QUOTE                           '"'

#define IS_SEPARATOR(ch)                ((ch) == ' ' || (ch) == '\0')
#define IS_VALID_ID_CHAR(ch)            (!IS_SEPARATOR(ch))
#define IS_UNQUOTED_ID_CHAR(ch)         (!IS_SEPARATOR(ch) && ch != QUOTE)
#define GETCHAR(charp, endp)            ((charp) < (endp) ? (int)*charp++ : EOS)
#define LOOKAHEAD(charp, endp)          (charp < endp - 1 ? (int)*charp : EOS)
#define PUSHBACK(ch, charp)             (*--charp = (char)ch)

#define SKIP_SEPARATORS(charp, endp) \
    while (charp < endp && IS_SEPARATOR(*charp)) ++charp

typedef enum {
    ST_INITIAL,
    ST_IN_QUOTED_ID,
    ST_IN_UNQUOTED_ID,
    ST_END,
} state_t;

static int debug_mode = 0;

int rfh_in_debug_mode(void)
{
    return debug_mode;
}

void rfh_set_debug_mode(int on_if_true)
{
    debug_mode = on_if_true;
}

#define ENUM_TUPLE(enum_val) { enum_val, #enum_val }
#define NUM_ELEMS(arr) (sizeof(arr) / sizeof(*arr))

static const char *rfh_state_to_s(state_t state)
{
    static struct
    {
        state_t state;
        const char *str;
    } state_map[] = 
    {
        ENUM_TUPLE(ST_INITIAL),
        ENUM_TUPLE(ST_IN_QUOTED_ID),
        ENUM_TUPLE(ST_IN_UNQUOTED_ID),
        ENUM_TUPLE(ST_END),
    };

    size_t i;

    for (i = 0; i < NUM_ELEMS(state_map); ++i)
        if (state_map[i].state == state)
            return state_map[i].str;

    return "";
}

const char *rfh_toktype_to_s(rfh_toktype_t toktype)
{
    static struct
    {
        rfh_toktype_t toktype;
        const char *str;
    } toktype_map[] = 
    {
        ENUM_TUPLE(TT_TOKEN),
        ENUM_TUPLE(TT_UNESCAPED_QUOTE),
        ENUM_TUPLE(TT_ILLEGAL_QUOTE),
        ENUM_TUPLE(TT_UNEXPECTED_EOS),
        ENUM_TUPLE(TT_TOKEN_TRUNCATED),
        ENUM_TUPLE(TT_INTERNAL_ERROR),
        ENUM_TUPLE(TT_ALLOC_FAILURE),
        ENUM_TUPLE(TT_END),
    };

    size_t i;

    for (i = 0; i < NUM_ELEMS(toktype_map); ++i)
        if (toktype_map[i].toktype == toktype)
            return toktype_map[i].str;

    return "";
}

static rfh_toktype_t get_token(const char **charpp, const char *endp, char *token, char *end_token)
{
    const char *charp = *charpp;
    char *tokp = token;
    int ch;
    state_t state = ST_INITIAL;
    rfh_toktype_t toktype = TT_TOKEN;

    SKIP_SEPARATORS(charp, endp);

    while (state != ST_END && tokp < end_token)
    {
        ch = GETCHAR(charp, endp);

        if (debug_mode)
            fprintf(stderr, "state = %s, char = %c [%d]\n", rfh_state_to_s(state), (char)ch, ch);
        
        switch (state)
        {
            case ST_INITIAL:
                if (ch == EOS)
                {
                    *tokp = '\0';
                    toktype = TT_END;
                    state = ST_END;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s, toktype = %s\n",
                                rfh_state_to_s(state), rfh_toktype_to_s(toktype));
                }                
                else if (IS_UNQUOTED_ID_CHAR(ch))
                {
                    *tokp++ = (char)ch;
                    state = ST_IN_UNQUOTED_ID;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s\n", rfh_state_to_s(state));
                }
                else if (ch == QUOTE)
                {
                    state = ST_IN_QUOTED_ID;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s\n", rfh_state_to_s(state));
                }
                break;

            case ST_IN_UNQUOTED_ID:
                if (IS_SEPARATOR(ch) || ch == EOS)
                {
                    *tokp = '\0';
                    state = ST_END;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s\n", rfh_state_to_s(state));
                }
                else if (ch == QUOTE) /* Not allowed in unquoted string */
                {
                    toktype = TT_ILLEGAL_QUOTE;
                    state = ST_END;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s, toktype = %s\n",
                                rfh_state_to_s(state), rfh_toktype_to_s(toktype));
                }
                else
                    *tokp++ = (char)ch;
                break;

            case ST_IN_QUOTED_ID:
                if (ch == QUOTE) /* Could be end of token, or first char of "" */
                {
                    int lookahead_ch = LOOKAHEAD(charp, endp);
                    if (lookahead_ch == QUOTE) /* Found escaped quote */
                    {
                        *tokp++ = QUOTE;
                        ++charp; /* Skip second quote */
                    }
                    else if (IS_SEPARATOR(lookahead_ch) || lookahead_ch == EOS) /* End of token */
                    {
                        *tokp = '\0';
                        state = ST_END;
                        if (debug_mode)
                            fprintf(stderr, "new state = %s\n", rfh_state_to_s(state));
                    }
                    else /* Error: Unescaped quote */
                    {
                        toktype = TT_UNESCAPED_QUOTE;
                        state = ST_END;
                        if (debug_mode)
                            fprintf(stderr, "new state = %s, toktype = %s\n",
                                    rfh_state_to_s(state), rfh_toktype_to_s(toktype));
                    }
                }
                else if (ch == EOS) /* Error */
                {
                    toktype = TT_UNEXPECTED_EOS;
                    state = ST_END;
                    if (debug_mode)
                        fprintf(stderr, "new state = %s, toktype = %s\n",
                                rfh_state_to_s(state), rfh_toktype_to_s(toktype));
                }
                else
                    *tokp++ = (char)ch;
                break;

            default:
                toktype = TT_INTERNAL_ERROR;
                state = ST_END;
                if (debug_mode)
                    fprintf(stderr, "new state = %s, toktype = %s\n",
                            rfh_state_to_s(state), rfh_toktype_to_s(toktype));
                break;
        } /* switch */
    } /* while */

    /* If state is not ST_END, an error may have occurred */
    if (state != ST_END && toktype == TT_TOKEN)
    {
        if (tokp >= end_token)
            toktype = TT_TOKEN_TRUNCATED;
        else
            toktype = TT_UNEXPECTED_EOS;

        if (debug_mode)
            fprintf(stderr, "end state = %s, toktype = %s\n",
                    rfh_state_to_s(state), rfh_toktype_to_s(toktype));
    }

    *charpp = charp;

    if (debug_mode)
        fprintf(stderr, "-- leaving; end state = %s, returned toktype = %s, returned token = [%s]\n",
                rfh_state_to_s(state), rfh_toktype_to_s(toktype), token);

    return toktype;
}

rfh_toktype_t rfh_decode_name_val_str(const char *nvstr, size_t len, CALLBACK_FN callback, void *user_data)
{
#if defined(USE_FIXED_BUFFERS)
    char name[MAX_TOK_LEN + 1];
    char value[MAX_TOK_LEN + 1];
    size_t name_len = sizeof(name);
    size_t value_len = sizeof(value);
#else
    size_t name_len = len + 1;
    size_t value_len = len + 1;
    char *buf = (char *)malloc(len * 2 * sizeof(char) + 2);
    char *name = buf;
    char *value = buf + len + 1;
#endif

    const char *charp = nvstr, *endp = nvstr + len;
    rfh_toktype_t toktype;

#if !defined(USE_FIXED_BUFFERS)
    if (buf == NULL) /* Out of mem */
        return TT_ALLOC_FAILURE;
#endif

    for (;;)
    {
        if ((toktype = get_token(&charp, endp, name, name + name_len)) != TT_TOKEN)
        {
            break;
        }

        if ((toktype = get_token(&charp, endp, value, value + value_len)) != TT_TOKEN)
        {
            if (toktype == TT_END) /* Expected a value! */
                toktype = TT_UNEXPECTED_EOS;
            break;
        }

        callback(name, value, user_data);
    }

#if !defined(USE_FIXED_BUFFERS)
    free(buf);
#endif

    return toktype;
}
