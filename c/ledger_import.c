/* ledger_import: read a ledger CSV on stdout as SQL, no external dependencies. */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FIELDS 5
#define MAX_FIELD_LEN 512

typedef struct {
    char text[MAX_FIELD_LEN];
} field;

static int failures;

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "ledger_import: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static void strip(field *f) {
    size_t start = 0;
    size_t end = strlen(f->text);
    while (start < end && isspace((unsigned char) f->text[start])) start++;
    while (end > start && isspace((unsigned char) f->text[end - 1])) end--;
    memmove(f->text, f->text + start, end - start);
    f->text[end - start] = '\0';
}

static bool parse_line(const char *line, field *out) {
    size_t i = 0;
    size_t n = strlen(line);
    int f = 0;

    while (f < MAX_FIELDS) {
        size_t len = 0;
        bool quoted = false;

        while (i < n && line[i] == ' ') i++;
        if (i < n && line[i] == '"') {
            quoted = true;
            i++;
        }
        while (i < n && len < MAX_FIELD_LEN - 1) {
            if (!quoted && line[i] == ',') break;
            if (quoted && line[i] == '"' && i + 1 < n && line[i + 1] == '"') {
                out[f].text[len++] = '"';
                i += 2;
                continue;
            }
            if (quoted && line[i] == '"') {
                i++;
                quoted = false;
                continue;
            }
            out[f].text[len++] = line[i++];
        }
        out[f].text[len] = '\0';
        strip(&out[f]);
        f++;
        if (i < n && line[i] == ',') {
            i++;
            continue;
        }
        break;
    }
    return f == MAX_FIELDS;
}

static bool valid_date(const char *s) {
    if (strlen(s) != 10) return false;
    for (int i = 0; i < 10; i++) {
        bool digit = (i == 4 || i == 7) ? (s[i] == '-') : (s[i] >= '0' && s[i] <= '9');
        if (!digit) return false;
    }
    return true;
}

static void emit(const char *sql, ...) {
    va_list ap;
    va_start(ap, sql);
    vprintf(sql, ap);
    va_end(ap);
    printf("\n");
}

static void quote(const char *value, char *buf, size_t cap) {
    size_t o = 0;
    buf[o++] = '\'';
    for (size_t i = 0; value[i] && o + 2 < cap; i++) {
        if (value[i] == '\'') buf[o++] = '\'';
        buf[o++] = value[i];
    }
    buf[o++] = '\'';
    buf[o] = '\0';
}

int main(int argc, char **argv) {
    if (argc > 2) die("usage: ledger_import [file.csv]");

    FILE *in = stdin;
    if (argc == 2) {
        in = fopen(argv[1], "r");
        if (!in) die("cannot open %s", argv[1]);
    }

    char line[4096];
    int account_id = 1;
    const char *via = "ledger_import";
    long lineno = 0;

    printf("BEGIN;\n");
    while (fgets(line, sizeof line, in)) {
        lineno++;
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0 || line[0] == '#') continue;

        field f[MAX_FIELDS];
        if (!parse_line(line, f)) {
            fprintf(stderr, "ledger_import: line %ld: expected %d columns\n", (long) lineno, MAX_FIELDS);
            failures++;
            continue;
        }
        if (lineno == 1 && strcmp(f[0].text, "recorded_on") == 0) continue;
        if (!valid_date(f[0].text)) {
            fprintf(stderr, "ledger_import: line %ld: bad date '%s'\n", (long) lineno, f[0].text);
            failures++;
            continue;
        }
        if (strcmp(f[1].text, "income") != 0 && strcmp(f[1].text, "expense") != 0) {
            fprintf(stderr, "ledger_import: line %ld: bad kind '%s'\n", (long) lineno, f[1].text);
            failures++;
            continue;
        }
        char *endp;
        long amount = strtol(f[2].text, &endp, 10);
        if (*endp != '\0' || amount < 0) {
            fprintf(stderr, "ledger_import: line %ld: bad amount '%s'\n", (long) lineno, f[2].text);
            failures++;
            continue;
        }
        if (f[3].text[0] == '\0') {
            fprintf(stderr, "ledger_import: line %ld: empty category\n", (long) lineno);
            failures++;
            continue;
        }

        char on[64], kind[64], cat[64], note[1024], via_buf[64];
        quote(f[0].text, on, sizeof on);
        quote(f[1].text, kind, sizeof kind);
        quote(f[3].text, cat, sizeof cat);
        quote(f[4].text, note, sizeof note);
        quote(via, via_buf, sizeof via_buf);
        const char *note_sql = f[4].text[0] == '\0' ? "NULL" : note;

        emit("INSERT INTO entry (account_id, recorded_on, kind, amount_cents, category, note, imported_via) "
             "VALUES (%d, %s, %s, %ld, %s, %s, %s);",
             account_id, on, kind, amount, cat, note_sql, via_buf);
    }
    printf("COMMIT;\n");

    if (in != stdin) fclose(in);
    return failures == 0 ? 0 : 1;
}
