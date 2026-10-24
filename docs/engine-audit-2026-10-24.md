# Engine Audit — `hardcode/` & `delegate/`, Performance & Memory

**Date:** 2026-10-24
**Scope:** `Core/lib/hardcode/hardcode.{h,c}`, `Core/lib/delegate/delegate.{h,c}`, with
cross-checks into `Core/Code/{lexer,evaluator,parser,mem_arena,sym_table}.c`,
`Core/session/session.c`, and the 15 `Library/*.math` modules.
**Commit audited:** `d403ab7` (“Update engine core, stdlib modules, JNI bridge, tests and docs”).

---

## 0. Ground Rules

The engine's founding contract (README, “Design Goals”) is:

1. **Zero hardcoded math, types, units, or operators.** Everything except
   *grammar* is declared in text (`.math`) and linked at startup.
2. `hardcode.c` may only quarantine **grammar & ergonomic rules**
   (statement separators, reserved words, implicit multiplication, `in`
   disambiguation, typed containers `vec<T>()/list<T>()/…`, comprehensions,
   lambdas, pattern-matched `switch`).
3. `delegate.c` holds the C `@<intrinsic>` registry — **tier-0 structural**
   (`@seq @store @if @lambda @pipe …`) and **tier-1 primitive** math kernels
   (`@add @sin @mmul @mlinsolve …`) that MEP implements *by delegation*, not by
   hardcoding.

Every recommendation below is checked against this contract: **nothing may move
semantics into C that the data-driven design says belongs in `.math`.**

Host measurements quoted here (from `DebugBuild/myapp --bench` on the audit
host): boot 604 µs, scalar eval 384 µs, HOF map/filter/sum 698 µs, 3×3
`linsolve` 642 µs, 50-step RK4 2 652 µs. Absolute numbers are slow on this
device; the *relative* costs are what matter.

---

## 1. Executive Summary

| # | Finding | Where | Impact | Verdict |
|---|---------|-------|--------|---------|
| P1 | Lexer scans **all symbols per token** (linear active-syntax/op walk) | `lexer.c:329-330,434` | Parse-time O(tokens × symbols) | **High** — cache buckets |
| P2 | `mem_intern_str` linear substring scan of the whole `src_buf` per intern | `mem_arena.c:95-104` | Compile-time O(symbols × buf) | **High** — suffix check / hash |
| P3 | `sym_find` linear scan of symbol table | `sym_table.h:402` | Every string→symbol lookup | **Med-High** — first-char buckets |
| P4 | `delegate_lookup_intrinsic` linear + `strlen` per candidate | `delegate.c:172-184` | Per `@`-token compile cost | **Med** — length bucket |
| P5 | `eval_dispatch_overload` calls `hardcode_is_additive_or_rel_sym` **per dispatch** (chain walk + AST body inspection) | `evaluator.c:244`, `hardcode.c:677` | Hot-path eval cost | **High** — move to compile-time bits in `OpOverload.meta_tag` |
| P6 | `AST_BINARY` calls `sym_active_meta_tag` per evaluation (5 sites) | `evaluator.c:47,62,946,1183,1314` | Hot-path eval cost | **High** — cache per active overload |
| P7 | `AST_CONST_SYM` linear const scan per evaluation | `evaluator.c:898-907` | O(consts) per const ref | **Med** — resolve at parse |
| P8 | `eng_session_eval` re-parses & re-evaluates **every** persistent var's canonical text each eval (`scratch rewind → text-as-truth`) | `session.c:52-175` | Session eval = O(vars) extra | **Med** — lazily materialize only referenced slots |
| P9 | Map get/has/del is a flat linear scan, no hashing | `delegate.c:~1000` | O(n) per key lookup | **Med** — FNV bucket index in header slot |
| P10 | Hardcode raw-glyph assumptions (`'=' '>'`, `":="`, `"<"`/`">"` in `vec<T>`) | `hardcode.c:470,640-648` | Brittle vs data-driven grammar | **Design debt** — keep, but mark |
| M1 | `meta_tag` bits 0x20/0x10 unused; 0x80/0x40 already lazy | `sym_table.h` overhead struct | free capacity for P5/P6 fix | **Opportunity** |
| Q1 | `@vsort` is an algorithm in C; sorting is policy, not arithmetic | `delegate.c` | borders on non-primitive | **Migratable** |
| Q2 | Vec/mat `+ - .* ./ .^` Hadamard/etc. are pure-MEP elementwise comprehension loops | `vec.math`, `mat.math` | O(n) per-element dispatch — the *slowest* kind of loop in the engine | **Add C kernel, keep `.math` signature** |

**Overall:** the separation architecture is healthy and followed. All ten
performance issues are *implement-level* (linear scans and repeated
re-derivation), none require architecture changes. The migration story is
small and honest: ~2 delegate items are debatable (Q1, plus a few string
helpers), while the opposite migration (putting O(n) kernels *behind*
unchanged MEP signatures) is the bigger win.

---

## 2. `hardcode/` Audit — What Belongs Where

`hardcode.c` today contains (per its own banners, §16 of README):

| Cluster | Examples | Contract status |
|---------|----------|-----------------|
| Statement/grammar tokens | newline `;` separators, `:=` vs `=` shapes | ✅ grammar — belongs |
| Reserved words | `if or then else default switch case jump run break continue return` | ✅ scanner-level — belongs |
| Implicit multiplication & postfix units | `50m`, `50mi` maximal munch, `(a)(b)` | ✅ ergonomic — belongs |
| `in` disambiguation | unit conversion vs membership `25m in km` | ✅ ergonomic — belongs |
| Parametrized containers | `vec<T>()`, `list<T>()`, `stack<T>()`, `queue<T>()` | ✅ grammar — belongs |
| Comprehensions & lambdas | `[f(i) for i in xs]`, `x => x*x` | ✅ grammar — belongs |
| Pattern-matched `switch` | `switch … case … default` | ✅ grammar — belongs |
| Distributed `&`/`|` relational chaining | `x & y == 1 | 2` | ⚠️ *semantic* flattening — see note |

One debatable item: the **distributed comparison** quarantine
(`hardcode_is_additive_or_rel_sym`, `hardcode_is_rel_comparison_op`,
`hardcode.c:671-720+`). Deciding “is this operator additive/relational” is a
*semantic* question about a `.math`-declared operator, answered by walking the
operator's AST at **eval time** over and over (P5). The answer is invariant
per overload per module-state — it should be a compile-time property encoded
on the overload (see §3.1). The hardcode layer then only interprets the
flags — grammar stays in C, meaning migrates to compile-time metadata derived
from `.math`.

**Raw-glyph assumptions (P10).** `hardcode_is_plain_store_sym`
(`hardcode.c:640-648`) matches the literal bytes `":="` and `"="`, and
`parser` sites (`hardcode.c:470`) pre-resolve the symbol-id of `":="`.
Given rule “only grammar may be hardcoded,” matching the glyph shapes of the
*store* operators is grammatical, but it silently assumes no `.math` extension
redefines `=` shapes. Practical and acceptable — but worth a startup *consistency
check*: if a module declares an operator with the exact glyphs of a
hardcoded-grammar operator, warn at link time. Similarly `vec<T>`-style
container syntax (`"<"`, `">"`) is glyph-matched grammar; a redeclaration of
`<`/`>` as infix with different arity inside the modules that define
containers could collide. A one-time link-time assertion closes this hole
without touching runtime.

**Everything else in `hardcode.c` is architecture-correct.** Migrating
comprehensions/lambda/`switch` grammar into `.math` would make those constructs
non-first-class (they alter grouping/fixity at parse time); keep.

---

## 3. `delegate/` Audit — The C `@` Registry

`k_delegate_intrinsics` (names ⇄ `IntrinsicId`) is the whole C surface. Grouped
by tier:

### Tier 0 — structural (keep in C, non-negotiable)
`@seq @store @if @while @do_while @lambda @pipe @brk @cnt @err`
These define MEP's evaluation *machine*: sequences, stores, control flow,
lambdas, piping, error propagation. They are the language's interpreter
core hooks — unreachable from `.math` in any meaningful way.

### Tier 1 — arithmetic/scientific primitives (keep in C)
`@add @sub @mul @rdiv @pow @neg @eq @ne @lt @le @gt @ge @and @or @not @in @muladd @powadd @lgamma @tgamma @fact @gcd @lcm @absf @sqrt @hypot @qmul @hyp_… @sin @cos @tan @asin @acos @atan @atan2 …`
Scalar kernels. C is the right tier: tight, branch-light, no per-element AST
walk. `.math` (arith/trig/complex) *defines* the user-level overload surface
in terms of these — architecture intact.

### Tier 1 — tensor kernels (keep in C, and *add more*, see Q2)
`@mmul @mdet @minv @mlinsolve @meig_* @msvd @mdim @mreshape @mhadamard @vnew…@vsort @map_new…@map_del`
LU/QR/eig/SVD/ solves: heavy numerics on flat pool arrays, exactly where an
AST-driven MEP loop would be 10-100× slower. Keep.

### Tier 1+ — serialization & strings
`@tostr @fromstr @str_fmt @regex_*`
Parser/format work is legitimately C-territory (they *parse*, which is engine
work). `@regex_*` embeds a regex engine — fine as infrastructure.

### Debatable items
| Item | Argument to migrate | Verdict |
|------|---------------------|---------|
| `@vsort` (sort algorithm) | Sorting is an *algorithm*, not an arithmetic primitive; a pure-MEP insertion/quick-sort would be written once in `vec.math` and remain user-visible/editable | **Migrate optional** — a MEP sort exists in the guide as an exercise either way; keep C for speed if `sort()` stays a stdlib workhorse. Borderline, document it as such. |
| `@str_*` slice-of-string helpers if any remain beyond `@str_fmt` | `string.math` already re-implements most helpers in pure MEP (`is_empty substr str_repeat str_reverse starts_with ends_with chars str_join`) | Already done — converged correctly |
| `@meig/inverse-family` fallbacks | `mat.math` carries pure-MEP LU det/inv/linsolve reference paths | Correct as-is: C kernels for speed, MEP implementation as the canonical *specification*, cross-validated in tests |

### The migration that actually matters (Q2)
Elementwise vector/matrix arithmetic (`+ - .* ./ .^` and Hadamard-style ops on
`vec`/`mat`/`vec2/3/4`) is currently implemented **in pure MEP as
elementwise-comprehension pipelines** in `vec.math` / `mat.math`. For an
n-element vector this costs n full overload dispatches + n AST walks, when a
single C kernel looping over the flat pool array would do. Recommendation:

* Add scalar-loop intrinsics `@vadd @vsub @vmul @vdiv @vpow @vhad` (vec+vec
  and vec+scalar) and `@madd @msub @mhadamard`-style envelope reuse in matrix
  space, **behind the exact same `.math` operator declarations** — the
  signatures, docs, and semantics in `vec.math`/`mat.math` do not change one
  byte. Users keep editing operators in `.math`; only the *body* delegates to a
  faster primitive. This is exactly how `@mmul` already serves `mat.math`.
* Guards (dim checks, unit propagation) stay in the MEP layer — C kernel
  assumes pre-validated inputs, matching the `@mmul` contract pattern.

---

## 4. Performance Findings & Fixes

All fixes below keep the architecture. Effort/S-risk: **L**arge/**M**edium/**S**mall.

### P5+P6 — the single biggest win: stop re-deriving operator semantics at eval time
`eval_dispatch_overload` (`evaluator.c:244`) calls
`hardcode_is_additive_or_rel_sym` (chain walk + AST body inspection) on *every
dispatch*; `AST_BINARY` and friends call `sym_active_meta_tag` on *every
evaluation* (`evaluator.c:47,62,946,1183,1314`). The information being
re-derived is **static per active overload**:

**Fix:** at *link time* (and on `eng_set_module` toggles), compute once for
each active overload: `is_additive`, `is_relational`, `is_store`, `is_seq`,
… and pack into free bits of `OpOverload.meta_tag`. Bits `0x80/0x40` are
already used lazily; `0x20/0x10` are free (M1). Then:
* `eval_dispatch_overload` reads the bits — no recursion into hardcode.c.
* `sym_active_meta_tag(n->ref, fixity)` at AST_BINARY sites keeps its call but
  reads a precomputed table (still per-symbol — consider caching the resolved
  overload id + meta in the AST node's spare fields at first eval, guarded by a
  generation counter bumped on module toggles).

*Estimated effect:* the five hottest sites stop doing linked-list walks +
AST peeks; RK4/HOF-style benchmarks should improve visibly (this is the inner
loop of every dispatch).

### P1 — lexer per-token symbol scans
`lex_next` decides token kinds by walking *all* active syntax/operator symbols
(`lexer.c:329-330`), repeatedly via `sym_has_active_op`/`sym_get_active_syntax`
(`lexer.c:346,376,434`).

**Fix:** maintain an **active-symbol first-byte index**: a 256-bucket table
mapping first byte → compact list of (sym_id). Link-time placement; module
toggle updates only that module's symbols. Byte-deterministic, allocation-free
(one flat `uint16_t` slab + counts), preserves maximal munch by keeping
longer-sym-first order within each bucket.
*Estimated effect:* parse time drops roughly with symbol count (~1.4k
overloads live); bench “boot” and test-suite wall time shrink accordingly.

### P2 — `mem_intern_str` linear substring scan
Every intern scans the full 48 KB `src_buf`
(`mem_arena.c:95-104`) — the classic quadratic compile cost, worst for common
short strings (`a`, `x`, `+` are interned thousands of times when compiling the
standard library).

**Fix (small):** intern table = open-addressed hash set of (off,len) keyed by a
rolling hash of the tail bytes, holding ≤ the existing engine constraints
(static table, no heap). A miss still appends but a hit costs O(len) hash, not
O(src_len) memcmp. `.math` compilation (the only phase that interns heavily)
gets near-linear.

### P3 — `sym_find`
Same disease one level up (`sym_table.h:402` implementation is a linear scan
of `syms[]` with slice compare).
**Fix:** per active symbol a small FNV index-table (rebuilt on link/toggle);
`sym_find` becomes hash-lookup with slice-compare confirmation.

### P4 — `delegate_lookup_intrinsic`
Linear list + `strlen` per entry (`delegate.c:172-184`) for what is
compile-time-only work.
**Fix:** static perfect-hash on the 100+ names (generated table — the
`delegate_intrinsic_table_hash` déjà-vu pattern already exists); or at minimum
compare length first (it does: `strlen(name)==len` costs a scan of the name
first — reorder to avoid per-candidate `strlen` by storing lengths in the
table). Cheap, safe, well-bounded.

### P7 — `AST_CONST_SYM`
Linear scan of all consts per constant *evaluation* (`evaluator.c:898-907`).
**Fix:** parse-time resolution (parser knows the active const symbol set);
fall back only for freshly linked modules. Reuses the P3 index.

### P8 — session re-materialization
`eng_session_eval` (session.c:52) re-parses *and re-evaluates* every
persistent var's canonical `val_text` **twice** (`pass 0` functions, `pass 1`
others — ordering required for closures referencing later vars). With N
session vars, each eval pays N parses + N evals before touching the user's
expression. This is the documented *text-as-truth* model (rewind scratch →
rebuild from canonical text), which guarantees snapshot/restore equivalence —
do **not** break that invariant.

**Fix preserving text-as-truth:** lazily materialize only the variables the
*new expression actually references* (parser already computes `slot` usage;
mark reachable slots transitively through lambda bodies). Others remain
canonical text for snapshot purposes but are not re-evaluated this round.
A small `frameneeds[]` bitmap decides.RK4-style sessions with many working
vars drop from O(N) to O(used).

### P9 — map linear scan
`@map_get/@map_has/@map_del` scan all entries (`delegate.c`, map section,
`~:1000`) and string-compare keys.
**Fix:** keep the flat pool layout (important for serialization & `sizeof`
stability) but store a 4-bit/8-bit FNV fingerprint byte + bucket head in the
map header slots that are currently dedicated (HDR slots have free room —
`MEM_MAP_HDR_SLOTS` covers 4 × uint16; a fully packed layout can fit
`len | cap | data_idx | hash_head_index`). Lookup then scans only the bucket
chain. Keys stay canonical texts; `mem_map_entry_key` layout unchanged.
*Constraint:* pool slots are uint16 — the bucket chain must be index-chained,
not pointer-chained — fine (that's the existing `ov_next` pattern).

### Summary table of expected wins (relative, this host)
| Fix | Site | Expected |
|-----|------|----------|
| P5+P6 meta-bits | eval dispatch | biggest single win for all workloads |
| P1 symbol buckets | lexer | big for boot + compile-heavy runs |
| P2 intern hash | arena | big for `.math` compiles (boot) |
| P8 lazy materialization | session | big for session-heavy / RK4-style |
| P3 sym index | lookup | moderate |
| P7 const resolution | eval | moderate |
| P9 map buckets | map ops | moderate for map-heavy |
| P4 intrinsic hash | compile | small |

---

## 5. Memory Findings

| # | Item | Where | Note |
|---|------|-------|------|
| M1 | `OpOverload.meta_tag` has 4 free bits (`0x20/0x10`) | `sym_table.h` | consumes P5 fix |
| M2 | `Engine ≈ 788.6 KB` (incl. 256 KB `.mathc` buf + 8 sessions) — acceptable, targets Android NDK as designed | README §2 | no action |
| M3 | `sizeof(Value)==8`, `sizeof(AstNode)==8` — flat 16-bit indices everywhere; keep | | guard rails for any new table structure |
| M4 | Float pool is uint16-indexed with `MEM_MAX_F64_POOL` slots — new kernels (Q2) must follow the same pool discipline (pack similarly to `@mmul`) | `mem_arena.c` alloc site | S |
| M5 | Session `val_text` snapshots are canonical strings — P8's lazy materialization must not alter produced snapshot text, or session-restore equivalence tests fail | `session.c`, `snapshot.c` | asserted by full test suite |

---

## 6. Recommended Order of Work

1. **P5+P6 meta-tag compile flags** (M1 enables; small diff, largest win).
2. **P2 intern hash** + **P4 named-table lengths** (compile path).
3. **P1 lexer first-byte buckets** (parse path).
4. **P8 lazy session materialization** (session path; test suite guards it).
5. **Q2 tensor kernels** behind unchanged `.math` signatures (runtime path).
6. **P3/P7/P9** indexes (lookup path).
7. **Q1 `@vsort`** — decide with a benchmark after 5 (if a MEP sort within
   2× of C on typical N, migrate; else keep — document the boundary either way).
8. **P10 glyph-consistency link-time checks** (hardening, no behavior change).

Each step is independently testable; the 555-test suite + the doc-example
harness used for the guides (`mepverify`) catch regressions at every layer.
