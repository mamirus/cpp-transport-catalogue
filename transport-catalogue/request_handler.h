#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_reader.h"
#include "svg.h"
#include <set>
#include <string_view>

using namespace transport_catalogue;

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& db, const MapRenderer& renderer, const JsonReader& reader)
        : db_(db), renderer_(renderer), json_reader_(reader) {
    }

    std::optional<BusInfo> GetBusStat(const std::string_view& bus_name) const;
    const std::set<std::string_view>& GetBusesByStop(const std::string_view& stop_name) const;
    void RenderMap(svg::Document& svg_map) const;

private:
    const TransportCatalogue& db_;
    const MapRenderer& renderer_;
    const JsonReader& json_reader_;
};