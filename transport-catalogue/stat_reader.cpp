#include "stat_reader.h"
#include <algorithm>
using namespace transport_catalogue;

void BusOutputQuery(const TransportCatalogue& transport_catalogue, std::string&& bus_name, std::ostream& out) {

    auto [it, bus_has_been_found] = transport_catalogue.GetBus(bus_name);

    if (bus_has_been_found) {
        out << *(*it).second;
    }
    else {
        out << std::string("Bus ") << bus_name << std::string(": not found");
    }
    out << "\n";
}

void StopOutputQuery(const TransportCatalogue& transport_catalogue, std::string&& stop_name, std::ostream& out) {
    using namespace transport_catalogue::stop_catalogue;
    auto [buses, stop_has_been_found] = transport_catalogue.GetBusesForStop(stop_name);

    if (stop_has_been_found) {
        if (!buses.empty()) {
            out << std::string("Stop ") << stop_name << std::string(": buses ") << buses;
        }
        else {
            out << std::string("Stop ") << stop_name << std::string(": no buses");
        }
    }
    else {
        out << std::string("Stop ") << stop_name << std::string(": not found");
    }
    out << "\n";
}

void OutputQuery(const TransportCatalogue& transport_catalogue, const std::string& query, std::ostream& out) {
    size_t space = query.find_first_of(' ');
    std::string query_type = query.substr(0, space);
    std::string name = query.substr(space + 1);

    static const std::string bus_query = std::string("Bus");
    static const std::string stop_query = std::string("Stop");

    if (query_type == bus_query) {
        BusOutputQuery(transport_catalogue, std::move(name), out);
    }
    else if (query_type == stop_query) {
        StopOutputQuery(transport_catalogue, std::move(name), out);
    }
}