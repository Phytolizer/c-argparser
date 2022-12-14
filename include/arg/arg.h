#pragma once

#include "gbuf.h"
#include "str.h"
#include "sum.h"

#include <stdio.h>

typedef enum {
	ARG_KIND_POS,
	ARG_KIND_OPT,
	ARG_KIND_FLAG,
} ArgKind;

typedef struct {
	ArgKind kind;
	char shortname;
	ArgStr longname;
	ArgStr help;
	// set if kind == ARGKIND_OPT or ARGKIND_POS
	ArgStr value;
	// set if kind == ARGKIND_FLAG
	bool flagValue;
} Arg;

#define ARG_POS(name, hlp) \
	(Arg) { .kind = ARG_KIND_POS, .longname = name, .help = hlp }

#define ARG_OPT(...) \
	(Arg) { .kind = ARG_KIND_OPT, __VA_ARGS__ }

#define ARG_FLAG(...) \
	(Arg) { .kind = ARG_KIND_FLAG, __VA_ARGS__ }

typedef ARG_GBUF(Arg*) ArgBuf;
typedef ARG_GBUF(ArgStr) ArgStrBuf;

#define ARG_BUF_ARRAY(a) (ArgBuf) ARG_GBUF_ARRAY(a)
#define ARG_BUF_REF(p, sz) (ArgBuf) ARG_GBUF_REF(p, sz)

typedef struct {
	ArgBuf args;
	ArgStr name;
	ArgStr help;
	ArgStrBuf extra;
} ArgParser;

typedef ARG_MAYBE(ArgStr) ArgParseErr;

ArgParser arg_parser_new(ArgStr name, ArgStr help, ArgBuf args);
ArgParseErr arg_parser_parse(ArgParser* parser, int argc, char** argv);
void arg_parser_show_help(ArgParser* parser, FILE* fp);
