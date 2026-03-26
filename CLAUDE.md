# C++ Development Guidelines for agclib

This project targets Windows, builds as a static library, and uses C++17. All code
must conform to the rules below, drawn from the C++ Core Guidelines (Stroustrup & Sutter)
and modern C++ best practices.

---

## Resource Management

- **Use RAII for every resource.** Tie acquisition to construction and release to
  destruction. COM interfaces, file handles, mutexes, and memory are all resources.
- **Prefer `std::unique_ptr` over raw `new`/`delete`.** Use `std::make_unique`.
  Use `std::shared_ptr` only when shared ownership is genuinely needed.
- **Never perform more than one explicit resource allocation per statement.**
  `f(unique_ptr<A>(new A), unique_ptr<B>(new B))` can leak; write them on separate lines.
- **Never transfer ownership via raw pointers** (I.11). Use smart pointers or
  move semantics at ownership boundaries.
- **COM objects must be wrapped in RAII.** Use a template guard or `CComPtr`/`wil::com_ptr`
  rather than manual `Release()` calls — any thrown exception will skip manual cleanup.

```cpp
// Bad
IFoo* p = nullptr;
comcheck(CreateFoo(&p));
comcheck(p->DoWork());  // throws -> p leaked
p->Release();

// Good
auto p = wil::CoCreateInstance<IFoo>(...);
p->DoWork();  // destructor always releases
```

---

## Error Handling

- **Use exceptions for task failures** (I.10). Signal errors by throwing, not by
  returning error codes that callers silently ignore.
- **Destructors must never throw.** Catch and swallow (or log) any exception inside
  a destructor. Mark them `noexcept`.
- **`noexcept` belongs on**: destructors, move constructors, move assignment operators,
  swap functions, and any other function with a no-throw guarantee.
- **`assert` is for invariants, not error handling.** Preconditions on internal code
  paths use `assert`; preconditions on public API boundaries throw or return an error.
- **Catch by `const` reference**: `catch (const std::exception& e)`.
- **Wrap `main` (or top-level entry) in a catch-all** to give a human-readable message
  instead of `std::terminate`.

---

## Type Safety & Const Correctness

- **Mark every method and parameter `const` that does not mutate state.**
  `const` is documentation the compiler enforces.
- **Prefer `const` local variables and `const` member functions** by default; remove
  `const` only when mutation is required.
- **Use strong types** over generic `int`/`float` parameters where values have units
  or constrained ranges (e.g., a `Volume` type that enforces [0, 1]).
- **Avoid C-style casts.** Use `static_cast`, `reinterpret_cast`, or
  `std::bit_cast` (C++20) with intent made explicit.
- **Avoid `void*`.** COM APIs require it; isolate those casts at the call site.

---

## Interfaces & Functions

- **State preconditions explicitly** (I.5/I.6). Document or assert them.
  `assert(volume >= 0.0f && volume <= 1.0f)` is fine for internal code; a
  `std::invalid_argument` throw is better for public API.
- **Keep argument counts low** (I.23). Prefer <4 parameters; bundle related arguments
  into a struct.
- **Avoid adjacent same-type parameters** (I.24). `setVolume(float min, float max)` is
  a silent bug waiting to happen; use a named struct.
- **Return values rather than output parameters** where possible.
- **Non-owning access: pass raw pointers or references** — not smart pointers.
  `void process(const Foo& foo)` not `void process(shared_ptr<Foo> foo)`.

---

## Classes

- **Follow the Rule of Zero**: if a class manages no raw resource itself, define none
  of destructor / copy / move. Let the compiler generate them correctly.
- **Follow the Rule of Five**: if a class manages a raw resource, explicitly define or
  `= delete` all five: destructor, copy constructor, copy assignment, move constructor,
  move assignment.
- **Virtual destructors are required on any base class** whose derived objects may be
  deleted through a pointer to base.
- **Avoid object slicing.** Do not pass polymorphic objects by value; use references
  or (owning) pointers.

---

## Naming Conventions

| Construct | Convention | Example |
|---|---|---|
| Types / Classes | `PascalCase` | `MicrophoneController` |
| Functions / Methods | `camelCase` | `evaluateMicrophoneTargetVolume` |
| Local variables | `camelCase` | `peakAmplitude` |
| Private data members | `m_camelCase` | `m_audioEndpointVolume` |
| Constants / `constexpr` | `UPPER_SNAKE_CASE` | `SAMPLING_RATE` |
| Template parameters | `PascalCase` | `T`, `InputIterator` |
| Namespaces | `lowercase` | `agc`, `pcm` |
| Macros | `UPPER_SNAKE_CASE` | avoid entirely; prefer `constexpr` |

- **No Hungarian notation.** The type system makes it redundant.
- **No global namespace pollution.** All library code lives inside the `agc` namespace.
- **Names should be self-documenting.** Prefer `favorNewFactor` over `f`.

---

## Headers & Includes

- **Every header is self-contained and uses `#pragma once`.**
- **Include what you use.** Don't rely on transitive includes.
- **Include order** (within each group, alphabetical):
  1. Corresponding `.h` for this `.cpp`
  2. Other project headers
  3. Windows / platform headers (`<Windows.h>`, `<mmdeviceapi.h>`, …)
  4. Standard library headers (`<vector>`, `<functional>`, …)
- **Minimize header includes.** Forward-declare where possible to reduce coupling
  and build times.

---

## Performance

- **Don't pay for what you don't use** (zero-overhead principle).
- **Prefer `std::array` over C arrays**, `std::span` over pointer+length pairs.
- **Pass large objects by `const` reference**; small trivially-copyable objects by value.
- **Reserve containers** when the final size is known.
- **Avoid premature optimization.** Profile before rewriting — but don't write
  obviously wasteful code (e.g., calling the same pure function twice with the same
  input in the same loop iteration).

---

## Tooling

- **Enable all warnings and treat them as errors**: `/W4 /WX` (MSVC).
- **Use a `.clang-format` file** to enforce formatting mechanically — no debates.
- **Static analysis**: run the MSVC analyzer (`/analyze`) or clang-tidy regularly.
- **Address Sanitizer (ASan)** should be enabled in debug builds.

---

## Windows / COM-Specific

- **Call `CoInitialize`/`CoUninitialize` via an RAII guard** (the existing `Com` class).
  Never assume COM is initialized; never leak the initialization.
- **Use `__uuidof`** instead of hardcoded GUIDs.
- **Check every `HRESULT`** — wrap with `comcheck()` or equivalent. Unchecked
  HRESULTs are the Windows equivalent of ignoring return codes.
- **Prefer the Windows Implementation Libraries (WIL)** (`wil::com_ptr`,
  `wil::unique_handle`, etc.) for idiomatic RAII over manual COM patterns.

---

## Sources

- [C++ Core Guidelines — Stroustrup & Sutter](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Best Practices — Jason Turner](https://lefticus.gitbooks.io/cpp-best-practices/content/03-Style.html)
- [ModernesCpp — Naming and Layout Rules](https://www.modernescpp.com/index.php/c-core-guidelines-naming-and-layout-rules/)
