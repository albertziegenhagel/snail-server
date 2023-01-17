
#include <snail/perf_data/detail/metadata.hpp>

#include <snail/perf_data/detail/attributes_database.hpp>

using namespace snail::perf_data::detail;

void perf_data_metadata::extract_event_attributes_database(event_attributes_database& database)
{
    database.all_attributes.clear();
    database.id_to_attributes.clear();
    database.all_attributes.reserve(event_desc.size());
    for(const auto& data : event_desc)
    {
        database.all_attributes.push_back(data.attribute);
        for(const auto id : data.ids)
        {
            database.id_to_attributes[id] = &database.all_attributes.back();
        }
    }
    database.validate();
}
