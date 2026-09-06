#include "SampleTool.h"
#include <iostream>

int main(int argc, char** argv) {
    // optional CLI args: outdir first, then comma-separated groups_bkg (optional)
    std::string outdir = "latex_tables";
    std::vector<std::string> groups_bkg = {"top","boson","diboson","triboson"};

    if (argc > 1) outdir = argv[1];
    if (argc > 2) {
        groups_bkg.clear();
        std::string g = argv[2];
        size_t start = 0, pos;
        while ((pos = g.find(',', start)) != std::string::npos) {
            groups_bkg.push_back(g.substr(start, pos-start));
            start = pos + 1;
        }
        groups_bkg.push_back(g.substr(start));
    }

    SampleTool st;
    st.LoadBkgs(groups_bkg);

    st.WriteLatexTablesForGroups(groups_bkg, outdir);
    st.LoadAllData();
    st.WriteLatexTablesForGroups({"data_obs"}, outdir);
    std::cout << "Done. LaTeX tables written to: " << outdir << "\n";
    return 0;
}

