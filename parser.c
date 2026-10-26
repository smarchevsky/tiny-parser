
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sbo_array.h"

// clang-format off
typedef enum : uint8_t {
    TokenType_Invalid, TokenType_Num, TokenType_Symbol, TokenType_Negate,
    TokenType_Sub, TokenType_Add, TokenType_Mul, TokenType_Div,
    TokenType_ParOpen, TokenType_ParClose
} TokenType;

typedef struct {
    const char* start;
    size_t size;
} String;

typedef struct {
    TokenType type;
    //union { String string;
        double number;
    //};
} Token;


ARR_DECLARE(TokenStack, Token, 10);
TokenStack tokens;

ARR_DECLARE(OpHoldingStack, TokenType, 4);
OpHoldingStack opstack;

void pushToken(Token t) { arr_push(tokens, t); }

int get_precedence(TokenType op)
{
    switch (op) {
    case TokenType_ParClose: return 1;
    case TokenType_ParOpen: return 0;
    case TokenType_Add:   return 4;
    case TokenType_Sub:  return 4;
    case TokenType_Mul:    return 5;
    case TokenType_Div:    return 5;
    case TokenType_Negate: return 20;
    default:
    assert(false && "no existing precedence for operator");
    return 0;
    }
}

bool is_op_binary(TokenType tokenType) { return tokenType >= TokenType_Sub && tokenType <= TokenType_Div; }
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

        // pushToken((Token) { .type = TokenType_Symbol, .string = (String) //
        //     { .start = strBegin, .size = (size_t)str - (size_t)strBegin } });
    }
    return str;
}

bool tryResolveArithmetic()
{
    if (opstack.size >= 2) {
        if (opstack.data[opstack.size - 1] == TokenType_ParClose
            && opstack.data[opstack.size - 2] == TokenType_ParOpen) {
            arr_pop(opstack);
            arr_pop(opstack);
            return true;
        }
    }

    // return false;

    if (is_op_unary(arr_top(opstack))) {
        if (tokens.size < 1)
            return false;

        if (arr_top(opstack) == TokenType_Negate && arr_top(tokens).type == TokenType_Num) {
            arr_pop(opstack);
            tokens.data[tokens.size - 1].number *= -1;
            return true;
        } else
            return false;
    }

    if (is_op_binary(arr_top(opstack))) {
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
        case TokenType_Sub: {
            result = t1.number - t0.number;
        } break;
        case TokenType_Add: {
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
        return true;
    }

    return false;
}

void pushOp(TokenType opNew)
{
    while (opstack.size) {
        if (get_precedence(opNew) <= get_precedence(arr_top(opstack))) {
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

    while (*str) {
        if (isspace(*str)) {
            str++;
            continue;
        }

        switch (*str) {
        case '-': {
            if (str[1] == '-') {
                return "unsupported --";
            }

            if (tokens.size == 0) {
                pushOp(TokenType_Negate);
                break;
            }

            TokenType opTop = arr_top(tokens).type;
            bool mustNegate = is_op_binary(opTop) || opTop == TokenType_ParOpen;
            pushOp(mustNegate ? TokenType_Negate : TokenType_Sub);
            break;
        }

        case '+': {
            if (str[1] == '+')
                return "unsupported ++";

            pushOp(TokenType_Add);
            break;
        }

        case '*': {
            pushOp(TokenType_Mul);
            break;
        }

        case '/': {
            pushOp(TokenType_Div);
            break;
        }

        case '(': {
            arr_push(opstack, TokenType_ParOpen);
            break;
        }

        case ')': {
            pushOp(TokenType_ParClose);
            break;
        }

        default: {
            const char* newPos;
            if ((newPos = evalSymbol(str)) != str) {
                str = newPos;
                continue;
            }

            if ((newPos = evalNum(str)) != str) {
                str = newPos;
                continue;
            }
        }
        }

        str++;
    }

    while (opstack.size) {
        if (!tryResolveArithmetic())
            pushToken((Token) { .type = arr_pop(opstack) });
    }

    return NULL;
}

int parse(const char* exp)
{
    const char* errorMsg = tokenize(exp);
    if (errorMsg) {
        printf("ERR: %s", errorMsg);
        return 1;
    }

    if (tokens.size == 1 && tokens.data[0].type == TokenType_Num) {
        printf("RES: ");
    } else {
        printf("RPN: ");
    }

    for (size_t i = 0; i < tokens.size; ++i) {
        const Token t = tokens.data[i];

        // clang-format off
        switch (t.type) {
        case TokenType_Negate: { printf("negate"); } break;
        case TokenType_Sub:  { printf("sub"); } break;
        case TokenType_Add:   { printf("add"); } break;
        case TokenType_Mul:    { printf("mul"); } break;
        case TokenType_Div:    { printf("div"); } break;
        case TokenType_ParOpen: { printf("("); } break;
        case TokenType_ParClose: { printf(")"); } break;
        case TokenType_Num:      { printf("%.2f", t.number); } break;
        //case TokenType_Symbol: { printf("%.*s", (int)t.string.size, t.string.start); } break;
        case TokenType_Invalid: { } break;
        }
        // clang-format on
        printf("  ");
        fflush(stdout);
    }
    return tokens.size;
}
