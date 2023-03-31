#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "domain.h"
#include <vector>

const std::string BASE_DATA = "base_requests";
const std::string USER_REQUESTS = "stat_requests";
const std::string RENDER_SETTINGS = "render_settings";

struct BusRouteJson {
    std::string bus_name;
    transport_catalogue::RouteType type;
    std::vector<std::string> route_stops;
};


using BaseRequest = std::variant<std::monostate, transport_catalogue::StopWithDistances, BusRouteJson>;


class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& tc) : transport_catalogue_(tc) {
    }

    size_t ReadJson(std::istream& input);

    size_t ReadJsonToTransportCatalogue(std::istream& input);
    size_t QueryTcWriteJsonToStream(std::ostream& out);

    size_t ReadJsonQueryTcWriteJsonToStream(std::istream & input, std::ostream& out);

    [[nodiscard]] RendererSettings GetRendererSetting() const;


private:
    transport_catalogue::TransportCatalogue& transport_catalogue_;
    std::vector<json::Document> root_;
    std::vector<transport_catalogue::StopWithDistances> raw_stops_;
    std::vector<BusRouteJson> raw_buses_;

    BaseRequest ParseDataNode(const json::Node& node) const;
    size_t ParseJsonToRawData();
    bool FillTransportCatalogue();
    json::Node ProcessOneUserRequestNode(const json::Node& user_request);
    std::optional<geo::Coordinates> ParseCoordinates(const json::Dict& dict) const;
    BaseRequest ParseDataStop(const json::Dict& dict) const;
    BaseRequest ParseDataBus(const json::Dict& dict) const;
    json::Node GenerateMapNode(int id) const;
    json::Node GenerateBusNode(int id, std::string& name) const;
    json::Node GenerateStopNode(int id, std::string& name) const;
};

svg::Color ParseColor(const json::Node& node);
inline json::Node GetErrorNode(int id);