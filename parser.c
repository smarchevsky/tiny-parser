
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sbo_array.h"

const char* const error_unsupported_unary_op = "unsupported unary operators";

// clang-format off
typedef enum : uint8_t {
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


ARR_DECLARE(TokenStack, Token, 10);
TokenStack tokens;

ARR_DECLARE(OpHoldingStack, TokenType, 4);
OpHoldingStack opstack;

void pushToken(Token t) { arr_push(tokens, t); }

int get_precedence(TokenType op)
{
    switch (op) {
    case TokenType_Plus:   return 2;
    case TokenType_Minus:  return 2;
    case TokenType_Mul:    return 3;
    case TokenType_Div:    return 3;
    case TokenType_Negate: return 1000;
    default: return 0;
    }
}

bool is_op_binary(TokenType tokenType) { return tokenType >= TokenType_Minus && tokenType <= TokenType_Div; }
bool is_op_unary(TokenType tokenType) { return tokenType == TokenType_Negate; }

// clang-format on

bool isident(char c) { return isalnum(c) || c == '_'; }

const char* evalNum(const char* str)
{
    const char* strEnd = str;
    if (isdigit(*str) || *str == '.') {
        double val = strtod(str, (char**)(void*)&strEnd);
        if (strEnd != str) {
            pushToken((Token) { .type = TokenType_Num, .number = val });
        }
    }
    return strEnd;
}

const char* evalSymbol(const char* str)
{
    const char* strBegin = str;
    if (isalpha(*str) || *str == '_') {
        str++;
        while (isident(*str))
            str++;

        pushToken((Token) { .type = TokenType_Symbol, .string = (String) //
            { .start = strBegin, .size = (size_t)str - (size_t)strBegin } });
    }
    return str;
}

bool tryResolveArithmetic()
{
    // return false;
    while (opstack.size) {
        if (is_op_unary(arr_top(opstack))) {
            if (tokens.size < 1)
                return false;

            if (arr_top(opstack) == TokenType_Negate && arr_top(tokens).type == TokenType_Num) {
                arr_pop(opstack);
                tokens.data[tokens.size - 1].number *= -1;
                return true;
            } else
                return false;

        } else if (is_op_binary(arr_top(opstack))) {
            if (tokens.size < 2)
                return false;

            Token t0 = tokens.data[tokens.size - 1];
            Token t1 = tokens.data[tokens.size - 2];
            TokenType op = arr_top(opstack);
            if (t0.type != TokenType_Num || t1.type != TokenType_Num)
                return false;
            arr_pop(tokens);
            arr_pop(tokens);
            arr_pop(opstack);
            double result = 0.0;
            switch (op) {
            case TokenType_Minus: {
                result = t1.number - t0.number;
            } break;
            case TokenType_Plus: {
                result = t1.number + t0.number;
            } break;
            case TokenType_Mul: {
                result = t1.number * t0.number;
            } break;
            case TokenType_Div: {
                result = t1.number / t0.number;
            } break;
            default:
                break;
            }
            pushToken((Token) { .type = TokenType_Num, .number = result });
        } else
            return false;
    }
    return true;
}

void pushOp(TokenType opNew)
{
    while (opstack.size > 0) {
        char opTop = arr_top(opstack);

        if (get_precedence(opNew) <= get_precedence(opTop)) {
            if (!tryResolveArithmetic())
                pushToken((Token) { .type = arr_pop(opstack) });

        } else {
            break;
        }
    }
    arr_push(opstack, opNew);
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
    while (opstack.size) {
        if (!tryResolveArithmetic())
            pushToken((Token) { .type = arr_pop(opstack) });
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

    if (tokens.size == 1 && tokens.data[0].type == TokenType_Num) {
        printf("RES: ");
    } else {
        printf("RPN: ");
    }

    for (size_t i = 0; i < tokens.size; ++i) {
        const Token t = tokens.data[i];

        switch (t.type) {
        case TokenType_Negate: {
            printf("negate");
        } break;

        case TokenType_Minus: {
            printf("sub");
        } break;

        case TokenType_Plus: {
            printf("add");
        } break;

        case TokenType_Mul: {
            printf("mul");
        } break;

        case TokenType_Div: {
            printf("div");
        } break;

        case TokenType_Num: {
            printf("%.2f", t.number);
        } break;

        case TokenType_Symbol: {
            printf("%.*s", (int)t.string.size, t.string.start);
        } break;

        case TokenType_Invalid: {
        } break;
        }
        printf("  ");
        fflush(stdout);
    }
    return tokens.size;
}
