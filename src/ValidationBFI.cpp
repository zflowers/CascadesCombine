#include "BuildFitInput.h"
#include <iostream>
#include <TLorentzVector.h>

bool BuildFitInput::ValidateUserCut(const ROOT::RDF::RNode& node,
                                           const CutDef &cut,
                                           unsigned nCheck,
                                           unsigned maxCheck) {
    try {
        ROOT::RDF::RNode tmpNode = node;

        auto colNames = tmpNode.GetColumnNames();
        for (const auto &col : cut.columns) {
            if (std::find(colNames.begin(), colNames.end(), col) == colNames.end()) {
                tmpNode = tmpNode.Define(col, [](){ return 0.0; });
            }
        }

        DerivedVar dv;
        dv.name = cut.name + "_test";
        dv.expr = cut.expression;

        return ValidateDerivedVar(tmpNode, dv, nCheck, maxCheck);

    } catch (const std::exception &e) {
        std::cerr << "[BuildFitInput] Exception validating user cut '" << cut.name
                  << "': " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[BuildFitInput] Unknown exception validating user cut '" << cut.name << "'\n";
        return false;
    }
}

std::map<std::string, CutDef>
BuildFitInput::ValidateCuts(const ROOT::RDF::RNode& node,
                            const std::map<std::string, CutDef>& cuts,
                            unsigned nCheck,
                            unsigned maxCheck) {
    std::map<std::string, CutDef> valid;

    for (const auto& kv : cuts) {
        const auto& cut = kv.second;
        if (BuildFitInput::ValidateUserCut(node, cut, nCheck, maxCheck)) {
            valid[kv.first] = cut;
        } else {
            std::cerr << "[BuildFitInput] Cut '" << cut.name
                      << "' is invalid and will not be available.\n";
        }
    }

    return valid;
}

