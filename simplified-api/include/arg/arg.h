#pragma once

#include <stdbool.h>
#include <stdio.h>

typedef enum {
	ARG_KIND_POS,
	ARG_KIND_OPT,
	ARG_KIND_FLAG,
} ArgKind;

typedef struct {
	ArgKind kind;
	char shortname;
	char const* longname;
	char const* help;
	// set if kind == ARGKIND_OPT or ARGKIND_POS
	char const* value;
	// set if kind == ARGKIND_FLAG
	bool flagValue;
} Arg;

#define ARG_POS(name, hlp) \
	(Arg) { .kind = ARG_KIND_POS, .longname = name, .help = hlp }

#define ARG_OPT(...) \
	(Arg) { .kind = ARG_KIND_OPT, __VA_ARGS__ }

#define ARG_FLAG(...) \
	(Arg) { .kind = ARG_KIND_FLAG, __VA_ARGS__ }

typedef struct {
	size_t optc;
	Arg* const* optv;
	char const* name;
	char const* help;
	char const** extra;
	size_t extra_len;
} ArgParser;

typedef enum {
	ARG_ERR,
	ARG_OK,
} ArgParseResult;

ArgParser arg_parser_new(char const* name, char const* help, size_t optc, Arg* const* optv);
ArgParseResult arg_parser_parse(ArgParser* parser, int argc, char* const* argv, char** out_why);
void arg_parser_show_help(ArgParser* parser, FILE* fp);
void arg_parser_free(ArgParser parser);

#define ARG_OPT_ARRAY(a) (sizeof(a) / sizeof(a[0])), a
