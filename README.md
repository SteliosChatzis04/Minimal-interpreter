# MiniCParser — Minimal++ Interpreter

A compiler front-end and tree-walking interpreter for **Minimal++**, a small
Pascal-like teaching language. Implements lexical analysis, parsing,
scope/semantic checks, AST construction, and direct interpretation — built
with Flex, Bison, and C.

## Requirements

- `gcc`
- `flex`
- `bison`
- `make`
- `-lfl` (the Flex runtime library)

On Windows, use WSL or MSYS2/MinGW — these tools are not available natively.

## Build

```bash
make
```

This runs `bison -d -v` on `parser.y`, `flex` on `scanner.l`, and compiles
everything into a single `minicc` executable.

```bash
make clean
```

removes all generated files (`lex.yy.c`, `parser.tab.c`, `parser.tab.h`,
`parser.output`, `minicc`).

## Run

```bash
./minicc path/to/program.min
```

The program is parsed, checked, and — if no errors are found — executed
immediately. Output (`print`) goes to stdout; errors go to stderr.

```bash
./minicc tests/test_simple.min
```

## Language reference (Minimal++)

- **Program shape**: `program name { ... }`
- **Variables**: `declare x, y;`
- **Assignment**: `x := expr;`
- **Control flow**:
  - `if (cond) { ... } else { ... }`
  - `while (cond) { ... }`
  - `doublewhile (cond) { ... } else { ... }` — loops while `cond` is true from
    the start; if `cond` is false initially, runs the `else` body once instead
  - `loop { ... }` — infinite loop, terminated with `exit`
  - `forcase when: (cond): { ... } ... default: { ... }` — runs the first
    matching `when` clause, or `default` if none match
  - `incase when: (cond): { ... } ...` — runs **all** matching `when` clauses
- **Functions**: `function name(in p1, inout p2) { ... return(expr); }`
- **Procedures**: `procedure name(in p1) { ... }`, invoked with `call name(...)`
- **I/O**: `input(x)`, `print(expr)`
- **Operators**: arithmetic `+ - * /`; comparison `= <> < > <= >=`; boolean
  `and or not`
- Boolean expressions are written in brackets, e.g. `if (x < 10 and [y > 0])`

### Parameter passing

- `in p` — passed by value (a copy). Changes inside the function are not
  visible to the caller.
- `inout p` — passed by reference. The caller argument **must be a bare
  variable** (not an expression); its value is written back after the call
  returns.

### Example

```
program main
{
    declare x, y;

    function add(in a, in b)
    {
        return(a + b);
    }

    x := 5;
    y := add(in x, in 10);
    print(y);   // 15
}
```

## Limits

- Identifiers: max 30 characters (longer ones are truncated with a warning).
- Numeric literals: max 65535 (larger ones are clamped with a warning).
- `return` requires parentheses: `return(expr);`, not `return expr;`.
- Up to 64 items in a single `when`-clause list (`forcase`/`incase`) or in a
  single function/procedure call's argument list.

## Error handling

The compiler reports as many problems as it can in a single pass rather than
stopping at the first one:

- **Lexical errors** (oversized identifiers, out-of-range numbers,
  unrecognized characters) are reported and recovered from automatically.
- **Semantic errors** (duplicate declarations, undeclared identifiers) are
  reported at the point they occur; parsing continues.
- **Runtime errors** (undefined variable, division by zero, unknown
  function) print a message and substitute a safe default (`0`) rather than
  crashing.

## Tests

Sample programs live in `tests/`:

```bash
./minicc tests/test_control.min      # control-flow constructs
./minicc tests/test_function.min     # functions/procedures
./minicc tests/test_interp_func.min  # interpreter: recursion, in/inout params
./minicc tests/test_duplicate.min    # expect: duplicate declaration error
./minicc tests/test_undeclared.min   # expect: undeclared identifier error
```
