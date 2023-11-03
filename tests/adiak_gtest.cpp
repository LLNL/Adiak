#include <gtest/gtest.h>

#include "adiak.hpp"
#include "adiak_tool.h"

#include <string>
#include <tuple>
#include <vector>


TEST(AdiakGeneral, CXX_BasicTypes)
{
    EXPECT_TRUE(adiak::value("cxx:int", -42));
    EXPECT_TRUE(adiak::value("cxx:uint", 42u));
    EXPECT_TRUE(adiak::value("cxx:int64", 9876543210ll));
    EXPECT_TRUE(adiak::value("cxx:const char*", "Hello Adiak"));
    EXPECT_TRUE(adiak::value("cxx:std::string", std::string("Hello Adiak")));
    EXPECT_TRUE(adiak::value("cxx:version", adiak::version("1.2.3a4")));
    EXPECT_TRUE(adiak::value("cxx:path", adiak::path("/a/b/c")));
    EXPECT_TRUE(adiak::value("cxx:catstring", adiak::catstring(std::string("cat"))));

    const int test_cat = 4242;
    EXPECT_TRUE(adiak::value("cxx:new_category", 42, test_cat, "subcat:cxx"));

    adiak_datatype_t* dtype = nullptr;
    adiak_value_t* val = nullptr;
    int cat = 0;
    const char* subcat = nullptr;
    adiak_value_t subval;
    adiak_datatype_t* subtype = nullptr;

    EXPECT_EQ(adiak_get_nameval("cxx:int", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(val->v_int, -42);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 0);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), -1);

    EXPECT_EQ(adiak_get_nameval("cxx:uint", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_uint);
    EXPECT_EQ(static_cast<unsigned int>(val->v_int), 42u);

    EXPECT_EQ(adiak_get_nameval("cxx:int64", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_longlong);
    EXPECT_EQ(val->v_longlong, 9876543210ll);

    EXPECT_EQ(adiak_get_nameval("cxx:const char*", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_string);
    EXPECT_STREQ(static_cast<char*>(val->v_ptr), "Hello Adiak");

    EXPECT_EQ(adiak_get_nameval("cxx:std::string", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_string);
    EXPECT_STREQ(static_cast<char*>(val->v_ptr), "Hello Adiak");

    EXPECT_EQ(adiak_get_nameval("cxx:version", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_version);
    EXPECT_STREQ(static_cast<char*>(val->v_ptr), "1.2.3a4");

    EXPECT_EQ(adiak_get_nameval("cxx:path", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_path);
    EXPECT_STREQ(static_cast<char*>(val->v_ptr), "/a/b/c");

    EXPECT_EQ(adiak_get_nameval("cxx:catstring", &dtype, &val, nullptr, nullptr), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_catstring);
    EXPECT_STREQ(static_cast<char*>(val->v_ptr), "cat");

    EXPECT_EQ(adiak_get_nameval("cxx:new_category", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(val->v_int, 42);
    EXPECT_EQ(cat, test_cat);
    EXPECT_STREQ(subcat, "subcat:cxx");
}

TEST(AdiakGeneral, CXX_CompoundTypes)
{
    EXPECT_TRUE(adiak::value("cxx:range:double", -1.0, 1.0));

    auto v_t = std::make_tuple(std::string("one"), 1ull);
    std::vector<int> v_ints { 1, 2, 3 };
    std::vector<std::tuple<const char*,int>> v_tuples {
        { "one", 1 }, { "two", 2 }, { "three", 3 }
    };

    EXPECT_TRUE(adiak::value("cxx:tuple", v_t));
    EXPECT_TRUE(adiak::value("cxx:int_vec", v_ints));
    EXPECT_TRUE(adiak::value("cxx:tpl_vec", v_tuples));

    adiak_datatype_t* dtype = nullptr;
    adiak_value_t* val = nullptr;
    int cat = 0;
    const char* subcat = nullptr;
    adiak_value_t subval;
    adiak_datatype_t* subtype = nullptr;

#if 0
    EXPECT_EQ(adiak_get_nameval("cxx:range:double", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_range);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 2);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_double);
    EXPECT_EQ(subval.v_double, -1.0);
    EXPECT_EQ(adiak_get_subval(dtype, val, 1, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_double);
    EXPECT_EQ(subval.v_double, 1.0);
#endif
    EXPECT_EQ(adiak_get_nameval("cxx:tuple", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_tuple);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 2);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_string);
    EXPECT_STREQ(static_cast<const char*>(subval.v_ptr), "one");
    EXPECT_EQ(adiak_get_subval(dtype, val, 1, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_ulonglong);
    EXPECT_EQ(static_cast<unsigned long long>(subval.v_longlong), 1ull);

    EXPECT_EQ(adiak_get_nameval("cxx:int_vec", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_list);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 3);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(subval.v_int, 1);
    EXPECT_EQ(adiak_get_subval(dtype, val, 2, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(subval.v_int, 3);

    EXPECT_EQ(adiak_get_nameval("cxx:tpl_vec", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_list);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 3);
    EXPECT_EQ(adiak_get_subval(dtype, val, 2, &subtype, &subval), 0);
    adiak_datatype_t *inner_subtype;
    adiak_value_t inner_subval;
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_tuple);
    EXPECT_EQ(adiak_num_subvals(subtype), 2);
    EXPECT_EQ(adiak_get_subval(subtype, &subval, 1, &inner_subtype, &inner_subval), 0);
    EXPECT_EQ(inner_subtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(inner_subval.v_int, 3);
}

TEST(AdiakGeneral, C_CompoundTypes)
{
    const double v_range[] = { -1.0, 1.0 };
    const int v_ints[] = { 1, 2, 3 };
    const struct tuple_t { const char* str; long long i; } v_tuples[3] = {
        { "one", 1ll }, { "two", 2ll }, { "three", 3ll }
    };

    EXPECT_EQ(adiak_namevalue("c:range:double", adiak_general, NULL, "<%f>", v_range), 0);
    EXPECT_EQ(adiak_namevalue("c:tuple", adiak_general, NULL, "(%s,%lld)", v_tuples, 2), 0);
    EXPECT_EQ(adiak_namevalue("c:vec:int", adiak_general, NULL, "{%d}", v_ints, 3), 0);
    EXPECT_EQ(adiak_namevalue("c:vec:tpl", adiak_general, NULL, "{(%s,%lld)}", v_tuples, 3, 2), 0);

    adiak_datatype_t* dtype = nullptr;
    adiak_value_t* val = nullptr;
    int cat = 0;
    const char* subcat = nullptr;
    adiak_value_t subval;
    adiak_datatype_t* subtype = nullptr;

    EXPECT_EQ(adiak_get_nameval("c:range:double", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_range);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 2);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_double);
    EXPECT_EQ(subval.v_double, -1.0);
    EXPECT_EQ(adiak_get_subval(dtype, val, 1, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_double);
    EXPECT_EQ(subval.v_double, 1.0);

    EXPECT_EQ(adiak_get_nameval("c:vec:int", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_list);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 3);
    EXPECT_EQ(adiak_get_subval(dtype, val, 0, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(subval.v_int, 1);
    EXPECT_EQ(adiak_get_subval(dtype, val, 2, &subtype, &subval), 0);
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_int);
    EXPECT_EQ(subval.v_int, 3);

    EXPECT_EQ(adiak_get_nameval("c:vec:tpl", &dtype, &val, &cat, &subcat), 0);
    EXPECT_EQ(dtype->dtype, adiak_type_t::adiak_list);
    EXPECT_EQ(cat, adiak_general);
    EXPECT_EQ(adiak_num_subvals(dtype), 3);
    EXPECT_EQ(adiak_get_subval(dtype, val, 2, &subtype, &subval), 0);
    adiak_datatype_t *inner_subtype;
    adiak_value_t inner_subval;
    EXPECT_EQ(subtype->dtype, adiak_type_t::adiak_tuple);
    EXPECT_EQ(adiak_num_subvals(subtype), 2);
    EXPECT_EQ(adiak_get_subval(subtype, &subval, 1, &inner_subtype, &inner_subval), 0);
    EXPECT_EQ(inner_subtype->dtype, adiak_type_t::adiak_longlong);
    EXPECT_EQ(inner_subval.v_longlong, 3ll);
}