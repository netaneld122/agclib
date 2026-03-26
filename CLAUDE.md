# Rust Development Guidelines for agclib

This project targets Windows, builds as a Cargo workspace, and uses the 2021 edition.
All code must conform to the rules below, drawn from the Rust API Guidelines, RFC 430,
and community best practices.

---

## Ownership & Borrowing

- **Work with the borrow checker, not against it.** Use the type system to make invalid
  states unrepresentable.
- **Prefer immutability.** Declare `let` by default; add `mut` only when needed.
- **Borrow instead of clone.** `clone()` is a red flag — reach for it only when sharing
  is genuinely required. Every gratuitous clone is a performance and correctness smell.
- **Pass by reference for read-only access.** Use `&T` or `&str`/`&[T]` in function
  signatures; take ownership only when the function must own the value.
- **Never use `Rc<RefCell<T>>` when `&mut T` suffices.** Interior mutability is a
  last resort, not a way to dodge the borrow checker.

---

## Error Handling

- **Return `Result<T, E>` for every fallible operation in library code.** Never panic in
  a library — panics are bugs, not error handling.
- **Use the `?` operator** to propagate errors. Chains of `?` are idiomatic; chains of
  `match` on `Result` are not.
- **Use `thiserror` for library error types.** Define a single `Error` enum per crate;
  wrap lower-level errors with `#[from]`.
- **Use `anyhow` in binary entry points** (`main`, integration tests) where you want
  easy error context without defining custom types.
- **Never call `.unwrap()` or `.expect()` in library code.** In binaries, `.expect()`
  is acceptable only for invariants that can never fail (e.g., known-good literals);
  add a message that explains the invariant.
- **Fail fast, recover gracefully.** Validate at boundaries; propagate specific errors;
  let callers decide how to recover.

---

## Type System & API Design

- **Make illegal states unrepresentable.** A `Volume(f32)` newtype that enforces
  `[0.0, 1.0]` is better than a raw `f32` parameter with a runtime check.
- **Implement standard traits** where meaningful: `Debug`, `Clone`, `PartialEq`,
  `Default`, `Display`. Derive them with `#[derive]` when possible.
- **Implement `Default` for types with a sensible zero value.** Pair it with a `new()`
  constructor for parameterised construction.
- **Prefer `impl Trait` in argument position** over generic bounds when you only need
  one trait: `fn process(items: impl Iterator<Item = i32>)`.
- **Prefer owned types in structs; references in function parameters.** Structs that
  store references require lifetime annotations — accept that cost only when you must.
- **Avoid stringly-typed APIs.** Use enums instead of strings for finite sets of values.

---

## Naming Conventions (RFC 430)

| Item | Convention | Example |
|---|---|---|
| Types & Traits | `UpperCamelCase` | `MicrophoneController` |
| Enum variants | `UpperCamelCase` | `AgcError::InvalidVolume` |
| Functions & methods | `snake_case` | `evaluate_microphone_target_volume` |
| Local variables | `snake_case` | `peak_amplitude` |
| Constants & statics | `SCREAMING_SNAKE_CASE` | `SAMPLING_RATE` |
| Type parameters | Concise `UpperCamelCase` | `T`, `InputIter` |
| Lifetimes | Short lowercase | `'a`, `'src` |
| Modules | `snake_case` | `pcm`, `automatic_gain_control` |
| Macros | `snake_case!` | `vec!` |

- **Acronyms count as one word in `UpperCamelCase`:** `Uuid` not `UUID`, `Stdin` not
  `StdIn`.
- **Constructors use `new` or `with_*`.** Conversion constructors use `from_*`.
- **Iterator-returning methods follow the standard names:** `iter()`, `iter_mut()`,
  `into_iter()` returning `Iter`, `IterMut`, `IntoIter` respectively.
- **No `_rs` / `_rust` suffixes on crate names.** Every crate is Rust.

---

## `unsafe` Code

- **Every `unsafe` block must have a `// SAFETY:` comment** explaining precisely why
  the invariants required for soundness are upheld, and under what conditions the block
  would become unsound.
- **Minimise the scope of `unsafe` blocks.** Isolate them at the lowest possible level
  and expose a safe API above.
- **Treat `unsafe` as a last resort.** Ask: is there a safe alternative? Can the unsafe
  be pushed into a well-maintained crate (e.g., `windows-rs`)?
- **Never write `unsafe` to silence the compiler without understanding why it complains.**

```rust
// Bad
unsafe { some_call() }

// Good
// SAFETY: `ptr` is non-null and points to a valid, aligned `WAVEHDR`
// that we own exclusively for the duration of this call.
unsafe { waveInPrepareHeader(handle, ptr, size_of::<WAVEHDR>() as u32) };
```

---

## Iterators & Functional Style

- **Prefer iterator chains over manual `for` loops** when transforming collections.
  `iter().map().filter().collect()` is clearer than an accumulator loop.
- **Use `.chunks_exact()` instead of manual index arithmetic** when processing
  fixed-size slices (e.g., PCM samples).
- **Avoid `collect()` when you can consume the iterator directly.**
- **Use `fold` for stateful accumulation** rather than a mutable variable outside a loop.

---

## Windows / COM-Specific

- **Use `windows-rs` (`windows` crate) for all Windows API access.** It provides safe
  wrappers, correct COM lifetime management, and generated bindings directly from the
  Windows SDK metadata.
- **COM interface pointers are reference-counted by `windows-rs`.** Types like
  `IAudioEndpointVolume` implement `Drop` via `Release` — do not call `Release` manually.
- **Wrap all `unsafe` Windows API calls in a safe function or RAII type.**
- **RAII-guard COM initialisation.** Wrap `CoInitializeEx`/`CoUninitialize` in a struct
  whose `Drop` calls `CoUninitialize`. Mark it `!Send` with `PhantomData<*mut ()>` —
  COM apartment-threading must stay on the initialising thread.
- **Check every fallible Windows call via `?`.** windows-rs methods return
  `windows::core::Result<T>` — always propagate with `?`.

---

## Tooling

- **`rustfmt`** must be run on every commit. Formatting is not negotiable; no
  bike-shedding.
- **`clippy --all-targets -- -D warnings`** is the CI gate. Treat every warning as an
  error. Use `#[allow(lint)]` sparingly and always with a comment justifying it.
- Consider enabling pedantic clippy lints (`-W clippy::pedantic`) for library crates.
- **`cargo test`** must pass at all times. Untested public functions are a liability.
- **`cargo doc`** must build without warnings. Public items need `///` doc comments.

---

## Documentation

- **Every public item needs a `///` doc comment** explaining *what* it does (not *how*).
- **Include an example** in the crate-level doc and in non-trivial public functions.
- **Document `# Panics`, `# Errors`, and `# Safety` sections** in doc comments where
  applicable.
- **Document every `unsafe fn`** with a `# Safety` section listing the caller's
  invariants.

---

## Sources

- [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/about.html)
- [RFC 430 — Naming Conventions](https://github.com/rust-lang/rfcs/blob/master/text/0430-finalizing-naming-conventions.md)
- [The Rust Programming Language Book](https://doc.rust-lang.org/book/)
- [Rust by Example](https://doc.rust-lang.org/rust-by-example/)
- [Idiomatic Rust — mre](https://github.com/mre/idiomatic-rust)
