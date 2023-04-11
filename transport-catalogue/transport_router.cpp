#include "transport_router.h"


TransportCatalogueGraph::TransportCatalogueGraph(const transport_catalogue::TransportCatalogue& tc, RoutingSettings rs):
        graph::DirectedWeightedGraph<double>(tc.RawStopsIndex().size()), tc_(tc), rs_(rs) {

    const auto& routes_index = tc_.GetAllRoutesIndex(); // Get all bus routes for all stops on routes

    // Register all stops that we have in catalogue as vertices
    for (const auto& [stop_name, stop_ptr] : tc_.RawStopsIndex()) {
        StopOnRoute stop {0, stop_name, {}};
        RegisterStop(stop);
    }

    // iterate for all routes
    for (const auto& [_, bus_route] : routes_index) {
        if (bus_route->type == transport_catalogue::RouteType::RETURN_ROUTE) {
            AddStopsOfReturnRoute(bus_route);
        } else {
            AddStopsOfCircleRoute(bus_route);
        }
    }
}

void TransportCatalogueGraph::AddStopsOfReturnRoute(const transport_catalogue::BusRoute *bus_route) {
    // iterate all stops in a route
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        size_t stop_distance = 1; // count between stops
        int accumulated_distance_direct = 0;
        int accumulated_distance_reverse = 0;

        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        auto from_id = GetStopVertexId((*start)->stop_name);
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            auto to_id = GetStopVertexId((*second)->stop_name);

            // make direct link and register Edge
            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_direct); // travel time between 2 stops
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            // make reverse link and register Edge
            TwoStopsLink reverse_link(bus_route->bus_name, to_id, from_id, stop_distance);
            const int reverse_distance = tc_.GetDistanceBetweenStops((*second)->stop_name, (*first)->stop_name);
            accumulated_distance_reverse += reverse_distance;
            const double reverse_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_reverse); // travel time between 2 stops
            const auto reverse_edge_id = AddEdge({to_id, from_id, reverse_link_time});
            StoreLink(reverse_link, reverse_edge_id);

            ++stop_distance;

            edge_count_ = reverse_edge_id;
        }
    }
}

void TransportCatalogueGraph::AddStopsOfCircleRoute(const transport_catalogue::BusRoute *bus_route) {
    // iterate all stops in a route
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        size_t stop_distance = 1; // count between stops
        int accumulated_distance_direct = 0;

        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        auto from_id = GetStopVertexId((*start)->stop_name);
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            auto to_id = GetStopVertexId((*second)->stop_name);

            // make direct link and register Edge
            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_direct); // travel time between 2 stops
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            ++stop_distance;

            edge_count_ = direct_edge_id;
        }
    }
}



graph::VertexId TransportCatalogueGraph::RegisterStop(const StopOnRoute& stop) {
    auto iter = stop_to_vertex_.find(stop);
    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, it already exists in the map
    }

    auto result = vertex_id_count_;

    stop_to_vertex_[stop] = vertex_id_count_;
    vertex_to_stop_[vertex_id_count_] = stop;
    ++vertex_id_count_;

    return result;
}

std::optional<graph::EdgeId> TransportCatalogueGraph::CheckLink(const TwoStopsLink &link) const {
    auto iter = stoplink_to_edge_.find(link);

    if (iter != stoplink_to_edge_.end()) {
        return iter->second; // return the stop vertex number, it already exists in the map
    }

    return {};
}

graph::EdgeId TransportCatalogueGraph::StoreLink(const TwoStopsLink &link, graph::EdgeId edge) {
    auto iter  = edge_to_stoplink_.find(edge);
    if (iter != edge_to_stoplink_.end()) {
        return iter->first;
    }

    stoplink_to_edge_[link] = edge;
    edge_to_stoplink_[edge] = link;

    return edge;
}

double TransportCatalogueGraph::CalculateTimeForDistance(int distance) const {
    return static_cast<double>(distance) / (rs_.bus_velocity * transport_catalogue::MET_MIN_RATIO);
}

graph::VertexId TransportCatalogueGraph::GetStopVertexId(std::string_view stop_name) const {
    StopOnRoute stop{0, stop_name, {}};
    auto iter = stop_to_vertex_.find(stop);

    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, if it exists in the map
    }

    throw std::logic_error("Error, no stop name: " + std::string (stop_name));
}

const TransportCatalogueGraph::StopOnRoute& TransportCatalogueGraph::GetStopById(graph::VertexId id) const {
    return vertex_to_stop_.at(id);
}

double TransportCatalogueGraph::GetBusWaitingTime() const {
    return static_cast<double>(rs_.bus_wait_time);
}

const TwoStopsLink& TransportCatalogueGraph::GetLinkById(graph::EdgeId id) const {
    auto iter = edge_to_stoplink_.find(id);

    if (iter != edge_to_stoplink_.end()) {
        return iter->second;
    }

    throw std::logic_error("Error fetching the TwoStopsLink, no Edge id: " + std::to_string(id));
}