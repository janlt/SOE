#ifndef SOEUTIL_H_
#define SOEUTIL_H_

#include <stdint.h>
#include <stddef.h>

#ifndef soe_log
#define soe_log fprintf
#endif

#ifndef SOE_INFO
#define SOE_INFO stdout
#endif

#ifndef SOE_DEBUG
#define SOE_DEBUG stdout
#endif

#ifndef SOE_ERROR
#define SOE_ERROR stderr
#endif

#pragma once

#if defined __cplusplus
   #define SOE_EXTERNC_BEGIN extern "C" {
   #define SOE_EXTERNC_END }
   #define SOE_EXTERNC extern "C"
   #define SOE_EXTERNC_EXT extern
#else
   #define SOE_EXTERNC_BEGIN
   #define SOE_EXTERNC_END
   #define SOE_EXTERNC
   #define SOE_EXTERNC_EXT
#endif

/**
   Any C interfaces that call C++ functions that may throw exceptions should use this qualifier.  It should be used like:

   \code
      // prototype.  foo.h:
      SOE_EXTERNC void foo() SOE_NOEXCEPT ;

      // source file: foo.cpp
      #include "foo.h" 

      void foo() noexcept
      {
         // ...
      }
   \endcode

   The result of such a qualifier is that should the programmer screw up, and not catch an exception that should have been
   caught, the function (foo in the example above) will terminate on return.  This is far superior to allowing the exception
   to unwind down to main and causing a terminate at that point where all the context has been lost.
*/

#if defined __cplusplus
   #define SOE_NOEXCEPT noexcept
#else
   #define SOE_NOEXCEPT
#endif


typedef enum JS_ECAP_SOE_ {
    JS_ECAP_SOE_LIST = 0,
    JS_ECAP_SOE_ARRAY,
    JS_ECAP_SOE_STRING,
    JS_ECAP_SOE_NUMBER,
    JS_ECAP_SOE_TRUE,
    JS_ECAP_SOE_FALSE,
    JS_ECAP_SOE_NULL
} JS_ECAP_SOE;

typedef enum JS_RET_SOE_ {
    JS_RET_SOE_OK = 0,
    JS_RET_SOE_NOMEM,
    JS_RET_SOE_BAD_UNICODE,
    JS_RET_SOE_BAD_START,
    JS_RET_SOE_NO_LIST_END,
    JS_RET_SOE_NO_ARRAY_END,
    JS_RET_SOE_NO_QUOTE,
    JS_RET_SOE_UNEXPECTED,
    JS_RET_SOE_EXPECTED_QUOTE,
    JS_RET_SOE_EXPECTED_COLON,
    JS_RET_SOE_EXPECTED_COMMA,
    JS_RET_SOE_BAD_EXPONENT,
    JS_RET_SOE_BAD_ESCAPE,
    JS_RET_SOE_UNESCAPED
} JS_RET_SOE;

struct JS_LIST_SOE {
    struct JS_DUPLE_SOE * first;
    struct JS_DUPLE_SOE * last;
};
typedef struct JS_LIST_SOE JS_LIST_SOE;

struct JS_VEC_SOE {
    struct JS_ELEM_SOE * first;
    struct JS_ELEM_SOE * last;
};
typedef struct JS_VEC_SOE JS_VEC_SOE;

struct JS_STRING_SOE {
    uint8_t * str_value;
    size_t    str_length;
};
typedef struct JS_STRING_SOE JS_STRING_SOE;

struct JS_VAL_SOE {
    union {
        JS_STRING_SOE string;
        JS_LIST_SOE list;
        JS_VEC_SOE array;
    } value;
    JS_ECAP_SOE eType;
};
typedef struct JS_VAL_SOE JS_VAL_SOE;

struct JS_DUPLE_SOE {
    struct JS_DUPLE_SOE * next;
    JS_STRING_SOE name;
    JS_VAL_SOE value;
};
typedef struct JS_DUPLE_SOE  JS_DUPLE_SOE;

struct JS_ELEM_SOE {
    struct JS_ELEM_SOE * next;
    JS_VAL_SOE value;
};

typedef struct JS_ELEM_SOE JS_ELEM_SOE;

typedef struct JS_BIN_SOE {
    struct JS_BIN_SOE * next;
    size_t free_offset;
    size_t free_bytes;
    uint8_t atbytes[0];
} JS_BIN_SOE;

struct JS_DAT_SOE {
    JS_VAL_SOE root_value;
    JS_BIN_SOE * bin_ptr;
    uint8_t * ptr_utf8;
    size_t utf8_bytes;
    size_t utf8_offset;
    unsigned int free_flag:1;
};
typedef struct JS_DAT_SOE JS_DAT_SOE;

extern size_t UTF8count(
    const uint8_t * pabIn,
    size_t          zInBytes);

extern size_t UTF8copy(
    const uint8_t * pabIn,
    size_t          zInBytes,
    uint8_t *       pabOut,
    size_t          zOutBytes);


extern const char * jsoeStrError(JS_RET_SOE);

extern JS_DAT_SOE * jsoeGetJsonData(void);

extern JS_RET_SOE jsoeParse(
    JS_DAT_SOE * pJson,
    const uint8_t * pabUnicode,
    size_t zUnicodeSize);

extern void jsoeFree(
    JS_DAT_SOE * pJson,
    int jsoeData);

extern const JS_VAL_SOE * jsoeFindNamedValue(
    const JS_VAL_SOE * pListValue,
    const char * pacName,
    size_t zNameLength);

extern int jsoeGetDouble(
    const JS_VAL_SOE * pValue,
    double * pdValue);
extern int jsoeGetS64(
    const JS_VAL_SOE * pValue,
    int64_t * piValue);
extern int jsoeGetU64(
    const JS_VAL_SOE * pValue,
    uint64_t * puValue);

extern int jsoeGetDecimal(
    const JS_VAL_SOE * pValue,
    int64_t * piValue);

extern int jsoeGetValueNumber(const JS_VAL_SOE * jsoeValue);

extern const JS_VAL_SOE *   jsoeGetTopValue(const JS_DAT_SOE * jsoeData);

extern size_t               jsoeGetErrorPosition(const JS_DAT_SOE * pJson);

extern int                  jsoeHasChildren(const JS_VAL_SOE * jsoeValue);

extern int                  jsoeIsArray(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsList(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsString(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsNumber(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsTrue(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsFalse(const JS_VAL_SOE * jsoeValue);
extern int                  jsoeIsNull(const JS_VAL_SOE * jsoeValue);
extern JS_ECAP_SOE            jsoeGetNodeType(const JS_VAL_SOE * jsoeValue);

extern const JS_ELEM_SOE *    jsoeGetFirstItem(const JS_VAL_SOE * jsoeValue);
extern const JS_ELEM_SOE *    jsoeGetNextItem(const JS_ELEM_SOE * jsoeItem);
extern const JS_VAL_SOE *   jsoeGetItemValue(const JS_ELEM_SOE * jsoeItem);

extern const JS_DUPLE_SOE *    jsoeGetFirstPair(const JS_VAL_SOE * jsoeValue);
extern const JS_DUPLE_SOE *    jsoeGetNextPair(const JS_DUPLE_SOE * jsoePair);
extern const JS_VAL_SOE *   jsoeGetPairValue(const JS_DUPLE_SOE * jsoePair);
extern void *               jsoeGetPairNameUTF8(const JS_DUPLE_SOE * jsoePair,
                                                void * pszOut,
                                                size_t zOutBytes);
extern const void *         jsoeGetPairNamePtrUTF8(const JS_DUPLE_SOE * jsoePair);
extern size_t               jsoeGetPairNameLenUTF8(const JS_DUPLE_SOE * jsoePair);

extern void *               jsoeGetValueStringUTF8(const JS_VAL_SOE * jsoeValue,
                                                   void * pszOut,
                                                   size_t zOutBytes);
extern char *               jsoeGetValueStringISO88591(const JS_VAL_SOE * jsoeValue,
                                                       char * pszOut,
                                                       size_t zOutBytes);
extern uint8_t *            jsoeGetValueStringIBM1047(const JS_VAL_SOE * jsoeValue,
                                                      uint8_t * pszOut,
                                                      size_t zOutBytes,
                                                      size_t *pCopiedBytes);
extern size_t               jsoeGetValueStringLenUTF8(const JS_VAL_SOE * jsoeValue);
extern size_t               jsoeGetValueStringLenChars(const JS_VAL_SOE * jsoeValue);

#endif

