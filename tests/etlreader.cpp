
#include <gtest/gtest.h>

#include <etl/etlreader.hpp>

TEST(EtlReader, Init)
{
    perfreader::etl_reader reader(R"(C:\Users\aziegenhagel\source\perfreader\tests\data\sc.user_aux.etl)");
}