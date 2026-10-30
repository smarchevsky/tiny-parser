#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "sbo_array.h"

static bool isident(char c) { return isalnum(c) || c == '_'; }

typedef enum : uint8_t {
    TokenType_Invalid = '\0',
    TokenType_NumFloat,
    TokenType_NumInt,
    TokenType_Symbol,
    TokenType_CommentLine,
    TokenType_CommentScoped,
    TokenType_ParOpen = '(', // 40
    TokenType_ParClose = ')',
    TokenType_Mul = '*',
    TokenType_Add = '+',
    TokenType_Sub = '-',
    TokenType_Div = '/',
} TokenType;

typedef struct {
    TokenType type;
    const char* begin;
    size_t size;
} Identifier;

ARR_DECLARE(Identifiers, Identifier, 100);
Identifiers identifiers;

void lex(const char* str)
{
    arr_clear(identifiers);

    while (*str) {
        const char *begin = str, *end = NULL;
        TokenType type = TokenType_Invalid;

        if (isspace(*str)) {
            ++str;
            continue;

        } else if (strchr("()*+-/", *str) != NULL) {
            switch (*str) {
            case '(':
            case ')':
            case '*':
            case '+':
            case '-':
                type = (TokenType)*str;
                break;

            case '/':
                switch (str[1]) {

                case '/': { // line comment
                    begin = str + 2;
                    while (*++str && *str != '\n') { }
                    type = TokenType_CommentLine, end = str;
                } break;

                case '*': { // scoped comment
                    str += 2;
                    begin = str;
                    while (*str && !(str[0] == '*' && str[1] == '/'))
                        ++str;

                    type = TokenType_CommentScoped, end = str;
                    ++str;
                } break;

                default:
                    type = TokenType_Div;
                    break;
                }
            }
            ++str;

        } else if (isdigit(*str)) {
            while (isdigit(*++str)) { }
            if (*str == '.') {
                while (isdigit(*++str)) { }
                type = TokenType_NumFloat;
            } else {
                type = TokenType_NumInt;
            }
            end = str;

        } else if (*str == '.') {
            if (isdigit(*++str)) {
                while (isdigit(*++str)) { }
                type = TokenType_NumFloat;
            }
            end = str;

        } else if (isalpha(*str) || *str == '_') {
            while (isident(*++str)) { }
            type = TokenType_Symbol;
            end = str;

        } else {
            ++str;
        }

        // if (type != TokenType_Invalid)
        arr_push(identifiers, (Identifier) { .type = type, .begin = begin, .size = end - begin });
    }

    for (size_t i = 0; i < identifiers.size; ++i) {
        const Identifier t = identifiers.data[i];
        // symbol:a- ( symbol:b+ symbol:c)
        // clang-format off
        switch (t.type) {
        case TokenType_Sub:      { printf("-"); } break;
        case TokenType_Add:      { printf("+"); } break;
        case TokenType_Mul:      { printf("*"); } break;
        case TokenType_Div:      { printf("/"); } break;

        case TokenType_ParOpen:  { printf("("); } break;
        case TokenType_ParClose: { printf(")"); } break;

        case TokenType_NumFloat: { printf("float:%.*s(%zu)",  (int)t.size, t.begin, t.size); } break;
        case TokenType_NumInt:   { printf("int:%.*s(%zu)",    (int)t.size, t.begin, t.size); } break;
        case TokenType_Symbol:   { printf("symbol:%.*s(%zu)", (int)t.size, t.begin, t.size); } break;

        case TokenType_CommentLine:   { printf("comment_line:%.*s(%zu)",   (int)t.size, t.begin, t.size); } break;
        case TokenType_CommentScoped: { printf("comment_scoped:%.*s(%zu)", (int)t.size, t.begin, t.size); } break;

        case TokenType_Invalid:  { printf("?"); } break;
        }
        // clang-format on
        printf(" ");
    }
    fflush(stdout);
}
