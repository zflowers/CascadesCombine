#ifndef VALIDATIONTOOLS_H
#define VALIDATIONTOOLS_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "Math/VectorUtil.h"
#include <type_traits>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include "TInterpreter.h"
#pragma once

// ==================================================
// User Defined Var Validation Helpers
// ==================================================

// ----------------------
// Derived variables
// ----------------------
struct DerivedVar {
    std::string name; // variable name to be created in the RDataFrame
    std::string expr; // expression to define it (ROOT/C++ expression)
};

// ----------------------
// CutDef
// ----------------------
struct CutDef {
    std::string name;                   // user-defined name for the cut
    std::vector<std::string> columns;   // columns needed to compute/apply the cut
    std::string expression;             // string to be used in node.Filter(...)
};

// ==================================================
// Type-detection helper: container-like detection
// ==================================================
template <typename, typename = void>
struct has_value_type : std::false_type {};

template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

// ==================================================
// TryValidateType
// ==================================================
template <typename T>
bool TryValidateType(ROOT::RDF::RNode node,
                     const DerivedVar &dv,
                     unsigned nCheck = 50,
                     unsigned maxCheck = 5000) {
    try {
        auto subset = node.Range(0, static_cast<long>(nCheck));
        auto vals = subset.Take<T>(dv.name + "_test").GetValue();

        if constexpr (std::is_floating_point_v<T>) {
            bool anyFinite = std::any_of(vals.begin(), vals.end(), [](auto v){ return std::isfinite(v); });
            if (!anyFinite) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[ValidationTools] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but produced no finite values in first " << maxCheck << " events.\n";
                }
            }
            return true;
        }
        else if constexpr (has_value_type<T>::value) {
            using Inner = typename T::value_type;
            if constexpr (std::is_floating_point_v<Inner>) {
                bool anyFiniteInner = false;
                for (const auto &container : vals) {
                    for (const auto &inner : container) {
                        if (std::isfinite(inner)) { anyFiniteInner = true; break; }
                    }
                    if (anyFiniteInner) break;
                }
                if (!anyFiniteInner) {
                    if (nCheck < maxCheck) {
                        unsigned next = std::min(nCheck * 2u, maxCheck);
                        return TryValidateType<T>(node, dv, next, maxCheck);
                    } else {
                        std::cerr << "[ValidationTools] WARNING: '" << dv.name
                                  << "' evaluated as container of " << typeid(Inner).name()
                                  << " but produced no finite inner values.\n";
                    }
                }
                return true;
            } else {
                if (vals.empty()) {
                    if (nCheck < maxCheck) {
                        unsigned next = std::min(nCheck * 2u, maxCheck);
                        return TryValidateType<T>(node, dv, next, maxCheck);
                    } else {
                        std::cerr << "[ValidationTools] WARNING: '" << dv.name
                                  << "' evaluated as container type " << typeid(T).name()
                                  << " but returned empty sequence.\n";
                    }
                }
                return true;
            }
        }
        else {
            if (vals.empty()) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[ValidationTools] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but returned empty vector.\n";
                }
            }
            return true;
        }
    } catch (...) {
        return false;
    }
}

// ==================================================
// ValidateDerivedVar
// ==================================================
inline bool ValidateDerivedVar(ROOT::RDF::RNode node,
                               const DerivedVar &dv,
                               unsigned nCheck = 50,
                               unsigned maxCheck = 5000) {
    try {
        ROOT::RDF::RNode tmpNode = node.Define(dv.name + "_test", dv.expr);

        if (TryValidateType<double>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<float>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<int>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned int>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<long long>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned long long>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<bool>(tmpNode, dv, nCheck, maxCheck))   return true;

        if (TryValidateType<ROOT::VecOps::RVec<double>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<float>>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<ROOT::VecOps::RVec<int>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned int>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<long long>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned long long>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<bool>>(tmpNode, dv, nCheck, maxCheck))   return true;

        std::cerr << "[ValidationTools] ERROR validating '" << dv.name
                  << "' from expression: " << dv.expr << "\n";
        if (dv.expr.find("/") != std::string::npos &&
            dv.expr.find("SafeDiv") == std::string::npos) {
            std::cerr << "  HINT: use SafeDiv(num, den, def) instead of '/'\n";
        }
        if (dv.expr.find("[") != std::string::npos &&
            dv.expr.find("SafeIndex") == std::string::npos) {
            std::cerr << "  HINT: use SafeIndex(vec, idx, defaultVal) instead of '[]'\n";
        }
        return false;
    } catch (const std::exception &e) {
        std::cerr << "[ValidationTools] WARNING: Exception validating '" << dv.name
                  << "': " << e.what() << "\n";
        return false;
    }
}

// ==================================================
// ValidateUserCut (from ValidateCuts.cpp)
// ==================================================
inline bool ValidateUserCut(ROOT::RDF::RNode node,
                            const CutDef &cut,
                            unsigned nCheck = 50,
                            unsigned maxCheck = 5000) {
    try {
        auto subset = node.Range(0, static_cast<long>(nCheck));
        auto count = subset.Filter(cut.expression, cut.name).Count().GetValue();

        if (count == 0 && nCheck < maxCheck) {
            unsigned next = std::min(nCheck * 2u, maxCheck);
            return ValidateUserCut(node, cut, next, maxCheck);
        }
        return true;
    } catch (const std::exception &e) {
        std::cerr << "[ValidationTools] ERROR: Cut '" << cut.name
                  << "' with expression '" << cut.expression
                  << "' is invalid: " << e.what() << "\n";
        return false;
    }
}

// ==================================================
// ValidateCuts: batch validate a map of cuts
// ==================================================
inline std::map<std::string, CutDef>
ValidateCuts(ROOT::RDF::RNode node,
             const std::map<std::string, CutDef>& cuts,
             unsigned nCheck = 50,
             unsigned maxCheck = 5000) {
    std::map<std::string, CutDef> valid;
    for (const auto &[name, cut] : cuts) {
        if (ValidateUserCut(node, cut, nCheck, maxCheck)) {
            valid[name] = cut;
        } else {
            std::cerr << "[ValidationTools] Skipping invalid cut '" << name << "'\n";
        }
    }
    return valid;
}

// ==================================================
// Register helper functions with ROOT's Cling
// ==================================================
inline void RegisterSafeHelpers() {
    gInterpreter->Declare(R"(
        #include "ROOT/RVec.hxx"
        #include <cmath>
        inline double SafeDiv(double num, double den, double def = 0.0) {
            return (den != 0.0) ? num / den : def;
        }
        template <typename T>
        inline T SafeIndex(const ROOT::RVec<T>& vec, unsigned idx, T def = -1) {
            return (idx < vec.size()) ? vec[idx] : def;
        }
    )");
}
#endif
