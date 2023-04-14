#include "json_reader.h"
#include <stdexcept>

namespace reader {

    void ParseBaseRequests(const json::Node& data, SortedJSONQueries& queries) {
        auto& requests = data.AsArray();
        for (auto& node : requests) {
            auto& req_json = node.AsDict();
            if (req_json.at("type").AsString() == "Stop") {
                queries.stops_queries_.insert(&req_json);
            } else if (req_json.at("type").AsString() == "Bus") {
                queries.buses_queries_.insert(&req_json);
            }
        }
    }

    void ParseStatRequests(const json::Node& data, SortedJSONQueries& queries) {
        auto& requests = data.AsArray();
        for (auto& node : requests) {
            queries.stat_requests_.push_back(&node.AsDict());
        }
    }

    svg::Point MakeOffset(const json::Array& values) {
        if (values.size() != 2) throw std::logic_error("Incorrect format of points");
        return {values[0].AsDouble(), values[1].AsDouble()};
    }

    svg::Color MakeColorForSVG(const json::Node& node) {
        if (node.IsString()) {
            return {node.AsString()};
        } else if (node.IsArray() && node.AsArray().size() == 3) {
            const auto& arr = node.AsArray();
            return svg::Rgb{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt())};
        } else if (node.IsArray() && node.AsArray().size() == 4) {
            const auto& arr = node.AsArray();
            return svg::Rgba{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()), arr[3].AsDouble()};
        } else {
            return {std::monostate()};
        }
    }

    std::vector<svg::Color> MakeArrayOfColors(const json::Array& array) {
        std::vector<svg::Color> res;
        for (auto& elem : array) {
            res.push_back(MakeColorForSVG(elem));
        }
        return res;
    }

    void ParseRenderSettings(const json::Node& data, SortedJSONQueries& queries) {
        auto& settings = data.AsDict();
        renderer::RenderSettings set;
        set.width_ = settings.at("width").AsDouble();
        set.height_ = settings.at("height").AsDouble();
        set.padding_ = settings.at("padding").AsDouble();
        set.line_width_ = settings.at("line_width").AsDouble();
        set.stop_radius_ = settings.at("stop_radius").AsDouble();
        set.bus_label_font_size_ = settings.at("bus_label_font_size").AsInt();
        set.bus_label_offset_ = MakeOffset(settings.at("bus_label_offset").AsArray());
        set.stop_label_font_size_ = settings.at("stop_label_font_size").AsInt();
        set.stop_label_offset_ = MakeOffset(settings.at("stop_label_offset").AsArray());
        set.underlayer_color_ = MakeColorForSVG(settings.at("underlayer_color"));
        set.underlayer_width_ = settings.at("underlayer_width").AsDouble();
        set.color_palette_ = MakeArrayOfColors(settings.at("color_palette").AsArray());
        queries.render_settings_ = std::move(set);
    }

    void ParseRoutingSettings(const json::Node& data, SortedJSONQueries& queries) {
        auto& settings = data.AsDict();
        transport_router::RoutingSettings set{
            settings.at("bus_wait_time").AsDouble(),
            settings.at("bus_velocity").AsDouble()
        };
        queries.routing_settings_ = set;
    }

    json::Document ReadJSON(std::istream& input) {
        return json::Load(input);
    }

    SortedJSONQueries ParseJSON(json::Document& doc) {
        SortedJSONQueries queries;
        for (auto& [query, data] : doc.GetRoot().AsDict()) {
            if (query == "base_requests"s) {
                ParseBaseRequests(data, queries);
            } else if (query == "stat_requests"s) {
                ParseStatRequests(data, queries);
            } else if (query == "render_settings") {
                ParseRenderSettings(data, queries);
            } else if (query == "routing_settings") {
                ParseRoutingSettings(data, queries);
            }
        }
        return queries;
    }

    void AddStopsFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries) {
        for (const json::Dict* query : queries) {
            const std::string stop_name = query->at("name").AsString();
            transport_catalogue.AddStop({
                               stop_name,
                               query->at("latitude").AsDouble(),
                               query->at("longitude").AsDouble(),
                       });
        }
    }

    void AddStopsDistancesFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries) {
        for (const json::Dict* query : queries) {
            std::unordered_map<std::string, int> distances;
            for (auto& [key, value] : query->at("road_distances").AsDict()) {
                distances.insert({key, value.AsInt()});
            }
            transport_catalogue.AddStopsDistances({query->at("name").AsString(), distances});
        }
    }

    void AddBusesFromJSON(transport_catalogue::TransportCatalogue& transport_catalogue, std::unordered_set<const json::Dict*>& queries) {
        for (const json::Dict* query : queries) {
            std::vector<std::string> stops;
            stops.reserve(query->at("stops").AsArray().size());
            for (auto& stop_node : query->at("stops").AsArray()) {
                stops.push_back(stop_node.AsString());
            }
            transport_catalogue.AddBus({
                              query->at("name").AsString(),
                              stops,
                              query->at("is_roundtrip").AsBool() ? domain::BusType::CIRCULAR : domain::BusType::REVERSE
                      });
        }
    }

    void FillTransportCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue,
                std::unordered_set<const json::Dict*>& stop_queries,
                std::unordered_set<const json::Dict*>& bus_queries)
    {
        AddStopsFromJSON(transport_catalogue, stop_queries);
        AddStopsDistancesFromJSON(transport_catalogue, stop_queries);
        AddBusesFromJSON(transport_catalogue, bus_queries);
    }

    json::Node ProcessStopQuery(RequestHandler& request_handler, const json::Dict* query) {
        auto stop_info = request_handler.GetBusesByStop(query->at("name").AsString());
        if (!stop_info.has_value()) {
            return MakeErrorResponse(query);
        } else {
            return MakeJSONStopResponse(stop_info.value(), query);
        }
    }

    json::Node ProcessBusQuery(RequestHandler& request_handler, const json::Dict* query) {
        auto bus_info = request_handler.GetBusStat(query->at("name").AsString());
        if (!bus_info.has_value()) {
            return MakeErrorResponse(query);
        } else {
            return MakeJSONBusResponse(bus_info.value(), query);
        }
    }

    json::Node ProcessMapQuery(RequestHandler& request_handler, const json::Dict* query) {
        std::ostringstream str;
        request_handler.Render(str);
        return MakeJSONMapResponse(str.str(), query);
    }

    json::Node ProcessRouteQuery(RequestHandler& request_handler, const json::Dict* query) {
        auto route_description =
                request_handler.BuildOptimalRoute(query->at("from").AsString(), query->at("to").AsString());

        if (!route_description.has_value()) {
            return MakeErrorResponse(query);
        } else {
            return MakeJSONRouteResponse(route_description.value(), query);
        }
    }

    json::Document ProcessStatRequests(RequestHandler& request_handler, std::vector<const json::Dict*>& stat_queries) {
        json::Array array;
        for (auto query : stat_queries) {
            if (query->at("type").AsString() == "Stop") {
                array.push_back(ProcessStopQuery(request_handler, query));
            } else if (query->at("type").AsString() == "Bus") {
                array.push_back(ProcessBusQuery(request_handler, query));
            } else if (query->at("type").AsString() == "Map") {
                array.push_back(ProcessMapQuery(request_handler, query));
            } else if (query->at("type").AsString() == "Route") {
                array.push_back(ProcessRouteQuery(request_handler, query));
            }
        }
        json::Document doc(array);
        return doc;
    }
}