#include <stdio.h>
#include <stdlib.h>

#include <parser/token.h>
#include <parser/lexer.h>

int main(void) {
    const char str[] = "let hej = 56 == 5 | 6 && 3;";
    lexer_t *lexer = lexer_create(str, sizeof str - 1);

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

    return EXIT_SUCCESS;
}
