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
#if !defined(DECODE_RFH_INCLUDED)
#define DECODE_RFH_INCLUDED

#include <stdlib.h> /* For size_t */

typedef void (*CALLBACK_FN)(const char *name, const char *value, void *user_data);

typedef enum {
    TT_TOKEN,
    TT_UNESCAPED_QUOTE, /* '"' not followed by '"' in quoted string */
    TT_ILLEGAL_QUOTE,   /* '"' in unquoted string */
    TT_UNEXPECTED_EOS,
    TT_TOKEN_TRUNCATED, /* Token buffer too short to receive full token */
    TT_INTERNAL_ERROR,
    TT_ALLOC_FAILURE,
    TT_END
} rfh_toktype_t;

/* Returns TT_END on success, other on error */
rfh_toktype_t
rfh_decode_name_val_str(const char *nvstr,
                        size_t len,
                        CALLBACK_FN callback,
                        void *user_data);

/* Translates toktype_t to string */
const char *rfh_toktype_to_s(rfh_toktype_t toktype);

int rfh_in_debug_mode(void);
void rfh_set_debug_mode(int on_if_true);

#endif
