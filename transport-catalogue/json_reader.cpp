#include "json_reader.h"
#include "json_builder.h"



using namespace std::literals;

size_t JsonReader::ReadJson(std::istream &input) {
    size_t result = 0;

    try {
        json::Document doc = json::Load(input);
        if (doc.GetRoot().IsDict()) {
            result = doc.GetRoot().AsDict().size();
            root_.emplace_back(std::move(doc));
        }
    } catch (const json::ParsingError& e) {
        std::cerr << e.what() << std::endl;
    }

    return result;
}

size_t JsonReader::ReadJsonToTransportCatalogue(std::istream &input) {
    size_t check_num = ReadJson(input);
    if (check_num == 0) return check_num;

    size_t result = ParseJsonToRawData();

    FillTransportCatalogue();

    routing_settings_ = GetRoutingSettings();
    graph_ptr_ = std::make_unique<TransportCatalogueGraph>(transport_catalogue_, routing_settings_);
    router_ptr_ = std::make_unique<graph::Router<double>>(*graph_ptr_);


    return result;
}

size_t JsonReader::ParseJsonToRawData() {
    size_t result = 0;

    const json::Node& root_node = root_.back().GetRoot();
    if (!root_node.IsDict()) {
        throw json::ParsingError("Error reading JSON data for database filling.");
    }
    const json::Dict& dict = root_node.AsDict();
    auto iter = dict.find(BASE_DATA);
    if (iter == dict.end() || !(iter->second.IsArray()) ) {
        throw json::ParsingError("Error reading JSON data for database filling.");
    }

    const json::Array& nodes = iter->second.AsArray();
    for (const auto& node : nodes) {
        using namespace transport_catalogue;
        BaseRequest data = std::move(ParseDataNode(node));
        if (auto* stop = std::get_if<StopWithDistances>(&data)) {
            raw_stops_.emplace_back(std::move(*stop));
        } else if (auto* bus = std::get_if<BusRouteJson>(&data) ) {
            raw_buses_.emplace_back(std::move(*bus));
        } else {
            throw json::ParsingError("Error reading JSON data for database filling.");
        }
        ++result;
    }

    return result;
}

BaseRequest JsonReader::ParseDataStop(const json::Dict& dict) const {
    using namespace transport_catalogue;
    StopWithDistances stop;

    if (const auto name_i = dict.find("name"s); name_i != dict.end() && name_i->second.IsString()) {
        stop.stop_name = name_i->second.AsString();
    } else {
        return {};
    }

    if (const auto coords = ParseCoordinates(dict); coords ) {
        stop.coordinates = coords.value();
    } else {
        return {};
    }

    const auto dist_i = dict.find("road_distances"s);
    if (dist_i != dict.end() && !(dist_i->second.IsDict())) return {}; // проверка, что это словарь. он необязательный для остановки.
    for (const auto& [other_name, other_dist] : dist_i->second.AsDict()) {
        if (!other_dist.IsInt()) return {};
        stop.distances.emplace_back(StopDistanceData{other_name, static_cast<size_t>(other_dist.AsInt())});
    }
    return {stop};
}

BaseRequest JsonReader::ParseDataBus(const json::Dict& dict) const {
    using namespace transport_catalogue;
    BusRouteJson route;

    if (const auto name_i = dict.find("name"s); name_i != dict.end() && name_i->second.IsString()) {
        route.bus_name = name_i->second.AsString();
    } else {
        return {};
    }
    if (const auto route_i = dict.find("is_roundtrip"s); route_i != dict.end() && route_i->second.IsBool()) {
        route.type = route_i->second.AsBool() ? RouteType::CIRCLE_ROUTE : RouteType::RETURN_ROUTE;
    } else {
        return {};
    }

    const auto stops_i = dict.find("stops"s);
    if (stops_i != dict.end() && !(stops_i->second.IsArray())) return {};
    for (const auto& stop_name : stops_i->second.AsArray()) {
        if (!stop_name.IsString()) return {};
        route.route_stops.emplace_back(stop_name.AsString());
    }

    return {route};
}

BaseRequest JsonReader::ParseDataNode(const json::Node &node) const {
    using namespace transport_catalogue;
    if (!node.IsDict()) return {};

    const json::Dict& dict = node.AsDict();
    const auto type_i = dict.find("type"s);
    if (type_i == dict.end()) return {};

    if (type_i->second == json::Node {"Stop"s}) {
        return ParseDataStop(dict);
    } else if (type_i->second == json::Node{"Bus"s}) {
        return ParseDataBus(dict);
    } else {
        return {};
    }
}

std::optional<geo::Coordinates> JsonReader::ParseCoordinates(const json::Dict& dict) const {
    geo::Coordinates result;

    if (const auto lat_i = dict.find("latitude"s); lat_i != dict.end() && lat_i->second.IsDouble()) {
        result.lat = lat_i->second.AsDouble();
    } else {
        return {};
    }

    if (const auto lng_i = dict.find("longitude"s); lng_i != dict.end() && lng_i->second.IsDouble()) {
        result.lng = lng_i->second.AsDouble();
    } else {
        return {};
    }

    return {result};
}



bool JsonReader::FillTransportCatalogue() {

    // fill stops data to catalogue
    for (const auto& stop : raw_stops_) {
        transport_catalogue_.AddStop(stop);
    }

    // fill stop distances to catalogue
    for (const auto& stop : raw_stops_) {
        for (const auto& [other, distance] : stop.distances) {
            bool pairOK = transport_catalogue_.SetDistanceBetweenStops(stop.stop_name, other, distance);
            if ( !pairOK ) {
                std::cerr << "ERROR while adding distance to stop pair of "s << stop.stop_name << " and "s << other << "." << std::endl;
            }
        }
    }

    // fill bus routes data to catalogue
    for (auto& route : raw_buses_) {

        if (route.route_stops.size() < 2) {
            std::cerr << "Error while adding bus routes for bus: "s << route.bus_name << ". Number of stops must be at least 2." << std::endl;
            continue;
        }
        transport_catalogue::BusRoute br;
        br.bus_name = std::move(route.bus_name);
        br.type = route.type;
        for (auto& route_stop : route.route_stops) {
            br.route_stops.emplace_back( &(transport_catalogue_.FindStop(route_stop).second) );
        }

        transport_catalogue_.AddBus(br);
    }

    return true;
}


size_t JsonReader::QueryTcWriteJsonToStream(std::ostream &out) {
    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsDict()) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    const json::Dict& dict = root_node.AsDict();
    auto iter = dict.find(USER_REQUESTS);
    if (iter == dict.end() || !(iter->second.IsArray()) ) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    json::Builder builder;
    builder.StartArray();

    for (const json::Node& node : iter->second.AsArray()) {
        if (!node.IsDict()) {
            throw json::ParsingError("Error reading JSON data with user requests to database. One of nodes is not a dictionary.");
        }

        builder.Value(std::move(ProcessOneUserRequestNode(node)));
    }
    json::Node res_node = builder.EndArray().Build();
    json::PrintNode(res_node, out);

    return res_node.AsArray().size();
}

size_t JsonReader::ReadJsonQueryTcWriteJsonToStream(std::istream &input, std::ostream &out) {
    ReadJson(input);
    return QueryTcWriteJsonToStream(out);
}

json::Node JsonReader::ProcessOneUserRequestNode(const json::Node &user_request) {
    using namespace transport_catalogue;

    if (!user_request.IsDict()) {
        throw json::ParsingError("Error reading JSON data with user requests to database. One of nodes is not a dictionary.");
    }
    const json::Dict& request_fields = user_request.AsDict();

    int id = -1;
    if (const auto id_i = request_fields.find("id"s); id_i != request_fields.end() && id_i->second.IsInt()) {
        id = id_i->second.AsInt();
    } else {
        throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");
    }

    const auto type_i = request_fields.find("type"s);
    if ( type_i == request_fields.end() || !(type_i->second.IsString()) ){
        throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");
    }
    std::string type = type_i->second.AsString();

    if ( type == "Map"s) {
        return GenerateMapNode(id);
    }

    if ( type == "Route"s) {
        std::string from_stop, to_stop;
        if (const auto from_it = request_fields.find("from"s); from_it != request_fields.end() && from_it->second.IsString()) {
            from_stop = from_it->second.AsString();
        } else {
            throw json::ParsingError("Error reading JSON data with user requests to database. Route->from field is crippled.");
        }
        if (const auto to_it = request_fields.find("to"s); to_it != request_fields.end() && to_it->second.IsString()) {
            to_stop = to_it->second.AsString();
        } else {
            throw json::ParsingError("Error reading JSON data with user requests to database. Route->to field is crippled.");
        }

        return GenerateRouteNode(id, from_stop, to_stop);
    }

    std::string name;
    if (const auto name_i = request_fields.find("name"s); name_i != request_fields.end() && name_i->second.IsString()) {
        name = name_i->second.AsString();
    } else {
        throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");
    }

    if ( type == "Bus"s) {
        return GenerateBusNode(id, name);
    }

    if (type == "Stop"s) {
        return GenerateStopNode(id, name);
    }

    throw json::ParsingError("Error reading JSON data with user requests to database. Node's type field contains invalid data.");
}

json::Node JsonReader::GenerateMapNode(int id) const {
    RendererSettings rs = GetRendererSetting();
    MapRenderer mr(rs);

    std::ostringstream stream;
    mr.RenderSvgMap(transport_catalogue_, stream);

    return json::Builder().StartDict().Key("request_id"s).Value(id).Key("map"s).Value(std::move(stream.str())).EndDict().Build();
}

json::Node JsonReader::GenerateBusNode(int id, std::string& name) const {
    using namespace transport_catalogue;

    BusInfo bi = transport_catalogue_.GetBusInfo(name);
    if (bi.type == RouteType::NOT_SET) {
        return GetErrorNode(id);
    }

    return json::Builder().StartDict().Key("request_id"s).Value(id).Key("curvature"s).Value(bi.curvature)
            .Key("route_length"s).Value(static_cast<int>(bi.route_length)).Key("stop_count"s).Value(static_cast<int>(bi.stops_number))
            .Key("unique_stop_count"s).Value(static_cast<int>(bi.unique_stops)).EndDict().Build();
}

json::Node JsonReader::GenerateStopNode(int id, std::string& name) const {
    using namespace transport_catalogue;

    if ( ! transport_catalogue_.FindStop(name).first ) {
        return GetErrorNode(id);
    }
    json::Dict result;
    json::Array buses;
    json::Builder builder;
    builder.StartDict().Key("buses"s).StartArray();
    const std::set<std::string_view>& bus_routes = transport_catalogue_.GetBusesForStop(name);
    for (auto bus_route : bus_routes) {
        builder.Value(std::move(std::string{bus_route}));
    }
    builder.EndArray();
    builder.Key("request_id"s).Value(id).EndDict();

    return builder.Build();
}


RendererSettings JsonReader::GetRendererSetting() const {

    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsDict()) {
        throw json::ParsingError("Error reading JSON data with render settings.");
    }

    const json::Dict& dict = root_node.AsDict();
    auto iter = dict.find(RENDER_SETTINGS);
    if (iter == dict.end() || !(iter->second.IsDict()) ) {
        throw json::ParsingError("Error reading JSON data with render settings..");
    }

    RendererSettings settings;
    const json::Dict render_settings = iter->second.AsDict();
    if (const auto width_i = render_settings.find("width"s); width_i != render_settings.end() && width_i->second.IsDouble()) {
        settings.width = width_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, width data.");
    }

    if (const auto height_i = render_settings.find("height"s); height_i != render_settings.end() && height_i->second.IsDouble()) {
        settings.height = height_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, height data.");
    }

    if (const auto padd_i = render_settings.find("padding"s); padd_i != render_settings.end() && padd_i->second.IsDouble()) {
        settings.padding = padd_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, padding data.");
    }

    if (const auto field_iter = render_settings.find("line_width"s); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.line_width = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, line width data.");
    }

    if (const auto field_iter = render_settings.find("stop_radius"s); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.stop_radius = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop radius data.");
    }

    if (const auto field_iter = render_settings.find("bus_label_font_size"s); field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.bus_label_font_size = field_iter->second.AsInt();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, bus label font size data.");
    }

    if (const auto field_iter = render_settings.find("bus_label_offset"s); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2) throw json::ParsingError("Error while parsing renderer settings, bus label font offset data.");
        settings.bus_label_offset.x = arr[0].AsDouble();
        settings.bus_label_offset.y = arr[1].AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, bus label font offset data.");
    }

    if (const auto field_iter = render_settings.find("stop_label_font_size"s); field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.stop_label_font_size = field_iter->second.AsInt();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop label font size data.");
    }

    if (const auto field_iter = render_settings.find("stop_label_offset"s); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2) throw json::ParsingError("Error while parsing renderer settings, stop label font offset data.");
        settings.stop_label_offset.x = arr[0].AsDouble();
        settings.stop_label_offset.y = arr[1].AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop label font offset data.");
    }

    if (const auto field_iter = render_settings.find("underlayer_color"s); field_iter != render_settings.end() ) {
        svg::Color color = ParseColor(field_iter->second);
        if (std::holds_alternative<std::monostate>(color)) {
            throw json::ParsingError("Error while parsing renderer settings, underlayer color data.");
        }

        settings.underlayer_color = color;
    } else {
        throw json::ParsingError("Error while parsing renderer settings, underlayer color data.");
    }

    if (const auto field_iter = render_settings.find("underlayer_width"s); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.underlayer_width = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, underlayer width data.");
    }

    if (const auto field_iter = render_settings.find("color_palette"s); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        for (const auto& color_node : arr) {
            svg::Color color = ParseColor(color_node);
            if (std::holds_alternative<std::monostate>(color)) {
                throw json::ParsingError("Error while parsing renderer settings, color palette data.");
            }
            settings.color_palette.emplace_back(color);
        }
    } else {
        throw json::ParsingError("Error while parsing renderer settings, color palette data.");
    }

    return settings;
}

RoutingSettings JsonReader::GetRoutingSettings() const {
    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsDict()) {
        throw json::ParsingError("Error reading JSON data with routing settings.");
    }

    const json::Dict& root_dict = root_node.AsDict();
    auto iter = root_dict.find(ROUTING_SETTINGS);
    if (iter == root_dict.end() || !(iter->second.IsDict()) ) {
        throw json::ParsingError("Error reading JSON data with routing settings..");
    }

    RoutingSettings settings {};
    const json::Dict& routing_settings = iter->second.AsDict();
    if (const auto& bus_wait = routing_settings.find("bus_wait_time"); bus_wait != routing_settings.end() && bus_wait->second.IsInt()) {
        settings.bus_wait_time = bus_wait->second.AsInt();
    } else {
        throw json::ParsingError("Error while parsing routing settings, bus wait time data.");
    }

    if (const auto& bus_velocity = routing_settings.find("bus_velocity"); bus_velocity != routing_settings.end() && bus_velocity->second.IsDouble()) {
        settings.bus_velocity = bus_velocity->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing routing settings, bus velocity data.");
    }

    return settings;
}

json::Node JsonReader::GenerateRouteNode(int id, std::string_view from, std::string_view to) const {
    const auto& [found_from, from_stop] = transport_catalogue_.FindStop(from);
    const auto& [found_to, to_stop] = transport_catalogue_.FindStop(to);
    if (!found_from || !found_to) {
        throw json::ParsingError("Error while parsing routing request, stops not found.");
    }
    graph::VertexId from_id = graph_ptr_->GetStopVertexId(from);
    graph::VertexId to_id = graph_ptr_->GetStopVertexId(to);

    auto route = router_ptr_->BuildRoute(from_id, to_id);
    if (!route) {
        return GetErrorNode(id);
    }

    json::Builder builder;
    builder.StartDict().Key("request_id"s).Value(id).Key("total_time"s).Value(route->weight).Key("items"s).StartArray();


    double waiting_time = graph_ptr_->GetBusWaitingTime();
    // int bus_count = 0;

    for (const auto& edge_id : route->edges) {
        const graph::Edge<double>& edge = graph_ptr_->GetEdge(edge_id);

        auto link = graph_ptr_->GetLinkById(edge_id);
        const auto& stop_from = graph_ptr_->GetStopById(edge.from);
        //const auto& stop_to = graph_ptr_->GetStopById(edge.to);

        json::Builder wait_builder;
        wait_builder.StartDict().Key("type"s).Value("Wait"s)
        .Key("stop_name"s).Value(std::string{stop_from.stop_name})
        .Key("time"s).Value(waiting_time).EndDict();
        builder.Value(std::move(wait_builder.Build()));


        json::Builder bus_builder;
        double time = edge.weight - waiting_time;
        bus_builder.StartDict().Key("type"s).Value("Bus"s).Key("bus"s).Value(std::string{link.bus_name})
                .Key("span_count"s).Value(static_cast<int>(link.number_of_stops)).Key("time"s).Value(time).EndDict();
        builder.Value(std::move(bus_builder.Build()));
    }
    builder.EndArray().EndDict();

    return builder.Build();
}

svg::Color ParseColor(const json::Node& node){
    if (node.IsString()) {
        return {node.AsString()};
    }

    if (node.IsArray()) {
        json::Array arr = node.AsArray();
        if (arr.size() == 3) {
            uint8_t red = arr[0].AsInt();
            uint8_t green = arr[1].AsInt();
            uint8_t blue = arr[2].AsInt();

            return { svg::Rgb(red, green, blue) };
        }
        if (arr.size() == 4) {
            uint8_t red = arr[0].AsInt();
            uint8_t green = arr[1].AsInt();
            uint8_t blue = arr[2].AsInt();
            double opacity = arr[3].AsDouble();

            return { svg::Rgba(red, green, blue, opacity) };
        }
    }

    return {};
}


inline json::Node GetErrorNode(int id) {
    return json::Builder().StartDict().Key("request_id"s).Value(id)
                .Key("error_message"s).Value("not found"s).EndDict()
                .Build();
}