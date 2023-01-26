#include <snail/analysis/detail/dwarf_resolver.hpp>

#include <cassert>
#include <format>
#include <iostream>
#include <optional>

#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/Object/ELFObjectFile.h>

#include <snail/common/cast.hpp>
#include <snail/common/hash_combine.hpp>

using namespace snail;
using namespace snail::analysis::detail;

dwarf_resolver::dwarf_resolver()  = default;
dwarf_resolver::~dwarf_resolver() = default;

const dwarf_resolver::symbol_info& dwarf_resolver::make_generic_symbol(instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = 0,
                                 .load_timestamp = 0},
        .address = address
    };
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    const auto new_symbol = symbol_info{
        .name       = std::format("{:#018x}", address),
        .is_generic = true};

    const auto [new_iter, _] = symbol_cache.emplace(key, new_symbol);
    return new_iter->second;
}

const dwarf_resolver::symbol_info& dwarf_resolver::make_generic_symbol(const module_info& module, instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = module.process_id,
                                 .load_timestamp = module.load_timestamp},
        .address = address
    };
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    auto delimiter_pos = module.image_filename.find_last_of("\\");
    if(delimiter_pos == std::u16string::npos) delimiter_pos = module.image_filename.find_last_of("//");

    const auto filename = delimiter_pos == std::u16string::npos ? module.image_filename : module.image_filename.substr(delimiter_pos + 1);

    const auto new_symbol = symbol_info{
        .name       = std::format("{}!{:#018x}", filename, address),
        .is_generic = true};

    const auto [new_iter, inserted] = symbol_cache.emplace(key, new_symbol);
    assert(inserted);
    return new_iter->second;
}

struct dwarf_resolver::context_storage
{
    std::unique_ptr<llvm::object::Binary> binary;
    std::unique_ptr<llvm::MemoryBuffer>   memory;
    llvm::object::ObjectFile*             object;
    std::unique_ptr<llvm::DWARFContext>   context;
};

const dwarf_resolver::symbol_info&
dwarf_resolver::resolve_symbol(const module_info& module, instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = module.process_id,
                                 .load_timestamp = module.load_timestamp},
        .address = address
    };
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    const auto relative_address = address - module.image_base + module.page_offset;

    auto* const dwarf_context = get_dwarf_context(module);
    if(dwarf_context == nullptr) return make_generic_symbol(module, address);

    llvm::object::SectionedAddress sectioned_address;

    sectioned_address.Address = relative_address;

    for(auto section : dwarf_context->object->sections())
    {
        if(!section.isText() || section.isVirtual())
            continue;

        if(relative_address >= section.getAddress() &&
           relative_address < section.getAddress() + section.getSize())
        {
            sectioned_address.SectionIndex = section.getIndex();
            break;
        }
    }

    auto line_info_specifier = llvm::DILineInfoSpecifier(llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath, llvm::DILineInfoSpecifier::FunctionNameKind::LinkageName);

    auto inlining_info = dwarf_context->context->getInliningInfoForAddress(sectioned_address, line_info_specifier);

    const auto number_of_inlined_frames = inlining_info.getNumberOfFrames();
    if(number_of_inlined_frames == 0)
    {
        return make_generic_symbol(module, address);
    }

    // auto line_info = dwarf_context->context->getLineInfoForAddress(sectioned_address, line_info_specifier);

    auto line_info = inlining_info.getFrame(number_of_inlined_frames - 1);

    if(line_info.FunctionName == llvm::DILineInfo::BadString)
    {
        return make_generic_symbol(module, address);
    }

    auto new_symbol = symbol_info{
        .is_generic = false};

    if(!llvm::nonMicrosoftDemangle(line_info.FunctionName.c_str(), new_symbol.name))
    {
        new_symbol.name = line_info.FunctionName;
    }

    const auto [new_iter, inserted] = symbol_cache.emplace(key, new_symbol);
    assert(inserted);
    return new_iter->second;
}

dwarf_resolver::context_storage* dwarf_resolver::get_dwarf_context(const module_info& module)
{
    const auto key = module_key{
        .process_id     = module.process_id,
        .load_timestamp = module.load_timestamp};

    auto iter = dwarf_context_cache.find(key);
    if(iter != dwarf_context_cache.end()) return iter->second.get();

    const auto pmsc   = R"(C:\Users\aziegenhagel\data\pmsc.cpython-311-x86_64-linux-gnu.so)";
    const auto python = R"(C:\Users\aziegenhagel\data\libpython3.11.so)";

    std::string_view filename = module.image_filename;
    if(filename == "/home/aziegenhagel/build/pmsc/ex-6/pmsc.cpython-311-x86_64-linux-gnu.so")
    {
        filename = pmsc;
    }
    if(filename == "/usr/lib64/libpython3.11.so.1.0")
    {
        filename = python;
    }

    auto binary = llvm::object::createBinary(llvm::StringRef(filename.data(), filename.size()));
    if(!binary)
    {
        llvm::consumeError(binary.takeError());
        return nullptr;
    }

    if(!binary->getBinary()->isObject())
    {
        return nullptr;
    }

    auto* const object = llvm::cast<llvm::object::ObjectFile>(binary->getBinary());

    auto context = llvm::DWARFContext::create(*object);

    if(context == nullptr)
    {
        return nullptr;
    }

    auto& storage = dwarf_context_cache[key];

    storage = std::make_unique<context_storage>();

    auto binary_pair = binary->takeBinary();
    storage->binary  = std::move(binary_pair.first);
    storage->memory  = std::move(binary_pair.second);

    storage->object  = object;
    storage->context = std::move(context);

    return storage.get();
}

bool dwarf_resolver::module_key::operator==(const module_key& other) const
{
    return process_id == other.process_id &&
           load_timestamp == other.load_timestamp;
}

std::size_t dwarf_resolver::module_key_hasher::operator()(const module_key& key) const
{
    return common::hash_combine(process_id_hasher(key.process_id), timestamp_hasher(key.load_timestamp));
}

bool dwarf_resolver::symbol_key::operator==(const symbol_key& other) const
{
    return module_key == other.module_key &&
           address == other.address;
}

std::size_t dwarf_resolver::symbol_key_hasher::operator()(const symbol_key& key) const
{
    return common::hash_combine(module_key_hasher(key.module_key), address_hasher(key.address));
}
