#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include <errno.h>

#include "soeutil.h"

typedef uint8_t BYTE;
typedef uint16_t UCS2;
typedef uint32_t UCS4;


JS_DAT_SOE * jsoeGetJsonData(void)
{
    return calloc(1, sizeof(JS_DAT_SOE));
}

static const char * jsoeErrorMsgs[] = {
    "OK", "Memory exhausted",
    "Invalid Unicode char encountered",
    "Invalid encoding encountered",
    "Missing }",
    "Missing ]",
    "Missing \" ",
    "Unexpected encoding",
    "Expected \"",
    "Expected :",
    "Expected ,",
    "Invalid exponent",
    "Invalid escaped sequence",
    "Invalid escape sequence encountered"
};

const char * jsoeStrError(JS_RET_SOE rc)
{
    if (rc < (sizeof(jsoeErrorMsgs) / sizeof(char *)))
        return jsoeErrorMsgs[rc];
    return "Unrecognized error";
}

void jsoeFree(JS_DAT_SOE * pJson, int jsoeData)
{
    JS_BIN_SOE * pMem;
    JS_BIN_SOE * pMemNext;
    for (pMem = pJson->bin_ptr; pMem; pMem = pMemNext) {
        pMemNext = pMem->next;
        free(pMem);
    }
    if (jsoeData) {
        free(pJson);
    } else {
        memset(pJson, 0, sizeof(JS_DAT_SOE));
    }
}

static void * jsoeAlloc(JS_DAT_SOE * pJson, size_t zBytes)
{
    if (zBytes <= 0)
        return NULL;
    zBytes = (zBytes + 7) & (~(size_t) 7);
    JS_BIN_SOE * pMem;
    for (pMem = pJson->bin_ptr; pMem; pMem = pMem->next) {
        if (pMem->free_bytes >= zBytes)
            break;
    }
    if (!pMem) {
        size_t zAlloc = zBytes;
        if (zAlloc > 512) {
            zAlloc = (zAlloc + 8191) & (~(size_t) 4095);
        } else {
            zAlloc = 4096;
        }
        pMem = calloc(1, zAlloc);
        if (!pMem)
            return NULL;
        pMem->free_bytes = zAlloc - sizeof(JS_BIN_SOE);
        pMem->next = pJson->bin_ptr;
        pJson->bin_ptr = pMem;
    }
    uint8_t * pabNew = pMem->atbytes + pMem->free_offset;
    pMem->free_offset += zBytes;
    pMem->free_bytes -= zBytes;
    return pabNew;
}

static void jsoeFindNext(JS_DAT_SOE * pJson)
{
    while (pJson->utf8_offset < pJson->utf8_bytes) {
        switch (pJson->ptr_utf8[pJson->utf8_offset]) {
        case 0x09:
        case 0x0A:
        case 0x0D:
        case 0x20:
            pJson->utf8_offset++;
            continue;
        }
        break;
    }
}

static JS_RET_SOE jsoeParseValue(JS_DAT_SOE * pJson, JS_VAL_SOE * pValueOut);

static uint32_t jsoeParseHex4(const uint8_t * pabIn)
{
    uint32_t uValue = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t b = *pabIn;
        pabIn++;
        if ((b >= (uint8_t) '0') && (b <= (uint8_t) '9')) {
            b -= (uint8_t) '0';
        } else if ((b >= 'a') && (b <= 'f')) {
            b -= (uint8_t)('a' - 10);
        } else if ((b >= 'A') && (b <= 'F')) {
            b -= (uint8_t)('A' - 10);
        } else {
            return 0x10000;
        }
        uValue = (uValue << 4) | b;
    }
    return uValue;
}

static JS_RET_SOE jsoeParseString(JS_DAT_SOE * pJson, JS_STRING_SOE * pString)
{
    pJson->utf8_offset++;
    if (pJson->utf8_offset >= pJson->utf8_bytes)
        return JS_RET_SOE_NO_QUOTE;
    pString->str_value = pJson->ptr_utf8 + pJson->utf8_offset;
    pString->str_length = pJson->utf8_offset;
    int iAnyEscapes = 0;
    for (int iInEscape = 0;;) {
        uint8_t b = pJson->ptr_utf8[pJson->utf8_offset];
        if (!iInEscape && (b == (uint8_t) '"'))
            break;
        if (b < (uint8_t) 0x20)
            return JS_RET_SOE_UNESCAPED;
        pJson->utf8_offset++;
        if (pJson->utf8_offset >= pJson->utf8_bytes)
            return JS_RET_SOE_NO_QUOTE;
        if (iInEscape) {
            switch (b) {
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'u':
                break;
            default:
                pJson->utf8_offset -= 2;
                return JS_RET_SOE_BAD_ESCAPE;
            }
            iInEscape = 0;
        } else if (b == (uint8_t) '\\') {
            iInEscape = 1;
            iAnyEscapes = 1;
        }
    }
    pString->str_length = pJson->utf8_offset - pString->str_length;
    pJson->utf8_offset++;
    if (!iAnyEscapes)
        return JS_RET_SOE_OK;

    size_t zInBytes = pString->str_length;
    uint8_t * pabIn = pString->str_value;
    uint8_t * pabOut = jsoeAlloc(pJson, zInBytes);
    pString->str_value = pabOut;
    pString->str_length = 0;
    do {
        uint8_t b = *pabIn;
        pabIn++;
        zInBytes--;
        if (b != (uint8_t) '\\') {
            *pabOut = b;
            pabOut++;
            continue;
        }

        b = *pabIn;
        pabIn++;
        zInBytes--;
        if (b != 'u') {
            switch (b) {
            case 'b':
                b = (uint8_t) 0x08;
                break;
            case 'f':
                b = (uint8_t) 0x0C;
                break;
            case 'n':
                b = (uint8_t) 0x0A;
                break;
            case 'r':
                b = (uint8_t) 0x0D;
                break;
            case 't':
                b = (uint8_t) 0x09;
                break;
            }
            *pabOut = b;
            pabOut++;
            continue;
        }

        if (zInBytes < 4) {
            pJson->utf8_offset = (pabIn - pJson->ptr_utf8) - 2;
            return JS_RET_SOE_BAD_ESCAPE;
        }
        uint32_t cp = jsoeParseHex4(pabIn);
        pabIn += 4;
        zInBytes -= 4;
        if (cp > 0xFFFF) {
            pJson->utf8_offset = (pabIn - pJson->ptr_utf8) - 6;
            return JS_RET_SOE_BAD_ESCAPE;
        }
        if ((cp >= 0xD800) && (cp < 0xE000)) {
            if ((cp >= 0xDC00) || (zInBytes < 6) || (pabIn[0] != (uint8_t) '\\')
                    || (pabIn[1] != (uint8_t) 'u')) {
                pJson->utf8_offset = (pabIn - pJson->ptr_utf8) - 6;
                return JS_RET_SOE_BAD_ESCAPE;
            }
            uint32_t cp2 = jsoeParseHex4(pabIn + 2);
            if ((cp2 < 0xDC00) || (cp2 >= 0xE000)) {
                pJson->utf8_offset = pabIn - pJson->ptr_utf8;
                return JS_RET_SOE_BAD_ESCAPE;
            }
            cp = 0x10000 + ((cp - 0xD800) << 10) + (cp2 - 0xDC00);
            pabIn += 6;
            zInBytes -= 6;
        }

        if (cp <= 0x7F) {
            pabOut[0] = (uint8_t) cp;
            pabOut++;
        } else if (cp <= 0x7FF) {
            pabOut[0] = (uint8_t) 0xC0 | (uint8_t)(cp >> 6);
            pabOut[1] = (uint8_t) 0x80 | (uint8_t)(cp & 0x3F);
            pabOut += 2;
        } else if (cp < 0xFFFF) {
            pabOut[0] = (uint8_t) 0xE0 | (uint8_t)(cp >> 12);
            pabOut[1] = (uint8_t) 0x80 | (uint8_t)((cp >> 6) & 0x3F);
            pabOut[2] = (uint8_t) 0x80 | (uint8_t)(cp & 0x3F);
            pabOut += 3;
        } else {
            pabOut[0] = (uint8_t) 0xF0 | (uint8_t)(cp >> 18);
            pabOut[1] = (uint8_t) 0x80 | (uint8_t)((cp >> 12) & 0x3F);
            pabOut[2] = (uint8_t) 0x80 | (uint8_t)((cp >> 6) & 0x3F);
            pabOut[3] = (uint8_t) 0x80 | (uint8_t)(cp & 0x3F);
            pabOut += 4;
        }
    } while (zInBytes > 0);
    pString->str_length = pabOut - pString->str_value;
    return JS_RET_SOE_OK;
}

static JS_RET_SOE jsoeParseArray(JS_DAT_SOE * pJson, JS_VAL_SOE * pValueOut)
{
    pValueOut->eType = JS_ECAP_SOE_ARRAY;
    pJson->utf8_offset++;
    for (;;) {
        jsoeFindNext(pJson);
        if (pJson->utf8_offset >= pJson->utf8_bytes)
            return JS_RET_SOE_NO_ARRAY_END;
        if (pJson->ptr_utf8[pJson->utf8_offset] == (uint8_t) ']')
            break;
        if (pValueOut->value.list.last) {
            if (pJson->ptr_utf8[pJson->utf8_offset] != (uint8_t) ',')
                return JS_RET_SOE_EXPECTED_COMMA;
            pJson->utf8_offset++;
            jsoeFindNext(pJson);
            if (pJson->utf8_offset >= pJson->utf8_bytes)
                return JS_RET_SOE_NO_ARRAY_END;
        }
        JS_ELEM_SOE * pItem = jsoeAlloc(pJson, sizeof(JS_ELEM_SOE));
        if (!pItem)
            return JS_RET_SOE_NOMEM;
        if (pValueOut->value.array.last) {
            pValueOut->value.array.last->next = pItem;
        } else {
            pValueOut->value.array.first = pItem;
        }
        pValueOut->value.array.last = pItem;
        JS_RET_SOE eReturn = jsoeParseValue(pJson, &(pItem->value));
        if (eReturn != JS_RET_SOE_OK)
            return eReturn;
    }
    pJson->utf8_offset++;
    return JS_RET_SOE_OK;
}

static JS_RET_SOE jsoeParseList(JS_DAT_SOE * pJson, JS_VAL_SOE * pValueOut)
{
    pValueOut->eType = JS_ECAP_SOE_LIST;
    pJson->utf8_offset++;
    for (;;) {
        jsoeFindNext(pJson);
        if (pJson->utf8_offset >= pJson->utf8_bytes)
            return JS_RET_SOE_NO_LIST_END;
        if (pJson->ptr_utf8[pJson->utf8_offset] == (uint8_t) '}')
            break;
        if (pValueOut->value.list.last) {
            if (pJson->ptr_utf8[pJson->utf8_offset] != (uint8_t) ',')
                return JS_RET_SOE_EXPECTED_COMMA;
            pJson->utf8_offset++;
            jsoeFindNext(pJson);
            if (pJson->utf8_offset >= pJson->utf8_bytes)
                return JS_RET_SOE_NO_LIST_END;
        }
        if (pJson->ptr_utf8[pJson->utf8_offset] != (uint8_t) '"')
            return JS_RET_SOE_EXPECTED_QUOTE;
        JS_DUPLE_SOE * pPair = jsoeAlloc(pJson, sizeof(JS_DUPLE_SOE));
        if (!pPair)
            return JS_RET_SOE_NOMEM;
        if (pValueOut->value.list.last) {
            pValueOut->value.list.last->next = pPair;
        } else {
            pValueOut->value.list.first = pPair;
        }
        pValueOut->value.list.last = pPair;
        JS_RET_SOE eReturn = jsoeParseString(pJson, &(pPair->name));
        if (eReturn != JS_RET_SOE_OK)
            return eReturn;
        jsoeFindNext(pJson);
        if (pJson->utf8_offset >= pJson->utf8_bytes)
            return JS_RET_SOE_NO_LIST_END;
        if (pJson->ptr_utf8[pJson->utf8_offset] != (uint8_t) ':')
            return JS_RET_SOE_EXPECTED_COLON;
        pJson->utf8_offset++;
        jsoeFindNext(pJson);
        if (pJson->utf8_offset >= pJson->utf8_bytes)
            return JS_RET_SOE_NO_LIST_END;
        eReturn = jsoeParseValue(pJson, &(pPair->value));
        if (eReturn != JS_RET_SOE_OK)
            return eReturn;
    }
    pJson->utf8_offset++;
    return JS_RET_SOE_OK;
}

static JS_RET_SOE jsoeParseValue(JS_DAT_SOE * pJson, JS_VAL_SOE * pValueOut)
{
    uint8_t * str_value = pJson->ptr_utf8 + pJson->utf8_offset;
    char c = (char) (*str_value);
    if ((c < '0') || (c > '9')) {
        switch (c) {
        case '{':
            return jsoeParseList(pJson, pValueOut);
        case '[':
            return jsoeParseArray(pJson, pValueOut);
        case '"':
            pValueOut->eType = JS_ECAP_SOE_STRING;
            return jsoeParseString(pJson, &(pValueOut->value.string));
        case 'f':
            if (((pJson->utf8_bytes - pJson->utf8_offset) < 5)
                    || (memcmp(str_value, "false", 5) != 0))
                return JS_RET_SOE_UNEXPECTED;
            pJson->utf8_offset += 5;
            pValueOut->eType = JS_ECAP_SOE_FALSE;
            return JS_RET_SOE_OK;
        case 'n':
            if (((pJson->utf8_bytes - pJson->utf8_offset) < 4)
                    || (memcmp(str_value, "null", 4) != 0))
                return JS_RET_SOE_UNEXPECTED;
            pJson->utf8_offset += 4;
            pValueOut->eType = JS_ECAP_SOE_NULL;
            return JS_RET_SOE_OK;
        case 't':
            if (((pJson->utf8_bytes - pJson->utf8_offset) < 4)
                    || (memcmp(str_value, "true", 4) != 0))
                return JS_RET_SOE_UNEXPECTED;
            pJson->utf8_offset += 4;
            pValueOut->eType = JS_ECAP_SOE_TRUE;
            return JS_RET_SOE_OK;
        case '-':
            break;
        default:
            return JS_RET_SOE_UNEXPECTED;
        }
    }

    pValueOut->eType = JS_ECAP_SOE_NUMBER;
    pValueOut->value.string.str_value = str_value;
    pValueOut->value.string.str_length = pJson->utf8_offset;
    if (c == '-') {
        pJson->utf8_offset++;
        if (pJson->utf8_offset >= pJson->utf8_bytes) {
            pJson->utf8_offset--;
            return JS_RET_SOE_UNEXPECTED;
        }
        c = (char) (*++str_value);
        if ((c < '0') || (c > '9'))
            return JS_RET_SOE_UNEXPECTED;
    }
    if (c == '0') {
        pJson->utf8_offset++;
        if (pJson->utf8_offset >= pJson->utf8_bytes) {
            c = 0;
        } else {
            c = (char) (*++str_value);
        }
        if ((c >= '0') && (c <= '9'))
            return JS_RET_SOE_UNEXPECTED;
    } else {
        do {
            pJson->utf8_offset++;
            if (pJson->utf8_offset >= pJson->utf8_bytes) {
                c = 0;
                break;
            }
            c = (char) (*++str_value);
        } while ((c >= '0') && (c <= '9'));
    }
    if (c == '.') {
        do {
            pJson->utf8_offset++;
            if (pJson->utf8_offset >= pJson->utf8_bytes) {
                c = 0;
                break;
            }
            c = (char) (*++str_value);
        } while ((c >= '0') && (c <= '9'));
    }
    if ((c == 'e') || (c == 'E')) {
        pJson->utf8_offset++;
        if (pJson->utf8_offset >= pJson->utf8_bytes) {
            pJson->utf8_offset--;
            return JS_RET_SOE_BAD_EXPONENT;
        }
        c = (char) (*++str_value);
        if ((c == '+') || (c == '-')) {
            pJson->utf8_offset++;
            if (pJson->utf8_offset >= pJson->utf8_bytes) {
                pJson->utf8_offset--;
                return JS_RET_SOE_BAD_EXPONENT;
            }
            c = (char) (*++str_value);
        }
        if ((c < '0') || (c > '9'))
            return JS_RET_SOE_BAD_EXPONENT;
        do {
            pJson->utf8_offset++;
            if (pJson->utf8_offset >= pJson->utf8_bytes)
                break;
            c = (char) (*++str_value);
        } while ((c >= '0') && (c <= '9'));
    }
    pValueOut->value.string.str_length = pJson->utf8_offset
            - pValueOut->value.string.str_length;
    return JS_RET_SOE_OK;
}

typedef enum UTF {
    UTF_UNKNOWN = 0, UTF_8
} UTF;

JS_RET_SOE jsoeParse(JS_DAT_SOE * pJson, const uint8_t * pabUnicode,
        size_t zUnicodeSize)
{
    UTF eUtf = UTF_UNKNOWN;

    memset(pJson, 0, sizeof(JS_DAT_SOE));

    if (zUnicodeSize <= 2)
        return JS_RET_SOE_BAD_UNICODE;

    {
        static const uint8_t abBomUtf8[3] = { (uint8_t) 0xEF, (uint8_t) 0xBB,
                (uint8_t) 0xBF };
        static const uint8_t abBomUtf32BE[4] = { 0, 0, (uint8_t) 0xFE,
                (uint8_t) 0xFF };
        static const uint8_t abBomUtf32LE[4] = { (uint8_t) 0xFF, (uint8_t) 0xFE,
                0, 0 };
        int iBomSize = 0;
        if ((zUnicodeSize >= 3)
                && (memcmp(pabUnicode, abBomUtf8, sizeof(abBomUtf8)) == 0)) {
            iBomSize = sizeof(abBomUtf8);
            eUtf = UTF_8;
        }
        if (iBomSize) {
            pabUnicode += iBomSize;
            zUnicodeSize -= iBomSize;
        }
    }

    pJson->utf8_bytes = zUnicodeSize;
    pJson->ptr_utf8 = (uint8_t *)pabUnicode;

    if (pJson->utf8_bytes == (size_t) - 1)
        return JS_RET_SOE_BAD_UNICODE;

    if (!(pJson->ptr_utf8)) {
        pJson->ptr_utf8 = jsoeAlloc(pJson, pJson->utf8_bytes);
        if (!(pJson->ptr_utf8))
            return JS_RET_SOE_NOMEM;
    }

    if (UTF8count(pabUnicode, zUnicodeSize) == (size_t)-1)
        return JS_RET_SOE_BAD_UNICODE;

    jsoeFindNext(pJson);
    {
        JS_RET_SOE eReturn = JS_RET_SOE_BAD_START;
        if (pJson->utf8_offset >= pJson->utf8_bytes) {
            pJson->utf8_offset = 0;
        } else if (pJson->ptr_utf8[pJson->utf8_offset] == (uint8_t) '{') {
            eReturn = jsoeParseList(pJson, &(pJson->root_value));
        } else if (pJson->ptr_utf8[pJson->utf8_offset] == (uint8_t) '[') {
            eReturn = jsoeParseArray(pJson, &(pJson->root_value));
        }
        if (eReturn != JS_RET_SOE_OK)
            return eReturn;
    }
    jsoeFindNext(pJson);
    if (pJson->utf8_offset < pJson->utf8_bytes)
        return JS_RET_SOE_UNEXPECTED;

    return JS_RET_SOE_OK;
}

const JS_VAL_SOE * jsoeFindNamedValue(const JS_VAL_SOE * pListValue,
        const char * pacName, size_t zNameLength)
{
    if (!pListValue || (pListValue->eType != JS_ECAP_SOE_LIST))
        return NULL;
    if (!zNameLength)
        zNameLength = strlen(pacName);
    for (JS_DUPLE_SOE * pPair = pListValue->value.list.first; pPair;
            pPair = pPair->next) {
        if ((pPair->name.str_length == zNameLength)
                && (memcmp(pPair->name.str_value, pacName, zNameLength) == 0))
            return &(pPair->value);
    }
    return NULL;
}

int jsoeGetDouble(const JS_VAL_SOE * pValue, double * pdValue)
{
    if (!pValue || (pValue->eType != JS_ECAP_SOE_NUMBER))
        return -1;

    char * pszValue = alloca(pValue->value.string.str_length + 1);
    memcpy(pszValue, pValue->value.string.str_value,
            pValue->value.string.str_length);
    pszValue[pValue->value.string.str_length] = '\0';
    errno = 0;
    double dValue = strtod(pszValue, NULL);
    if (errno)
        return -1;
    *pdValue = dValue;
    return 0;
}

int jsoeGetS64(const JS_VAL_SOE * pValue, int64_t * piValue)
{
    int64_t iValue;
    int iDecimals = jsoeGetDecimal(pValue, &iValue);
    if (iDecimals)
        return -1;
    *piValue = iValue;
    return 0;
}

int jsoeGetU64(const JS_VAL_SOE * pValue, uint64_t * puValue)
{
    if (!pValue || (pValue->eType != JS_ECAP_SOE_NUMBER))
        return -1;

    size_t str_length = pValue->value.string.str_length;
    char * pacValue = (char *) (pValue->value.string.str_value);
    if (*pacValue == '-')
        return -1;

    uint64_t uValue = 0;
    int iDecimals = -1;
    do {
        char c = *pacValue;
        ++pacValue;
        --str_length;
        if (c == '.') {
            iDecimals = 0;
            continue;
        }
        if ((c < '0') || (c > '9'))
            break;
        c -= '0';
        if (iDecimals >= 0) {
            if (!c) {
                if (!str_length)
                    break;
                size_t uPos;
                for (uPos = 0; uPos < str_length; ++uPos) {
                    if (pacValue[uPos] != '0')
                        break;
                }
                if (uPos >= str_length) {
                    str_length -= uPos;
                    pacValue += uPos;
                    break;
                }
                if ((pacValue[uPos] < '0') || (pacValue[uPos] > '9')) {
                    ++uPos;
                    str_length -= uPos;
                    pacValue += uPos;
                    break;
                }
            }
            ++iDecimals;
        }
        if (uValue >= (uint64_t)(UINT64_MAX / 10)) {
            if ((uValue > (uint64_t)(UINT64_MAX / 10))
                    || (c > (char) (UINT64_MAX % 10)))
                return -1;
        }
        uValue = uValue * 10 + (uint8_t) c;
    } while (str_length);
    if (!uValue) {
        *puValue = 0;
        return 0;
    }
    if (iDecimals < 0)
        iDecimals = 0;

    if (str_length) {
        uint8_t bExpNegative = 0;
        if ((*pacValue < '0') || (*pacValue > '9')) {
            if (*pacValue == '-')
                bExpNegative = 1;
            ++pacValue;
            --str_length;
        }
        int iExponent = 0;
        do {
            char c = *pacValue - '0';
            pacValue++;
            if (iExponent > 9999)
                return -1;
            iExponent = iExponent * 10 + c;
        } while (--str_length != 0);
        if (iExponent) {
            if (bExpNegative)
                iExponent = -iExponent;
            iDecimals -= iExponent;
            while (iDecimals < 0) {
                if (uValue > (uint64_t)(UINT64_MAX / 10))
                    return -1;
                uValue *= 10;
                ++iDecimals;
            }
            while (iDecimals > 0) {
                if ((uValue % 10) != 0)
                    break;
                uValue /= 10;
                --iDecimals;
            }
        }
    }

    if (iDecimals)
        return -1;
    *puValue = uValue;
    return 0;
}

int jsoeGetDecimal(const JS_VAL_SOE * pValue, int64_t * piValue)
{
    if (!pValue || (pValue->eType != JS_ECAP_SOE_NUMBER))
        return -1;

    char * pacValue = (char *) (pValue->value.string.str_value);
    size_t str_length = pValue->value.string.str_length;
    uint8_t bNegative = 0;
    if (*pacValue == '-') {
        bNegative = 1;
        ++pacValue;
        --str_length;
    }

    uint64_t uValue = 0;
    int iDecimals = -1;
    do {
        char c = *pacValue;
        ++pacValue;
        --str_length;
        if (c == '.') {
            iDecimals = 0;
            continue;
        }
        if ((c < '0') || (c > '9'))
            break;
        c -= '0';
        if (iDecimals >= 0) {
            if (!c) {
                if (!str_length)
                    break;
                size_t uPos;
                for (uPos = 0; uPos < str_length; ++uPos) {
                    if (pacValue[uPos] != '0')
                        break;
                }
                if (uPos >= str_length) {
                    str_length -= uPos;
                    pacValue += uPos;
                    break;
                }
                if ((pacValue[uPos] < '0') || (pacValue[uPos] > '9')) {
                    ++uPos;
                    str_length -= uPos;
                    pacValue += uPos;
                    break;
                }
            }
            ++iDecimals;
        }
        if (uValue >= (uint64_t)(INT64_MAX / 10)) {
            if (uValue > (uint64_t)(INT64_MAX / 10))
                return -1;
            if (!bNegative) {
                if (c > (char) (INT64_MAX % 10))
                    return -1;
            } else {
                if (c > (char) ((INT64_MAX - 9) % 10))
                    return -1;
            }
        }
        uValue = uValue * 10 + (uint8_t) c;
    } while (str_length);
    if (!uValue) {
        if (bNegative)
            return -1;
        *piValue = 0;
        return 0;
    }
    if (iDecimals < 0)
        iDecimals = 0;

    if (str_length) {
        uint8_t bExpNegative = 0;
        if ((*pacValue < '0') || (*pacValue > '9')) {
            if (*pacValue == '-')
                bExpNegative = 1;
            ++pacValue;
            --str_length;
        }
        int iExponent = 0;
        do {
            char c = *pacValue - '0';
            pacValue++;
            if (iExponent > 9999)
                return -1;
            iExponent = iExponent * 10 + c;
        } while (--str_length != 0);
        if (iExponent) {
            if (bExpNegative)
                iExponent = -iExponent;
            iDecimals -= iExponent;
            while (iDecimals < 0) {
                if (uValue > (uint64_t)(INT64_MAX / 10))
                    return -1;
                uValue *= 10;
                ++iDecimals;
            }
            while (iDecimals > 0) {
                if ((uValue % 10) != 0)
                    break;
                uValue /= 10;
                --iDecimals;
            }
        }
    }

    if (!bNegative) {
        *piValue = (int64_t) uValue;
    } else if (uValue <= (uint64_t) INT64_MAX) {
        *piValue = -((int64_t) uValue);
    } else {
        *piValue = INT64_MIN;
    }
    return iDecimals;
}

const JS_VAL_SOE * jsoeGetTopValue(const JS_DAT_SOE * jsoeData)
{
    return &(jsoeData->root_value);
}

size_t jsoeGetErrorPosition(const JS_DAT_SOE * pJson)
{
    return pJson->utf8_offset;
}

int jsoeHasChildren(const JS_VAL_SOE * jsoeValue)
{
    if ((jsoeValue->eType == JS_ECAP_SOE_LIST)
            || (jsoeValue->eType == JS_ECAP_SOE_ARRAY))
        return 1;
    return 0;
}

int jsoeIsArray(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_ARRAY)
        return 1;
    return 0;
}

int jsoeIsList(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_LIST)
        return 1;
    return 0;
}

int jsoeIsString(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_STRING)
        return 1;
    return 0;
}

int jsoeIsNumber(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_NUMBER)
        return 1;
    return 0;
}

int jsoeIsTrue(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_TRUE)
        return 1;
    return 0;
}

int jsoeIsFalse(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_FALSE)
        return 1;
    return 0;
}

int jsoeIsNull(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_NULL)
        return 1;
    return 0;
}

JS_ECAP_SOE jsoeGetNodeType(const JS_VAL_SOE * jsoeValue)
{
    return jsoeValue->eType;
}

const JS_ELEM_SOE * jsoeGetFirstItem(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_ARRAY)
        return jsoeValue->value.array.first;
    return NULL;
}

const JS_ELEM_SOE * jsoeGetNextItem(const JS_ELEM_SOE * jsoeItem)
{
    return jsoeItem->next;
}

const JS_VAL_SOE * jsoeGetItemValue(const JS_ELEM_SOE * jsoeItem)
{
    return &(jsoeItem->value);
}

const JS_DUPLE_SOE * jsoeGetFirstPair(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue->eType == JS_ECAP_SOE_LIST)
        return jsoeValue->value.list.first;
    return NULL;
}

const JS_DUPLE_SOE * jsoeGetNextPair(const JS_DUPLE_SOE * jsoePair)
{
    return jsoePair->next;
}

const JS_VAL_SOE * jsoeGetPairValue(const JS_DUPLE_SOE * jsoePair)
{
    return &(jsoePair->value);
}

void * jsoeGetPairNameUTF8(const JS_DUPLE_SOE * jsoePair, void * pszOut,
        size_t zOutBytes)
{
    if (!zOutBytes)
        return NULL;
    size_t zCopiedBytes = UTF8copy(jsoePair->name.str_value,
            jsoePair->name.str_length, pszOut, zOutBytes - 1U);
    ((char *) pszOut)[zCopiedBytes] = '\0';
    return pszOut;
}

const void * jsoeGetPairNamePtrUTF8(const JS_DUPLE_SOE * jsoePair)
{
    return jsoePair->name.str_value;
}

size_t jsoeGetPairNameLenUTF8(const JS_DUPLE_SOE * jsoePair)
{
    return jsoePair->name.str_length;
}

size_t jsoeGetValueStringLenUTF8(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue
            && ((jsoeValue->eType == JS_ECAP_SOE_STRING)
                    || (jsoeValue->eType == JS_ECAP_SOE_NUMBER)))
        return jsoeValue->value.string.str_length;
    return 0;
}

size_t jsoeGetValueStringLenChars(const JS_VAL_SOE * jsoeValue)
{
    if (jsoeValue
            && ((jsoeValue->eType == JS_ECAP_SOE_STRING)
                    || (jsoeValue->eType == JS_ECAP_SOE_NUMBER))
            && jsoeValue->value.string.str_length)
        return UTF8count(jsoeValue->value.string.str_value,
                jsoeValue->value.string.str_length);
    return 0;
}

void * jsoeGetValueStringUTF8(const JS_VAL_SOE * jsoeValue, void * pszOut,
        size_t zOutBytes)
{
    if (!zOutBytes || !jsoeValue
            || ((jsoeValue->eType != JS_ECAP_SOE_STRING)
                    && (jsoeValue->eType != JS_ECAP_SOE_NUMBER)))
        return NULL;
    size_t zCopiedBytes = UTF8copy(jsoeValue->value.string.str_value,
            jsoeValue->value.string.str_length, pszOut, zOutBytes - 1U);
    ((uint8_t *) pszOut)[zCopiedBytes] = '\0';
    return pszOut;
}

int jsoeGetValueNumber(const JS_VAL_SOE * jsoeValue)
{
    int results = 0;
    char number[256];

    if (jsoeValue && (jsoeValue->eType == JS_ECAP_SOE_NUMBER)
            && jsoeValue->value.string.str_length
            && (jsoeValue->value.string.str_length < sizeof(number))) {
        memcpy(number, jsoeValue->value.string.str_value,
                jsoeValue->value.string.str_length);
        number[jsoeValue->value.string.str_length] = 0;
        results = atoi(number);
    }
    return results;
}

size_t UTF8copy(const BYTE * pabIn, size_t zInBytes, BYTE * pabOut,
        size_t zOutBytes)
{
    size_t zCopiedBytes = zInBytes;
    if (!pabOut)
        zCopiedBytes = 0;
    if (zCopiedBytes > zOutBytes) {
        zCopiedBytes = 0;
        while ((zCopiedBytes < zInBytes) && (zCopiedBytes < zOutBytes)) {
            size_t str_length = zCopiedBytes + 1U;
            BYTE b = pabIn[zCopiedBytes];
            if (b & (BYTE) 0x80) {
                if (((b & (BYTE) 0xC0) == (BYTE) 0x80)
                        || ((b & (BYTE) 0xF8) == (BYTE) 0xF8))
                    break;
                if ((b & (BYTE) 0xE0) == (BYTE) 0xC0) {
                    str_length++;
                } else if ((b & (BYTE) 0xF0) == (BYTE) 0xE0) {
                    str_length += 2;
                } else {
                    str_length += 3;
                }
                if ((str_length > zInBytes) || (str_length > zOutBytes))
                    break;
            }
            zCopiedBytes = str_length;
        }
    }
    if (zCopiedBytes)
        memcpy(pabOut, pabIn, zCopiedBytes);
    return zCopiedBytes;
}

size_t UTF8count(const BYTE * pabIn, size_t zInBytes)
{
    size_t zCharacters = 0;
    while (zInBytes > 0) {
        BYTE b = *pabIn;
        pabIn++;
        zInBytes--;
        zCharacters++;
        if (!(b & (BYTE) 0x80))
            continue;
        if (((b & (BYTE) 0xC0) == (BYTE) 0x80)
                || ((b & (BYTE) 0xF8) == (BYTE) 0xF8))
            return (size_t) - 1;
        unsigned uBytes = 1;
        UCS4 cp;
        if ((b & (BYTE) 0xE0) == (BYTE) 0xC0) {
            cp = b & (BYTE) 0x1F;
        } else if ((b & (BYTE) 0xF0) == (BYTE) 0xE0) {
            uBytes = 2;
            cp = b & (BYTE) 0x0F;
        } else {
            uBytes = 3;
            cp = b & (BYTE) 0x07;
        }
        if (zInBytes < uBytes)
            return (size_t) - 1;
        do {
            b = *pabIn;
            pabIn++;
            zInBytes--;
            if ((b & (BYTE) 0xC0) != (BYTE) 0x80)
                return (size_t) - 1;
            cp = (cp << 6) | (b & (BYTE) 0x3F);
        } while ((--uBytes));
        if ((cp > (UCS4) 0x10FFFF)
                | ((cp >= (UCS4) 0xD800) && (cp <= (UCS4) 0xDFFF)))
            return (size_t) - 1;
    }
    return zCharacters;
}

size_t UTF32LEtoUTF8(const BYTE * pabIn, size_t zInBytes, BYTE * pabOut,
        size_t zOutSize)
{
    if (!zInBytes)
        return 0;
    if (zInBytes & 3)
        return (size_t) - 1;
    if (!pabOut)
        zOutSize = 0;
    size_t zOutBytes = 0;
    do {
        UCS4 cp = ((((((UCS4) pabIn[3] << 8) | (UCS4) pabIn[2]) << 8)
                | (UCS4) pabIn[1]) << 8) | pabIn[0];
        if ((cp > (UCS4) 0x10FFFF)
                | ((cp >= (UCS4) 0xD800) && (cp <= (UCS4) 0xDFFF)))
            return (size_t) - 1;
        pabIn += 4;
        zInBytes -= 4;
        unsigned uBytesNeeded = 1;
        if (cp > 0x7F) {
            uBytesNeeded = 2;
            if (cp > 0x7FF) {
                uBytesNeeded = 3;
                if (cp > 0xFFFF)
                    uBytesNeeded = 4;
            }
        }
        zOutBytes += uBytesNeeded;
        if (zOutSize >= uBytesNeeded) {
            switch (uBytesNeeded) {
            case 1:
                pabOut[0] = (BYTE) cp;
                break;
            case 2:
                pabOut[0] = (BYTE) 0xC0 | (BYTE) (cp >> 6);
                pabOut[1] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            case 3:
                pabOut[0] = (BYTE) 0xE0 | (BYTE) (cp >> 12);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            default:
                pabOut[0] = (BYTE) 0xF0 | (BYTE) (cp >> 18);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 12) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[3] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
            }
            pabOut += uBytesNeeded;
            zOutSize -= uBytesNeeded;
        }
    } while (zInBytes);
    return zOutBytes;
}

size_t UTF32BEtoUTF8(const BYTE * pabIn, size_t zInBytes, BYTE * pabOut,
        size_t zOutSize)
{
    if (!zInBytes)
        return 0;
    if (zInBytes & 3)
        return (size_t) - 1;
    if (!pabOut)
        zOutSize = 0;
    size_t zOutBytes = 0;
    do {
        UCS4 cp = ((((((UCS4) pabIn[0] << 8) | (UCS4) pabIn[1]) << 8)
                | (UCS4) pabIn[2]) << 8) | pabIn[3];
        if ((cp > (UCS4) 0x10FFFF)
                | ((cp >= (UCS4) 0xD800) && (cp <= (UCS4) 0xDFFF)))
            return (size_t) - 1;
        pabIn += 4;
        zInBytes -= 4;
        unsigned uBytesNeeded = 1;
        if (cp > 0x7F) {
            uBytesNeeded = 2;
            if (cp > 0x7FF) {
                uBytesNeeded = 3;
                if (cp > 0xFFFF)
                    uBytesNeeded = 4;
            }
        }
        zOutBytes += uBytesNeeded;
        if (zOutSize >= uBytesNeeded) {
            switch (uBytesNeeded) {
            case 1:
                pabOut[0] = (BYTE) cp;
                break;
            case 2:
                pabOut[0] = (BYTE) 0xC0 | (BYTE) (cp >> 6);
                pabOut[1] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            case 3:
                pabOut[0] = (BYTE) 0xE0 | (BYTE) (cp >> 12);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            default:
                pabOut[0] = (BYTE) 0xF0 | (BYTE) (cp >> 18);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 12) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[3] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
            }
            pabOut += uBytesNeeded;
            zOutSize -= uBytesNeeded;
        }
    } while (zInBytes);
    return zOutBytes;
}

size_t UTF16LEtoUTF8(const BYTE * pabIn, size_t zInBytes, BYTE * pabOut,
        size_t zOutSize)
{
    if (!zInBytes)
        return 0;
    if (zInBytes & 1)
        return (size_t) - 1;
    if (!pabOut)
        zOutSize = 0;
    size_t zOutBytes = 0;
    do {
        UCS4 cp = ((UCS4) pabIn[1] << 8) | (UCS4) pabIn[0];
        pabIn += 2;
        zInBytes -= 2;
        if ((cp >= (UCS4) 0xD800) && (cp <= (UCS4) 0xDFFF)) {
            if ((cp >= (UCS4) 0xDC00) || (zInBytes < 2))
                return (size_t) - 1;
            UCS2 cp2 = ((UCS2) pabIn[1] << 8) | (UCS2) pabIn[0];
            if ((cp2 < 0xDC00) || (cp2 > 0xDFFF))
                return (size_t) - 1;
            pabIn += 2;
            zInBytes -= 2;
            cp = (UCS4) 0x10000 + ((cp - (UCS4) 0xD800) << 10)
                    + (cp2 - (UCS2) 0xDC00);
        }
        unsigned uBytesNeeded = 1;
        if (cp > 0x7F) {
            uBytesNeeded = 2;
            if (cp > 0x7FF) {
                uBytesNeeded = 3;
                if (cp > 0xFFFF)
                    uBytesNeeded = 4;
            }
        }
        zOutBytes += uBytesNeeded;
        if (zOutSize >= uBytesNeeded) {
            switch (uBytesNeeded) {
            case 1:
                pabOut[0] = (BYTE) cp;
                break;
            case 2:
                pabOut[0] = (BYTE) 0xC0 | (BYTE) (cp >> 6);
                pabOut[1] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            case 3:
                pabOut[0] = (BYTE) 0xE0 | (BYTE) (cp >> 12);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            default:
                pabOut[0] = (BYTE) 0xF0 | (BYTE) (cp >> 18);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 12) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[3] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
            }
            pabOut += uBytesNeeded;
            zOutSize -= uBytesNeeded;
        }
    } while (zInBytes);
    return zOutBytes;
}

size_t UTF16BEtoUTF8(const BYTE * pabIn, size_t zInBytes, BYTE * pabOut,
        size_t zOutSize)
{
    if (!zInBytes)
        return 0;
    if (zInBytes & 1)
        return (size_t) - 1;
    if (!pabOut)
        zOutSize = 0;
    size_t zOutBytes = 0;
    do {
        UCS4 cp = ((UCS4) pabIn[0] << 8) | (UCS4) pabIn[1];
        pabIn += 2;
        zInBytes -= 2;
        if ((cp >= (UCS4) 0xD800) && (cp <= (UCS4) 0xDFFF)) {
            if ((cp >= (UCS4) 0xDC00) || (zInBytes < 2))
                return (size_t) - 1;
            UCS2 cp2 = ((UCS2) pabIn[0] << 8) | (UCS2) pabIn[1];
            if ((cp2 < 0xDC00) || (cp2 > 0xDFFF))
                return (size_t) - 1;
            pabIn += 2;
            zInBytes -= 2;
            cp = (UCS4) 0x10000 + ((cp - (UCS4) 0xD800) << 10)
                    + (cp2 - (UCS2) 0xDC00);
        }
        unsigned uBytesNeeded = 1;
        if (cp > 0x7F) {
            uBytesNeeded = 2;
            if (cp > 0x7FF) {
                uBytesNeeded = 3;
                if (cp > 0xFFFF)
                    uBytesNeeded = 4;
            }
        }
        zOutBytes += uBytesNeeded;
        if (zOutSize >= uBytesNeeded) {
            switch (uBytesNeeded) {
            case 1:
                pabOut[0] = (BYTE) cp;
                break;
            case 2:
                pabOut[0] = (BYTE) 0xC0 | (BYTE) (cp >> 6);
                pabOut[1] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            case 3:
                pabOut[0] = (BYTE) 0xE0 | (BYTE) (cp >> 12);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
                break;
            default:
                pabOut[0] = (BYTE) 0xF0 | (BYTE) (cp >> 18);
                pabOut[1] = (BYTE) 0x80 | (BYTE) ((cp >> 12) & 0x3F);
                pabOut[2] = (BYTE) 0x80 | (BYTE) ((cp >> 6) & 0x3F);
                pabOut[3] = (BYTE) 0x80 | (BYTE) (cp & 0x3F);
            }
            pabOut += uBytesNeeded;
            zOutSize -= uBytesNeeded;
        }
    } while (zInBytes);
    return zOutBytes;
}

