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
} // namespace Rux
