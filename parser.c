
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sbo_array.h"

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


ARR_DECLARE(TokenStack, Token, 100);
TokenStack tokens;

ARR_DECLARE(OpHoldingStack, TokenType, 10);
OpHoldingStack opstack;

int get_precedence(TokenType op)
{
    switch (op) {
    case TokenType_Plus:  return 2;
    case TokenType_Minus: return 2;
    case TokenType_Mul:   return 3;
    case TokenType_Div:   return 3;
    default: return 0;
    }
}

bool is_operator(TokenType tokenType) { return tokenType >= TokenType_Minus && tokenType <= TokenType_Div; }

// clang-format on

bool isident(char c) { return isalnum(c) || c == '_'; }

const char* evalNum(const char* str)
{
    const char* strEnd = str;
    if (isdigit(*str) || *str == '.') {
        double val = strtod(str, (char**)(void*)&strEnd);
        if (strEnd != str) {
            if (opstack.size != 0 && arr_top(opstack) == TokenType_Negate) {
                arr_pop(opstack);
                arr_push(tokens, (Token) { .type = TokenType_Num, .number = -val });
            } else {
                arr_push(tokens, (Token) { .type = TokenType_Num, .number = val });
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
        arr_push(tokens, (Token) { .type = TokenType_Symbol, .string = (String) //
                             { .start = strBegin, .size = (size_t)str - (size_t)strBegin } });
    }
    return str;
}

void pushOp(TokenType op)
{
    arr_push(opstack, op);
    if (opstack.size == 0) {
    } else {
    }
}

const char* tokenize(const char* str)
{
    arr_clear(tokens);
    arr_clear(opstack);

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

            if (tokens.size == 0) {
                pushOp(TokenType_Negate);

            } else {
                if (isident(str[1])) {
                    TokenType topType = arr_top(opstack);
                    if (topType == TokenType_Symbol || topType == TokenType_Num)
                        pushOp(TokenType_Minus);
                    else
                        pushOp(TokenType_Negate);
                } else {
                    pushOp(TokenType_Minus);
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
            pushOp(TokenType_Plus);
            str++;
            continue;
        }

        case '*': {
            pushOp(TokenType_Mul);
            str++;
            continue;
        }

        case '/': {
            pushOp(TokenType_Div);
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

    for (size_t i = 0; i < tokens.size; ++i) {
        const Token t = tokens.data[i];
        printf("\nToken %zu: ", i);

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
        case TokenType_Invalid:
        case TokenType_Div:
            break;
        }
    }
    return tokens.size;
}
