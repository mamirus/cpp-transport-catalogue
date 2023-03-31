#include "request_handler.h"

std::optional<BusInfo> RequestHandler::GetBusStat(const std::string_view &bus_name) const {
    BusInfo bi = db_.GetBusInfo(bus_name);

    if (bi.type == RouteType::NOT_SET) {
        return {};
    }

    return {bi};
}

const std::set<std::string_view> &RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
    return db_.GetBusesForStop(stop_name);
}

void RequestHandler::RenderMap(svg::Document& svg_doc) const {
    RendererSettings rs = json_reader_.GetRendererSetting();
    MapRenderer mr(rs);
    mr.RenderSvgMap(db_, svg_doc);
}


