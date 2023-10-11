#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <parser/token.h>
#include <parser/lexer.h>

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
    char *buf = malloc(sz);
    if(fread(buf, 1, sz, f) != sz) {
        fprintf(stderr, "Failed to read entire file\n");
        fclose(f);
        ret = EXIT_FAILURE;
        goto ret_free;
    }
    fclose(f);

    lexer_t *lexer = lexer_create(buf, sz - 1);

    for(;;) {
        token_t *token = lexer_next(lexer);

        if(token->type == TEOF) {
            token_destroy(token);
            break;
        }

        printf("type: %s (", token_type_str(token->type));
        for(size_t i = 0; i < token->sz; i++)
            putchar(token->start[i]);
        puts(")");
    }

    lexer_destroy(lexer);
ret_free:
    free(buf);
    exit(ret);
}
