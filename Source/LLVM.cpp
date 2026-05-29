/*
    Rux Compiler
    Copyright © 2026 Rux Contributors
    Licensed under the MIT License
*/

#include "Rux/LLVM.h"

#ifdef USE_LLVM_BACKEND
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Host.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Target/TargetOptions.h>
#endif

#include <cstdlib>

namespace Rux {
#ifdef USE_LLVM_BACKEND
    LLVMTypeMapper::LLVMTypeMapper(llvm::LLVMContext& context, const std::string& targetTriple)
        : context(context)
        , targetTriple(targetTriple) {
    }

    llvm::Type* LLVMTypeMapper::MapType(const TypeRef& type) {
        switch (type.kind) {
            case TypeRef::Kind::Unknown:
            case TypeRef::Kind::Opaque:
                return nullptr;

            // Primitive types
            case TypeRef::Kind::Bool8:
                return llvm::Type::getInt8Ty(context);
            case TypeRef::Kind::Bool16:
                return llvm::Type::getInt16Ty(context);
            case TypeRef::Kind::Bool32:
                return llvm::Type::getInt32Ty(context);
            case TypeRef::Kind::Char8:
                return llvm::Type::getInt8Ty(context);
            case TypeRef::Kind::Char16:
                return llvm::Type::getInt16Ty(context);
            case TypeRef::Kind::Char32:
                return llvm::Type::getInt32Ty(context);
            case TypeRef::Kind::Int8:
                return llvm::Type::getInt8Ty(context);
            case TypeRef::Kind::Int16:
                return llvm::Type::getInt16Ty(context);
            case TypeRef::Kind::Int32:
                return llvm::Type::getInt32Ty(context);
            case TypeRef::Kind::Int64:
                return llvm::Type::getInt64Ty(context);
            case TypeRef::Kind::UInt8:
                return llvm::Type::getInt8Ty(context);
            case TypeRef::Kind::UInt16:
                return llvm::Type::getInt16Ty(context);
            case TypeRef::Kind::UInt32:
                return llvm::Type::getInt32Ty(context);
            case TypeRef::Kind::UInt64:
                return llvm::Type::getInt64Ty(context);
            case TypeRef::Kind::Float32:
                return llvm::Type::getFloatTy(context);
            case TypeRef::Kind::Float64:
                return llvm::Type::getDoubleTy(context);

            // Platform-dependent types
            case TypeRef::Kind::Int:
            case TypeRef::Kind::UInt:
                return MapPlatformType(type);

            // Complex types
            case TypeRef::Kind::Pointer:
                return MapPointerType(type);
            case TypeRef::Kind::Slice:
                return MapSliceType(type);
            case TypeRef::Kind::Range:
                return MapRangeType(type);
            case TypeRef::Kind::Tuple:
                return MapTupleType(type);
            case TypeRef::Kind::Func:
                return MapFuncType(type);
            case TypeRef::Kind::Named:
                return MapNamedType(type);
            case TypeRef::Kind::Str:
                // String is a slice of char32
                return MapSliceType(TypeRef::MakeChar32());
            case TypeRef::Kind::TypeParam:
                // Generic parameters - for now, treat as opaque
                return llvm::PointerType::get(context, 0);

            default:
                return nullptr;
        }
    }

    llvm::Type* LLVMTypeMapper::MapPrimitiveType(const TypeRef& type) {
        // Handled in MapType
        return nullptr;
    }

    llvm::Type* LLVMTypeMapper::MapPlatformType(const TypeRef& type) {
        // Determine pointer size based on target triple
        bool is64Bit = targetTriple.find("x86_64") != std::string::npos ||
                       targetTriple.find("aarch64") != std::string::npos;

        if (is64Bit) {
            return llvm::Type::getInt64Ty(context);
        } else {
            return llvm::Type::getInt32Ty(context);
        }
    }

    llvm::Type* LLVMTypeMapper::MapPointerType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;
        llvm::Type* pointee = MapType(type.inner[0]);
        if (!pointee) return nullptr;
        return llvm::PointerType::get(context, 0);
    }

    llvm::Type* LLVMTypeMapper::MapSliceType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;
        llvm::Type* elementType = MapType(type.inner[0]);
        if (!elementType) return nullptr;

        // Slice is { pointer, length } - 16 bytes on 64-bit
        llvm::Type* fields[] = {
            llvm::PointerType::get(context, 0),
            llvm::Type::getInt64Ty(context)
        };
        return llvm::StructType::create(context, fields, "slice");
    }

    llvm::Type* LLVMTypeMapper::MapRangeType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;
        llvm::Type* elementType = MapType(type.inner[0]);
        if (!elementType) return nullptr;

        // Range is { start, end } - both same type
        llvm::Type* fields[] = {
            elementType,
            elementType
        };
        return llvm::StructType::create(context, fields, "range");
    }

    llvm::Type* LLVMTypeMapper::MapTupleType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;

        std::vector<llvm::Type*> fields;
        for (const auto& innerType : type.inner) {
            llvm::Type* field = MapType(innerType);
            if (!field) return nullptr;
            fields.push_back(field);
        }

        return llvm::StructType::create(context, fields, "tuple");
    }

    llvm::Type* LLVMTypeMapper::MapFuncType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;

        // Last element is return type, rest are parameters
        llvm::Type* returnType = MapType(type.inner.back());
        if (!returnType) return nullptr;

        std::vector<llvm::Type*> paramTypes;
        for (size_t i = 0; i < type.inner.size() - 1; ++i) {
            llvm::Type* param = MapType(type.inner[i]);
            if (!param) return nullptr;
            paramTypes.push_back(param);
        }

        return llvm::FunctionType::get(returnType, paramTypes, false);
    }

    llvm::Type* LLVMTypeMapper::MapNamedType(const TypeRef& type) {
        // Check cache first
        auto it = namedTypeCache.find(type.name);
        if (it != namedTypeCache.end()) {
            return it->second;
        }

        // For now, create an opaque struct type
        // This will be filled in later when we process struct/enum/union declarations
        llvm::StructType* structType = llvm::StructType::create(context, type.name);
        namedTypeCache[type.name] = structType;
        return structType;
    }

    llvm::CallingConv::ID LLVMTypeMapper::MapCallingConvention(Rux::CallingConvention conv) {
        switch (conv) {
            case Rux::CallingConvention::Default:
                // Let LLVM choose the default for the target
                return llvm::CallingConv::C;
            case Rux::CallingConvention::Win64:
                return llvm::CallingConv::Win64;
            default:
                return llvm::CallingConv::C;
        }
    }
#endif

    LLVM::LLVM(LirPackage package, std::string packageName, std::string targetTriple)
        : lir(std::move(package))
        , packageName(std::move(packageName))
        , targetTriple(std::move(targetTriple)) {
#ifdef USE_LLVM_BACKEND
        context = std::make_unique<llvm::LLVMContext>();
        // Initialize LLVM targets
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        // Detect host target triple if not provided
        if (this->targetTriple.empty()) {
            this->targetTriple = DetectHostTargetTriple();
        }

        // Setup target machine
        if (!SetupTargetMachine()) {
            // TODO: Handle error
        }

        // Create LLVM module
        module = std::make_unique<llvm::Module>(packageName.empty() ? "rux_module" : packageName, *context);
        module->setTargetTriple(llvm::Triple(this->targetTriple));

        // Create IR builder
        builder = std::make_unique<llvm::IRBuilder<>>(*context);

        // Create type mapper
        typeMapper = std::make_unique<LLVMTypeMapper>(*context, this->targetTriple);
#endif
    }

    std::vector<std::filesystem::path> LLVM::Generate() const {
#ifdef USE_LLVM_BACKEND
        std::vector<std::filesystem::path> objectFiles;

        // Translate all modules in the package
        for (const auto& mod : lir.modules) {
            if (!const_cast<LLVM*>(this)->TranslateModule(mod)) {
                // TODO: Handle error
                return {};
            }

            // Generate object file for this module
            std::filesystem::path objectPath = mod.name + GetObjectFileExtension();
            if (!EmitObjectFile(objectPath)) {
                // TODO: Handle error
                return {};
            }

            objectFiles.push_back(objectPath);
        }

        return objectFiles;
#else
        return {};
#endif
    }

    bool LLVM::EmitIR(const std::filesystem::path& path) const {
#ifdef USE_LLVM_BACKEND
        // TODO: Implement LLVM IR emission
        return false;
#else
        return false;
#endif
    }

#ifdef USE_LLVM_BACKEND
    std::string LLVM::GetObjectFileExtension() const {
        if (targetTriple.find("windows") != std::string::npos ||
            targetTriple.find("win32") != std::string::npos ||
            targetTriple.find("msvc") != std::string::npos) {
            return ".obj";
        }
        return ".o";
    }

    bool LLVM::EmitObjectFile(const std::filesystem::path& path) const {
        if (!targetMachine) {
            return false;
        }

        // Set module data layout
        module->setDataLayout(targetMachine->createDataLayout());

        std::error_code ec;
        llvm::raw_fd_ostream dest(path.string(), ec, llvm::sys::fs::OF_None);

        if (ec) {
            return false;
        }

        // Create pass manager
        llvm::legacy::PassManager pass;

        // Add pass to emit object file
        if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            return false;
        }

        // Run the pass manager
        pass.run(*module);

        dest.flush();

        return true;
    }
#endif

#ifdef USE_LLVM_BACKEND
    llvm::Value* LLVM::TranslateInstruction(const LirInstr& instr) {
        switch (instr.op) {
            case LirOpcode::Const:
                return TranslateConst(instr);
            case LirOpcode::Alloca:
                return TranslateAlloca(instr);
            case LirOpcode::Load:
                return TranslateLoad(instr);
            case LirOpcode::Store:
                return TranslateStore(instr);
            case LirOpcode::Add:
            case LirOpcode::Sub:
            case LirOpcode::Mul:
            case LirOpcode::Div:
            case LirOpcode::Mod:
            case LirOpcode::Pow:
            case LirOpcode::And:
            case LirOpcode::Or:
            case LirOpcode::Xor:
            case LirOpcode::Shl:
            case LirOpcode::Shr:
                return TranslateBinaryOp(instr);
            case LirOpcode::Neg:
            case LirOpcode::Not:
            case LirOpcode::BitNot:
                return TranslateUnaryOp(instr);
            case LirOpcode::CmpEq:
            case LirOpcode::CmpNe:
            case LirOpcode::CmpLt:
            case LirOpcode::CmpLe:
            case LirOpcode::CmpGt:
            case LirOpcode::CmpGe:
                return TranslateCmp(instr);
            case LirOpcode::Cast:
                return TranslateCast(instr);
            case LirOpcode::Call:
            case LirOpcode::CallIndirect:
                return TranslateCall(instr);
            case LirOpcode::FieldPtr:
                return TranslateFieldPtr(instr);
            case LirOpcode::IndexPtr:
                return TranslateIndexPtr(instr);
            case LirOpcode::Phi:
                return TranslatePhi(instr);
            case LirOpcode::GlobalAddr:
                return TranslateGlobalAddr(instr);
            default:
                return nullptr;
        }
    }

    llvm::Value* LLVM::TranslateConst(const LirInstr& instr) {
        llvm::Type* llvmType = typeMapper->MapType(instr.type);
        if (!llvmType) return nullptr;

        // Parse the literal value from strArg
        // For now, handle simple integer constants
        // TODO: Handle float, string, and other constant types
        if (llvmType->isIntegerTy()) {
            uint64_t value = std::stoull(instr.strArg);
            return llvm::ConstantInt::get(llvmType, value);
        }

        return nullptr;
    }

    llvm::Value* LLVM::TranslateAlloca(const LirInstr& instr) {
        llvm::Type* llvmType = typeMapper->MapType(instr.type);
        if (!llvmType) return nullptr;

        return builder->CreateAlloca(llvmType, nullptr, "alloca");
    }

    llvm::Value* LLVM::TranslateLoad(const LirInstr& instr) {
        if (instr.srcs.empty()) return nullptr;

        llvm::Value* ptr = valueMap[instr.srcs[0]];
        if (!ptr) return nullptr;

        return builder->CreateLoad(typeMapper->MapType(instr.type), ptr, "load");
    }

    llvm::Value* LLVM::TranslateStore(const LirInstr& instr) {
        if (instr.srcs.size() < 2) return nullptr;

        llvm::Value* val = valueMap[instr.srcs[0]];
        llvm::Value* ptr = valueMap[instr.srcs[1]];
        if (!val || !ptr) return nullptr;

        builder->CreateStore(val, ptr);
        return nullptr; // Store has no result
    }

    llvm::Value* LLVM::TranslateBinaryOp(const LirInstr& instr) {
        if (instr.srcs.size() < 2) return nullptr;

        llvm::Value* lhs = valueMap[instr.srcs[0]];
        llvm::Value* rhs = valueMap[instr.srcs[1]];
        if (!lhs || !rhs) return nullptr;

        llvm::Type* type = lhs->getType();

        switch (instr.op) {
            case LirOpcode::Add:
                if (type->isIntegerTy())
                    return builder->CreateAdd(lhs, rhs, "add");
                else if (type->isFloatingPointTy())
                    return builder->CreateFAdd(lhs, rhs, "fadd");
                break;
            case LirOpcode::Sub:
                if (type->isIntegerTy())
                    return builder->CreateSub(lhs, rhs, "sub");
                else if (type->isFloatingPointTy())
                    return builder->CreateFSub(lhs, rhs, "fsub");
                break;
            case LirOpcode::Mul:
                if (type->isIntegerTy())
                    return builder->CreateMul(lhs, rhs, "mul");
                else if (type->isFloatingPointTy())
                    return builder->CreateFMul(lhs, rhs, "fmul");
                break;
            case LirOpcode::Div:
                if (type->isIntegerTy())
                    return builder->CreateSDiv(lhs, rhs, "sdiv");
                else if (type->isFloatingPointTy())
                    return builder->CreateFDiv(lhs, rhs, "fdiv");
                break;
            case LirOpcode::Mod:
                if (type->isIntegerTy())
                    return builder->CreateSRem(lhs, rhs, "srem");
                else if (type->isFloatingPointTy())
                    return builder->CreateFRem(lhs, rhs, "frem");
                break;
            case LirOpcode::And:
                return builder->CreateAnd(lhs, rhs, "and");
            case LirOpcode::Or:
                return builder->CreateOr(lhs, rhs, "or");
            case LirOpcode::Xor:
                return builder->CreateXor(lhs, rhs, "xor");
            case LirOpcode::Shl:
                return builder->CreateShl(lhs, rhs, "shl");
            case LirOpcode::Shr:
                return builder->CreateAShr(lhs, rhs, "shr");
            case LirOpcode::Pow:
                // TODO: Implement pow (may need runtime call)
                break;
            default:
                break;
        }

        return nullptr;
    }

    llvm::Value* LLVM::TranslateUnaryOp(const LirInstr& instr) {
        if (instr.srcs.empty()) return nullptr;

        llvm::Value* operand = valueMap[instr.srcs[0]];
        if (!operand) return nullptr;

        switch (instr.op) {
            case LirOpcode::Neg:
                if (operand->getType()->isIntegerTy())
                    return builder->CreateNeg(operand, "neg");
                else if (operand->getType()->isFloatingPointTy())
                    return builder->CreateFNeg(operand, "fneg");
                break;
            case LirOpcode::Not:
                // Logical not (bool)
                return builder->CreateNot(operand, "not");
            case LirOpcode::BitNot:
                // Bitwise not
                return builder->CreateNot(operand, "bitnot");
            default:
                break;
        }

        return nullptr;
    }

    llvm::Value* LLVM::TranslateCmp(const LirInstr& instr) {
        if (instr.srcs.size() < 2) return nullptr;

        llvm::Value* lhs = valueMap[instr.srcs[0]];
        llvm::Value* rhs = valueMap[instr.srcs[1]];
        if (!lhs || !rhs) return nullptr;

        llvm::Type* type = lhs->getType();
        bool isFloat = type->isFloatingPointTy();

        switch (instr.op) {
            case LirOpcode::CmpEq:
                return isFloat ? builder->CreateFCmpOEQ(lhs, rhs, "cmpeq")
                               : builder->CreateICmpEQ(lhs, rhs, "cmpeq");
            case LirOpcode::CmpNe:
                return isFloat ? builder->CreateFCmpONE(lhs, rhs, "cmpne")
                               : builder->CreateICmpNE(lhs, rhs, "cmpne");
            case LirOpcode::CmpLt:
                return isFloat ? builder->CreateFCmpOLT(lhs, rhs, "cmplt")
                               : builder->CreateICmpSLT(lhs, rhs, "cmplt");
            case LirOpcode::CmpLe:
                return isFloat ? builder->CreateFCmpOLE(lhs, rhs, "cmple")
                               : builder->CreateICmpSLE(lhs, rhs, "cmple");
            case LirOpcode::CmpGt:
                return isFloat ? builder->CreateFCmpOGT(lhs, rhs, "cmpgt")
                               : builder->CreateICmpSGT(lhs, rhs, "cmpgt");
            case LirOpcode::CmpGe:
                return isFloat ? builder->CreateFCmpOGE(lhs, rhs, "cmpge")
                               : builder->CreateICmpSGE(lhs, rhs, "cmpge");
            default:
                break;
        }

        return nullptr;
    }

    llvm::Value* LLVM::TranslateCast(const LirInstr& instr) {
        if (instr.srcs.empty()) return nullptr;

        llvm::Value* src = valueMap[instr.srcs[0]];
        if (!src) return nullptr;

        llvm::Type* dstType = typeMapper->MapType(instr.type);
        if (!dstType) return nullptr;

        llvm::Type* srcType = src->getType();

        // TODO: Implement proper cast based on src and dst types
        // For now, use bitcast as a simple fallback
        return builder->CreateBitCast(src, dstType, "cast");
    }

    llvm::Value* LLVM::TranslateCall(const LirInstr& instr) {
        // TODO: Implement call translation
        return nullptr;
    }

    llvm::Value* LLVM::TranslateFieldPtr(const LirInstr& instr) {
        if (instr.srcs.empty()) return nullptr;

        llvm::Value* base = valueMap[instr.srcs[0]];
        if (!base) return nullptr;

        // TODO: Parse field index from strArg
        uint32_t fieldIndex = 0;
        try {
            fieldIndex = std::stoul(instr.strArg);
        } catch (const std::exception&) {
            return nullptr;
        }

        llvm::Type* baseType = base->getType();
        if (!baseType->isPointerTy()) return nullptr;

        // With opaque pointers, we need to know the element type
        // For now, use the instruction type to infer the struct type
        llvm::Type* elementType = typeMapper->MapType(instr.type);
        if (!elementType) return nullptr;

        llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(elementType);
        if (!structType) return nullptr;

        return builder->CreateStructGEP(structType, base, fieldIndex, "fieldptr");
    }

    llvm::Value* LLVM::TranslateIndexPtr(const LirInstr& instr) {
        if (instr.srcs.size() < 2) return nullptr;

        llvm::Value* base = valueMap[instr.srcs[0]];
        llvm::Value* idx = valueMap[instr.srcs[1]];
        if (!base || !idx) return nullptr;

        llvm::Type* baseType = base->getType();
        if (!baseType->isPointerTy()) return nullptr;

        // With opaque pointers, use CreateInBoundsGEP with element type
        // The element type should be inferred from the instruction type
        llvm::Type* elementType = typeMapper->MapType(instr.type);
        if (!elementType) return nullptr;

        return builder->CreateInBoundsGEP(elementType, base, idx, "indexptr");
    }

    llvm::Value* LLVM::TranslatePhi(const LirInstr& instr) {
        llvm::Type* phiType = typeMapper->MapType(instr.type);
        if (!phiType) return nullptr;

        llvm::PHINode* phi = builder->CreatePHI(phiType, instr.phiPreds.size(), "phi");

        for (const auto& [reg, blockIndex] : instr.phiPreds) {
            llvm::Value* val = valueMap[reg];
            if (!val) return nullptr;
            // TODO: Map blockIndex to LLVM BasicBlock
            // phi->addIncoming(val, block);
        }

        return phi;
    }

    llvm::Value* LLVM::TranslateGlobalAddr(const LirInstr& instr) {
        // TODO: Look up global variable by name
        llvm::GlobalVariable* global = module->getGlobalVariable(instr.strArg);
        if (!global) return nullptr;

        return builder->CreateLoad(global->getType(), global, "globaladdr");
    }

    bool LLVM::TranslateTerminator(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap) {
        switch (term.kind) {
            case LirTermKind::Jump:
                return TranslateJump(term, blockMap);
            case LirTermKind::Branch:
                return TranslateBranch(term, blockMap);
            case LirTermKind::Return:
                return TranslateReturn(term);
            case LirTermKind::Switch:
                return TranslateSwitch(term, blockMap);
            default:
                return false;
        }
    }

    bool LLVM::TranslateJump(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap) {
        auto it = blockMap.find(term.trueTarget);
        if (it == blockMap.end()) return false;

        builder->CreateBr(it->second);
        return true;
    }

    bool LLVM::TranslateBranch(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap) {
        llvm::Value* cond = valueMap[term.cond];
        if (!cond) return false;

        auto trueIt = blockMap.find(term.trueTarget);
        auto falseIt = blockMap.find(term.falseTarget);
        if (trueIt == blockMap.end() || falseIt == blockMap.end()) return false;

        builder->CreateCondBr(cond, trueIt->second, falseIt->second);
        return true;
    }

    bool LLVM::TranslateReturn(const LirTerminator& term) {
        if (term.retVal) {
            llvm::Value* retVal = valueMap[*term.retVal];
            if (!retVal) return false;
            builder->CreateRet(retVal);
        } else {
            builder->CreateRetVoid();
        }
        return true;
    }

    bool LLVM::TranslateSwitch(const LirTerminator& term, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap) {
        llvm::Value* cond = valueMap[term.cond];
        if (!cond) return false;

        auto defaultIt = blockMap.find(term.defaultTarget);
        if (defaultIt == blockMap.end()) return false;

        // TODO: Parse case values and create switch instruction
        // For now, create a simple branch to default
        builder->CreateBr(defaultIt->second);
        return true;
    }

    bool LLVM::TranslateFunction(const LirFunc& func) {
        // Clear value map for this function
        valueMap.clear();

        // Create function type
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : func.params) {
            llvm::Type* paramType = typeMapper->MapType(param.type);
            if (!paramType) return false;
            paramTypes.push_back(paramType);
        }

        llvm::Type* returnType = typeMapper->MapType(func.returnType);
        if (!returnType) return false;

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

        // Create function
        llvm::Function* llvmFunc = llvm::Function::Create(
            funcType,
            func.isExtern ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage,
            func.name,
            module.get()
        );

        // Set calling convention
        llvmFunc->setCallingConv(typeMapper->MapCallingConvention(func.callConv));

        // For extern functions, we're done
        if (func.isExtern) {
            return true;
        }

        // Create basic blocks
        std::unordered_map<std::uint32_t, llvm::BasicBlock*> blockMap;
        for (size_t i = 0; i < func.blocks.size(); ++i) {
            const auto& block = func.blocks[i];
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context, block.label, llvmFunc);
            blockMap[static_cast<std::uint32_t>(i)] = bb;
        }

        // Translate each block
        for (size_t i = 0; i < func.blocks.size(); ++i) {
            const auto& block = func.blocks[i];
            llvm::BasicBlock* bb = blockMap[static_cast<std::uint32_t>(i)];
            builder->SetInsertPoint(bb);

            if (!TranslateBlock(block, blockMap)) {
                return false;
            }
        }

        return true;
    }

    bool LLVM::TranslateBlock(const LirBlock& block, const std::unordered_map<std::uint32_t, llvm::BasicBlock*>& blockMap) {
        // Translate instructions
        for (const auto& instr : block.instrs) {
            llvm::Value* result = TranslateInstruction(instr);
            if (instr.dst != LirNoReg) {
                valueMap[instr.dst] = result;
            }
        }

        // Translate terminator
        if (block.term) {
            return TranslateTerminator(*block.term, blockMap);
        }

        return true;
    }

    bool LLVM::TranslateStructDecl(const LirStructDecl& decl) {
        std::vector<llvm::Type*> fieldTypes;
        for (const auto& field : decl.fields) {
            llvm::Type* fieldType = typeMapper->MapType(field.type);
            if (!fieldType) return false;
            fieldTypes.push_back(fieldType);
        }

        llvm::StructType* structType = llvm::StructType::create(*context, fieldTypes, decl.name);
        (void)typeMapper->MapType(TypeRef{TypeRef::Kind::Named, decl.name}); // Cache the type
        return true;
    }

    bool LLVM::TranslateEnumDecl(const LirEnumDecl& decl) {
        // For now, treat enums as their base type
        // TODO: Implement proper enum representation with discriminant
        llvm::Type* baseType = typeMapper->MapType(decl.baseType);
        if (!baseType) return false;

        (void)typeMapper->MapType(TypeRef{TypeRef::Kind::Named, decl.name}); // Cache the type
        return true;
    }

    bool LLVM::TranslateUnionDecl(const LirUnionDecl& decl) {
        // For now, create a struct with the largest field
        // TODO: Implement proper union representation with tag
        llvm::Type* largestType = nullptr;
        size_t largestSize = 0;

        for (const auto& field : decl.fields) {
            llvm::Type* fieldType = typeMapper->MapType(field.type);
            if (!fieldType) return false;

            // TODO: Calculate actual size
            // For now, just use the first field type
            if (!largestType) {
                largestType = fieldType;
            }
        }

        if (largestType) {
            llvm::StructType* unionType = llvm::StructType::create(*context, {largestType}, decl.name);
            (void)typeMapper->MapType(TypeRef{TypeRef::Kind::Named, decl.name}); // Cache the type
        }

        return true;
    }

    bool LLVM::TranslateConstDecl(const LirConstDecl& decl) {
        llvm::Type* constType = typeMapper->MapType(decl.type);
        if (!constType) return false;

        // TODO: Parse the constant value from decl.value
        // For now, create a null constant
        llvm::Constant* init = llvm::Constant::getNullValue(constType);

        new llvm::GlobalVariable(
            *module,
            constType,
            true, // is constant
            llvm::GlobalValue::PrivateLinkage,
            init,
            decl.name
        );

        return true;
    }

    bool LLVM::TranslateExternVar(const LirExternVar& var) {
        llvm::Type* varType = typeMapper->MapType(var.type);
        if (!varType) return false;

        new llvm::GlobalVariable(
            *module,
            varType,
            false, // not constant
            llvm::GlobalValue::ExternalLinkage,
            nullptr, // external, no initializer
            var.name
        );

        return true;
    }

    bool LLVM::TranslateModule(const LirModule& mod) {
        // Translate type declarations first
        for (const auto& decl : mod.structs) {
            if (!TranslateStructDecl(decl)) return false;
        }

        for (const auto& decl : mod.enums) {
            if (!TranslateEnumDecl(decl)) return false;
        }

        for (const auto& decl : mod.unions) {
            if (!TranslateUnionDecl(decl)) return false;
        }

        // Translate constants and extern variables
        for (const auto& decl : mod.consts) {
            if (!TranslateConstDecl(decl)) return false;
        }

        for (const auto& var : mod.externVars) {
            if (!TranslateExternVar(var)) return false;
        }

        // Translate functions
        for (const auto& func : mod.funcs) {
            if (!TranslateFunction(func)) return false;
        }

        return true;
    }

#ifdef USE_LLVM_BACKEND
    std::string LLVM::DetectHostTargetTriple() const {
        return llvm::sys::getDefaultTargetTriple();
    }

    bool LLVM::SetupTargetMachine() {
        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

        if (!target) {
            // TODO: Handle error
            return false;
        }

        // Create target machine with default settings
        // TODO: Allow configuration of optimization level, relocation model, code model
        llvm::TargetOptions opt;
        auto relocModel = llvm::Reloc::Model::PIC_; // Position-independent code
        auto codeModel = llvm::CodeModel::Small;
        llvm::CodeGenOptLevel optLevel = llvm::CodeGenOptLevel::Default;

        targetMachine = std::unique_ptr<llvm::TargetMachine>(
            target->createTargetMachine(
                llvm::Triple(targetTriple),
                "", // CPU (default)
                "", // Features (default)
                opt,
                relocModel,
                codeModel,
                optLevel
            )
        );

        if (!targetMachine) {
            return false;
        }

        return true;
    }

    std::string LLVM::DetectSystemLinker() const {
        if (targetTriple.find("windows") != std::string::npos ||
            targetTriple.find("win32") != std::string::npos ||
            targetTriple.find("msvc") != std::string::npos) {
            return "link.exe";
        } else if (targetTriple.find("apple") != std::string::npos ||
                   targetTriple.find("darwin") != std::string::npos) {
            return "cc";
        } else {
            return "cc";
        }
#else
        return "cc";
#endif
    }

    std::string LLVM::GenerateLinkerCommand(const std::vector<std::filesystem::path>& objectFiles, const std::filesystem::path& outputPath) const {
#ifdef USE_LLVM_BACKEND
        std::string linker = DetectSystemLinker();
        std::string cmd = linker;

        if (linker == "link.exe") {
            // Windows linker
            cmd += " /OUT:" + outputPath.string();
            for (const auto& obj : objectFiles) {
                cmd += " " + obj.string();
            }
            cmd += " kernel32.lib user32.lib";
        } else if (linker == "ld64") {
            // macOS linker
            cmd += " -o " + outputPath.string();
            for (const auto& obj : objectFiles) {
                cmd += " " + obj.string();
            }
            cmd += " -lSystem";
            // TODO: Add -syslibroot with xcrun
        } else {
            // Linux/BSD - use cc as linker driver
            cmd += " -o " + outputPath.string();
            for (const auto& obj : objectFiles) {
                cmd += " " + obj.string();
            }
            cmd += " -lc -lm";
        }

        return cmd;
#else
        return "";
#endif
    }

    bool LLVM::LinkObjectFiles(const std::vector<std::filesystem::path>& objectFiles, const std::filesystem::path& outputPath) const {
#ifdef USE_LLVM_BACKEND
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outputPath.parent_path());

        std::string cmd = GenerateLinkerCommand(objectFiles, outputPath);
        if (cmd.empty()) {
            return false;
        }

        int result = std::system(cmd.c_str());
        return result == 0;
#else
        return false;
#endif
    }
#endif
} // namespace Rux
