#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <argparse/argparse.hpp>

std::string getDLLName(const std::string& path)
{
    size_t dllPos = path.find(".dll");

    std::string aux = path.substr(0, dllPos);

    if (dllPos == std::string::npos) {
        throw std::invalid_argument("Provide a valid dll");
    }
    
    size_t lastSlash = path.rfind("/");

    if (lastSlash == std::string::npos) {
        lastSlash = path.rfind("\\");
        if (lastSlash == std::string::npos)
            lastSlash = 0;
        else
            ++lastSlash;
    }
    else
    {
        ++lastSlash;
    }

    return path.substr(lastSlash, dllPos - lastSlash);
}

std::vector<std::string> parseExports(const std::string& output) {
    std::vector<std::string> exports;
    std::istringstream stream{ output };
    std::string line;
    bool inExportsSection = false;

    while (std::getline(stream, line)) {
        // Find line common to all export blocks
        if (line.find("ordinal hint RVA") != std::string::npos) {
            inExportsSection = true;
            continue;
        }

        // Detect section finish
        if (inExportsSection && line.empty()) {
            break;
        }

        // Process lines in exports function
        if (inExportsSection) {
            std::istringstream lineStream(line);
            std::string ordinal, hint, rva, name;
            lineStream >> ordinal >> hint >> rva; // Read first 3 names
            std::getline(lineStream, name);       // Read the remaining as name

            size_t parenthesisPos = name.find(':');
            if (parenthesisPos != std::string::npos) {
                name = name.substr(parenthesisPos + 1);
                size_t firstNonSpace = name.find_first_not_of(" ");
                if (firstNonSpace != std::string::npos) {
                    name = name.substr(firstNonSpace);      // Everything after first ':'
                }
                    
                size_t lastParenthesis = name.rfind(')');
                if (lastParenthesis != std::string::npos) {
                    name = name.substr(0, lastParenthesis); // Everything before the last ')'
                    exports.push_back(name);
                }
            }

        }
    }

    if (exports.size() == 0)
    {
        throw std::length_error("No exports found");
    }

    return exports;
}

void printExportsToConsole(const std::vector<std::string>& exports)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 2);
    std::cout << "Found exports:" << std::endl;
    SetConsoleTextAttribute(hConsole, 13);
    for (const auto& exp : exports) {
        std::cout << " -> " << exp << std::endl;
    }
    SetConsoleTextAttribute(hConsole, 7);
}

void printExportsToMD(const std::vector<std::string>& exports, const std::string& name, const std::string& path = "")
{
    std::string final_path{ path };
    if (path.empty())
    {
        std::filesystem::create_directory("./out");
        final_path = "out/" + name + "_exports.md";
    }

    std::ofstream exports_file(final_path);

    if (!exports_file.is_open())
    {
        throw std::ofstream::failure("Failed to open exports markdown file...");
    }

    exports_file << "# " + name + ".dll export table" << std::endl;
    exports_file << std::endl;
    exports_file << "| Symbol | Description |" << std::endl;
    exports_file << "| ---    | ---         |" << std::endl;

    for (const auto& exp : exports) {
        exports_file << "| " << exp << " | {placeholder} |" << std::endl;
    }
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("DLLDoc");
    program.add_argument("dll-path")
        .help("path to the dll to document");
    program.add_argument("-o", "--output")
        .help("specify the output file");
    program.add_argument("-c", "--console")
        .flag()
        .help("output DLL exports to the console");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    auto dllPath = program.get<std::string>("dll-path");
    auto outPath = program.present("-o") ? program.get<std::string>("-o") : "";
    auto console = program["-c"] == true;
    std::string dllName = getDLLName(dllPath);

    const char* dumpbinCommand = "dumpbin";

    std::string command = std::string("\"") + dumpbinCommand + "\" /EXPORTS \"" + dllPath + "\"";

    // Pipelines for output redirection
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        std::cerr << "Failed to create pipe. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = nullptr;

    if (!CreateProcessA(
        nullptr,
        &command[0],
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        std::cerr << "Could not execute dumpbin. Make sure to have Visual Studio and the dumpbin.exe directory in the PATH. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    CloseHandle(hWrite);

    // Write out process output
    std::string output;
    char buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    // Wait for process to end
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::vector<std::string> exports = parseExports(output);
    if (console)
        printExportsToConsole(exports);
    else
        printExportsToMD(exports, dllName, outPath);
    
    return 0;
}
