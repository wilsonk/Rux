/*
    Rux Compiler
    Copyright © 2026 Rux Contributors
    Licensed under the MIT License
*/

#pragma once

#include "Rux/Lir.h"
#include "Rux/Type.h"

#ifdef USE_LLVM_BACKEND
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#endif

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rux {
#ifdef USE_LLVM_BACKEND
    /**
     * @brief Type mapper for converting Rux TypeRef to LLVM types
     */
    class LLVMTypeMapper {
    public:
        explicit LLVMTypeMapper(llvm::LLVMContext& context, const std::string& targetTriple);

        /**
         * @brief Convert a Rux TypeRef to an LLVM type
         * @param type Rux type to convert
         * @return LLVM type, or nullptr on failure
         */
        [[nodiscard]] llvm::Type* MapType(const TypeRef& type);

        /**
         * @brief Convert a Rux calling convention to LLVM calling convention
         * @param conv Rux calling convention
         * @return LLVM calling convention
         */
        [[nodiscard]] llvm::CallingConv::ID MapCallingConvention(Rux::CallingConvention conv);

    private:
        llvm::LLVMContext& context;
        std::string targetTriple;
        std::unordered_map<std::string, llvm::Type*> namedTypeCache;

        [[nodiscard]] llvm::Type* MapPrimitiveType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapPlatformType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapPointerType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapSliceType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapRangeType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapTupleType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapFuncType(const TypeRef& type);
        [[nodiscard]] llvm::Type* MapNamedType(const TypeRef& type);
    };
#endif

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
        mutable std::unique_ptr<llvm::Module> module;
        mutable std::unique_ptr<llvm::IRBuilder<>> builder;
        mutable std::unordered_map<LirReg, llvm::Value*> valueMap; // Symbol table for LIR registers
        mutable std::unique_ptr<LLVMTypeMapper> typeMapper;

        [[nodiscard]] llvm::Value* TranslateInstruction(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateConst(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateAlloca(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateLoad(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateStore(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateBinaryOp(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateUnaryOp(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateCmp(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateCast(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateCall(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateFieldPtr(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateIndexPtr(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslatePhi(const LirInstr& instr);
        [[nodiscard]] llvm::Value* TranslateGlobalAddr(const LirInstr& instr);

        [[nodiscard]] bool TranslateTerminator(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap);
        [[nodiscard]] bool TranslateJump(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap);
        [[nodiscard]] bool TranslateBranch(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap);
        [[nodiscard]] bool TranslateReturn(const LirTerminator& term);
        [[nodiscard]] bool TranslateSwitch(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap);

        [[nodiscard]] bool TranslateFunction(const LirFunc& func);
        [[nodiscard]] bool TranslateBlock(const LirBlock& block, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap);
#endif
    };
} // namespace Rux
