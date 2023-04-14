#pragma once

#include "transport_catalogue.h"
#include "router.h"
#include "graph.h"
#include <memory>

namespace transport_router {
    struct RoutingSettings {
        double bus_wait_time_ = 0.0;
        double bus_velocity_ = 0.0;
    };

    enum class EdgeType {
        WAIT,
        BUS
    };

    struct EdgeDescription {
        EdgeType type_;
        std::string_view edge_name_;
        double time_ = 0.0;
        std::optional<int> span_count_ = 0;
    };

    using Router = graph::Router<double>;
    using Graph = graph::DirectedWeightedGraph<double>;
    using EdgeDescriptions = std::vector<EdgeDescription>;

    class TransportRouter {
    public:
        TransportRouter(RoutingSettings settings, const transport_catalogue::TransportCatalogue& transport_catalogue)
        : routing_settings_(settings),
          transport_catalogue_(transport_catalogue),
          graph_(std::make_unique<Graph>(transport_catalogue_.GetAmountOfUsedStops() * 2)),
          router_(nullptr)
        {
            FillGraph();
            router_ = std::make_unique<Router>(*graph_);
        }

        std::optional<EdgeDescriptions> BuildRoute(std::string_view stop_from, std::string_view stop_to) const;

    private:
        RoutingSettings routing_settings_;
        const transport_catalogue::TransportCatalogue& transport_catalogue_;
        std::unique_ptr<Graph> graph_;
        std::unique_ptr<Router> router_;
        std::unordered_map<std::string_view, std::pair<size_t, size_t>> pairs_of_vertices_for_each_stop;
        EdgeDescriptions edges_descriptions_;

        constexpr static const double METERS_PER_KM = 1000.0;
        constexpr static const double MIN_PER_HOUR = 60.0;

        void FillGraph();
        void AddWaitEdgesToGraph();

        template<typename InputIterator>
        void AddBusEdgesToGraph(InputIterator first, InputIterator last, std::string_view bus_name);
    };

    template<typename InputIterator>
    void TransportRouter::AddBusEdgesToGraph(InputIterator first, InputIterator last, std::string_view bus_name) {
        for (; std::distance(first, last) != 1; first++) {
            graph::VertexId from_id = pairs_of_vertices_for_each_stop.at((*first)->name_).second;
            const domain::Stop* from_stop = *first;
            double time = 0.0;
            InputIterator next_after_first = first;
            for (std::advance(next_after_first, 1); next_after_first != last; next_after_first++) {
                graph::VertexId to_id = pairs_of_vertices_for_each_stop.at((*next_after_first)->name_).first;
                time += transport_catalogue_.GetDistancesBetweenStops(from_stop, *next_after_first) / METERS_PER_KM / routing_settings_.bus_velocity_ * MIN_PER_HOUR;
                graph_->AddEdge({from_id, to_id, time});
                from_stop = *next_after_first;
                edges_descriptions_.push_back({
                        EdgeType::BUS,
                        bus_name,
                        time,
                        std::distance(first, next_after_first)
                });
            }
        }
    }
}