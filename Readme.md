# C++alculator

A C++ desktop calculator featuring **Basic Arithmetic** and **Numerical Calculus**, powered by Dear ImGui, GLFW, and OpenGL.

C++alculator brings two calculator modes together in a single Windows application with a dark interface, interactive controls, and dedicated result and error panels.

## Features

### Basic Calculator

Seven arithmetic operations:

- Addition (`+`)
- Subtraction (`-`)
- Multiplication (`*`)
- Division (`/`)
- Power (`a^b`)
- Modulo (`%`)
- Square root (`sqrt`)

### Calculus Calculator

Eight numerical calculus operations:

- First derivative
- Second derivative
- Definite integration using the trapezoidal rule
- Definite integration using Simpson's rule
- Numerical limit approximation
- Taylor series approximation
- Newton's method for root finding
- Arc length calculation

### Predefined Functions

Select from 12 built-in functions:

| Function | Expression |
|---|---|
| 1 | \(x^2\) |
| 2 | \(\sin(x)\) |
| 3 | \(e^x\) |
| 4 | \(\ln(x)\) |
| 5 | \(1/x\) |
| 6 | \(x^3 + 2x^2 + 3x + 4\) |
| 7 | \(\cos(x)\) |
| 8 | \(\tan(x)\) |
| 9 | \(\sqrt{x}\) |
| 10 | Sigmoid function |
| 11 | \(x e^x\) |
| 12 | \(\sin(x)\cos(x)\) |

Trigonometric functions use radians.

### Interface

- Dark desktop UI built with Dear ImGui
- Separate Basic and Calculus workspaces
- Interactive numeric input fields and function selectors
- Operation-specific input controls
- Result display and error feedback
- Native Windows executable

## Download and Run

You can use the prebuilt application without installing a compiler.

1. Open this repository on GitHub.
2. Select **Code → Download ZIP**.
3. Extract the downloaded archive.
4. Open the extracted folder.
5. Double-click `MathLab.exe`.

Keep the executable and its required DLL files together in the same directory.

The distributed application should include these runtime dependencies:

- `glfw3.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

If Windows reports a missing DLL, verify that the required runtime files are present beside the executable.

## Built With

- **C++17** — Application logic and numerical algorithms
- **Dear ImGui** — Immediate-mode graphical user interface
- **GLFW** — Window creation and input handling
- **OpenGL3** — Interface rendering
- **MSYS2 MINGW64** — Windows development environment
- **g++** — C++ compiler

## Build From Source

### Requirements

- Windows
- [MSYS2](https://www.msys2.org/)
- MSYS2 **MINGW64** terminal
- `g++`
- GLFW
- `pkg-config` compatible tooling
- Dear ImGui source files

### 1. Install the required libraries

Open the MSYS2 MINGW64 terminal and run:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-pkgconf
```

If MSYS2 requests confirmation, enter `Y` and press Enter.

### 2. Obtain Dear ImGui

If the repository already contains the `imgui-1.92.9` directory, you can skip this step.

Otherwise, download and extract the source:

```bash
curl -L --fail -o imgui.tar.gz https://github.com/ocornut/imgui/archive/refs/tags/v1.92.9.tar.gz
tar -xzf imgui.tar.gz
```

### 3. Compile the application

Open the MINGW64 terminal in the project directory containing `main.cpp` and `imgui-1.92.9`.

Compile with:

```bash
g++ -std=c++17 -O2 -mwindows -Iimgui-1.92.9 -Iimgui-1.92.9/backends main.cpp imgui-1.92.9/imgui.cpp imgui-1.92.9/imgui_draw.cpp imgui-1.92.9/imgui_tables.cpp imgui-1.92.9/imgui_widgets.cpp imgui-1.92.9/backends/imgui_impl_glfw.cpp imgui-1.92.9/backends/imgui_impl_opengl3.cpp -o MathLab.exe $(pkg-config --cflags --libs glfw3) -lopengl32 -lgdi32
```

The command compiles the application, Dear ImGui core, and the GLFW/OpenGL3 backends into `MathLab.exe`.

The `-mwindows` option requests a Windows GUI-subsystem executable, avoiding a separate console window when launched normally. For debugging, remove `-mwindows` to use a console-subsystem build.

### 4. Copy runtime dependencies

If you want to run the executable directly from File Explorer, copy the required runtime DLLs into the project directory:

```bash
cp /mingw64/bin/glfw3.dll .
cp /mingw64/bin/libgcc_s_seh-1.dll .
cp /mingw64/bin/libstdc++-6.dll .
cp /mingw64/bin/libwinpthread-1.dll .
```

These files come from the MINGW64 installation. Do not download replacement DLLs from untrusted websites.

### 5. Run the application

From the MINGW64 terminal:

```bash
./MathLab.exe
```

Alternatively, open the project directory in File Explorer and double-click `MathLab.exe`.

## Project Structure

```text
C++alculator/
├── .gitignore
├── main.cpp
├── MathLab.exe
├── glfw3.dll
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
├── libwinpthread-1.dll
└── imgui-1.92.9/
    ├── imgui.cpp
    ├── imgui_draw.cpp
    ├── imgui_tables.cpp
    ├── imgui_widgets.cpp
    └── backends/
        ├── imgui_impl_glfw.cpp
        └── imgui_impl_opengl3.cpp
```

The tree above shows the main project files; Dear ImGui contains additional source and header files.

## Notes on Numerical Results

The calculus tools use numerical approximation methods rather than symbolic algebra.

- Derivatives use finite differences.
- Definite integrals use numerical quadrature.
- Limits are estimated by checking nearby values from both sides.
- Taylor approximations depend on the selected degree and center.
- Newton's method depends on the initial guess and may not converge for every function.
- Some functions are undefined for particular inputs, such as \(\ln(x)\) for \(x \leq 0\) or \(1/x\) at \(x = 0\).

Results can be affected by floating-point precision, function domains, and numerical instability.

## License

Dear ImGui is distributed under the MIT license. Refer to the license included with its source distribution for its terms.

The license for C++alculator itself should be specified separately by the project author if the project is to be distributed under a particular open-source license.