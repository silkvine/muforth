/*
 * $Id$
 *
 * This file is part of muforth.
 *
 * Copyright (c) 1997-2005 David Frech. All rights reserved, and all wrongs
 * reversed.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * or see the file LICENSE in the top directory of this distribution.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* basic tools to compile code and data */

#include "muforth.h"

void mu_push_h()        /* "here" code space pointer */
{
    PUSH(&pcd);
}

void mu_push_r()        /* "ram" space pointer */
{
    PUSH(&pdt);
}

void mu_push_s0()       /* address of stack bottom */
{
    PUSH(S0);
}

void mu_push_sp()       /* address of sp variable */
{
    PUSH(&SP);
}

/* the char *string param here does _not_ need to be zero-terminated!! */
static char *compile_counted_string(char *string, size_t length)
{
    struct counted_string *cs;
    struct string s;

    cs = (struct counted_string *)pdt;
    s.data = string;
    s.length = length;
    cs->length = s.length;  /* prefix count cell */
    memcpy(&cs->data[0], s.data, s.length);
    cs->data[s.length] = 0; /* zero terminate */
    return &cs->data[0];
}

/*
 * copy string; return a counted string (addr of first character;
 * prefix count cell _precedes_ first character of string).
 * This does _not_ allot space in the dictionary!
 */
void mu_scrabble()  /* ( a u - z") */
{
    T = (cell)compile_counted_string((char *)SND, T);
    NIP;
}

/*
 * this is _only_ called from C; zstring _must_ be zero-terminated!!
 * this routine _does_ allot space in the dictionary!
 */
char *to_counted_string(char *zstring)
{
    size_t length;
    char *counted_string;

    length = strlen(zstring);
    counted_string = compile_counted_string(zstring, length);
    pdt = (uint8 *)ALIGNED(counted_string + length + 1);  /* count null */
    return counted_string;
}

void mu_colon()
{
    mu_make_new_name();
    mu_set_colon_code();
    mu_minus_rbracket();
}

void mu_semicolon()
{
    PUSH(&p_mu_exit);
    mu_compile();
    mu_lbracket();
}

void mu_set_colon_code() { *pcd++ = (cell)&mu_do_colon; }
void mu_set_does_code()  { *pcd++ = (cell)&mu_do_does; }

void mu_compile() { *pcd++ = POP; }

/*
  compiler definitions
  : ^   lit unnest compile,  ;   ( right now this doesn't do anything fancy)
  : ;     \ ^  \ [  ;
  forth definitions
*/


