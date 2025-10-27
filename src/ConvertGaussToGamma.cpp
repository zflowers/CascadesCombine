// replace_gmN_inplace.C
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.txt>\n";
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << "\n";
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;

    // Prepare regex:
    // Matches "__gmN__" followed immediately by a number (integer or decimal),
    // then whitespace, then "lnN". Example: "__gmN__4 lnN" or "__gmN__1.10 lnN"
    std::regex gmN_number_lnN(R"(__gmN__([0-9]+(?:\.[0-9]+)?)\s+lnN)");

    while (std::getline(infile, line)) {
        if (line.find("__gmN__") != std::string::npos) {
            // First: handle the special pattern "__gmN__<num> lnN" -> "gmN <num>"
            // Use regex_replace to handle all occurrences in the line.
            line = std::regex_replace(line, gmN_number_lnN, " gmN $1");
        }

        lines.push_back(line);
    }

    infile.close();

    // Rewrite file with modified content (in-place)
    std::ofstream outfile(filename, std::ios::trunc);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot write to file " << filename << "\n";
        return 1;
    }

    for (const auto& l : lines)
        outfile << l << '\n';

    outfile.close();
    return 0;
}

