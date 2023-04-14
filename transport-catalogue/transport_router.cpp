#include "transport_router.h"

namespace transport_router {
    std::optional<EdgeDescriptions> TransportRouter::BuildRoute(std::string_view stop_from, std::string_view stop_to) const {
        EdgeDescriptions result;

        if (stop_from == stop_to) return result;
        if (pairs_of_vertices_for_each_stop.count(stop_from) == 0
            || pairs_of_vertices_for_each_stop.count(stop_to) == 0) return std::nullopt;

        graph::VertexId from_id = pairs_of_vertices_for_each_stop.at(stop_from).first;
        graph::VertexId to = pairs_of_vertices_for_each_stop.at(stop_to).first;
        std::optional<Router::RouteInfo> route = router_->BuildRoute(from_id, to);

        if (!route.has_value()) return std::nullopt;

        for (graph::VertexId id : route.value().edges) {
            result.push_back(edges_descriptions_[id]);
        }
        return result;
    }

    void TransportRouter::FillGraph() {
        AddWaitEdgesToGraph();
        for (auto [name, bus_ptr] : transport_catalogue_.GetBusIndexes()) {
            AddBusEdgesToGraph(bus_ptr->stops_.begin(), bus_ptr->stops_.end(), name);
            if (bus_ptr->type_ == domain::BusType::REVERSE) {
                AddBusEdgesToGraph(bus_ptr->stops_.rbegin(), bus_ptr->stops_.rend(), name);
            }
        }
    }

    void TransportRouter::AddWaitEdgesToGraph() {
        graph::VertexId from_id = 0;
        graph::VertexId to_id = 1;
        for (std::string_view name: transport_catalogue_.GetUsedStopNames()) {
            graph_->AddEdge({from_id, to_id, routing_settings_.bus_wait_time_});
            pairs_of_vertices_for_each_stop.insert({name, {from_id, to_id}});
            edges_descriptions_.push_back({
                EdgeType::WAIT,
                name,
                routing_settings_.bus_wait_time_,
                std::nullopt
            });
            from_id += 2;
            to_id += 2;
        }
    }
}