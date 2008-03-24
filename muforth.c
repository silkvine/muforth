/*
 * This file is part of muFORTH: http://pages.nimblemachines.com/muforth
 *
 * Copyright (c) 1997-2008 David Frech. All rights reserved, and all wrongs
 * reversed. (See the file COPYRIGHT for details.)
 */

#include "muforth.h"
#include "version.h"

#include <stdlib.h>
#include <sys/mman.h>

/* data stack */
cell stack[STACK_SIZE];
cell *SP;

/* return stack */
xtk *rstack[STACK_SIZE];
xtk **RP;

cell   TOP;      /* top of stack */
xtk   *IP;     /* instruction pointer */
xtk    W;      /* on entry, points to the current Forth word */

int  cmd_line_argc;
char **cmd_line_argv;

int   names_size;   /* count of bytes alloted to names */

cell  *pcd0;   /* pointer to start of code & names space */
uint8 *pdt0;   /* ... data space */

cell  *pcd;    /* ptrs to next free byte in code & names space */
uint8 *pdt;    /* ... data space */


/* XXX: Gross hack alert! */
char *ate_the_stack;
char *ate_the_rstack;
char *isnt_defined;

static void init_stacks()
{
    mu_sp_reset();
    RP = R0;
}

static void mu_find_init_file()
{
    /* These don't have to be proper counted strings because they are
     * used only by C code hereafter.
     */
    PUSH("startup.mu4");
    mu_readable_q();
    if (POP) return;

    PUSH("/usr/local/share/muforth/startup.mu4");
    mu_readable_q();
    if (POP) return;

    die("couldn't find startup.mu4 init file");
}

void mu_push_code_size()
{
    PUSH(((caddr_t)pcd - (caddr_t)pcd0) - names_size);
}

void mu_push_names_size()
{
    PUSH(names_size);
}

void mu_push_data_size()
{
    PUSH(pdt - pdt0);
}

static void allocate()
{
    pcd0 = (cell *)  mmap(0, 256 * 4096, PROT_READ | PROT_WRITE,
                            MAP_ANON | MAP_PRIVATE, -1, 0);

    pdt0 = (uint8 *) mmap(0, 1024 * 4096, PROT_READ | PROT_WRITE,
                            MAP_ANON | MAP_PRIVATE, -1, 0);

    if (pcd0 == MAP_FAILED || pdt0 == MAP_FAILED)
        die("couldn't allocate memory");

    /* init compiler ptrs */
    pcd = pcd0;
    pdt = pdt0;
}

/*
 * This is a horrendous hack. gcc 3.3 is smart enough to let me do what
 * I want using initializers, but 2.95 complains. So I have to run a bit
 * of code that compiles some strings into the dictionary, and sets a
 * few globals to point to them. It's really ugly.
 */
static void make_constant_strings()
{
    ate_the_stack = to_counted_string("ate the stack");
    ate_the_rstack = to_counted_string("ate the return stack");
    isnt_defined =  to_counted_string("isn't defined");
}

/*
  ( We need to rebuild the command line - says something about the
  inefficiency of C's way of doing cmd line params - and parse that.)

  ( copy with trailing blank, but don't include the blank in the new addr)
  : "copy  ( to from u - to+u)   push  over  r@ cmove  pop +  bl over c!  ;

  : command-line  ( skip first one: the program name)
  ram  argv cell+  ram  argc 1- ?for  
  over @  dup string-length "copy  1+ ( keep blank)  cell u+  next  then
  nip  ( start end)  over -  ( a u)  dup allot ( aligns) ;
*/

static struct counted_string *pcmd_line;

static char *copy_string(char *to, char *from, size_t length)
{
    bcopy(from, to, length);
    to += length;
    *to++ = ' ';
    return to;
}

static void convert_command_line(int argc, char *argv[])
{
    char *pline;

    /* skip arg[0] */
    argc--;
    argv++;

    pcmd_line = (struct counted_string *)pdt;
    pline = pcmd_line->data;

    while (argc--)
    {
        pline = copy_string(pline, *argv, strlen(*argv));
        argv++;
    }
    pcmd_line->length = pline - pcmd_line->data;

    /* null terminate and align */
    pdt = (uint8 *)pline;
    *pdt++ = 0;
    pdt = (uint8 *)ALIGNED(pdt);
}

void mu_push_command_line()
{
    PUSH(&pcmd_line->data);
}

void mu_push_build_time()
{
    PUSH(build_time);
}

static void mu_start_up()
{
    PUSH("warm");       /* push the token "warm" */
    PUSH(4);
    _mu__lbracket();    /* ... and execute it! */
}

void mu_bye()
{
    exit(0);
}

int main(int argc, char *argv[])
{
    allocate();
    init_dict();
    convert_command_line(argc, argv);
    make_constant_strings();    /* XXX: Hack! */
    init_stacks();
    mu_find_init_file();
    mu_load_file();
    mu_start_up();
    return 0;
}

