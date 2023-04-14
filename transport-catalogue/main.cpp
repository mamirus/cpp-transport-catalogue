#include "json_reader.h"
#include "request_handler.h"
#include <iostream>

int main() {
    transport_catalogue::TransportCatalogue transport_catalogue;
    auto doc{reader::ReadJSON(std::cin)};
    auto queries{reader::ParseJSON(doc)};
    reader::FillTransportCatalogue(transport_catalogue, queries.stops_queries_, queries.buses_queries_);
    transport_catalogue.SetArrayOfUsedStops();
    RequestHandler request_handler{transport_catalogue, queries.render_settings_, queries.routing_settings_};
    json::Document response{reader::ProcessStatRequests(request_handler, queries.stat_requests_)};
    json::Print(response, std::cout);
}