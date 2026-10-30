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
        const char* begin = str;
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
                    str += 2;
                    while (*str && *str != '\n')
                        str++;
                    type = TokenType_CommentLine;
                    arr_push(identifiers, (Identifier) { .type = type, .begin = begin, .size = str - begin });
                    continue;
                } break;

                case '*': { // scoped comment
                    str += 2;
                    while (*str) {
                        if (str[0] == '*' && str[1] == '/') {
                            str += 2;
                            break;
                        }
                        ++str;
                    }

                    type = TokenType_CommentScoped;
                    arr_push(identifiers, (Identifier) { .type = type, .begin = begin, .size = str - begin });
                    continue;
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

        } else if (*str == '.') {
            if (isdigit(*++str)) {
                while (isdigit(*++str)) { }
                type = TokenType_NumFloat;
            }

        } else if (isalpha(*str) || *str == '_') {
            while (isident(*++str)) { }
            type = TokenType_Symbol;

        } else {
            ++str;
        }

        if (type != TokenType_Invalid)
            arr_push(identifiers, (Identifier) { .type = type, .begin = begin, .size = str - begin });
    }
}
void printIdentifiers()
{
    printf("\n");
    for (size_t i = 0; i < identifiers.size; ++i) {
        const Identifier t = identifiers.data[i];
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

void printLexed(const char* exp)
{
    static const char* ansiColors[] = {
        "\033[0m", "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m", "\033[37m"
    };

    if (identifiers.size) {
        const Identifier t0 = identifiers.data[0];
        printf("%s%.*s", ansiColors[0], (int)(t0.begin - exp), exp);
    } else {
        printf("%s%s", ansiColors[0], exp);
    }
    for (size_t i = 0; i < identifiers.size; ++i) {
        const Identifier t0 = identifiers.data[i];
        // clang-format off
        int colorIndex = 0;
        switch (t0.type) {
        case TokenType_Sub:      { colorIndex = 2; } break;
        case TokenType_Add:      { colorIndex = 2; } break;
        case TokenType_Mul:      { colorIndex = 2; } break;
        case TokenType_Div:      { colorIndex = 2; } break;
        case TokenType_ParOpen:  { colorIndex = 1; } break;
        case TokenType_ParClose: { colorIndex = 1; } break;
        case TokenType_NumFloat: { colorIndex = 4; } break;
        case TokenType_NumInt:   { colorIndex = 4; } break;
        case TokenType_Symbol:   { colorIndex = 5; } break;
        case TokenType_CommentLine:   { colorIndex = 7; } break;
        case TokenType_CommentScoped: { colorIndex = 7; } break;
        case TokenType_Invalid:  {} break; }
        // clang-format on
        if (i == identifiers.size - 1) { // last
            printf("%s%s", ansiColors[colorIndex], t0.begin);
        } else {
            const Identifier t1 = identifiers.data[i + 1];
            printf("%s%.*s", ansiColors[colorIndex], (int)(t0.size), t0.begin);
            const char* t0end = t0.begin + t0.size;
            int gapSize = t1.begin - t0end;
            assert(gapSize >= 0);
            printf("%s%.*s", ansiColors[0], gapSize, t0end);
        }
    }
    printf("%s", ansiColors[0]);
}
