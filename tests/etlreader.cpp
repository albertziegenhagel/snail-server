
#include <gtest/gtest.h>

#include <etl/etlreader.hpp>

TEST(EtlReader, Init)
{
    snail::etl_reader reader(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_sc.user_aux.etl)");
}