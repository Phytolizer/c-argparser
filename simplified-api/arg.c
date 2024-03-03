// vi: ts=8 sw=8 noet
#include "arg/arg.h"
#include "arg/gbuf.h"
#include "arg/macro.h"

#include <stdarg.h>
#include <string.h>

typedef size_t* IndexBuf;

typedef struct {
	IndexBuf indices;
	size_t numConsumed;
} PositionalInfo;

static IndexBuf count_positionals(size_t optc, Arg* const* optv)
{
	IndexBuf indices = NULL;
	for (size_t i = 0; i < optc; i++) {
		if (optv[i]->kind == ARG_KIND_POS) {
			ARG_GBUF_PUSH(indices, i);
		}
	}
	return indices;
}

ArgParser arg_parser_new(char const* name, char const* help, size_t optc, Arg* const* optv)
{
	return (ArgParser) {
		.name = name,
		.optc = optc,
		.optv = optv,
		.help = help,
		.extra = NULL,
		.extra_len = 0,
	};
}

typedef struct {
	int argc;
	char* const* argv;
	int ofs;
} ArgInfo;

static char* arg__asprintf(char const* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int length = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	char* result = malloc(length + 1);
	if (result == NULL) {
		return NULL;
	}

	va_start(args, fmt);
	vsnprintf(result, length + 1, fmt, args);
	va_end(args);
	return result;
}

static ArgParseResult parse_single_longopt(ArgParser* parser, char const* arg, ArgInfo info,
                int* out_ofs, char** out_why)
{
	char const* eqPos = strchr(arg, '=');
	size_t name_len = eqPos ? eqPos - arg : strlen(arg);
	Arg* foundArg = NULL;
	for (size_t i = 0; i < parser->optc; ++i) {
		if (strlen(parser->optv[i]->longname) == name_len
		    && strncmp(parser->optv[i]->longname, arg, name_len) == 0) {
			foundArg = parser->optv[i];
			break;
		}
	}
	if (foundArg == NULL) {
		if (out_why != NULL) {
			*out_why = arg__asprintf("unknown option: '--%.*s'", name_len, arg);
		}
		return ARG_ERR;
	}

	switch (foundArg->kind) {
	case ARG_KIND_POS: {
		if (out_why != NULL) {
			*out_why = arg__asprintf("option '%.*s' is positional", name_len, arg);
		}
		return ARG_ERR;
	}
	case ARG_KIND_FLAG:
		if (eqPos != NULL) {
			if (out_why != NULL) {
				*out_why = arg__asprintf("option '--%.*s' takes no value", name_len, arg);
			}
			return ARG_ERR;
		}
		foundArg->flagValue = true;
		// didn't move the offset
		*out_ofs = info.ofs + 1;
		return ARG_OK;
	case ARG_KIND_OPT:
		if (eqPos != NULL) {
			ptrdiff_t afterEqPos = eqPos - arg + 1;
			foundArg->value = arg + afterEqPos;
			// didn't move the offset
			*out_ofs = info.ofs + 1;
			return ARG_OK;
		} else if (info.ofs + 1 >= info.argc) {
			if (out_why != NULL) {
				*out_why = arg__asprintf("option '--%.*s' requires a value", name_len, arg);
			}
			return ARG_ERR;
		} else {
			foundArg->value = info.argv[info.ofs + 1];
			// consumed the next arg
			*out_ofs = info.ofs + 2;
			return ARG_OK;
		}
	default:
		ARG_UNREACHABLE();
	}
}

static ArgParseResult parse_shortopts(ArgParser* parser, char const* arg, ArgInfo info,
                                      int* out_ofs, char** out_why)
{
	size_t len = strlen(arg);
	for (size_t i = 0; i < len; i++) {
		char shortname = arg[i];
		Arg* foundArg = NULL;
		for (size_t i = 0; i < parser->optc; ++i) {
			if (parser->optv[i]->shortname == shortname) {
				foundArg = parser->optv[i];
				break;
			}
		}
		if (foundArg == NULL) {
			if (out_why != NULL) {
				*out_why = arg__asprintf("unknown option: '-%c'", shortname);
			}
			return ARG_ERR;
		}

		switch (foundArg->kind) {
		case ARG_KIND_POS: {
			if (out_why != NULL) {
				*out_why = arg__asprintf("option '%c' is positional", shortname);
			}
			return ARG_ERR;
		}
		case ARG_KIND_FLAG:
			foundArg->flagValue = true;
			break;
		case ARG_KIND_OPT:
			if (i + 1 < len) {
				foundArg->value = arg + (i + 1);
				// consumed the rest of the arg
				*out_ofs = info.ofs + 1;
				return ARG_OK;
			} else if (info.ofs + 1 >= info.argc) {
				if (out_why != NULL) {
					*out_why = arg__asprintf("option '-%c' requires a value", shortname);
				}
			} else {
				foundArg->value = info.argv[info.ofs + 1];
				// consumed the next arg
				*out_ofs = info.ofs + 2;
				return ARG_OK;
			}
		}
	}
	*out_ofs = info.ofs + 1;
	return ARG_OK;
}

static ArgParseResult parse_positional(
        ArgParser* parser,
        char const* arg,
        ArgInfo info,
        PositionalInfo* positionals,
        int* out_ofs,
        char** out_why
)
{
	if (positionals->numConsumed >= ARG_GBUF_LEN(positionals->indices)) {
		ARG_GBUF_PUSH(parser->extra, arg);
		*out_ofs = info.ofs + 1;
		return ARG_OK;
	}

	size_t index = positionals->indices[positionals->numConsumed];
	positionals->numConsumed++;
	parser->optv[index]->value = arg;
	*out_ofs = info.ofs + 1;
	return ARG_OK;
}

static ArgParseResult parse_arg(ArgParser* parser, char const* arg, ArgInfo info,
                                PositionalInfo* positionals, int* out_ofs, char** out_why)
{
	if (arg[0] == '-') {
		if (arg[1] == '-') {
			arg += 2;
			return parse_single_longopt(parser, arg, info, out_ofs, out_why);
		}
		arg += 1;
		return parse_shortopts(parser, arg, info, out_ofs, out_why);
	}
	return parse_positional(parser, arg, info, positionals, out_ofs, out_why);
}

static char* arg__join(char const* sep, size_t count, char** strs)
{
	if (count == 0) {
		char* empty = malloc(1);
		if (empty == NULL) {
			return NULL;
		}
		empty[0] = '\0';
		return empty;
	}

	size_t sepLen = strlen(sep);
	size_t totalLen = 0;
	for (size_t i = 0; i < count; i++) {
		totalLen += strlen(strs[i]);
	}

	char* result = malloc(totalLen + sepLen * (count - 1) + 1);
	if (result == NULL) {
		return NULL;
	}

	char* p = result;
	for (size_t i = 0; i < count; i++) {
		size_t len = strlen(strs[i]);
		memcpy(p, strs[i], len);
		p += len;
		if (i + 1 < count) {
			memcpy(p, sep, sepLen);
			p += sepLen;
		}
	}
	*p = '\0';
	return result;
}

static char* generate_missing_positional_message(size_t optc, Arg* const* optv,
                PositionalInfo positionals)
{
	if (ARG_GBUF_LEN(positionals.indices) == 0) {
		ARG_UNREACHABLE();
	}

	char** buf = NULL;
	for (size_t i = positionals.numConsumed; i < ARG_GBUF_LEN(positionals.indices); i++) {
		size_t index = positionals.indices[i];
		Arg const* arg = optv[index];
		ARG_GBUF_PUSH(buf, arg__asprintf("'%s'", arg->longname));
	}
	char* joined = arg__join(", ", ARG_GBUF_LEN(buf), buf);
	for (size_t i = 0; i < ARG_GBUF_LEN(buf); i++) {
		free(buf[i]);
	}
	ARG_GBUF_FREE(buf);
	char* msg = arg__asprintf("missing positional arguments: %s", joined);
	free(joined);
	return msg;
}

ArgParseResult arg_parser_parse(ArgParser* parser, int argc, char* const* argv, char** out_why)
{
	ArgInfo info = {
		.argc = argc,
		.argv = argv,
		.ofs = 1,
	};
	PositionalInfo positionals = {
		.indices = count_positionals(parser->optc, parser->optv),
		.numConsumed = 0,
	};
	while (info.ofs < argc) {
		char const* arg = argv[info.ofs];
		ArgParseResult res = parse_arg(parser, arg, info, &positionals, &info.ofs, out_why);
		if (res == ARG_ERR) {
			ARG_GBUF_FREE(positionals.indices);
			return ARG_ERR;
		}
	}

	if (positionals.numConsumed < ARG_GBUF_LEN(positionals.indices)) {
		*out_why = generate_missing_positional_message(parser->optc, parser->optv, positionals);
		ARG_GBUF_FREE(positionals.indices);
		return ARG_ERR;
	}

	ARG_GBUF_FREE(positionals.indices);
	parser->extra_len = ARG_GBUF_LEN(parser->extra);
	return ARG_OK;
}

void arg_parser_show_help(ArgParser* parser, FILE* fp)
{
	(void)fprintf(
	        fp,
	        "%s: %s\n",
	        parser->name,
	        parser->help
	);
	if (parser->optc == 0) {
		return;
	}
	(void)fprintf(fp, "USAGE: %s [OPTIONS]", parser->name);
	IndexBuf indices = count_positionals(parser->optc, parser->optv);
	for (size_t i = 0; i < ARG_GBUF_LEN(indices); i++) {
		Arg const* arg = parser->optv[indices[i]];
		(void)fprintf(fp, " <%s>", arg->longname);
	}
	(void)fprintf(fp, "\n");
	ARG_GBUF_FREE(indices);
	(void)fprintf(fp, "OPTIONS:\n");
	for (size_t i = 0; i < parser->optc; i++) {
		Arg const* arg = parser->optv[i];
		switch (arg->kind) {
		case ARG_KIND_POS:
			(void)fprintf(fp, "  %s", arg->longname);
			break;
		case ARG_KIND_FLAG:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (strlen(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (strlen(arg->longname) > 0) {
				(void)fprintf(fp, "--%s", arg->longname);
			}
			break;
		case ARG_KIND_OPT:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (strlen(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (strlen(arg->longname) > 0) {
				(void)fprintf(fp, "--%s <VALUE>", arg->longname);
			} else {
				(void)fprintf(fp, "<VALUE>");
			}
			break;
		}
		(void)fprintf(fp, "    %s\n", arg->help);
	}
}

void arg_parser_free(ArgParser parser)
{
	ARG_GBUF_FREE(parser.extra);
}
