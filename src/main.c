#include <stdio.h>
#include <stdlib.h>

#include <parser/token.h>
#include <parser/lexer.h>

int main(void) {
    const char *str = "hejsan";
    lexer_t *lexer = lexer_create(str, 6);

    for(;;) {
        token_t *token = lexer_next(lexer);

        if(token->type == TEOF) {
            token_destroy(token);
            break;
        }

        printf("type: %s\n", token_type_str(token->type));
    }

    lexer_destroy(lexer);

    return EXIT_SUCCESS;
}
