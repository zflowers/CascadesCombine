#include "SampleTool.h"
#include <iostream>

int main(int argc, char** argv) {
    // optional CLI args: outdir first, then comma-separated groups (optional)
    std::string outdir = "latex_tables";
    std::vector<std::string> groups = {"top","boson","Vfakeleps"};

    if (argc > 1) outdir = argv[1];
    if (argc > 2) {
        groups.clear();
        std::string g = argv[2];
        size_t start = 0, pos;
        while ((pos = g.find(',', start)) != std::string::npos) {
            groups.push_back(g.substr(start, pos-start));
            start = pos + 1;
        }
        groups.push_back(g.substr(start));
    }

    SampleTool st;
    st.LoadBkgs(groups);

    st.WriteLatexTablesForGroups(groups, outdir);
    std::cout << "Done. LaTeX tables written to: " << outdir << "\n";
    return 0;
}

