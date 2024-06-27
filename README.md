# csh - C Shell

Welcome to **csh**, a custom shell program implemented in C! 🎉

## Features ✨

- **Command Execution** 🛠️
  - Supports executing built-in and external commands.
- **Redirection** 🔀
  - Input and output redirection for commands.
- **Alias Management** 🏷️
  - Create, view, and manage command aliases.
- **Modular Design** 🧩
  - Organized codebase for better readability and maintainability.
- **Environment Variable Initialization** 🌐
  - Automatically sets the `SHELL` environment variable to the shell's executable path.
- **Configuration File Support** 📜
  - Reads and executes commands from `~/.cshrc` configuration file on startup.
  - Creates a default `~/.cshrc` if it doesn't exist.
- **Comment Handling** 📝
  - Skips lines starting with `#` in the configuration file.
- **Cross-Platform Compatibility** 🌍
  - Supports macOS (using `_NSGetExecutablePath`) and Linux (using `/proc/self/exe`).

## Next Step

- **Configuration Appearance**
- **Refactor main.c**

## Getting Started 🚀

### Prerequisites

- GCC (GNU Compiler Collection)

### Building the Project

1. Clone the repository:

   ```sh
   git clone https://github.com/yourusername/csh.git
   cd csh
   ```

2. Build the project using `make`:

   ```sh
   make
   ```

3. Run the shell:

   ```sh
   ./csh
   ```

4. Or install the shell:
   ```sh
   sudo make install
   ```

### Cleaning Up

To clean up the generated files:

```sh
make clean
```

## Usage 📝

- **Interactive Mode**:
  Simply run `./csh` to start the shell in interactive mode.

- **Batch Mode**:
  Provide a script file as an argument to execute commands from the file:
  ```sh
  ./csh script.txt
  ```

### Example Commands

- **Running a command**:

  ```sh
  ls -l
  ```

- **Redirecting output**:

  ```sh
  ls -l > output.txt
  ```

- **Creating an alias**:
  ```sh
  alias ll='ls -l'
  ```

## File Structure 📁

- `main.c`: Main entry point of the shell program.
- `commands.c`: Implementation of command execution functions.
- `redirection.c`: Implementation of input/output redirection.
- `alias.c`: Implementation of alias management.
- `utils.c`: Utility functions.
- `fdwrite.c`: Implementation of file descriptor writing functions.
- `commands.h`, `redirection.h`, `alias.h`, `utils.h`, `fdwrite.h`: Header files declaring functions and structures.

## Contributing 🤝

Contributions are welcome! Please fork the repository and submit a pull request.

## License 📄

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements 🙏

Thanks to all the contributors and open-source projects that made this possible.

---

Happy coding! 💻
