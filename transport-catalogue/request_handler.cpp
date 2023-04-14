#include "request_handler.h"
#include "json_builder.h"

RequestHandler::OptionalBusInfo RequestHandler::GetBusStat(std::string_view bus_name) const {
    return transport_catalogue_.GetBusInfo(std::string (bus_name));
}

RequestHandler::OptionalStopInfo RequestHandler::GetBusesByStop(std::string_view stop_name) const {
    return transport_catalogue_.GetStopInfo(std::string (stop_name));
}

void RequestHandler::Render(std::ostream& out) const {
    renderer_.Render(out);
}

std::optional<transport_router::EdgeDescriptions> RequestHandler::BuildOptimalRoute(std::string_view stop_from, std::string_view stop_to) const {
    return router_.BuildRoute(stop_from, stop_to);
}

json::Array MakeBusesArray(domain::StopInfo& stop_info) {
    if (stop_info.buses_ == nullptr) return {};
    json::Array buses;
    buses.reserve(stop_info.buses_->size());
    for (auto bus : *stop_info.buses_) {
        buses.emplace_back(std::string(bus));
    }
    return buses;
}

json::Node MakeErrorResponse(const json::Dict* query) {
    return json::Builder{}.StartDict()
                              .Key("request_id").Value(query->at("id").GetValue())
                              .Key("error_message").Value("not found")
                          .EndDict()
                      .Build();
}

json::Node MakeJSONStopResponse(domain::StopInfo& stop_info, const json::Dict* query) {
    json::Array buses = MakeBusesArray(stop_info);
    return json::Builder{}.StartDict()
                                .Key("buses").Value(buses)
                                .Key("request_id").Value(query->at("id").GetValue())
                           .EndDict()
                       .Build();
}

json::Node MakeJSONBusResponse(domain::BusInfo& bus_info, const json::Dict* query) {
    return json::Builder{}.StartDict()
                                .Key("curvature").Value(bus_info.curvature_)
                                .Key("request_id").Value(query->at("id").GetValue())
                                .Key("route_length").Value(bus_info.route_length_road_)
                                .Key("stop_count").Value(bus_info.stops_on_route_)
                                .Key("unique_stop_count").Value(bus_info.unique_stops_)
                           .EndDict()
                       .Build();
}

json::Node MakeJSONMapResponse(const std::string& str, const json::Dict* query) {
    return json::Builder{}.StartDict()
                                .Key("map").Value(str)
                                .Key("request_id").Value(query->at("id").GetValue())
                          .EndDict()
                      .Build();
}

json::Node MakeJSONRouteResponse(const transport_router::EdgeDescriptions& route_description, const json::Dict* query) {
    json::Array items;
    double total_time = 0.0;
    for (auto description : route_description) {
        total_time += description.time_;
        if (description.type_ == transport_router::EdgeType::WAIT) {
            json::Node dict = json::Builder{}.StartDict()
                                                 .Key("type").Value("Wait")
                                                 .Key("stop_name").Value(std::string (description.edge_name_))
                                                 .Key("time").Value(description.time_)
                                             .EndDict()
                                         .Build();
            items.push_back(dict);
        } else if (description.type_ == transport_router::EdgeType::BUS) {
            json::Node dict = json::Builder{}.StartDict()
                                                 .Key("type").Value("Bus")
                                                 .Key("bus").Value(std::string (description.edge_name_))
                                                 .Key("span_count").Value(description.span_count_.value())
                                                 .Key("time").Value(description.time_)
                                             .EndDict()
                                         .Build();
            items.push_back(dict);
        }
    }
    return json::Builder{}.StartDict()
                                .Key("request_id").Value(query->at("id").GetValue())
                                .Key("total_time").Value(total_time)
                                .Key("items").Value(items)
                          .EndDict()
                      .Build();
}