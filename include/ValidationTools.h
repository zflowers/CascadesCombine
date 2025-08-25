// User Defined Var Validation Helpers

// ----------------------
// Derived variables
// ----------------------
struct DerivedVar {
    std::string name; // variable name to be created in the RDataFrame
    std::string expr; // expression to define it (ROOT/C++ expression)
};

// --------------------------------------------------
// Type-detection helper: container-like detection
// --------------------------------------------------
template <typename, typename = void>
struct has_value_type : std::false_type {};

template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

// --------------------------------------------------
// TryValidateType: attempt to Take<T> over the first nCheck events
// - node is the tmpNode (the Define was already done on it)
// - Recurses with larger nCheck when results are sparse (empty / all-NaN for floats)
// - Returns true if the type T is compatible (even if sparse); false only when Take<T> throws
// --------------------------------------------------
template <typename T>
bool TryValidateType(ROOT::RDF::RNode node,
                            const DerivedVar &dv,
                            unsigned nCheck = 50,
                            unsigned maxCheck = 5000) {
    try {
        // create a subset for this attempt (so recursion with larger nCheck works)
        auto subset = node.Range(0, static_cast<long>(nCheck));

        // This will throw if T is not compatible with the column type (caught below)
        auto vals = subset.Take<T>(dv.name + "_test").GetValue();

        // If we reached here, the column can be interpreted as T.
        // Now decide whether the data are "sparse" and we should retry with more events.

        // --- scalar floating point (e.g. double) ---
        if constexpr (std::is_floating_point_v<T>) {
            bool anyFinite = std::any_of(vals.begin(), vals.end(), [](auto v){ return std::isfinite(v); });
            if (!anyFinite) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[BFI_condor] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but produced no finite values in first " << maxCheck << " events (sparse data).\n";
                }
            }
            return true; // type compatible
        }

        // --- container-like (e.g. ROOT::VecOps::RVec<Inner>) ---
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
                        std::cerr << "[BFI_condor] WARNING: '" << dv.name
                                  << "' evaluated as container of " << typeid(Inner).name()
                                  << " but produced no finite inner values in first " << maxCheck << " events (sparse data).\n";
                    }
                }
                return true;
            } else {
                // inner is not floating (int/bool etc.)
                if (vals.empty()) {
                    if (nCheck < maxCheck) {
                        unsigned next = std::min(nCheck * 2u, maxCheck);
                        return TryValidateType<T>(node, dv, next, maxCheck);
                    } else {
                        std::cerr << "[BFI_condor] WARNING: '" << dv.name
                                  << "' evaluated as container type " << typeid(T).name()
                                  << " but returned empty sequence in first " << maxCheck << " events (sparse data).\n";
                    }
                }
                return true;
            }
        }

        // --- non-floating scalar (int, bool, etc.) ---
        else {
            if (vals.empty()) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[BFI_condor] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but returned empty vector in first " << maxCheck << " events (sparse data).\n";
                }
            }
            return true;
        }

    } catch (const std::exception &e) {
        // Take<T> threw T is incompatible with the column type (or some other error)
        // Return false to allow caller to try the next candidate type.
        return false;
    } catch (...) {
        return false;
    }
}

// --------------------------------------------------
// ValidateDerivedVar: try a set of candidate scalar/vector types
// --------------------------------------------------
inline bool ValidateDerivedVar(ROOT::RDF::RNode node,
                               const DerivedVar &dv,
                               unsigned nCheck = 50,
                               unsigned maxCheck = 5000) {
    try {
        // Define temporary test column (tmpNode holds the Define)
        ROOT::RDF::RNode tmpNode = node.Define(dv.name + "_test", dv.expr);

        // Try candidate scalar types first (order matters a bit: prefer double/float)
        if (TryValidateType<double>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<float>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<int>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned int>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<long long>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned long long>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<bool>(tmpNode, dv, nCheck, maxCheck))   return true;

        // Try container types (ROOT::VecOps::RVec<T>)
        if (TryValidateType<ROOT::VecOps::RVec<double>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<float>>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<ROOT::VecOps::RVec<int>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned int>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<long long>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned long long>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<bool>>(tmpNode, dv, nCheck, maxCheck))   return true;

        // Nothing matched: emit hints and fail
        std::cerr << "[BFI_condor] ERROR validating '" << dv.name
                  << "' from expression: " << dv.expr << "\n";
        if (dv.expr.find("/") != std::string::npos &&
            dv.expr.find("SafeDiv") == std::string::npos) {
            std::cerr << "  HINT: Expression contains '/', consider using SafeDiv(num, den, def)\n";
        }
        if (dv.expr.find("[") != std::string::npos &&
            dv.expr.find("SafeIndex") == std::string::npos) {
            std::cerr << "  HINT: Expression uses indexing '[]', consider using SafeIndex(vec, idx, defaultVal)\n";
        }

        return false;

    } catch (const std::exception &e) {
        std::cerr << "[BFI_condor] WARNING: Exception validating '" << dv.name
                  << "': " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[BFI_condor] WARNING: Unknown exception validating '" << dv.name << "'\n";
        return false;
    }
}

// Register helper functions with ROOT's Cling interpreter
inline void RegisterSafeHelpers() {
    gInterpreter->Declare(R"(
        #include "ROOT/RVec.hxx"
        #include <cmath>

        // --- Safe division ---
        inline double SafeDiv(double num, double den, double def = 0.0) {
            return (den != 0.0) ? num / den : def;
        }

        // --- Safe index ---
        template <typename T>
        inline T SafeIndex(const ROOT::RVec<T>& vec, unsigned idx, T def = -1) {
            return (idx < vec.size()) ? vec[idx] : def;
        }
    )");
}
