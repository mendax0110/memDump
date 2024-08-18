# memDumper

`memDumper` is a utility designed to dump the memory from a executable. This can be useful for security analysis or debugging purposes.

## Features

- **Process Memory Dumping**: Dump the memory of a specified process by PID.
- **Custom Output**: Save dumps to a file or output them to the console.
- **Keyword Search**: Search for specific words within the memory dump.
- **Graphical User Interface (GUI)**: Start the application with a GUI for easier interaction.


## Usage

To use `memDumper`, follow these steps:

## Build Instructions
1. Clone the repository
```bash
git clone https://github.com/mendax0110/memDumper.git
```

2. Change directory to the cloned repository
```bash
cd memDumper
```

3. Init and update the submodules
```bash
git submodule update --init --recursive
```

4. Checkout the docking branch of dearImgui
```bash
git checkout docking
```

5. Create the build directory
```bash
mkdir build
```

6. Change directory to the build directory
```bash
cd build
```

7. Build CMake files
```bash
cmake ..
```

8. Build the project
```bash
cmake --build .
```

## Usage CLI
```bash
memDump.exe -p <PID> [-o <output_file>] [-w <word_to_search_1> -w <word_to_search_2>]
```

## Usage GUI
```bash
memDump.exe --gui
```

## Supported Platforms
- Windows