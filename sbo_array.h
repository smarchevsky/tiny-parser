#ifndef SBO_ARRAY_H
#define SBO_ARRAY_H

#include <assert.h>
#include <string.h>

// clang-format off
#define ARR_DECLARE(TypeName, sbo_size) \
typedef struct { \
    TypeName* data; \
    size_t cap; \
    size_t size; \
    TypeName sbo[sbo_size]; \
}

#define arr_init(arr) \
do { \
    arr.data = arr.sbo; \
    arr.cap = sizeof(arr.sbo) / sizeof(arr.sbo[0]); \
    arr.size = 0; \
} while (0)

#define arr_clear(arr) \
do { \
    if (arr.data != arr.sbo) { \
        free(arr.data); \
        arr.data = arr.sbo; \
    } \
    arr.cap = sizeof(arr.sbo) / sizeof(arr.sbo[0]); \
    arr.size = 0; \
} while (0)

#define arr_push(arr, ...) \
do { \
    if (arr.size >= arr.cap) { \
        size_t new_cap = arr.cap * 2; \
        void* new_data; \
        if (arr.data == arr.sbo) { \
            new_data = malloc(new_cap * sizeof(arr.data[0])); \
            assert(new_data && "malloc failed"); \
            memcpy(new_data, arr.sbo, arr.size * sizeof(arr.data[0])); /* NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */ \
        } else { \
            new_data = realloc(arr.data, new_cap * sizeof(arr.data[0])); \
            assert(new_data && "realloc failed"); \
        } \
        arr.data = new_data; \
        arr.cap = new_cap; \
    } \
    arr.data[arr.size++] = (__VA_ARGS__); \
} while (0)

#define arr_top(arr) (assert(arr.size > 0 && "arr_top: Array is empty!"), arr.data[arr.size - 1])
#define arr_pop(arr) (assert(arr.size > 0 && "arr_pop: Array is empty!"), arr.data[--arr.size])

#endif // SBO_ARRAY_H
