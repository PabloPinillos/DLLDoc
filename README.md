# DLLDoc: dump your C++ DLL's exported symbols and document them

DLLDoc is a Windows command-line tool that uses Dumpbin, a utility included in Visual Studio (required to run this program), to get a DLL's exported symbols.
 
Note: C DLLs do not disclose function or argument types and, therefore, are not supported.

## Motivation

Many C++ libraries in the enterprise world are compiled as dynamic libraries and have no documentation whatsoever. Knowing what functionality they provide is challenging and oftentimes confined to those senior engineers that coded them or have been using them for a long time.
Inspired by the Raylib cheatsheet format (https://www.raylib.com/cheatsheet/cheatsheet.html) I created DLLDoc to help as a first step towards documentation... good luck!

## Installation

Compile DLLDoc yourself downloading the project. It is a single cpp file with a solution (since you are forced to have Visual Studio anyway).

## Usage

DLLDoc can be used to get a console dump of a DLL's exported symbols with the `-c` or `--console` flag:
`DLLDoc.exe path-to-dll\example.dll -c`

![image](https://github.com/user-attachments/assets/fb009874-3a0b-4425-90d1-acef0b44b1f8)

Aditionally, you can get a Markdown table with the symbols, by default located at `.\out\DLLName.md`. You can specify a different output file with `-o` or `--output`:
`DLLDoc.exe path-to-dll\example.dll -o my-path\output.md`

![image](https://github.com/user-attachments/assets/83af1737-657d-485a-a990-e5c9ea6a2756)


## Dependencies

You need Windows with a Visual Studio installation and in your PATH you should have the directory with `dumpin.exe`, typically `C:\Program Files\Microsoft Visual Studio\[version]\Community\VC\Tools\MSVC\[version]\bin\Host[arch]\[arch]\`. 

DLLDoc includes the single-header standalone Argparse to parse arguments: https://github.com/p-ranav/argparse
