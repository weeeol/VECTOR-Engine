#pragma once

namespace VECTOR {

    enum class KeyCode {
        Space = 32,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        // Alphanumeric
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9
    };

    enum class MouseButton {
        Left = 0,
        Right = 1,
        Middle = 2
    };

} // namespace VECTOR
