#include "arg.h"
#include "macro.h"

#include <inttypes.h>

typedef ARG_BUF(uint64_t) IndexBuf;

typedef struct {
	IndexBuf indices;
	uint64_t numConsumed;
} PositionalInfo;

static IndexBuf count_positionals(ArgBuf args)
{
	IndexBuf indices = ARG_BUF_NEW;
	for (uint64_t i = 0; i < args.len; i++) {
		if (args.ptr[i]->kind == ARGKIND_POS) {
			ARG_BUF_PUSH(&indices, i);
		}
	}
	return indices;
}

ArgParser arg_parser_new(arg_str name, arg_str help, ArgBuf args)
{
	return (ArgParser) {
		.name = name,
		.args = args,
		.help = help,
		.extra = ARG_BUF_NEW,
	};
}

static bool longname_eq(Arg* arg, arg_str longname)
{
	return arg_str_eq(arg->longname, longname);
}

typedef struct {
	int argc;
	char** argv;
	int ofs;
} ArgInfo;

typedef ARG_RESULT(int, arg_str) ParseResult;

static ParseResult parse_single_longopt(ArgParser* parser, arg_str arg, ArgInfo info)
{
	StrFindResult eqPos = arg_str_find(arg, '=');
	arg_str name = eqPos.present
	               ? arg_str_ref_chars(arg.ptr, eqPos.value)
	               : arg;
	Arg** foundArg = NULL;
	ARG_BUF_FIND(parser->args, name, longname_eq, &foundArg);
	if (foundArg == NULL) {
		arg_str msg = arg_str_fmt("unknown option: '--" ARG_STR_FMT "'", ARG_STR_ARG(name));
		return (ParseResult)ARG_ERR(msg);
	}

	switch ((*foundArg)->kind) {
	case ARGKIND_POS: {
		arg_str msg =
		        arg_str_fmt(
		                "option '" ARG_STR_FMT "' is positional",
		                ARG_STR_ARG(name)
		        );
		return (ParseResult)ARG_ERR(msg);
	}
	case ARGKIND_FLAG:
		(*foundArg)->flagValue = true;
		// didn't move the offset
		return (ParseResult)ARG_OK(info.ofs + 1);
	case ARGKIND_OPT:
		if (eqPos.present) {
			uint64_t afterEqPos = eqPos.value + 1;
			(*foundArg)->value =
			        arg_str_ref_chars(
			                &arg.ptr[afterEqPos],
			                arg_str_len(arg) - afterEqPos
			        );
			// didn't move the offset
			return (ParseResult)ARG_OK(info.ofs + 1);
		} else if (info.ofs + 1 >= info.argc) {
			arg_str msg =
			        arg_str_fmt(
			                "option '--" ARG_STR_FMT "' requires a value",
			                ARG_STR_ARG(name)
			        );
			return (ParseResult)ARG_ERR(msg);
		} else {
			(*foundArg)->value = arg_str_ref(info.argv[info.ofs + 1]);
			// consumed the next arg
			return (ParseResult)ARG_OK(info.ofs + 2);
		}
	default:
		ARG_UNREACHABLE();
	}
}

static bool shortname_eq(Arg* arg, char shortname)
{
	return arg->shortname == shortname;
}

static ParseResult parse_shortopts(ArgParser* parser, arg_str arg, ArgInfo info)
{
	for (uint64_t i = 0; i < arg_str_len(arg); i++) {
		char shortname = arg.ptr[i];
		Arg** foundArg = NULL;
		ARG_BUF_FIND(parser->args, shortname, shortname_eq, &foundArg);
		if (foundArg == NULL) {
			arg_str msg = arg_str_fmt("unknown option: '-%c'", shortname);
			return (ParseResult)ARG_ERR(msg);
		}

		switch ((*foundArg)->kind) {
		case ARGKIND_POS: {
			arg_str msg = arg_str_fmt("option '%c' is positional", shortname);
			return (ParseResult)ARG_ERR(msg);
		}
		case ARGKIND_FLAG:
			(*foundArg)->flagValue = true;
			break;
		case ARGKIND_OPT:
			if (i + 1 < arg_str_len(arg)) {
				(*foundArg)->value = arg_str_shifted(arg, i + 1);
				// consumed the rest of the arg
				return (ParseResult)ARG_OK(info.ofs + 1);
			} else if (info.ofs + 1 >= info.argc) {
				return (ParseResult)ARG_ERR(
				               arg_str_fmt(
				                       "option '-%c' requires a value",
				                       shortname
				               )
				       );
			} else {
				(*foundArg)->value = arg_str_ref(info.argv[info.ofs + 1]);
				// consumed the next arg
				return (ParseResult)ARG_OK(info.ofs + 2);
			}
		}
	}
	return (ParseResult)ARG_OK(info.ofs + 1);
}

static ParseResult parse_positional(
        ArgParser* parser,
        arg_str arg,
        ArgInfo info,
        PositionalInfo* positionals
)
{
	if (positionals->numConsumed >= positionals->indices.len) {
		ARG_BUF_PUSH(&parser->extra, arg);
		return (ParseResult)ARG_OK(info.ofs + 1);
	}

	uint64_t index = positionals->indices.ptr[positionals->numConsumed];
	positionals->numConsumed++;
	parser->args.ptr[index]->value = arg;
	return (ParseResult)ARG_OK(info.ofs + 1);
}

static ParseResult parse_arg(ArgParser* parser, arg_str arg, ArgInfo info,
                             PositionalInfo* positionals)
{
	if (arg.ptr[0] == '-') {
		if (arg.ptr[1] == '-') {
			arg = arg_str_ref_chars(arg.ptr + 2, arg_str_len(arg) - 2);
			return parse_single_longopt(parser, arg, info);
		}
		arg = arg_str_ref_chars(arg.ptr + 1, arg_str_len(arg) - 1);
		return parse_shortopts(parser, arg, info);
	}
	return parse_positional(parser, arg, info, positionals);
}

static arg_str generate_missing_positional_message(ArgBuf args, PositionalInfo positionals)
{
	if (positionals.indices.len == 0) {
		ARG_UNREACHABLE();
	}

	ArgStrBuf buf = ARG_BUF_NEW;
	for (uint64_t i = positionals.numConsumed; i < positionals.indices.len; i++) {
		uint64_t index = positionals.indices.ptr[i];
		Arg* arg = args.ptr[index];
		ARG_BUF_PUSH(&buf, arg_str_fmt("'" ARG_STR_FMT "'", ARG_STR_ARG(arg->longname)));
	}
	arg_str joined = arg_str_join(arg_str_lit(", "), buf.len, buf.ptr);
	for (uint64_t i = 0; i < buf.len; i++) {
		arg_str_free(buf.ptr[i]);
	}
	ARG_BUF_FREE(buf);
	arg_str msg =
	        arg_str_fmt(
	                "missing positional arguments: " ARG_STR_FMT,
	                ARG_STR_ARG(joined)
	        );
	arg_str_free(joined);
	return msg;
}

ArgParseErr arg_parser_parse(ArgParser* parser, int argc, char** argv)
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
		arg_str arg = arg_str_ref(argv[info.ofs]);
		ParseResult res = parse_arg(parser, arg, info, &positionals);
		if (!res.ok) {
			return (ArgParseErr)ARG_JUST(res.get.error);
		}
		info.ofs = res.get.value;
	}

	if (positionals.numConsumed < positionals.indices.len) {
		arg_str msg = generate_missing_positional_message(parser->args, positionals);
		ARG_BUF_FREE(positionals.indices);
		return (ArgParseErr)ARG_JUST(msg);
	}

	ARG_BUF_FREE(positionals.indices);
	return (ArgParseErr)ARG_NOTHING;
}

void arg_parser_show_help(ArgParser* parser, FILE* fp)
{
	(void)fprintf(
	        fp,
	        ARG_STR_FMT ": " ARG_STR_FMT "\n",
	        ARG_STR_ARG(parser->name),
	        ARG_STR_ARG(parser->help)
	);
	if (parser->args.len == 0) {
		return;
	}
	(void)fprintf(fp, "USAGE: " ARG_STR_FMT " [OPTIONS]", ARG_STR_ARG(parser->name));
	IndexBuf indices = count_positionals(parser->args);
	for (uint64_t i = 0; i < indices.len; i++) {
		Arg* arg = parser->args.ptr[indices.ptr[i]];
		(void)fprintf(fp, " <" ARG_STR_FMT ">", ARG_STR_ARG(arg->longname));
	}
	(void)fprintf(fp, "\n");
	ARG_BUF_FREE(indices);
	(void)fprintf(fp, "OPTIONS:\n");
	for (uint64_t i = 0; i < parser->args.len; i++) {
		Arg* arg = parser->args.ptr[i];
		switch (arg->kind) {
		case ARGKIND_POS:
			(void)fprintf(fp, "  " ARG_STR_FMT, ARG_STR_ARG(arg->longname));
			break;
		case ARGKIND_FLAG:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (arg_str_len(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (arg_str_len(arg->longname) > 0) {
				(void)fprintf(fp, "--" ARG_STR_FMT, ARG_STR_ARG(arg->longname));
			}
			break;
		case ARGKIND_OPT:
			if (arg->shortname != 0) {
				(void)fprintf(fp, "  -%c", arg->shortname);
				if (arg_str_len(arg->longname) > 0) {
					(void)fprintf(fp, ", ");
				} else {
					(void)fprintf(fp, " ");
				}
			} else {
				(void)fprintf(fp, "      ");
			}
			if (arg_str_len(arg->longname) > 0) {
				(void)fprintf(
				        fp,
				        "--" ARG_STR_FMT " <VALUE>",
				        ARG_STR_ARG(arg->longname)
				);
			} else {
				(void)fprintf(fp, "<VALUE>");
			}
			break;
		}
		(void)fprintf(fp, "    " ARG_STR_FMT "\n", ARG_STR_ARG(arg->help));
	}
}
