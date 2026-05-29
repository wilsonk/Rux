/*
    Rux Compiler
    Copyright © 2026 Rux Contributors
    Licensed under the MIT License
*/

#pragma once

#include "Rux/Lir.h"

#ifdef USE_LLVM_BACKEND
#include <llvm/IR/LLVMContext.h>
#endif

#include <filesystem>
#include <string>
#include <vector>

namespace Rux {
    /**
     * @brief LLVM backend for code generation
     *
     * Translates LIR (Low-level Intermediate Representation) to LLVM IR,
     * then uses LLVM's backend to generate native object files.
     */
    class LLVM {
    public:
        /**
         * @brief Construct LLVM backend from LIR package
         * @param package LIR package to translate
         * @param packageName Name of the package (for symbol naming)
         * @param targetTriple LLVM target triple (e.g., "x86_64-linux-gnu")
         */
        explicit LLVM(LirPackage package, std::string packageName = {}, std::string targetTriple = {});

        /**
         * @brief Generate object files from LIR package
         * @return Vector of object file paths (one per module)
         */
        [[nodiscard]] std::vector<std::filesystem::path> Generate() const;

        /**
         * @brief Emit LLVM IR to a file for debugging
         * @param path Output file path (.ll for text IR, .bc for bitcode)
         * @return true on success, false on failure
         */
        [[nodiscard]] bool EmitIR(const std::filesystem::path& path) const;

    private:
        LirPackage lir;
        std::string packageName;
        std::string targetTriple;
#ifdef USE_LLVM_BACKEND
        mutable std::unique_ptr<llvm::LLVMContext> context;
#endif
    };
} // namespace Rux
