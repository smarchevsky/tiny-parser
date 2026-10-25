
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKEN 100

const char* const error_unsupported_unary_op = "unsupported unary operators";

// clang-format off
typedef enum {
    TokenType_Invalid, TokenType_Num, TokenType_Symbol, TokenType_Negate,
    TokenType_Minus, TokenType_Plus, TokenType_Mul, TokenType_Div
} TokenType;

typedef struct {
    const char* start;
    size_t size;
} String;

typedef struct {
    TokenType type;
    union {
        String string;
        double number;
    };
} Token;

// clang-format on

Token tokens[MAX_TOKEN];
size_t tokenNum = 0;

#define tokenStackPush(...) (assert(tokenNum < MAX_TOKEN), tokens[tokenNum++] = (__VA_ARGS__))
#define tokenStackPop() (assert(tokenNum > 0 && tokenNum <= MAX_TOKEN), tokens[--tokenNum])
#define tokenStackTop() (assert(tokenNum > 0 && tokenNum <= MAX_TOKEN), tokens[tokenNum - 1])
#define tokenStackClear() (tokenNum = 0)
#define tokenStackIsEmpty() (tokenNum == 0)

bool isident(char c) { return isalnum(c) || c == '_'; }

const char* evalNum(const char* str)
{
    const char* strEnd = str;
    if (isdigit(*str) || *str == '.') {
        double val = strtod(str, &strEnd);
        if (strEnd != str) {
            if (!tokenStackIsEmpty() && tokenStackTop().type == TokenType_Negate) {
                tokenStackPop();
                tokenStackPush((Token) { .type = TokenType_Num, .number = -val });
            } else {
                tokenStackPush((Token) { .type = TokenType_Num, .number = val });
            }
        }
    }
    return strEnd;
}

const char* evalSymbol(const char* str)
{
    const char* strBegin = str;
    if (isalpha(*str) || *str == '_') {
        str++;
        while (isident(*str)) {
            str++;
        }
        tokenStackPush((Token) { .type = TokenType_Symbol, .string = (String) { .start = strBegin, .size = (size_t)str - (size_t)strBegin } });
    }
    return str;
}

const char* tokenize(const char* str)
{
    tokenStackClear();
    const char* errorMsg = NULL;
    const char* newPos;

    while (*str) {
        if (isspace(*str)) {
            str++;
            continue;
        }

        if ((newPos = evalSymbol(str)) != str) {
            str = newPos;
            continue;
        }

        if ((newPos = evalNum(str)) != str) {
            str = newPos;
            continue;
        }

        switch (*str) {
        case '-': {
            if (str[1] == '-') {
                errorMsg = error_unsupported_unary_op;
                goto ErrorExit;
            }

            if (tokenStackIsEmpty()) {
                tokenStackPush((Token) { .type = TokenType_Negate });

            } else {
                if (isident(str[1])) {
                    TokenType topType = tokenStackTop().type;
                    if (topType == TokenType_Symbol || topType == TokenType_Num)
                        tokenStackPush((Token) { .type = TokenType_Minus });
                    else
                        tokenStackPush((Token) { .type = TokenType_Negate });
                } else {
                    tokenStackPush((Token) { .type = TokenType_Minus });
                }
            }
            str++;
            continue;
        }

        case '+': {
            if (str[1] == '+') {
                errorMsg = error_unsupported_unary_op;
                goto ErrorExit;
            }
            tokenStackPush((Token) { TokenType_Plus });
            str++;
            continue;
        }

        case '*': {
            tokenStackPush((Token) { TokenType_Mul });
            str++;
            continue;
        }
        }

        str++;
    }

ErrorExit:
    return errorMsg;
}

int parse(const char* exp)
{
    const char* errorMsg = tokenize(exp);
    if (errorMsg) {
        printf("Error: %s.", errorMsg);
        return 1;
    }

    for (size_t i = 0; i < tokenNum; ++i) {
        const Token t = tokens[i];
        printf("\nToken %d: ", i);

        switch (t.type) {
        case TokenType_Negate: {
            printf("Negate");
        } break;

        case TokenType_Minus: {
            printf("Minus");
        } break;

        case TokenType_Plus: {
            printf("Plus");
        } break;

        case TokenType_Mul: {
            printf("Mul");
        } break;

        case TokenType_Num: {
            printf("Number: %f", t.number);
        } break;

        case TokenType_Symbol: {
            printf("Symbol: %.*s", (int)t.string.size, t.string.start);
        } break;
        }
    }
    return tokenNum;
}
