#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <readline/readline.h>

#define SV_IMPLEMENTATION
#include "sv.h"

#define BUF_SZ 128

#define GUESSES 6
#define LETTERS 5
#define ALPHABET_SZ 26

#define ASCII_A 0x61

#define ANSI_UP_LINE "\033[F"
#define ANSI_UP_N_LINE "\033[%dF"
#define ANSI_DOWN_N_LINE "\033[%dB"
#define ANSI_BLACK "\033[30m"
#define ANSI_GRAY "\033[30;1m"
#define ANSI_BACK_GREEN "\033[42m"
#define ANSI_BACK_YELLOW "\033[43m"
#define ANSI_BACK_WHITE "\033[47m"
#define ANSI_RESET "\033[0m"

#define VT100_ERASE "\033[2K"

#define SOLUTION_FILE "solutions.txt"
#define SOLUTION_INDEX 0

#define WORDS_FILE "words.txt"
#define WORDS_INDEX 1

#define MMAPPED_FILES 2

enum GuessQuality {
    RightPlace,
    WrongPlace,
    Wrong,
    Unknown,
};

struct CharInfo {
    char chr;
    enum GuessQuality quality;
};

struct WordArray {
    sv *array;
    size_t len;
};

struct Mmapped {
    void *ptr;
    size_t len;
};

static struct CharInfo alphabet[ALPHABET_SZ];
static struct WordArray words;

/* Here, files, which are mapped into memory are registered
 * to be munmap'd in cleanup. */
static struct Mmapped mmap_register[MMAPPED_FILES];

static sv solution;

/* Cursor position on the y-axis */
static int y = 3;

static struct termios termios_disable_echo(void)
{
    struct termios old, new;
    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    return old;
}

static void termios_restore(const struct termios *old)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, old) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

static sv map_file(const char *file)
{
    int fd = open(file, O_RDONLY);

    if (fd == -1) {
        perror(file);
        exit(1);
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        perror("fstat");
        exit(1);
    }

    char *contents = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (contents == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    return sv_from_data(contents, statbuf.st_size);
}

static size_t count_lines(const sv file)
{
    size_t ret = 0;

    for (size_t i = 0; i < file.len; i++) {
        ret += file.ptr[i] == '\n';
    }

    return ret;
}

/* Chooses a random solution from the solution file */
static void choose_solution(void)
{
    sv file = map_file(SOLUTION_FILE);
    mmap_register[SOLUTION_INDEX] = (struct Mmapped){
        .ptr = (void *)file.ptr,
        .len = file.len,
    };

    sv buf;

    size_t lines = count_lines(file);

    sv *words = malloc(lines * sizeof(*words));

    size_t i = 0;
    while (sv_chop_delim('\n', &file, &buf)) {
        words[i++] = buf;
    }

    solution = words[rand() % lines];

    free(words);
}

static void init_words(void)
{
    sv file = map_file(WORDS_FILE);
    mmap_register[WORDS_INDEX] = (struct Mmapped){
        .ptr = (void *)file.ptr,
        .len = file.len,
    };

    sv buf;

    size_t lines = count_lines(file);

    words.array = malloc(lines * sizeof(sv));
    words.len = lines;

    size_t i = 0;
    while (sv_chop_delim('\n', &file, &buf)) {
        words.array[i++] = buf;
    }
}

static void init_alphabet(void)
{
    for (size_t c = 'a' - ASCII_A; c <= 'z' - ASCII_A; c++) {
        alphabet[c] = (struct CharInfo){
            .chr = c + ASCII_A,
            .quality = Unknown,
        };
    }
}

/* NOTE: This could be a hashtable but I won't bother ... */
static bool valid(const char *word)
{
    for (size_t i = 0; i < words.len; i++) {
        if (sv_cstr_eq(words.array[i], word)) {
            return true;
        }
    }

    return false;
}

static enum GuessQuality qualify_guess(const char *guess, size_t index)
{
    const char c = guess[index];

    if (solution.ptr[index] == c)
        return RightPlace;

    for (size_t i = 0; i < LETTERS; i++) {
        /* If we find the letter somewhere we have to ensure it has not already been guessed correctly there */
        if (solution.ptr[i] == c && guess[i] != c)
            return WrongPlace;
    }

    return Wrong;
}

/* Does the guess quality new have higher importance than orig?
 * E.g.: Character 'c' is colored yellow but was now guessed in
 * the right spot. It should now be colored green. Character 'b'
 * is colored green and was now guessed in the wrong spot. It should
 * not be recolored. */
static bool overrides(enum GuessQuality orig, enum GuessQuality new)
{
    assert(new != Unknown);

    if (orig == RightPlace)
        return false;

    if (orig == Unknown || orig == Wrong)
        return true;

    if (orig == WrongPlace && new == RightPlace)
        return true;

    return false;
}

static void print_qualified_char(char c, enum GuessQuality quality)
{
    switch (quality) {
        case RightPlace:
            printf(ANSI_BACK_GREEN ANSI_BLACK "%c" ANSI_RESET, c);
            break;
        case WrongPlace:
            printf(ANSI_BACK_YELLOW ANSI_BLACK "%c" ANSI_RESET, c);
            break;
        case Wrong:
            printf(ANSI_BACK_WHITE ANSI_GRAY "%c" ANSI_RESET, c);
            break;
        case Unknown:
            printf("%c", c);
            break;
    }
}

/* Goes to the first line, erases it, prints msg, waits a moment
 * and goes back to where the next input is */
static void misinput(const char *msg)
{
    static const struct timespec nanosleep_request = { 0, 750000000 };

    struct termios old = termios_disable_echo();

    printf(ANSI_UP_N_LINE VT100_ERASE "%s", y, msg);
    fflush(stdout);

    nanosleep(&nanosleep_request, NULL);

    printf("\r" VT100_ERASE ANSI_DOWN_N_LINE VT100_ERASE, y - 1);
    fflush(stdout);

    termios_restore(&old);
}

/* Prints the alphabet in the line under the current one and goes back up */
static void reprint_alphabet(void)
{
    printf("\n");
    for (size_t i = 0; i < ALPHABET_SZ; i++) {
        print_qualified_char(alphabet[i].chr, alphabet[i].quality);
    }
    printf(ANSI_UP_LINE);
    fflush(stdout);
}

/* Goes up line and reprints chars with colored quality
 * and waits between each char. */
static void color_word_and_update_alphabet(const char *guess)
{
    static const struct timespec nanosleep_request = { 0, 250000000 };

    struct termios old = termios_disable_echo();

    printf(ANSI_UP_LINE);

    for (size_t i = 0; i < LETTERS; i++) {
        enum GuessQuality quality = qualify_guess(guess, i);

        print_qualified_char(guess[i], quality);
        fflush(stdout);

        if (overrides(alphabet[(int)guess[i] - ASCII_A].quality, quality)) {
            alphabet[(int)guess[i] - ASCII_A].quality = quality; /* Update alphabet coloring accordingly (see overrides function) */
        }

        nanosleep(&nanosleep_request, NULL);
    }
    printf("\n");

    termios_restore(&old);
}

static inline bool check_correct(const char *guess)
{
    return sv_cstr_eq(solution, guess);
}

/* Called at exit. It is good practice to clean up after yourself. */
static void cleanup(void)
{
    free(words.array);

    for (size_t i = 0; i < MMAPPED_FILES; i++) {
        if (munmap(mmap_register[i].ptr, mmap_register[i].len) == -1) {
            perror("munmap");
        }
    }
}

int main(void)
{
    /* rand init */
    srand(time(NULL));

    /* Clidle init */
    init_alphabet();
    init_words();
    choose_solution();

    atexit(cleanup);

    /* Readline init */
    rl_editing_mode = 0; /* Put readline into vi-mode */

    printf("\n\n");

    for (int i = 0; i < GUESSES; i++) {
        reprint_alphabet();

        char *line = readline("");

        if (!line) {
            return 0; /* EOF was typed, exit */
        } else if (strlen(line) == 0) {
            free(line);
            i -= 1;
            continue;
        }

        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) != LETTERS) {
            misinput("Wrong length");
            i -= 1; /* Misinput does not count as guess */
        } else if (!valid(line)) {
            misinput("Not in word list");
            i -= 1; /* Misinput does not count as guess */
        } else {
            color_word_and_update_alphabet(line);

            if (check_correct(line)) {
                free(line);
                return 0;
            }

            /* Clear the now current line that has the alphabet on it */
            printf(VT100_ERASE);

            y += 1;
        }

        free(line);
    }

    printf("The word was: "SV_Fmt"\n", SV_Arg(solution));

    return 0;
}
