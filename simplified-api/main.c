#include "arg/arg.h"
#include <stdlib.h>

int main(int argc, char** argv)
{
	Arg exampleArg = ARG_FLAG(
	                         .shortname = 'x',
	                         .longname = "example",
	                         .help = "An example flag"
	                 );
	Arg* supportedArgs[] = { &exampleArg };
	ArgParser parser = arg_parser_new(
	                           "example-program",
	                           "An example program",
	                           ARG_OPT_ARRAY(supportedArgs)
	                   );
	char* why = NULL;
	ArgParseResult err = arg_parser_parse(&parser, argc, argv, &why);
	if (why != NULL) {
		arg_parser_show_help(&parser, stderr);
		(void)fprintf(stderr, "ERROR: %s\n", why);
		free(why);
		arg_parser_free(parser);
		return 1;
	}

	for (size_t i = 0; i < parser.optc; ++i) {
		Arg* arg = parser.optv[i];
		switch (arg->kind) {
		case ARG_KIND_POS:
			printf("Positional argument: %s = %s\n", arg->longname, arg->value);
			break;
		case ARG_KIND_FLAG:
			printf("Flag argument: %s = %s\n", arg->longname, arg->flagValue ? "true" : "false");
			break;
		case ARG_KIND_OPT:
			printf("Optional argument: %s = %s\n", arg->longname, arg->value);
			break;
		}
	}
	for (size_t i = 0; i < parser.extra_len; ++i) {
		printf("Extra argument: %s\n", parser.extra[i]);
	}

	arg_parser_free(parser);
}
