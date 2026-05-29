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
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#endif

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
            case TypeRef::Kind::Bool:
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
            case TypeRef::Kind::Char:
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
            case TypeRef::Kind::Float:
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
                return llvm::Type::getInt8Ty(context)->getPointerTo();

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
        return pointee->getPointerTo();
    }

    llvm::Type* LLVMTypeMapper::MapSliceType(const TypeRef& type) {
        if (type.inner.empty()) return nullptr;
        llvm::Type* elementType = MapType(type.inner[0]);
        if (!elementType) return nullptr;

        // Slice is { pointer, length } - 16 bytes on 64-bit
        llvm::Type* fields[] = {
            elementType->getPointerTo(),
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
            case Rux::CallingConvention::C:
                return llvm::CallingConv::C;
            case Rux::CallingConvention::StdCall:
                return llvm::CallingConv::X86_StdCall;
            case Rux::CallingConvention::FastCall:
                return llvm::CallingConv::X86_FastCall;
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

        // Create LLVM module
        module = std::make_unique<llvm::Module>(packageName.empty() ? "rux_module" : packageName, *context);
        if (!targetTriple.empty()) {
            module->setTargetTriple(targetTriple);
        }

        // Create IR builder
        builder = std::make_unique<llvm::IRBuilder<>>(*context);

        // Create type mapper
        typeMapper = std::make_unique<LLVMTypeMapper>(*context, targetTriple);
#endif
    }

    std::vector<std::filesystem::path> LLVM::Generate() const {
#ifdef USE_LLVM_BACKEND
        // TODO: Implement LIR to LLVM IR translation
        // TODO: Generate object files using LLVM backend
        return {};
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
        uint32_t fieldIndex = std::stoul(instr.strArg);

        llvm::Type* baseType = base->getType();
        if (!baseType->isPointerTy()) return nullptr;

        llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(baseType->getPointerElementType());
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

        return builder->CreateGEP(baseType->getPointerElementType(), base, idx, "indexptr");
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
#endif
} // namespace Rux
