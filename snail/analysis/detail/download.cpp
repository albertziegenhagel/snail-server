#include <snail/analysis/detail/download.hpp>

#include <format>
#include <fstream>

#include <curl/curl.h>

namespace {

struct download_file_info
{
    const std::filesystem::path& path;
    std::ofstream                file;
};

std::size_t download_write_callback(void* contents, std::size_t size, std::size_t nmemb, void* user_data)
{
    auto* file_info = static_cast<download_file_info*>(user_data);
    auto& file      = file_info->file;

    if(!file.is_open())
    {
        file.open(file_info->path, std::ios::binary);
        if(!file.is_open())
        {
            return CURL_WRITEFUNC_ERROR;
        }
    }

    const auto start_pos = file.tellp();
    file.write(static_cast<const char*>(contents), size * nmemb);
    const auto written = file.tellp() - start_pos;

    return static_cast<std::size_t>(written);
}

} // namespace

bool snail::analysis::detail::try_download_file(const std::string&           url,
                                                const std::filesystem::path& output_path)
{
    auto* curl = curl_easy_init();
    if(!curl) throw std::runtime_error("Failed to initialize curl");

    auto file_info = download_file_info{
        .path = output_path,
        .file = {}};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file_info);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    const auto result_code = curl_easy_perform(curl);

    file_info.file.close();

    if(result_code == CURLE_HTTP_RETURNED_ERROR)
    {
        constexpr long http_notfound = 404;

        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if(http_code == http_notfound)
        {
            // We do not consider "not found" a hard error
            curl_easy_cleanup(curl);
            return false;
        }
    }

    curl_easy_cleanup(curl);

    if(result_code != CURLE_OK)
    {
        std::filesystem::remove(output_path);
        throw std::runtime_error(std::format("Failed to download file: {}", curl_easy_strerror(result_code)));
    }

    return true;
}
