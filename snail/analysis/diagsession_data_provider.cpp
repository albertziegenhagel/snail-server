
#include <snail/analysis/diagsession_data_provider.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>

#include <libzippp.h>

#include <snail/common/filename.hpp>
#include <snail/common/trim.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

std::optional<std::string_view> extract_xml_attribute(std::string_view xml_node, std::string_view attribute_name)
{
    const auto attr_name_offset = xml_node.find(attribute_name);
    if(attr_name_offset == std::string_view::npos) return {};
    if(attr_name_offset + attribute_name.size() + 3 >= xml_node.size()) return {}; // we need at least '<node_name>=""'
    if(xml_node.substr(attr_name_offset + attribute_name.size(), 2) != "=\"") return {};

    const auto attr_value_offset = attr_name_offset + attribute_name.size() + 2;

    const auto attr_value_end = xml_node.find('\"', attr_value_offset);
    if(attr_value_end == std::string_view::npos) return {};

    return xml_node.substr(attr_value_offset, attr_value_end - attr_value_offset);
}

} // namespace

diagsession_data_provider::diagsession_data_provider(pdb_symbol_find_options find_options,
                                                     path_map                module_path_map) :
    etl_data_provider(std::move(find_options), std::move(module_path_map))
{}

diagsession_data_provider::~diagsession_data_provider()
{
    try_cleanup();
}

void diagsession_data_provider::process(const std::filesystem::path& file_path)
{
    libzippp::ZipArchive archive(file_path.string());
    if(!archive.open(libzippp::ZipArchive::ReadOnly))
    {
        throw std::runtime_error(std::format("Could not open diagsession file '{}'", file_path.string()));
    }

    // Extract the ETL file path from the metadata within the diagsession file.
    std::optional<std::string> archive_etl_file_path;
    {
        const auto metadata_entry = archive.getEntry("metadata.xml");
        if(metadata_entry.isNull())
        {
            throw std::runtime_error(std::format("Could not find metadata.xml in '{}'", file_path.string()));
        }

        std::stringstream metadata_stream;
        metadata_entry.readContent(metadata_stream);

        // NOTE: We want to spare ourselves from a dependency to a real XML parser library and hence
        //       we try to extract only the necessary information "by hand".
        //       This assumes that the XML file "behaves well enough". Hopefully that assumption is reasonable
        //       since currently we only deal with diagsession and files written by VCDiagnostics.exe.
        std::string line;
        while(std::getline(metadata_stream, line))
        {
            const auto trimmed = common::trim(line);

            // Try to find the line that includes the `<Resource />` XML Node.
            // IMPLICIT ASSUMPTION 1: We ignore the path of the XML Node, and assume that it is 'Package.Content.Resource'.
            // IMPLICIT ASSUMPTION 2: The whole node is actually within a single line.
            if(!trimmed.starts_with("<Resource ") || !trimmed.ends_with("/>")) continue;
            if(!trimmed.contains("Type=\"DiagnosticsHub.Resource.EtlFile\"")) continue;

            // Try to extract the name and prefix attributes
            const auto etl_file_name   = extract_xml_attribute(trimmed, "Name");
            const auto etl_file_prefix = extract_xml_attribute(trimmed, "ResourcePackageUriPrefix");

            if(!etl_file_name) continue;
            if(!etl_file_prefix) continue;

            archive_etl_file_path = std::format("{}/{}", *etl_file_prefix, *etl_file_name);

            break;
        }
    }

    if(!archive_etl_file_path)
    {
        throw std::runtime_error(std::format("Could not extract ETL file path from metadata in '{}'", file_path.string()));
    }

    // Extract the ETL file to a temporary directory
    const auto temp_dir = std::filesystem::temp_directory_path() / "snail" / common::make_random_filename(20);
    std::filesystem::create_directories(temp_dir);

    // Clean-up any previously created temporary files.
    try_cleanup();

    temp_etl_file_path_ = temp_dir / std::filesystem::path(*archive_etl_file_path).filename();

    {
        const auto etl_file_entry = archive.getEntry(*archive_etl_file_path);
        if(etl_file_entry.isNull())
        {
            throw std::runtime_error(std::format("Could not find ETL file '{}' in '{}'", *archive_etl_file_path, file_path.string()));
        }

        std::ofstream etl_file_stream(*temp_etl_file_path_, std::ios::binary);
        etl_file_entry.readContent(etl_file_stream);
    }

    etl_data_provider::process(*temp_etl_file_path_);
}

void diagsession_data_provider::try_cleanup() noexcept
{
    if(!temp_etl_file_path_) return;

    std::error_code error_code;
    std::filesystem::remove_all(temp_etl_file_path_->parent_path(), error_code);
    if(!error_code)
    {
        temp_etl_file_path_ = std::nullopt;
    }
    // NOTE: we simply ignore the case when there was an error:
    //   1. There is nothing we can do about it anyways?!
    //   2. This needs to be callable from a destructor, hence it should not throw.
    //   3. The directory we failed to delete is located in an official temporary directory. The OS can still clean it up.
}
