# SharedPtr / WeakPtr

Header-only implementation of shared_ptr and weak_ptr with reference counting.

## Features
- Shared ownership with reference counting
- Weak pointers for cyclic dependencies
- Custom deleters
- Unbounded array support (`T[]`)
- Polymorphic conversions
- Strong exception safety (copy-and-swap)

## Build & Run

```bash
clone repo
run ./buildAndRun.sh