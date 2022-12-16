# C Argument Parser

A simple-enough argument parser written in pure C.

## Building

To build the library, simply run `make` in the root directory. This will
create a static library in the root directory.

## Usage

To use the library, simply include `arg.h` in your project and link with
`-largparser`.

## Example

```c
// main.c

#include "arg/arg.h"

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
	                           ARG_BUF_ARRAY(supportedArgs)
	                   );
	ArgParseErr err = arg_parser_parse(&parser, argc, argv);
	if (err.present) {
		arg_parser_show_help(&parser, stderr);
		(void)fprintf(stderr, "ERROR: " ARG_STR_FMT "\n", ARG_STR_ARG(err.value));
		arg_str_free(err.value);
		return 1;
	}
}

```
