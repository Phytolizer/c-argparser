#pragma once

#include "buf.h"
#include "str.h"
#include "sum.h"

#include <stdio.h>

typedef enum {
	ARGKIND_POS,
	ARGKIND_OPT,
	ARGKIND_FLAG,
} ArgKind;

typedef struct {
	ArgKind kind;
	char shortname;
	arg_str longname;
	arg_str help;
	// set if kind == ARGKIND_OPT or ARGKIND_POS
	arg_str value;
	// set if kind == ARGKIND_FLAG
	bool flagValue;
} Arg;

#define ARG_POS(name, hlp) \
	(Arg) { .kind = ARGKIND_POS, .longname = name, .help = hlp }

#define ARG_OPT(...) \
	(Arg) { .kind = ARGKIND_OPT, __VA_ARGS__ }

#define ARG_FLAG(...) \
	(Arg) { .kind = ARGKIND_FLAG, __VA_ARGS__ }

typedef ARG_BUF(Arg*) ArgBuf;
typedef ARG_BUF(arg_str) ArgStrBuf;

typedef struct {
	ArgBuf args;
	arg_str name;
	arg_str help;
	ArgStrBuf extra;
} ArgParser;

typedef ARG_MAYBE(arg_str) ArgParseErr;

ArgParser arg_parser_new(arg_str name, arg_str help, ArgBuf args);
ArgParseErr arg_parser_parse(ArgParser* parser, int argc, char** argv);
void arg_parser_show_help(ArgParser* parser, FILE* fp);
