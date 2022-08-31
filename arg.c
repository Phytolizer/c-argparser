#include "arg.h"
#include "macro.h"

#include <inttypes.h>

typedef BUF(uint64_t) IndexBuf;

typedef struct {
	IndexBuf indices;
	uint64_t numConsumed;
} PositionalInfo;

static IndexBuf count_positionals(ArgBuf args)
{
	IndexBuf indices = BUF_NEW;
	for (uint64_t i = 0; i < args.len; i++) {
		if (args.ptr[i]->kind == ARGKIND_POS) {
			BUF_PUSH(&indices, i);
		}
	}
	return indices;
}

ArgParser argparser_new(str name, str help, ArgBuf args)
{
	return (ArgParser) {
		.name = name,
		.args = args,
		.help = help,
		.extra = BUF_NEW,
	};
}

static bool longname_eq(Arg* arg, str longname)
{
	return str_eq(arg->longname, longname);
}

typedef struct {
	int argc;
	char** argv;
	int ofs;
} ArgInfo;

typedef RESULT(int, str) ParseResult;

static ParseResult parse_single_longopt(ArgParser* parser, str arg, ArgInfo info)
{
	StrFindResult eqPos = str_find(arg, '=');
	str name = eqPos.present
	           ? str_ref_chars(arg.ptr, eqPos.value)
	           : arg;
	Arg** foundArg = NULL;
	BUF_FIND(parser->args, name, longname_eq, &foundArg);
	if (foundArg == NULL) {
		return (ParseResult)ERR(str_fmt("unknown option: '--" STR_FMT "'", STR_ARG(name)));
	}

	switch ((*foundArg)->kind) {
	case ARGKIND_POS: {
		str msg = str_fmt("option '" STR_FMT "' is positional", STR_ARG(name));
		return (ParseResult)ERR(msg);
	}
	case ARGKIND_FLAG:
		(*foundArg)->flagValue = true;
		// didn't move the offset
		return (ParseResult)OK(info.ofs + 1);
	case ARGKIND_OPT:
		if (eqPos.present) {
			uint64_t afterEqPos = eqPos.value + 1;
			(*foundArg)->value =
			        str_ref_chars(&arg.ptr[afterEqPos], str_len(arg) - afterEqPos);
			// didn't move the offset
			return (ParseResult)OK(info.ofs + 1);
		} else if (info.ofs + 1 >= info.argc) {
			str msg = str_fmt("option '--" STR_FMT "' requires a value", STR_ARG(name));
			return (ParseResult)ERR(msg);
		} else {
			(*foundArg)->value = str_ref(info.argv[info.ofs + 1]);
			// consumed the next arg
			return (ParseResult)OK(info.ofs + 2);
		}
	}
}

static bool shortname_eq(Arg* arg, char shortname)
{
	return arg->shortname == shortname;
}

static ParseResult parse_shortopts(ArgParser* parser, str arg, ArgInfo info)
{
	for (uint64_t i = 0; i < str_len(arg); i++) {
		char shortname = arg.ptr[i];
		Arg** foundArg = NULL;
		BUF_FIND(parser->args, shortname, shortname_eq, &foundArg);
		if (foundArg == NULL) {
			return (ParseResult)ERR(str_fmt("unknown option: '-%c'", shortname));
		}

		switch ((*foundArg)->kind) {
		case ARGKIND_POS:
			return (ParseResult)ERR(str_fmt("option '%c' is positional", shortname));
		case ARGKIND_FLAG:
			(*foundArg)->flagValue = true;
			break;
		case ARGKIND_OPT:
			if (i + 1 < str_len(arg)) {
				(*foundArg)->value = str_shifted(arg, i + 1);
				// consumed the rest of the arg
				return (ParseResult)OK(info.ofs + 1);
			} else if (info.ofs + 1 >= info.argc) {
				return (ParseResult)ERR(
				               str_fmt(
				                       "option '-%c' requires a value",
				                       shortname
				               )
				       );
			} else {
				(*foundArg)->value = str_ref(info.argv[info.ofs + 1]);
				// consumed the next arg
				return (ParseResult)OK(info.ofs + 2);
			}
		}
	}
	return (ParseResult)OK(info.ofs + 1);
}

static ParseResult parse_positional(
        ArgParser* parser,
        str arg,
        ArgInfo info,
        PositionalInfo* positionals
)
{
	if (positionals->numConsumed >= positionals->indices.len) {
		BUF_PUSH(&parser->extra, arg);
		return (ParseResult)OK(info.ofs + 1);
	}

	uint64_t index = positionals->indices.ptr[positionals->numConsumed];
	positionals->numConsumed++;
	parser->args.ptr[index]->value = arg;
	return (ParseResult)OK(info.ofs + 1);
}

static ParseResult parse_arg(ArgParser* parser, str arg, ArgInfo info, PositionalInfo* positionals)
{
	if (arg.ptr[0] == '-') {
		if (arg.ptr[1] == '-') {
			arg = str_ref_chars(arg.ptr + 2, str_len(arg) - 2);
			return parse_single_longopt(parser, arg, info);
		}
		arg = str_ref_chars(arg.ptr + 1, str_len(arg) - 1);
		return parse_shortopts(parser, arg, info);
	}
	return parse_positional(parser, arg, info, positionals);
}

str generate_missing_positional_message(ArgBuf args, PositionalInfo positionals)
{
	if (positionals.indices.len == 0) {
		UNREACHABLE();
	}

	StrBuf buf = BUF_NEW;
	for (uint64_t i = positionals.numConsumed; i < positionals.indices.len; i++) {
		uint64_t index = positionals.indices.ptr[i];
		Arg* arg = args.ptr[index];
		BUF_PUSH(&buf, str_fmt("'" STR_FMT "'", STR_ARG(arg->longname)));
	}
	str joined = str_join(str_lit(", "), buf.len, buf.ptr);
	for (uint64_t i = 0; i < buf.len; i++) {
		str_free(buf.ptr[i]);
	}
	BUF_FREE(buf);
	str msg = str_fmt("missing positional arguments: " STR_FMT, STR_ARG(joined));
	str_free(joined);
	return msg;
}

ArgParseErr argparser_parse(ArgParser* parser, int argc, char** argv)
{
	ArgInfo info = {
		.argc = argc,
		.argv = argv,
		.ofs = 1,
	};
	PositionalInfo positionals = {
		.indices = count_positionals(parser->args),
		.numConsumed = 0,
	};
	while (info.ofs < argc) {
		str arg = str_ref(argv[info.ofs]);
		ParseResult res = parse_arg(parser, arg, info, &positionals);
		if (!res.ok) {
			return (ArgParseErr)JUST(res.get.error);
		}
		info.ofs = res.get.value;
	}

	if (positionals.numConsumed < positionals.indices.len) {
		str msg = generate_missing_positional_message(parser->args, positionals);
		BUF_FREE(positionals.indices);
		return (ArgParseErr)JUST(msg);
	}

	BUF_FREE(positionals.indices);
	return (ArgParseErr)NOTHING;
}

void argparser_show_help(ArgParser* parser, FILE* fp)
{
	(void)fprintf(fp, STR_FMT ": " STR_FMT "\n", STR_ARG(parser->name), STR_ARG(parser->help));
	if (parser->args.len == 0) {
		return;
	}
	(void)fprintf(fp, "USAGE: " STR_FMT " [OPTIONS]", STR_ARG(parser->name));
	IndexBuf indices = count_positionals(parser->args);
	for (uint64_t i = 0; i < indices.len; i++) {
		Arg* arg = parser->args.ptr[indices.ptr[i]];
		(void)fprintf(fp, " <" STR_FMT ">", STR_ARG(arg->longname));
	}
	(void)fprintf(fp, "\n");
	BUF_FREE(indices);
	(void)fprintf(fp, "OPTIONS:\n");
	for (uint64_t i = 0; i < parser->args.len; i++) {
		Arg* arg = parser->args.ptr[i];
		switch (arg->kind) {
		case ARGKIND_POS:
			(void)fprintf(fp, "  " STR_FMT, STR_ARG(arg->longname));
			break;
		case ARGKIND_FLAG:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (str_len(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (str_len(arg->longname) > 0) {
				(void)fprintf(fp, "--" STR_FMT, STR_ARG(arg->longname));
			}
			break;
		case ARGKIND_OPT:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (str_len(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (str_len(arg->longname) > 0) {
				(void)fprintf(fp, "--" STR_FMT " <VALUE>", STR_ARG(arg->longname));
			} else {
				(void)fprintf(fp, "<VALUE>");
			}
			break;
		}
		(void)fprintf(fp, "    " STR_FMT "\n", STR_ARG(arg->help));
	}
}
