# The Cmath Engine — Architecture Guide

*How the engine is put together, why the split between C and `.math` is what it
is, and how every subsystem behaves — with the numbers that were measured on
the day this was written (2026-10-24). Companion documents: the
**MEP Language Guide** (docs/MEP_Language_Guide.md, every example
machine-verified) and the **Engine Audit** (docs/engine-audit-2026-10-24.md,
implementation-level findings P1–P10 / M1–M5).*

---

## 1. What this engine is

`libmathengine.a` is a small, embeddable C engine for a language called **MEP**
(Math Engine Programming). Its defining trade — and the one architectural
decision that shapes everything else — is:

> **C owns grammar and intrinsics. `.math` owns meaning.**

- The C core (`Core/`) implements the lexer, parser, AST, evaluator, session
  machinery and a tiny `@`-intrinsic registry. It knows *how* things group.
- The scientific library (`Library/*.math`, ≈ 3 300 lines) is **plain MEP
  source**, compiled to a binary `.mathc` image at build time and embedded in
  `stdlib_precompiled.h`. It knows *what* things mean — statistics, units,
  calculus, ODEs, physics constants, string algorithms.
- Linking a `.math` module is the same act whether it happens at boot, via a
  user extension, or after flipping a module toggle: the image is
  memcpy'd and its symbols become live.

Measured shape of the shipped engine (this host):

| Metric | Value |
|---|---|
| Boot (eng_init, zero-parse `.mathc` load) | **600–730 µs** (bench mean 726.51 µs, 2026-10-24) |
| Modules / overloads / constants at boot | **19 / 1380 / 30** (REPL banner, `:modules`) |
| Engine footprint | **≈ 788.6 KB** (256 KB `.mathc` compile buffer + 8 session slots)¹ |
| `sizeof(Value)` == `sizeof(AstNode)` | **8 bytes** each |
| `@ext` link re-emission (link-time `.mathc` refresh) | **216 137 bytes** for the canonical `[units_demo]` body |
| Library source | ≈ 3 300 lines `.math` across 15 modules (incl. `bootstrap`) |
| Core source | hardcode 1 832 · delegate 4 642 · parser 2 941 · evaluator 1 458 · compiler 1 219 · session 298 · snapshot 697 · linker 236 lines |

¹ `eng_init` allocates the compile buffer and the 8 sessions in one arena;
the number is the audit's measured `sizeof(Engine)`-visible total, not a
promise about other builds.

Throughput (same bench run as above; treat cross-machine deltas of ±50 % as
normal: see §9):

```
boot                 726.51 us   |  scalar eval       426.56 us
vector HOF pipeline  582.99 us   |  3x3 linsolve      544.34 us
50-step RK4 eval    2225.74 us
```

The *relative* structure — boot ≪ scalar eval ≈ HOF ≈ linsolve — is the
invariant that matters: subsystems contribute roughly comparable
per-evaluation work, and the O(1) boot claim holds.

---

## 2. Layer map (repository)

```
Core/                 all C, no policy
  Headers/            math_engine.h (public C API), sym_table.h,
                      evaluator.h, parser.h, lexer.h, mem_arena.h,
                      stdlib_precompiled.h  ← embedded .mathc image
  Code/               parser.c, evaluator.c        — grammar-independent AST walk
  compile/            compiler.c (tokens→AST→image), linker.c (topo link),
                      mathc_bin.c (48-byte-header serializer),
                      user_loader.c (host extension API)
  session/            session.c (8-slot state machine),
                      snapshot.c (MSP1 hash chain, full-history restore)
  lib/hardcode/       hardcode.c — grammar-level C (see §5)
  lib/delegate/       delegate.c — the @-intrinsic registry (see §6)
Library/              15 .math modules + stdlib.mathc (linked image text)
Test/                 main.c — 555-expression suite + CLI (--repl/--bench/--eval)
DebugBuild/           myapp binary + libmathengine.a (build products)
docs/                 guides + audit (this tree)
```

There is no "short-circuit path": nothing in the evaluator knows the name
`sqrt` or `linsolve`. Everything matches through the same symbol/overload
table that user `.math` populates.

---

## 3. The value model

`Value` is one 8-byte tagged word. Tag families:

| family | used for |
|---|---|
| `real` (f64) | all numbers; **units ride along as a 1-byte group id** (`eng_val_unit` returns it, `eng_unit_name` maps id → name) — bare display, metadata only |
| `iota`/`complex` | postfix `i` builds an imaginary half; complex results are `$complex` values (display `a + bi`) |
| `str` | interned slices |
| `vec<T>`/`map`/`mat` | rank-1..4 tensors, dicts; growable (cap 0→4→…; see §4) |
| `fn` | ML-style closures over AST bodies (`<fn/1>` display) |
| `struct` | `$vec2 $vec3 $circle $mat2 $complex` — row-struct display |
| `none`/`unit` | void-ish results and unit-only values |

Unit metadata is a full 7-D SI exponent vector declared per group:
`@dim(group, m, kg, s, A, K, mol, cd)` (`Library/units.math:11-…`), and
`unit_dim(v, axis)` reads it back (verified in §19.4 of the Language Guide:
`5 J` → L=2, M=1, T=-2). Dimensional correctness is therefore *arithmetical*,
not stringly-typed: `10 N * 2 m` is dimension-compatible with `5 J` because
exponent vectors add — `same_dim` returns 1.

---

## 4. Memory model — arena + scratch + sessions

1. **Permanent arena** holds the AST and library constants; grown arenas are
   never freed per-eval (boot loads the precompiled image wholesale).
2. **Scratch pool** serves per-evaluation temporaries; `pool_used()` reports
   occupancy, `gc_compact()` mark-sweeps and returns the bytes reclaimed
   (verified: a vec built with 120 pushes survives compaction intact and
   reads back — 555-test `[Exp 6]`).
3. **8 session slots** each carry their own variable frame, module-toggle
   mask and MSP1 history. A session eval's canonical form is *text*: the
   engine re-materializes persistent variables from their serialized text
   each eval (`session.c` slot-rewind; audit finding **P8** quantifies the
   O(vars) cost and sketches lazy materialization).

### 4.1 MSP1 — the snapshot chain

Each evaluated line writes one **MSP1 record** holding the complete session
state (`vars` serialization) and the line result:

```
MSP1|seq:1|prev:00000000|hash:B01BE54B|vars:1|qq:real:none:*:*=42|res:real:none=42
```

- `seq` — 1-based line index; `prev` — previous record's hash (a session is
  a **hash-linked audit trail**, per-history tamper evidence comes free);
- `hash` — content hash of this record;
- `vars` — serialized slots `name:type:unit:ops:pairs=value`;
- `res` — this line's result value.

`eng_session_snapshot_rewind/next` walk the chain; `eng_session_restore(texts, n)`
rebuilds a *new* session by **replaying** the chain (hashes re-derived as they
replay, so the restored session's own history is well-formed). Verbatim
`@ext → session → save → close → restore` workflow with `w` surviving intact
(→ `84`) is reproduced as §19.3 of the Language Guide.

---

## 5. `hardcode/` — what C is allowed to know

The `hardcode` layer is the **grammar** fast-path: the things that *must* be
known before any `.math` runs because they change parsing itself. Rule of
thumb from the architecture banner (§16 of the test README):

> only grammar may be hardcoded — never meaning.

Current clusters (audit §2 cross-referenced against `hardcode.c`):

| cluster | why it is grammar |
|---|---|
| `;`/newline statement grouping, `:=`/`=` store shapes | structural |
| reserved words (`if or then else default switch case …`) | scanner-level |
| implicit multiplication & maximal-munch unit suffix (`50m`, `50mi`) | resegmentation |
| `in`-repair (`25km in m` vs `x in coll`) | ergonomic disambiguation |
| `vec<T>() list<T>() stack<T>() queue<T>()` | token-shape parameterization (`vec<int>` correctly errors — element type set is fixed ([real str …], verified in §21 of the Guide)) |
| comprehensions `[f(i) for i in xs]`, lambdas `x => …`, `((a,b) => …)` | fixity-changing constructs |
| switch-pattern matching (`case default` binding) | parse-time |

The one *semantic* quarantine — distributed comparisons over `&`/`|` —
decides, at eval time, whether an infix symbol is additive/relational by
walking its AST (`hardcode_is_additive_or_rel_sym`). That is the engine's
only rule-bending in this layer, and audit finding **P5** records how it
should be collapsed into a compile-time `meta_tag` bit. Note the raw-glyph
debt (**P10**): `":="`, `"="`, `"<"`/`">"` shapes are glyph-matched; the
audit's recommendation is a *link-time consistency warn*, not a runtime fix.

The container-constructor grammar is worth stating precisely (verified in
Language Guide §8): `vec()` empty (cap 0), `vec(n)` capacity-n (len 0),
`vec(n, v)` = **n copies of v**, ≥ 3 args or a non-unitless-real first arg
= literal list; `vec<T>(…)` (T ∈ {real, str, iota, …}, **not int**) locks the
element type; `list(v)` singleton; `map()` even key/value pairs; 2-arg
`map(coll, fn)` is special-cased as a HOF.

---

## 6. `delegate/` — the `@` registry

`delegate.c` (4 642 lines) registers host primitives as `@name` tokens the
compiler resolves at parse time:

```c
deleg_def("@sqrt", 1, /*variadic*/0);   // registry sketch, see delegate.h
```

- **Lookup cost** today: linear name walk + per-candidate length check
  (audit **P4** — length-bucket fix sketch included).
- Anything callable from `.math` that *must* be a C function (I/O, the `.math`
  serializer, the bin ops behind the hardwired fast paths) lives here;
  *everything else* belongs in `.math` (audit Q1/Q2 migration verdicts).
- The **console/fmt intrinsic set** (`@str_*`, `@fmt`, `@regex_*`) is
  registered the same way — string chapter of the Language Guide shows the
  plain-MEP face of those calls.

The registry is intentionally *boring*: no dispatch magic, no state. The
overload table (§7) is where polymorphism happens.

---

## 7. Overload resolution & precedence mechanics

Both sides of a decl, `.math` and hardcode, funnel into one overload table:

- Each symbol carries up to `SYM_MAX_OVERLOADS` overloads with fixity
  (FIX_FUNC · INFIX_L/R · PREFIX · POSTFIX) and precedence `uint8_t`.
- `.math` can pin user precedence with the **`:<N>` annotation**
  (`(val, T) in (unit, T) :<42`, member converters `:90`, unit suffixes
  `:95` — `Library/conversions.math:11`, `Library/units.math`).
- Every eval of an operator-call goes through `eval_dispatch_overload`
  (scoring by arity+types+units); the Language Guide §16.1 shows the
  *visible* behavior: `ode_rk4(t, y, rhs, …)` matches its **lazy-symbol**
  overload while an fn in the rhs slot walks to a *different* overload set
  and fails with `error: No active overload for 'raw_val' with type (fn)`.
- Module toggling (`@mod|name=0/1`, `eng_set_module`) filters the table:
  disabling `[complex_ext]` removes even the **postfix `i` suffix** and
  `carg` from the language (verified, §19.3 of the Guide) — syntax comes
  from data too.
- Precedence/associativity effects (right-assoc `^`, `<42` `in`, `:95`
  suffix juxtaposition vs `:90` member calls) are documented as the §20
  master table of the Language Guide; parser hooks are
  `sym_get_active_prec`-driven at the 6 sites listed in audit **P6**.

`meta_tag` bits (audit **M1**) already classify lazy-eval families
(sym_diff/integ/solve/taylor/raw_val/with_unit …); the P5/P6 plan occupies
the free 0x20/0x10 bits with *relational* and *additive* flags so the
distributed-comparison walk happens once per overload, at link time.

---

## 8. Compile & link pipeline (`Core/compile/`)

```
source text ─ tokenize ─ parse(AST, 8B nodes) ─ type/unit checks
    ─ mathc_bin serialize (48-byte header + symbol/AST payload)
        ─ linker topological merge with active chain ─ engine state
```

- `mathc_bin.c` serializes to the **`.mathc` image**: flat, memcpy-loadable,
  re-emitted wholesale on every link (that is why `@ext` prints
  `mathc bytes=216137` — the *complete* active image, not a delta).
- `user_loader.c` gives hosts two calls:
  `user_compile_ext(text) -> image bytes`, `user_load_mathc(image) -> live`.
  The 555-test suite round-trips both (compile → disk → load → calls work,
  `[3]` and `[4]` blocks of `main.c`).
- `linker.c` performs **topological** module linking: a `[mod]` header's
  `<[dep]` list is resolved depth-first before the module's own symbols are
  installed — so `physics` gets `units`' suffixes without any manual order.
- Session/pool memory is *not* part of the image; only code+constants are.

**Why zero-parse boot works.** The engine boot is just
`eng_init → memcpy(image) → rebuild hash/sym index`: no lexer, no parser.
That is the 604–730 µs number; a text-based re-boot of the same library
would pay full 19-module parse (multiply the byte-parse cost of every
`.math` line) — the design buys a ~30–80× boot advantage and pays for it
with a re-emit-on-link policy rather than incremental patching.

---

## 9. Evaluation (`Core/Code/evaluator.c`)

The tree-walk wheelhouse:

| AST kind | behavior |
|---|---|
| literals / consts | `real str` payloads; const-*symbol* refs currently linear-scan the const table (**P7** fix: parse-time resolution) |
| AST_CALL | score-match overloads; lazy-symbol families bind unquoted args as *names* (diff/solve/ode_rk4/…) |
| AST_BINARY | operator overload scan + hardcode additive/relational consult (**P5/P6** hot sites) |
| comprehension / lambda | closures capture their **defining frame** (see below) |
| assignment/into | session slot write; compound assigns verified (`x %= 4` → §20 of Guide) |

**Closure frame semantics — the honest limitation.** A lambda reading or
returning *out of its defining frame* does **not** capture: the frame's slots
are *not* retained, so an escaping closure reads whatever the global frame
holds (stale/zero). Verified (Guide §9, audit Q-series):
`add_n(10)(5)` → `5` (the returned closure lost `10`), and `outer_fn`'s
inner lambda failed with `error: No active overload for '*' with types
(fn, real)`. Within its defining frame — recursion, ordinary HOFs, lazy
calculus binding — everything works; only *escaping* is unsafe. Keep data
immortality by design: pass closures to callees that call them *now*.

**Per-eval cost structure** (bench anchors): scalar arithmetic ~0.43 ms
dominated by dispatch lookups **P6/P7**; HOF pipelines ~0.58 ms; linsolve
~0.54 ms; the RK4 loop ~2.2 ms/call for 50 RK4 steps with unit-carrying
values in all stages. Audit findings P5–P7 list the re-derivation sites;
P8 quantifies session re-materialization; the recommended order of attack
(priority ladder) is §6 of the audit.

---

## 10. Session subsystem (`Core/session/`)

8-slot state machine with `eng_session_open/close/eval`; the **snapshot**
engine underneath is independent enough to describe alone:

- **Chain write:** each eval appends one MSP1 record (snapshot.c:697 lines).
- **Chain read:** rewind/next cursor walk (`mepverify` uses exactly this to
  dump; the format was shown in §4.1).
- **Restore:** `eng_session_restore(records, count)` replays into a new
  slot and returns the new id (`ok: restored as session 0`).
- **Full-history intact:** replays preserve seq/prev-hash linkage, so the
  restored session passes its own audit (the driver verified an MSP1
  chain of *two* links round-tripping through save → close → restore →
  `w` read-back).

The design intent is legibility: because state is text-serialized inside
each record, a session's complete past is greppable — that is what the
hash-chained `vars:…` segments are for — and because slots are
`text-as-truth`, *any* rule (module toggles, type locks) is re-checked on
the way back up.

---

## 11. Public C API (`Core/Headers/math_engine.h`)

Integrators touch exactly these (all exercised by `Test/main.c` today):

| phase | calls |
|---|---|
| boot/shutdown | `eng_init(&eng)` · `eng_init_blank(&eng)` (verified: rejects `1 + 2` before library install, `[1]` block) |
| free-form eval | `eng_eval(eng, text, &Value)` |
| sessionized eval | `eng_session_open/eval/close` |
| errors | `eng_last_error(eng)` → exact message string |
| output | `eng_format_value(buf)` (CLI form adds `[unit]`) · `eng_val_unit` · `eng_unit_name` |
| introspection | `eng_module_count eng_overload_count eng_const_count` (banner: 19/1380/30) · `eng_set_module(name, state)` |
| memory | `eng_scratch_pool_used` (via `pool_used()` expr) |
| extension compile/link | `user_compile_ext` · `user_load_mathc` |
| snapshots | `eng_session_snapshot_rewind/next/restore` |

`eng_init_blank` + `register_all_host_externs` is the pattern the test
suite uses to prove the *blank-slate guarantee*: before the `.mathc` image
is installed, even `1 + 2` must fail (verified, `[1]` block of main.c) —
the engine has no cuddled-in builtin library beyond what the image brings.

---

## 12. Language-surface facts an integrator must know

Cross-linked to the verifiable examples in the Language Guide:

- **Truthiness** is 1/0, never `true/false` literals (§21 of Guide, `q17`).
- **`==` has a tiny tolerance guard** (0.1 + 0.2 equals 0.3 → 1; a 1e-5
  delta does not) while `~` is the documented loose matcher (≤ 0.001 slice).
- **Comparisons chain left-associatively** (`1 < 4 == 4` → 0).
- **`^` is right-associative**; unary minus binds below it (`-3 ^ 2` → -9).
- **Units are metadata**: bare display, `unit_of`/`unit_dim`/`same_dim` read
  them; group suffix tables are data (`[units]`/`[conversions]`), wrong
  tokens fail as *unknown symbols* (`s`, not `sec`, in `3km + 2s`).
- **Bare `for` is a parse error** outside comprehensions
  (`error: Unexpected 'for' outside of '[expr for x in ...]' comprehension`).
- All arch-diagnostic expressions (`type_of/unit_of/pool_used/gc_compact/…`)
  evaluate in the same engine, no privileged channel (§19.4 of Guide).

---

## 13. The verification discipline

The repo ships with an unusual but decisive quality bar: **every doc
example is mechanically re-executed**.

1. `Test/main.c` — 555-expression pass/fail suite (all green as of the
   audit runs) plus CLI entry points (`--eval`, `--repl`, `--bench`).
2. The doc harness (`mepverify`, outside the repo) — a manifest runner
   over `@eval/@sess/@ext/@mod/@snap*` feeding `libmathengine.a`
   statically.
3. The doc driver (`verify_examples.js`) — idempotent substitution of
   `expr ⟿ «id»` placeholders in the Markdown; the Language Guide
   contains **71 blocks / 110+ machine-verified records**, all produced by
   the same kernel that ships to integrators. Re-verification is one
   command: `node verify_examples.js docs/MEP_Language_Guide.md`.

The architecture consequence is worth spelling out: **docs can never claim
more than the kernel does** — any drift between guide and engine surfaces
as a substitution failure, not as silent rot.

---

## 14. What comes next (from the audit)

Pointers, not repetition — each is fully specified in
`docs/engine-audit-2026-10-24.md`:

1. **P5/P6** — precompute additive/relational bits and cache per-active
   overload (`meta_tag` 0x20/0x10) → biggest hot-path win.
2. **P1–P3** — symbol/intern hash buckets → parse-time O(n) instead of
   O(n·symbols).
3. **P7** — resolve const symbols at parse time.
4. **P8** — lazy session-slot materialization (the `text-as-truth` tax).
5. **Q2** — move O(n) elementwise kernels behind unchanged `.math`
   signatures (C kernel, `vec.math` keeps the face).
6. **P10** — link-time consistency warnings for glyph-shadowed grammar.
7. Optional migrations (Q1 `@vsort`, some string helpers) — debatable, do
   last, and only if the `.math` loops prove hot in profile.

The end-state worth aiming at: **hardcode shrinks to pure grammar,
delegate keeps only I/O-shaped intrinsics, and every kernel that deserves
C speed hides behind an unchanged `.math` face** — the three-part contract
the repo already obeys 90 % of the way.

---

*End of architecture guide.*
