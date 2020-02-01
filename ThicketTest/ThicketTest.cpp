// ThicketTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#define BOOST_TEST_MODULE mytests
#include <boost/test/included/unit_test.hpp>
#include "../tree/tree.h"
#include "../tree/SelfLimitingPolygonTree.h"
#include <string>

BOOST_AUTO_TEST_CASE(my_boost_test)
{
    std::string expected_value = "Bill";

    std::ifstream infile("tree0399.settings.json");
    BOOST_CHECK(infile);

    json j;
    infile >> j;
    auto pTree = qtree::createTreeFromJson(j);

    BOOST_CHECK(pTree->transforms.size() == 5);
}
