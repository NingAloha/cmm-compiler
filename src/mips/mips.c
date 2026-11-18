#define _POSIX_C_SOURCE 200809L
#include "mips.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Leave room for alignment and the saved-register area. */
#define MAX_AREA (INT32_MAX - 15)

typedef enum {
  FUNCTION,
  LABEL,
  DEC,
  PARAM,
  ARG,
  CALL,
  RETURN,
  READ,
  WRITE,
  GOTO,
  IF,
  COPY,
  BINARY
} Opcode;
typedef enum { SLOT, IMMEDIATE, ADDRESS, INDIRECT } Mode;
typedef struct {
  Mode mode;
  long value;
} Operand;
typedef struct {
  Opcode op;
  char *text;
  char *word[6];
  const char *pattern;
  Operand operand[6];
  size_t target, count;
  long bytes;
} Line;

typedef struct {
  const char *name;
  long offset;
  int array;
} Slot;

static int grow(void **data, size_t *capacity, size_t width) {
  if (*capacity > SIZE_MAX / width / 2 || 16 > SIZE_MAX / width)
    return 0;
  size_t next = *capacity ? *capacity * 2 : 16;
  void *p = realloc(*data, next * width);
  if (!p)
    return 0;
  *data = p;
  *capacity = next;
  return 1;
}

static int letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int digit(char c) { return c >= '0' && c <= '9'; }

static int name(const char *s) {
  if (!letter(*s++))
    return 0;
  for (; *s; ++s)
    if (!letter(*s) && !digit(*s))
      return 0;
  return 1;
}

static int number(const char *s, long min, long max, long *result) {
  const char *p = s + (*s == '-');
  if (!digit(*p))
    return 0;
  while (digit(*p))
    ++p;
  if (*p)
    return 0;
  errno = 0;
  long value = strtol(s, NULL, 10);
  if (errno == ERANGE || value < min || value > max)
    return 0;
  *result = value;
  return 1;
}

static int member(const char *s, const char *const *set, size_t count) {
  for (size_t i = 0; i < count; ++i)
    if (!strcmp(s, set[i]))
      return 1;
  return 0;
}

static int parse(Line *line) {
  /* Lowercase fields describe variable tokens; other fields are literal. */
  static const struct {
    Opcode op;
    const char *pattern;
  } syntax[] = {
      {FUNCTION, "FUNCTION n :"},
      {LABEL, "LABEL n :"},
      {DEC, "DEC n z"},
      {PARAM, "PARAM n"},
      {ARG, "ARG v"},
      {CALL, "d := CALL n"},
      {RETURN, "RETURN v"},
      {READ, "READ d"},
      {WRITE, "WRITE v"},
      {GOTO, "GOTO n"},
      {IF, "IF v r v GOTO n"},
      {COPY, "d := v"},
      {BINARY, "d := v a v"},
  };
  static const char *const arithmetic[] = {"+", "-", "*", "/"};
  static const char *const relations[] = {"==", "!=", "<", ">", "<=", ">="};
  size_t count = 0;
  char *save;
  for (char *word = strtok_r(line->text, " \t\r\n\v\f", &save); word;
       word = strtok_r(NULL, " \t\r\n\v\f", &save)) {
    if (count == sizeof(line->word) / sizeof(*line->word))
      return 0;
    line->word[count++] = word;
  }
  if (!count)
    return 1;
  for (size_t i = 0; i < sizeof(syntax) / sizeof(*syntax); ++i) {
    const char *field = syntax[i].pattern;
    size_t j = 0;
    for (; *field && j < count; ++j) {
      size_t n = strcspn(field, " ");
      const char *word = line->word[j];
      int ok;
      switch (*field) {
      case 'n':
        ok = name(word);
        break;
      case 'z':
        ok = number(word, 4, MAX_AREA, &line->bytes) && line->bytes % 4 == 0;
        break;
      case 'r':
        ok = member(word, relations, sizeof(relations) / sizeof(*relations));
        break;
      case 'a':
        ok = member(word, arithmetic, sizeof(arithmetic) / sizeof(*arithmetic));
        break;
      case 'd':
      case 'v': {
        Operand *operand = &line->operand[j];
        operand->mode = SLOT;
        if (*word == '#' && *field == 'v') {
          operand->mode = IMMEDIATE;
          ok = number(word + 1, INT32_MIN, INT32_MAX, &operand->value);
        } else {
          if (*word == '*') {
            operand->mode = INDIRECT;
            ++word;
          } else if (*word == '&' && *field == 'v') {
            operand->mode = ADDRESS;
            ++word;
          }
          ok = name(word);
        }
        break;
      }
      default:
        ok = strlen(word) == n && !strncmp(word, field, n);
        break;
      }
      if (!ok)
        break;
      field += n;
      if (*field)
        ++field;
    }
    if (j == count && !*field) {
      line->op = syntax[i].op;
      line->pattern = syntax[i].pattern;
      return 1;
    }
  }
  return 0;
}

static int read_ir(FILE *ir, Line **lines, size_t *count) {
  size_t capacity = 0;
  char *text = NULL;
  size_t length = 0;
  ssize_t n;
  int ok = 1;
  while ((n = getline(&text, &length, ir)) >= 0) {
    Line line = {0};
    line.text = text;
    if (memchr(text, '\0', (size_t)n) || !parse(&line)) {
      ok = 0;
      break;
    }
    if (!line.pattern)
      continue;
    if (*count == capacity) {
      void *storage = *lines;
      if (!grow(&storage, &capacity, sizeof(line))) {
        ok = 0;
        break;
      }
      *lines = storage;
    }
    (*lines)[(*count)++] = line;
    text = NULL;
    length = 0;
  }
  free(text);
  return ok && feof(ir) && !ferror(ir);
}

static size_t find_line(const Line *lines, size_t first, size_t end, Opcode op,
                        const char *id) {
  for (size_t i = first; i < end; ++i)
    if (lines[i].op == op && !strcmp(lines[i].word[1], id))
      return i;
  return end;
}

static Slot *find_slot(Slot *slots, size_t count, const char *id) {
  for (size_t i = 0; i < count; ++i)
    if (!strcmp(slots[i].name, id))
      return &slots[i];
  return NULL;
}

static Slot *add_slot(Slot **slots, size_t *count, size_t *capacity,
                      const char *id, long bytes, long *used, int array) {
  if (bytes > MAX_AREA - *used)
    return NULL;
  if (*count == *capacity) {
    void *storage = *slots;
    if (!grow(&storage, capacity, sizeof(**slots)))
      return NULL;
    *slots = storage;
  }
  *used += bytes;
  Slot *slot = &(*slots)[(*count)++];
  *slot = (Slot){id, -*used, array};
  return slot;
}

/* Discover function signatures before resolving calls, including forward
 * calls and recursion. Labels are resolved only within the current function. */
static int signatures(Line *lines, size_t count) {
  for (size_t i = 0; i < count;) {
    if (lines[i].op != FUNCTION ||
        find_line(lines, 0, i, FUNCTION, lines[i].word[1]) != i)
      return 0;
    size_t j = i + 1;
    while (j < count && lines[j].op == PARAM)
      ++j;
    lines[i].count = j - i - 1;
    if (lines[i].count > MAX_AREA / 4)
      return 0;
    while (j < count && lines[j].op != FUNCTION) {
      if (lines[j].op == PARAM)
        return 0;
      ++j;
    }
    lines[i].target = j;
    i = j;
  }
  return 1;
}

static int layout(Line *lines, size_t count, size_t start, Slot **slots,
                  size_t *capacity) {
  size_t end = lines[start].target;
  size_t used_slots = 0, pending = 0;
  long used = 0;
  /* Reserve whole arrays before any operand can introduce a scalar slot. */
  for (size_t i = start + 1; i < end; ++i) {
    Line *line = &lines[i];
    if (line->op == DEC || line->op == PARAM) {
      const char *id = line->word[1];
      if (find_slot(*slots, used_slots, id))
        return 0;
      Slot *slot =
          add_slot(slots, &used_slots, capacity, id,
                   line->op == DEC ? line->bytes : 4, &used, line->op == DEC);
      if (!slot)
        return 0;
      line->operand[1] = (Operand){SLOT, slot->offset};
    } else if (line->op == LABEL &&
               find_line(lines, start + 1, i, LABEL, line->word[1]) != i) {
      return 0;
    }
  }
  for (size_t i = start + 1; i < end; ++i) {
    Line *line = &lines[i];
    if (line->op == ARG) {
      ++pending;
    } else if (line->op == CALL) {
      size_t callee = find_line(lines, 0, count, FUNCTION, line->word[3]);
      if (callee == count || lines[callee].count != pending)
        return 0;
      line->target = callee;
      line->count = pending;
      pending = 0;
    } else if (pending) {
      return 0;
    }
    if (line->op == GOTO || line->op == IF) {
      line->target = find_line(lines, start + 1, end, LABEL,
                               line->word[line->op == IF ? 5 : 1]);
      if (line->target == end)
        return 0;
    }
    size_t j = 0;
    for (const char *field = line->pattern; *field; ++j) {
      Operand *operand = &line->operand[j];
      if ((*field == 'v' || *field == 'd') && operand->mode != IMMEDIATE) {
        const char *id = line->word[j] + (operand->mode != SLOT);
        Slot *slot = find_slot(*slots, used_slots, id);
        if (!slot && operand->mode != ADDRESS)
          slot = add_slot(slots, &used_slots, capacity, id, 4, &used, 0);
        if (!slot || (operand->mode == ADDRESS && !slot->array) ||
            (*field == 'd' && slot->array))
          return 0;
        operand->value = slot->offset;
      }
      field += strcspn(field, " ");
      if (*field)
        ++field;
    }
  }
  lines[start].bytes = (used + 7) & ~7L;
  return pending == 0;
}

typedef struct {
  FILE *file;
  int ok;
} Emitter;

static void emit(Emitter *out, const char *format, ...) {
  if (!out->ok)
    return;
  va_list args;
  va_start(args, format);
  out->ok = vfprintf(out->file, format, args) >= 0;
  va_end(args);
}

static int short_offset(long offset) {
  return offset >= -32768 && offset <= 32767;
}

static void add_offset(Emitter *out, const char *dst, const char *base,
                       long offset) {
  if (short_offset(offset))
    emit(out, "  addiu %s, %s, %ld\n", dst, base, offset);
  else
    emit(out, "  li $t9, %ld\n  addu %s, %s, $t9\n", offset, dst, base);
}

static void memory(Emitter *out, const char *op, const char *reg,
                   const char *base, long offset) {
  if (!short_offset(offset)) {
    add_offset(out, "$t9", base, offset);
    base = "$t9";
    offset = 0;
  }
  emit(out, "  %s %s, %ld(%s)\n", op, reg, offset, base);
}

static void load(Emitter *out, const char *reg, Operand operand) {
  switch (operand.mode) {
  case IMMEDIATE:
    emit(out, "  li %s, %ld\n", reg, operand.value);
    break;
  case ADDRESS:
    add_offset(out, reg, "$fp", operand.value);
    break;
  case INDIRECT:
    memory(out, "lw", "$t8", "$fp", operand.value);
    emit(out, "  nop\n  lw %s, 0($t8)\n  nop\n", reg);
    break;
  case SLOT:
    memory(out, "lw", reg, "$fp", operand.value);
    emit(out, "  nop\n");
    break;
  }
}

static void store(Emitter *out, const char *reg, Operand operand) {
  if (operand.mode == INDIRECT) {
    memory(out, "lw", "$t8", "$fp", operand.value);
    emit(out, "  nop\n  sw %s, 0($t8)\n", reg);
  } else {
    memory(out, "sw", reg, "$fp", operand.value);
  }
}

static void branch(Emitter *out, const Line *line, size_t i) {
  load(out, "$t0", line->operand[1]);
  load(out, "$t1", line->operand[3]);
  const char *rel = line->word[2];
  const char *inverse;
  const char *left = "$t0", *right = "$t1";
  if (*rel == '=' || *rel == '!') {
    inverse = *rel == '=' ? "bne" : "beq";
  } else {
    int swap = (*rel == '>') != (rel[1] == '=');
    emit(out, "  slt $t2, %s, %s\n", swap ? right : left, swap ? left : right);
    left = "$t2";
    right = "$zero";
    inverse = rel[1] == '=' ? "bne" : "beq";
  }
  /* Keep the relative branch local, even for a very large function. */
  emit(out,
       "  %s %s, %s, __cmm_skip_%zu\n  nop\n"
       "  j __cmm_label_%zu\n  nop\n__cmm_skip_%zu:\n",
       inverse, left, right, i, line->target, i);
}

static void binary(Emitter *out, const Line *line) {
  load(out, "$t0", line->operand[2]);
  load(out, "$t1", line->operand[4]);
  switch (*line->word[3]) {
  case '+':
    emit(out, "  addu $t2, $t0, $t1\n");
    break;
  case '-':
    emit(out, "  subu $t2, $t0, $t1\n");
    break;
  case '*':
    emit(out, "  mult $t0, $t1\n  mflo $t2\n");
    break;
  case '/':
    emit(out, "  div $t0, $t1\n  mflo $t2\n");
    break;
  }
  store(out, "$t2", line->operand[0]);
}

static void function(Emitter *out, const Line *lines, size_t start) {
  const Line *fn = &lines[start];
  if (!strcmp(fn->word[1], "main"))
    emit(out, ".globl main\nmain:\n");
  /* $fp points at the saved $fp/$ra pair, with locals below and arguments
   * above. Only the caller adjusts the outgoing argument area. */
  emit(out,
       "__cmm_fn_%zu:\n  addiu $sp, $sp, -8\n  sw $fp, 0($sp)\n"
       "  sw $ra, 4($sp)\n  move $fp, $sp\n",
       start);
  if (fn->bytes)
    add_offset(out, "$sp", "$sp", -fn->bytes);
  for (size_t i = start + 1; i < fn->target; ++i) {
    const Line *line = &lines[i];
    switch (line->op) {
    case LABEL:
      emit(out, "__cmm_label_%zu:\n", i);
      break;
    case PARAM:
      memory(out, "lw", "$t0", "$fp", 8 + 4 * (long)(i - start - 1));
      emit(out, "  nop\n");
      store(out, "$t0", line->operand[1]);
      break;
    case CALL: {
      long bytes = (4 * (long)line->count + 7) & ~7L;
      if (bytes)
        add_offset(out, "$sp", "$sp", -bytes);
      for (size_t j = 0; j < line->count; ++j) {
        load(out, "$t0", lines[i - 1 - j].operand[1]);
        memory(out, "sw", "$t0", "$sp", 4 * (long)j);
      }
      emit(out, "  jal __cmm_fn_%zu\n  nop\n", line->target);
      if (bytes)
        add_offset(out, "$sp", "$sp", bytes);
      store(out, "$v0", line->operand[0]);
      break;
    }
    case RETURN:
      load(out, "$v0", line->operand[1]);
      emit(out, "  j __cmm_ret_%zu\n  nop\n", start);
      break;
    case READ:
      emit(out, "  li $v0, 4\n  la $a0, __cmm_prompt\n  syscall\n"
                "  li $v0, 5\n  syscall\n");
      store(out, "$v0", line->operand[1]);
      break;
    case WRITE:
      load(out, "$a0", line->operand[1]);
      emit(out, "  li $v0, 1\n  syscall\n  li $v0, 11\n"
                "  li $a0, 10\n  syscall\n");
      break;
    case GOTO:
      emit(out, "  j __cmm_label_%zu\n  nop\n", line->target);
      break;
    case IF:
      branch(out, line, i);
      break;
    case COPY:
      load(out, "$t0", line->operand[2]);
      store(out, "$t0", line->operand[0]);
      break;
    case BINARY:
      binary(out, line);
      break;
    case ARG:
    case DEC:
    case FUNCTION:
      break;
    }
  }
  emit(out,
       "  move $v0, $zero\n__cmm_ret_%zu:\n  move $sp, $fp\n"
       "  lw $fp, 0($sp)\n  lw $ra, 4($sp)\n  addiu $sp, $sp, 8\n"
       "  jr $ra\n  nop\n",
       start);
}

int mips_generate(FILE *ir, FILE *output) {
  if (!ir || !output || ir == output)
    return 0;
  Line *lines = NULL;
  Slot *slots = NULL;
  size_t count = 0, capacity = 0;
  int ok = read_ir(ir, &lines, &count) && signatures(lines, count);
  for (size_t i = 0; ok && i < count; i = lines[i].target)
    ok = layout(lines, count, i, &slots, &capacity);
  if (ok) {
    Emitter out = {output, 1};
    emit(&out, ".data\n__cmm_prompt: .asciiz \"Enter an integer:\"\n.text\n");
    for (size_t i = 0; i < count; i = lines[i].target)
      function(&out, lines, i);
    ok = out.ok && fflush(output) == 0 && !ferror(output);
  }
  free(slots);
  for (size_t i = 0; i < count; ++i)
    free(lines[i].text);
  free(lines);
  return ok;
}
