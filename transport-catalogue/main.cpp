#include <iostream>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace transport_catalogue;

int main() {
    std::istream& input = std::cin;
    std::ostream& output = std::cout;
    output.precision(6);

    TransportCatalogue catalogue;

    int query_count = 0;

    query_count = ReadLineWithNumber(input);
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        std::string query = ReadLine(input);
        queries.push_back(query);
    }
    InputQueries(catalogue, queries);

    query_count = ReadLineWithNumber(input);
    for (int i = 0; i < query_count; ++i) {
        std::string query = ReadLine(input);
        OutputQuery(catalogue, query, output);
    }

}