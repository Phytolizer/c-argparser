#include "arg.h"

int main(int argc, char** argv)
{
	Arg exampleArg = ARG_FLAG(
	                         .shortname = 'x',
	                         .longname = arg_str_lit("example"),
	                         .help = arg_str_lit("An example flag")
	                 );
	Arg* supportedArgs[] = { &exampleArg };
	ArgParser parser = arg_parser_new(
	                           arg_str_lit("example-program"),
	                           arg_str_lit("An example program"),
	                           (ArgBuf)ARG_BUF_ARRAY(supportedArgs)
	                   );
	ArgParseErr err = arg_parser_parse(&parser, argc, argv);
	if (err.present) {
		arg_parser_show_help(&parser, stderr);
		(void)fprintf(stderr, "ERROR: " ARG_STR_FMT "\n", ARG_STR_ARG(err.value));
		arg_str_free(err.value);
		return 1;
	}
}
