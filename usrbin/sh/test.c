/*
    test.c - test logical expressions containing strings, numbers, and files.
    Copyright (C) 1992 Stephan Neuhaus

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
    You can reach me at neuhaus@informatik.uni-kl.de, or by writing
    paper mail to
    
    Stephan Neuhaus
    Hilgardring 32
    D-6750 Kaiserslautern
    Germany
*/

#include <compiler.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Return value after bad expressions, bad arguments, etc. */
#define EXIT_ERROR 2

/* Operators */
#define OP_AND 1	/* Logical and */
#define OP_OR 2		/* Logical or */
#define OP_NOT 3	/* Logical not */

#define OP_NUMEQ 4	/* Numeric equality */
#define OP_NUMNE 5	/* Numeric inequality */
#define OP_NUMGE 6	/* Numeric greater than or equal */
#define OP_NUMGT 7	/* Numeric greater than */
#define OP_NUMLE 8	/* Numeric less than or equal */
#define OP_NUMLT 9	/* Numeric less than */

#define OP_STREQ 10	/* String equality */
#define OP_STRNE 11	/* String inequality */
#define OP_STRGE 12	/* String greater than or equal */
#define OP_STRGT 13	/* String greater than */
#define OP_STRLE 14	/* String less than or equal */
#define OP_STRLT 15	/* String less than */

#define OP_EXFILE 16	/* File exists */
#define OP_REGFILE 17	/* File exists and is a regular file */
#define OP_DIRFILE 18	/* File exists and is a directory file */
#define OP_BLKFILE 19	/* File exists and is a block special file */
#define OP_CHRFILE 20	/* File exists and is a character special file */
#define OP_LNKFILE 21	/* File exists and is a link */
#define OP_FIFFILE 22	/* File exists and is a fifo file */
#define OP_RFILE 23	/* File exists and is a readable file */
#define OP_WFILE 24	/* File exists and is a writable file */
#define OP_XFILE 25	/* File exists and is a executable file */


/* Token types */
#define TOK_EOF 1
#define TOK_OPERATOR 2
#define TOK_LPAREN 3
#define TOK_RPAREN 4
#define TOK_STRING 6

struct token {
  int type;
  union {
    int operator;
    const char *operand;
  } value;
};

static struct token curr_token;
static int token_saved;
static int tokens_left;
static int token_index;
static const char *const *tokens;

static const char *program_name;

static __EXITING error (const char *format, ...);
static void message (const char *format, ...);

static void set_tokens (int argc, const char *const *argv);
static int get_token (void);
static void unget_token (void);
static void accept (int type);
static int more_tokens (void);

static int expression (void);
static int term (void);
static int factor (void);
static int file_op (int operator, const char *file_name);
static int string_op (int operator, const char *opd1, const char *opd2);
static int num_op (int operator, const char *opd1, const char *opd2);

static int is_unary_op (int operator);
static int is_binary_op (int operator);
static int is_string_op (int operator);
static int is_num_op (int operator);
static int is_number (const char *string, int *value);

static const char *op_name (int operator);
static const char *type_name (int token_type);
static const char *token_name (const struct token *tok);

static int bool_to_status (int boolval);

#ifdef DEBUG
static int indent_level = 0;

static void indent (void)
{
  int i;
  for (i = 0; i < indent_level; i++)
    fprintf (stderr, "  ");
}

static void do_enter (const char *format, ...)
{
  va_list args;
  
  indent ();
  indent_level++;
  fprintf (stderr, "Entering ");
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

static void do_leave (const char *format, ...)
{
  va_list args;
  
  indent_level--;
  indent ();
  fprintf (stderr, "Leaving ");
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

static void do_out (const char *format, ...)
{
  va_list args;
  
  indent ();
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

#  define enter(x) do_enter x
#  define leave(x) do_leave x
#  define out(x) do_out x
#else
#  define enter(x)
#  define leave(x)
#  define out(x)
#endif


int
main (int argc, const char *const *argv)
{
  int retval;
    
  program_name = argv[0];

  set_tokens (argc, argv);
  
  if (get_token () == TOK_EOF)
    retval = 1;		/* Empty expression is true */
  else
    {
      unget_token ();
      retval = expression ();
    }
  
  if (get_token () != TOK_EOF)
    error ("garbage at end of expression");

  exit (bool_to_status (retval));
    
  /* NOTREACHED */
  return bool_to_status (retval);
}


static int
expression (void)
{
  /* Please don't optimize the expression below to
   * `expr_val = expr_val || term()' or some such
   * thing, because then the C short-circuit evalu-
   * ation will bite you in the back, because term()
   * isn't called when expr_val is already true.  Same
   * holds for the code in term().  */
  int term_val;
  int expr_val;
  
  enter (("expression"));
  expr_val = term ();
  while (get_token () == TOK_OPERATOR && curr_token.value.operator == OP_OR)
    {
      out (("expression: or"));
      term_val = term ();
      expr_val = expr_val || term_val;
    }

  unget_token ();
  leave (("expression: %d", expr_val));
  return expr_val;
}


static int
term (void)
{
  int factor_val;
  int term_val;
  
  enter (("term"));
  term_val = factor ();
  while (get_token () == TOK_OPERATOR && curr_token.value.operator == OP_AND)
    {
      out (("term: and"));
      factor_val = factor ();
      term_val = term_val && factor_val;
    }

  unget_token ();
  leave (("term: %d", term_val));
  return term_val;
}


static int
factor (void)
{
  int token_type;
  int retval;
  const char *operand1;
  const char *operand2;
  int file_operator;
  int binary_operator;
    
  enter (("factor"));
  token_type = get_token ();   
  if (token_type == TOK_LPAREN)
    {
      retval = expression ();
      accept (TOK_RPAREN);
    }
  else if (token_type == TOK_OPERATOR)
    {
      if (!is_unary_op (curr_token.value.operator))
        error ("factor: expected unary operator, found %s",
               op_name (curr_token.value.operator));
      if (curr_token.value.operator == OP_NOT)
        retval = !expression ();
      else 
        {
          file_operator = curr_token.value.operator;
          if (get_token () != TOK_STRING)
            error ("factor: expected pathname, found %s",
                   token_name (&curr_token));
          retval = file_op (file_operator, curr_token.value.operand);
        }
    }
  else
    {
      /* Binary operator.  Got first operand in curr_token */
      operand1 = curr_token.value.operand;
      token_type = get_token ();
      binary_operator = curr_token.value.operator;
      if (token_type != TOK_OPERATOR || !is_binary_op (binary_operator))
        error ("factor: expected binary operator, found %s",
               token_name (&curr_token));
      if (get_token () != TOK_STRING)
        error ("factor: expected operand for binary operator, found %s",
               token_name (&curr_token));
      operand2 = curr_token.value.operand;
      if (is_string_op (binary_operator))
        retval = string_op (binary_operator, operand1, operand2);
      else if (is_num_op (binary_operator))
        retval = num_op (binary_operator, operand1, operand2);
      else
        error ("factor: internal error: %s is neither string nor numeric op",
               op_name (binary_operator));
    }

  leave (("factor: %d", retval));
  return retval;
}


static int
string_op (int operator, const char *operand1, const char *operand2)
{
  int retval;
  int compare_result = strcmp (operand1, operand2);
  
  enter (("string_op (\"%s\" %s \"%s\")", operand1, op_name (operator), operand2));
  switch (operator)
    {
    case OP_STREQ: retval = compare_result == 0; break;
    case OP_STRNE: retval = compare_result != 0; break;
    case OP_STRGE: retval = compare_result >= 0; break;
    case OP_STRGT: retval = compare_result > 0; break;
    case OP_STRLE: retval = compare_result <= 0; break;
    case OP_STRLT: retval = compare_result < 0; break;
    default:
      error ("string_op: unknown string operator %s", op_name (operator));
      /* NOTREACHED */
      break;
    }
  leave (("string_op", retval));
  return retval;
}


static int
num_op (int operator, const char *operand1, const char *operand2)
{
  int retval;
  int value1, value2;

  enter (("num_op (%s %s %s)", operand1, op_name (operator), operand2));
  if (!is_number (operand1, &value1))
    error ("num_op: expected number, found %s", operand1);
  if (!is_number (operand2, &value2))
    error ("num_op: expected number, found %s", operand2);

  switch (operator)
    {
    case OP_NUMEQ: retval = value1 == value2; break;
    case OP_NUMNE: retval = value1 != value2; break;
    case OP_NUMGE: retval = value1 >= value2; break;
    case OP_NUMGT: retval = value1 > value2; break;
    case OP_NUMLE: retval = value1 <= value2; break;
    case OP_NUMLT: retval = value1 < value2; break;
    default:
      error ("num_op: unknown numeric operator %s", op_name (operator));
      /* NOTREACHED */
      break;
    }
  leave (("num_op: %d", retval));
  return retval;
}


static int
file_op (int operator, const char *pathname)
{
  struct stat statb;
  int retval;
  int access_mode;

  enter (("file_op (%s \"%s\")", op_name (operator), pathname));
  if (operator == OP_RFILE || operator == OP_WFILE || operator == OP_XFILE)
    {
      switch (operator)
        {
        case OP_RFILE: access_mode = R_OK; break;
        case OP_WFILE: access_mode = W_OK; break;
        case OP_XFILE: access_mode = X_OK; break;
        /* NOTREACHED */
        default: access_mode = F_OK; break;
        }
      retval = access (pathname, access_mode) >= 0;
    }
  else if (stat (pathname, &statb) < 0)
    retval = 0;
  else
    {
      switch (operator)
        {
        case OP_EXFILE: retval = 1; break;
        case OP_REGFILE: retval = (statb.st_mode & S_IFMT) == S_IFREG; break;
        case OP_DIRFILE: retval = (statb.st_mode & S_IFMT) == S_IFDIR; break;
        case OP_BLKFILE: retval = (statb.st_mode & S_IFMT) == S_IFBLK; break;
        case OP_CHRFILE: retval = (statb.st_mode & S_IFMT) == S_IFCHR; break;
        case OP_LNKFILE: retval = (statb.st_mode & S_IFMT) == S_IFLNK; break;
        case OP_FIFFILE: retval = (statb.st_mode & S_IFMT) == S_IFIFO; break;
        default:
          error ("file_op: unknown file operator %s", op_name (operator));
          break;
        }
    }

  leave (("file_op: %d", retval));
  return retval;
}


static int
is_number (const char *string, int *value)
{
  const char *curchar;
  int digit;
  int sign;
  
  *value = 0;

  if (string[0] == '-')
    {
      sign = -1;
      curchar = string + 1;
    }
  else
    {
      sign = 1;
      curchar = string;
    }

  /* Accumulate return value as negative number in all cases, because that
   * makes overflow impossible when string denotes INT_MIN.  (Assumes two's
   * complement representation of integers.)  */
  if (*curchar == '\0' || !isdigit (*curchar))
    return 0;
  else
    {
      *value = -(*curchar - '0');
      curchar++;
    }
      
  for (; *curchar != '\0'; curchar++)
    {
      if (isdigit (*curchar))
        {
          digit = *curchar - '0';
          if (*value < (INT_MIN + digit)/10)
            {
              message ("is_number: %s overflows integer range", string);
              return 0;
            }
          else
            *value = 10*(*value) - digit;
        }
      else
        return 0;
    }

  if (sign == 1 && *value == INT_MIN)
      {
        message ("is_number: %s just overflows positive integers", string);
        return 0;
      }
  else if (sign == 1)
    *value = -(*value);
  
  return 1;
}


static void
set_tokens (int argc, const char *const *argv)
{
  tokens_left = argc - 1;
  if (strcmp (argv[tokens_left], "]") == 0)
    tokens_left--;
    
  tokens = argv + 1;  
  token_index = 0;
  token_saved = 0;
}


static int
get_token (void)
{
  struct operator_name {
    const char *name;
    int value;
  };
  static const struct operator_name names[] = {
    {"-a", OP_AND},     {"-o", OP_OR},      {"!", OP_NOT},
    {"=", OP_STREQ},   {"!=", OP_STRNE},   {">=", OP_STRGE},
    {">", OP_STRGT},    {"<=", OP_STRLE},   {"<", OP_STRLT},
    {"-eq", OP_NUMEQ},  {"-ne", OP_NUMNE},  {"-ge", OP_NUMGE},
    {"-gt", OP_NUMGT},  {"-le", OP_NUMLE},  {"-lt", OP_NUMLT},
    {"-e", OP_EXFILE},  {"-f", OP_REGFILE}, {"-d", OP_DIRFILE},
    {"-b", OP_BLKFILE}, {"-c", OP_CHRFILE}, {"-l", OP_LNKFILE},
    {"-F", OP_FIFFILE}, {"-r", OP_RFILE},   {"-w", OP_WFILE},
    {"-x", OP_XFILE},   {0, 0}
  };
  const struct operator_name *search;
  const char *token_string;
    
  if (token_saved || curr_token.type == TOK_EOF)
    ;
  else if (!more_tokens ())
    curr_token.type = TOK_EOF;
  else if (strcmp (tokens[token_index], "(") == 0)
    {
      token_index++;
      tokens_left--;
      curr_token.type = TOK_LPAREN;
    }
  else if (strcmp (tokens[token_index], ")") == 0)
    {
      token_index++;
      tokens_left--;
      curr_token.type = TOK_RPAREN;
    }
  else
    {
      token_string = tokens[token_index];
      token_index++;
      tokens_left--;
      for (search = names; search->name != 0; search++)
        if (strcmp (token_string, search->name) == 0)
          break;
          
      if (search->name == 0)
        {
          curr_token.type = TOK_STRING;
          curr_token.value.operand = token_string;
        }
      else
        {
          curr_token.type = TOK_OPERATOR;
          curr_token.value.operator = search->value;
        }
    }
  token_saved = 0;
  out (("get_token: Returning token %s", token_name (&curr_token)));
  return curr_token.type;
}


static void
unget_token (void)
{
  if (token_saved)
    error ("unget_token: unget token when token was already saved");
  token_saved = 1;
}


static void
accept (int type)
{
  int token_type = get_token ();
  if (token_type != type)
    error ("accept: expected token type %s, found token %s",
           type_name (token_type), token_name (&curr_token));
}

static int
more_tokens (void)
{
  return tokens_left != 0;
}


static int
is_num_op (int operator)
{
  return operator == OP_NUMEQ || operator == OP_NUMNE ||
    operator == OP_NUMGE || operator == OP_NUMGT || operator == OP_NUMLE ||
    operator == OP_NUMLT;
}


static int
is_string_op (int operator)
{
  return operator == OP_STREQ || operator == OP_STRNE ||
    operator == OP_STRGE || operator == OP_STRGT || operator == OP_STRLE ||
    operator == OP_STRLT;
}


static int
is_unary_op (int operator)
{
  return operator == OP_NOT || operator == OP_EXFILE ||
    operator == OP_REGFILE || operator == OP_DIRFILE ||
    operator == OP_BLKFILE || operator == OP_CHRFILE ||
    operator == OP_LNKFILE || operator == OP_FIFFILE ||
    operator == OP_RFILE || operator == OP_WFILE ||
    operator == OP_XFILE;
}


static int
is_binary_op (int operator)
{
  return operator == OP_AND || operator == OP_OR ||
    operator == OP_NUMEQ || operator == OP_NUMNE ||
    operator == OP_NUMGE || operator == OP_NUMGT ||
    operator == OP_NUMLE || operator == OP_NUMLT ||
    operator == OP_STREQ || operator == OP_STRNE ||
    operator == OP_STRGE || operator == OP_STRGT ||
    operator == OP_STRLE || operator == OP_STRLT;
}


static __EXITING
error (const char *format, ...)
{
  va_list args;
#ifdef PRINTERRORS
  fprintf (stderr, "%s: ", program_name);
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
#endif
  exit (EXIT_ERROR);
}


static void
message (const char *format, ...)
{
  va_list args;

  fprintf (stderr, "%s: ", program_name);
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}


static const char *
op_name (int operator)
{
  struct op_name {
    const char *name;
    int value;
  };
  static const struct op_name names[] = {
    {"-a", OP_AND},     {"-o", OP_OR},      {"!", OP_NOT},
    {"=", OP_STREQ},    {"!=", OP_STRNE},   {">=", OP_STRGE},
    {">", OP_STRGT},    {"<=", OP_STRLE},   {"<", OP_STRLT},
    {"-eq", OP_NUMEQ},  {"-ne", OP_NUMNE},  {"-ge", OP_NUMGE},
    {"-gt", OP_NUMGT},  {"-le", OP_NUMLE},  {"-lt", OP_NUMLT},
    {"-e", OP_EXFILE},  {"-f", OP_REGFILE}, {"-d", OP_DIRFILE},
    {"-b", OP_BLKFILE}, {"-c", OP_CHRFILE}, {"-l", OP_LNKFILE},
    {"-F", OP_FIFFILE}, {"-r", OP_RFILE},   {"-w", OP_WFILE},
    {"-x", OP_XFILE},   {0, 0}
  };
  const struct op_name *search;
  
  for (search = names; search->value != 0; search++)
    if (search->value == operator)
      return search->name;
  return 0;
}


static const char *
type_name (int type)
{
  struct type_name {
    int type;
    const char *name;
  };
  static const struct type_name names[] = {
    {TOK_EOF, "EOF"}, {TOK_OPERATOR, "operator"},
    {TOK_STRING, "string"}, {TOK_LPAREN, "left parenthesis"},
    {TOK_RPAREN, "right parenthesis"}, {0, 0}
  };
  const struct type_name *search;
  
  for (search = names; search->type != 0; search++)
    if (search->type == type)
      return search->name;
  return 0;
}


static const char *
token_name (const struct token *tok)
{
  if (tok->type == TOK_EOF || tok->type == TOK_LPAREN ||
      tok->type == TOK_RPAREN)
    return type_name (tok->type);
  else if (tok->type == TOK_OPERATOR)
    return op_name (tok->value.operator);
  else
    return tok->value.operand;
}

  
static int
bool_to_status (int boolval)
{
  return boolval ? EXIT_SUCCESS : EXIT_FAILURE;
}


