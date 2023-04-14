#pragma once

#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"
#include "json.h"

class RequestHandler {
public:
    using OptionalBusInfo = const std::optional<domain::BusInfo>;
    using OptionalStopInfo = const std::optional<domain::StopInfo>;
    
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& transport_catalogue,
                            renderer::RenderSettings& render_settings,
                            transport_router::RoutingSettings routing_settings)
    : transport_catalogue_(transport_catalogue),
      renderer_(render_settings, std::move(transport_catalogue.GetValidCoordinates()),
                             transport_catalogue.GetSortedBuses(),
                             transport_catalogue.GetSortedStops()),
      router_(routing_settings, transport_catalogue_) {}

    [[nodiscard]] OptionalBusInfo GetBusStat(std::string_view bus_name) const;
    [[nodiscard]] OptionalStopInfo GetBusesByStop(std::string_view stop_name) const;
    void Render(std::ostream& out) const;
    std::optional<transport_router::EdgeDescriptions> BuildOptimalRoute(std::string_view stop_from, std::string_view stop_to) const;

private:
    const transport_catalogue::TransportCatalogue& transport_catalogue_;
    renderer::MapRenderer renderer_;
    transport_router::TransportRouter router_;
};

json::Array MakeBusesArray(domain::StopInfo& stop_info);
json::Node MakeErrorResponse(const json::Dict* query);
json::Node MakeJSONStopResponse(domain::StopInfo& stop_info, const json::Dict* query);
json::Node MakeJSONBusResponse(domain::BusInfo& bus_info, const json::Dict* query);
json::Node MakeJSONMapResponse(const std::string& str, const json::Dict* query);
json::Node MakeJSONRouteResponse(const transport_router::EdgeDescriptions& route_description, const json::Dict* query);