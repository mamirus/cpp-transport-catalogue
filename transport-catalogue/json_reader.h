#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <vector>
#include <unordered_set>
#include <utility>

namespace reader {
    using namespace std::string_literals;

    struct SortedJSONQueries {
        std::unordered_set<const json::Dict*> stops_queries_;
        std::unordered_set<const json::Dict*> buses_queries_;
        transport_router::RoutingSettings routing_settings_;
        std::vector<const json::Dict*> stat_requests_;
        renderer::RenderSettings render_settings_;
    };

    json::Document ReadJSON(std::istream& input);

    void ParseBaseRequests(const json::Node& data, SortedJSONQueries& queries);
    void ParseStatRequests(const json::Node& data, SortedJSONQueries& queries);
    void ParseRenderSettings(const json::Node& data, SortedJSONQueries& queries);
    void ParseRoutingSettings(const json::Node& data, SortedJSONQueries& queries);

    SortedJSONQueries ParseJSON(json::Document& doc);

    void AddStopsFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries);
    void AddStopsDistancesFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries);
    void AddBusesFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries);

    svg::Point MakeOffset(const json::Array& values);
    svg::Color MakeColorForSVG(const json::Node& node);
    std::vector<svg::Color> MakeArrayOfColors(const json::Array& array);

    void FillTransportCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue,
                                std::unordered_set<const json::Dict*>& stop_queries,
                                std::unordered_set<const json::Dict*>& bus_queries);

    json::Node ProcessStopQuery(RequestHandler& request_handler, const json::Dict* query);
    json::Node ProcessBusQuery(RequestHandler& request_handler, const json::Dict* query);
    json::Node ProcessRouteQuery(RequestHandler& request_handler, const json::Dict* query);
    json::Document ProcessStatRequests(RequestHandler& request_handler, std::vector<const json::Dict*>& stat_queries);
}