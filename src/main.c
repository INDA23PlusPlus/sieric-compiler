#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <parser/token.h>
#include <parser/lexer.h>
#include <parser/parser.h>
#include <parser/semantics.h>
#include <parser/code.h>

#define MAX(a, b) ((a)>(b)?(a):(b))

#define PROGRAM_NAME "compiler"

const char *help_str = ""
"Usage: "PROGRAM_NAME" [option]... infile\n"
"\n"
"  -h            print this help message\n"
"  -c            only transpile to assembly\n"
"  -o outfile    output program to outfile\n"
"  -a file       output assembly to file\n"
;

struct options {
    const char *infile, *outfile, *asmfile;
    int compile;
} options;

int main(int argc, char *argv[]) {
    int ret = EXIT_SUCCESS;

    options = (struct options){
        .outfile = "./a.out",
        .asmfile = NULL,
        .compile = 1,
    };

    int c;
    while((c = getopt(argc, argv, "hco:a:")) != -1) {
        switch(c) {
        case 'o':
            options.outfile = optarg;
            break;

        case 'c':
            options.compile = 0;
            break;

        case 'a':
            options.asmfile = optarg;
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
    ir_code_t *code;
    if(!parser->error) ast_print((void *)root);
    else goto ret_free_parser;

    semantics_analyze(semantics_new(), (void *)root);
    if(!root->ctx->error) putchar('\n'), semantics_dump_tables((void *)root);
    else goto ret_free_parser;

    code = code_new(root);
    if(code) puts("\nCode:"), code_dump(code);
    else goto ret_free_parser;

    char asm_path[] = "/tmp/dpp_XXXXXX";
    if(!options.asmfile) {
        int fd;
        if(!(fd = mkstemp(asm_path))) {
            perror("tmpfile");
            ret = EXIT_FAILURE;
            goto ret_free_code;
        }
        if(!(f = fdopen(fd, "w"))) {
            perror("fdopen");
            ret = EXIT_FAILURE;
            goto ret_free_code;
        }
        options.asmfile = asm_path;
    } else if(!(f = fopen(options.asmfile, "w"))) {
        perror("fopen");
        ret = EXIT_FAILURE;
        goto ret_free_code;
    }

    if(asm_generate(f, code))
        fprintf(stderr, "[Error] Failed to generate assembly\n");
    fclose(f);

    if(options.compile) {
        char cmd[512];
        snprintf(cmd, sizeof cmd, "nasm -felf64 %s", options.asmfile);
        system(cmd);
        snprintf(cmd, sizeof cmd, "gcc -no-pie -o '%s' %s.o",
                options.outfile, options.asmfile);
        system(cmd);
    }

ret_free_code:
    code_free(code);
ret_free_parser:
    lexer_free(lexer);
    parser_free(parser);
ret_free:
    free(buf);
    exit(ret);
}
