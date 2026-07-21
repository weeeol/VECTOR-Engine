# VECTOR-Engine AI Coding Guidelines

These rules dictate how AI agents should interact with and write code for the VECTOR-Engine codebase. This file exists to optimize the AI's efficiency, reduce token usage, and enforce clean architecture.

## 1. File Size and Modularity
- **Keep files small**: Avoid letting any `.cpp` or `.hpp` file exceed 500-800 lines of code. 
- **Proactive Refactoring**: If a file you are asked to edit is approaching or exceeds this limit, proactively suggest breaking it down before adding new features.
- **Single Responsibility Principle**: Favor Composition over Inheritance. Break large monolithic classes into smaller, focused components.

## 2. Header and Implementation Separation (C++)
- **Clean Headers**: Keep `.hpp` files strictly for declarations. Avoid inline implementations unless absolutely necessary for performance (e.g., templates or simple getters).
- **Minimal Includes**: Forward declare classes in headers whenever possible instead of using `#include`. Only `#include` what is strictly required to compile the header, reducing the AI's need to traverse include trees.

## 3. Predictable Structure
- **Symmetry**: Ensure every `.cpp` file in `src/` has a corresponding `.hpp` file in `include/` following the exact same directory structure.
- **Naming Conventions**: Use clear, descriptive names. Maintain existing conventions but ensure new components are intuitively named so the AI can predict file paths.

## 4. Intent-Driven Comments
- **Explain Why, Not What**: Write comments that explain the reasoning and business logic behind a decision. This helps future AI agents understand the *intent* of the code, preventing accidental breaks during refactors.

## 5. Token Efficiency
- When writing code or refactoring, prioritize changes that isolate dependencies. The goal is to allow future agents to understand a component by reading only its header and immediate implementation, without needing engine-wide context.
