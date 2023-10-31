#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <parser/token.h>
#include <parser/lexer.h>
#include <parser/parser.h>
#include <parser/semantics.h>

#define PROGRAM_NAME "compiler"

const char *help_str = ""
"Usage: "PROGRAM_NAME" [option]... infile\n"
"\n"
"  -h            print this help message\n"
"  -o outfile    output program to outfile\n"
;

struct options {
    const char *infile, *outfile;
} options;

int main(int argc, char *argv[]) {
    int ret = EXIT_SUCCESS;

    options = (struct options){
        .outfile = "./a.out",
    };

    int c;
    while((c = getopt(argc, argv, "ho:")) != -1) {
        switch(c) {
        case 'o':
            options.outfile = optarg;
            break;

        default:
            fprintf(stderr, "%s", help_str);
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind, argv += optind;

    if(argc < 1) {
        fprintf(stderr, "%s", help_str);
        exit(EXIT_FAILURE);
    }
    options.infile = argv[0];

    FILE *f;
    if(!(f = fopen(options.infile, "r"))) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);
    unsigned char *buf = malloc(sz);
    if(fread(buf, 1, sz, f) != sz) {
        fprintf(stderr, "Failed to read entire file\n");
        fclose(f);
        ret = EXIT_FAILURE;
        goto ret_free;
    }
    fclose(f);

    lexer_t *lexer = lexer_new(buf, sz - 1);
    parser_t *parser = parser_new(lexer);
    ast_node_tu_t *root = parser_parse(parser);
    if(!parser->error) ast_print((void *)root);
    else goto ret_free_parser;

    semantics_analyze(semantics_new(), (void *)root);
    if(!root->ctx->error) semantics_dump_tables((void *)root);
    else goto ret_free_parser;

ret_free_parser:
    lexer_free(lexer);
    parser_free(parser);
ret_free:
    free(buf);
    exit(ret);
}
