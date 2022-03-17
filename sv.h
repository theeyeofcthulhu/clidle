/**
 * @file
 * @brief Declarations for string view library.
 *
 * @author theeyeofcthulhu
 * @copyright Copyright (C) 2022 theeyeofcthulhu. All rights reserved.
 * @par License
 *      This project is released under the MIT License.
 */

#ifndef SV_H_
#define SV_H_

#include <stdint.h> /* SIZE_MAX */
#include <stddef.h> /* size_t */
#include <stdbool.h> /* bool */

#ifndef SVDEF
#  ifdef SV_STATIC
#    define SVDEF static
#  else
#    define SVDEF extern
#  endif
#endif

/**
 * printf() format specifier for a string view.
 *
 * Use together with SV_Arg(x) like this:
 * \code
 * printf("My string view: "SV_Fmt"\n", SV_Arg(my_sv));
 * \endcode
 */
#define SV_Fmt "%.*s"

/**
 * See SV_Fmt.
 */
#define SV_Arg(x) (int) (x).len, (x).ptr

/**
 * Prints \p x and a trailing newline to \a stdout.
 */
#define SV_Puts(x) printf(SV_Fmt"\n", SV_Arg(x));

/**
 * Constructs a string view from the string literal \p lit.
 *
 * This takes advantage of the fact that \a sizeof knows
 * the size of a string literal at compile time, ridding
 * us of a \a strlen call.
 */
#define SV_Lit(lit) sv_from_data((lit), sizeof(lit) - 1)

/**
 * Expands to a boolean expression describing if \p sv contains \p searched.
 */
#define SV_Contains(searched, sv) (sv_idx_long((searched), (sv)) != SV_END_POS)

/**
 * Indicates last or unreachable position in a string.
 */
#define SV_END_POS SIZE_MAX

/**
 * The string view type.
 *
 * Holds the length of the area being pointed to and the base pointer.
 */
typedef struct {
    size_t len; /**< The length of the string. */
    const char *ptr; /**< The pointer to the beginning of the string. */
} sv;

/**
 * Constructs a string view from a NULL-terminated C string.
 * Do not use for string literals. Use SV_Lit as it avoids a
 * call to strlen(3).
 *
 * @param str A NULL-terminated C string for the new string view to point to.
 * @return The new string view.
 */
SVDEF sv sv_from_cstr(const char* str);

/**
 * Constructs a string view using the parameters.
 *
 * @param str A string of character data.
 * @param len The length of the new view.
 * @return The new string view.
 */
SVDEF sv sv_from_data(const char *str, size_t len);

/**
 * Constructs a new view pointing to \p len characters of the C-string
 * \p str offset by \p beg.
 *
 * If the result of \p beg + \p len is greater than the length of \p str
 * or \p len is equal to the constant \a SV_END_POS,
 * the resulting string view will capture from \p beg to the end of the
 * string. If \p beg is out bounds for the length of the string \p str
 * or \p len is equal to 0, an empty view is returned.
 *
 * @param beg The beginning of the substring frame. 
 * @param len The number of characters to capture. Can be set to \a SV_END_POS to capture to the end of the string.
 * @param str The NULL-terminated C-string to construct the view from.
 * @return The new string view constructed by the specification.
 *         See the above for special cases.
 */
SVDEF sv sv_from_sub_cstr(size_t beg, size_t len, const char* str);

/**
 * Chops \p n characters from the beginning of the view \p sv.
 *
 * Any \p n greater than or equal to \p sv->len causes \p sv to become
 * empty after the call.
 *
 * @param n How many characters to chop.
 * @param sv The view being operated on.
 */
SVDEF void sv_chopl(size_t n, sv *sv);

/**
 * Chops \p n characters starting from the end of the view \p sv.
 *
 * Any \p n greater than or equal to \p sv->len causes \p sv to become
 * empty after the call.
 *
 * @param n How many characters to chop.
 * @param sv The view being operated on.
 */
SVDEF void sv_chopr(size_t n, sv *sv);

/**
 * Constructs a new view pointing to \p len characters starting from the area pointed to
 * by \p in offset by \p beg.
 *
 * If the result of \p beg + \p len is greater than \p in->len
 * or \p len is equal to the constant \a SV_END_POS,
 * the resulting string view will capture from \p beg to the end of \p in. 
 * If \p beg is out bounds for the length of the view \p in
 * or \p len is equal to 0, an empty view is returned.
 *
 * @param beg The beginning of the substring frame.
 * @param len The number of characters to capture. Can be set to \a SV_END_POS to capture to the end of the string.
 * @param in The view to construct the substring from.
 * @return The new string view constructed by the specification.
 *         See the above for special cases.
 */
SVDEF sv sv_substr(size_t beg, size_t len, sv in);

/**
 * Write the contents of \p sv to \a stdout.
 *
 * @param sv The view to write.
 * @return The return value of write.
 */
SVDEF int sv_write(sv sv);

/**
 * Write the contents of \p sv to \p fd.
 *
 * @param sv The view to write.
 * @param fd The file descriptor to write to.
 * @return The return value of write.
 */
SVDEF int sv_fwrite(sv sv, int fd);

/**
 * Chop in until a \p delim is reached and put the result (excluding any delims) into \p out.
 *
 * Once a delimiter is found, the function chops all following continuous occurences of
 * the delimiter aswell. If \p in points to the following string:
 * \code "A    space" \endcode The result will look the following: \p in: "space" and \p out: "A". If no delimiter is found,
 * \p in will be empty and \p out will now hold the entirety of \p in.
 *
 * @param delim The delimiter to chop.
 * @param in The string view to operate on.
 * @param out The string view buffer to place the matched string before \p delim into.
 * @return Boolean value which indicates if \p in is empty. This is useful to employ \a sv_chop_delim()
 *         in loops. If no delimiter is found, \p in->len is set to zero and \a true is returned. If
 *         \p in->len is zero at the beginning of the function, \a false is returned.
 */
SVDEF bool sv_chop_delim(char delim, sv *in, sv *out);

/**
 * Get the index of the first occurence of \p c in the area pointed to by \p sv.
 *
 * @param c The character to search for.
 * @param sv The view to search in.
 * @return The deduced index; \a SV_END_POS if no character \p c was found.
 */
SVDEF size_t sv_idx(char c, sv sv);

/**
 * Get the index of the last occurence of \p c in the area pointed to by \p sv.
 *
 * @param c The character to search for.
 * @param sv The view to search in.
 * @return The deduced index; \a SV_END_POS if no character \p c was found.
 */
SVDEF size_t sv_last_idx(char c, sv sv);

/**
 * Get the index of the beginning of the first occurence of the view \p search
 * in the view \p sv.
 *
 * @param search View to try and locate in \p sv.
 * @param sv The view to search in.
 * @return The deduced index; \a SV_END_POS if no string matching \p search was found.
 */
SVDEF size_t sv_idx_long(sv search, sv sv);

/**
 * Get the index of the beginning of the last occurence of the view \p search
 * in the view \p sv.
 *
 * @param search View to try and locate in \p sv.
 * @param sv The view to search in.
 * @return The deduced index; \a SV_END_POS if no string matching \p search was found.
 */
SVDEF size_t sv_last_idx_long(sv search, sv sv);

/**
 * Checks if two views are effectively equal in the strings they point to.
 *
 * I.e: they have the same \a len and their \a ptr's point to the same string
 * (not necessarily the same address).
 *
 * @param s1 The first operand.
 * @param s2 The second operand.
 * @return The result of the comparison.
 */
SVDEF bool sv_eq(sv s1, sv s2);

/**
 * Checks if a string view and a NULL-terminated C-string are effectively equal.
 *
 * @param sv A string view, the first operand of the comparison.
 * @param cstr A NULL-terminated C-string, the second operand of the comparison.
 * @return The result of the comparison.
 */
SVDEF bool sv_cstr_eq(sv sv, const char *cstr);

/**
 * Writes the contents of the view \p sv into the buffer \p buf.
 *
 * After the written characters, a terminating NULL-byte is appended into \p
 * buf. If the length of \p sv is greater than or equal to \p buflen, \p
 * sv->len - 1 characters are written, in order to still append the NULL-byte.
 *
 * @param sv The string view to write.
 * @param buf The buffer to write into.
 * @param buflen The capacity of the buffer \p buf.
 * @return \p buf.
 */
SVDEF char *sv_to_cstr(sv sv, char *buf, size_t buflen);

/**
 * Checks if the view \p sv starts with the view \p start.
 *
 * @param start View to check.
 * @param sv View to check against.
 * @return Boolean result of the comparison.
 */
SVDEF bool sv_starts_with(sv start, sv sv);

/**
 * Checks if the view \p sv ends with the view \p end.
 *
 * @param end View to check.
 * @param sv View to check against.
 * @return Boolean result of the comparison.
 */
SVDEF bool sv_ends_with(sv end, sv sv);

#endif // SV_H_

#ifdef SV_IMPLEMENTATION

#include <string.h> /* memcmp, memcpy, strlen */
#include <unistd.h> /* write */

SVDEF sv sv_from_cstr(const char *str)
{
    sv ret = {
        .len = strlen(str),
        .ptr = str,
    };

    return ret;
}

SVDEF sv sv_from_data(const char *str, size_t len)
{
    sv ret = {
        .len = len,
        .ptr = str,
    };

    return ret;
}

SVDEF sv sv_from_sub_cstr(size_t beg, size_t len, const char *str)
{
    size_t str_len;
    sv ret = {0};

    str_len = strlen(str);

    if (len == 0 || beg >= str_len) {
        return ret;
    }

    if (len == SV_END_POS || beg + len > str_len) {
        len = str_len - beg;
    }

    ret.len = len;
    ret.ptr = str + beg;

    return ret;
}

SVDEF void sv_chopl(size_t n, sv *sv)
{
    if (n == 0) {
        return;
    }

    if (n >= sv->len) {
        sv->len = 0;
        return;
    }

    sv->len -= n;
    sv->ptr += n;
}

SVDEF void sv_chopr(size_t n, sv *sv)
{
    if (n == 0) {
        return;
    }

    if (n >= sv->len) {
        sv->len = 0;
        return;
    }

    sv->len -= n;
}

SVDEF sv sv_substr(size_t beg, size_t len, sv in)
{
    sv ret = {0};

    if (len == 0 || beg > in.len) {
        return ret;
    }

    if (len == SV_END_POS || beg + len > in.len) {
        len = in.len - beg;
    }

    ret.len = len;
    ret.ptr = in.ptr + beg;

    return ret;
}

SVDEF int sv_write(sv sv) 
{
    /* The direct write(2)-call does not depend on NULL-strings */
    return write(STDOUT_FILENO, sv.ptr, sv.len);
}

SVDEF int sv_fwrite(sv sv, int fd) 
{
    return write(fd, sv.ptr, sv.len);
}

SVDEF bool sv_chop_delim(char delim, sv *in, sv *out)
{
    if (in->len == 0)
        return false;

    bool found = false;
    size_t j; /* Holds count of delimiters */

    out->ptr = in->ptr;

    for (size_t i = 0; i < in->len && !found; i++) {
        if (in->ptr[i] == delim) {
            for (j = i; j < in->len && in->ptr[j] == delim; j++)
                ;
            j -= i;

            out->len = i;
            found = true;
        }
    }

    if (!found) {
        out->len = in->len;
        in->len = 0;
        return true;
    }

    in->len -= out->len + j;
    in->ptr += out->len + j;

    return true;
}

SVDEF size_t sv_idx(char c, sv sv)
{
    for (size_t i = 0; i < sv.len; i++) {
        if (sv.ptr[i] == c)
            return i;
    }

    return SV_END_POS;
}

SVDEF size_t sv_last_idx(char c, sv sv)
{
    /* In this loop the unsigned value i will wrap around, as defined in
     * §H.2.2, 1 of the C standard. Thus, once the calculation (size_t(0) - 1) is
     * performed, the value is no longer < sv->len. */

    for (size_t i = sv.len - 1; i < sv.len; i--) {
        if (sv.ptr[i] == c)
            return i;
    }

    return SV_END_POS;
}

SVDEF size_t sv_idx_long(sv search, sv sv)
{
    if (search.len == 0 || search.len > sv.len) {
        return SV_END_POS;
    }

    for (size_t i = 0; i < sv.len - search.len + 1; i++) {
        if (sv.ptr[i] == search.ptr[0]) {
            if (memcmp(sv.ptr + i, search.ptr, search.len) == 0) {
                return i;
            }
        }
    }

    return SV_END_POS;
}

SVDEF size_t sv_last_idx_long(sv search, sv sv)
{
    if (search.len == 0 || search.len > sv.len) {
        return SV_END_POS;
    }

    for (size_t i = sv.len - search.len; i < sv.len; i--) {
        if (sv.ptr[i] == search.ptr[0]) {
            if (memcmp(sv.ptr + i, search.ptr, search.len) == 0) {
                return i;
            }
        }
    }

    return SV_END_POS;
}

SVDEF bool sv_eq(sv s1, sv s2)
{
    if (s1.len != s2.len)
        return false;

    if (s1.ptr == s2.ptr)
        return true;

    return memcmp(s1.ptr, s2.ptr, s1.len) == 0;
}

SVDEF bool sv_cstr_eq(sv sv, const char *cstr)
{
    size_t cstr_len = strlen(cstr);

    if (sv.len != cstr_len)
        return false;

    if (sv.ptr == cstr)
        return true;

    return memcmp(sv.ptr, cstr, sv.len) == 0;
}

SVDEF char *sv_to_cstr(sv sv, char *buf, size_t buflen)
{
    size_t len = sv.len >= buflen ? buflen - 1 : sv.len;

    memcpy(buf, sv.ptr, len);
    buf[len] = '\0';

    return buf;
}

SVDEF bool sv_starts_with(sv start, sv sv)
{
    if (start.len > sv.len) {
        return false;
    }

    return sv_eq(start, sv_from_data(sv.ptr, start.len));
}

SVDEF bool sv_ends_with(sv end, sv sv)
{
    if (end.len > sv.len) {
        return false;
    }

    return sv_eq(end, sv_from_data(sv.ptr + sv.len - end.len, end.len));
}

#endif // SV_IMPLEMENTATION
