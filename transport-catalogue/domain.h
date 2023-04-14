#pragma once

#include <utility>
#include <string>
#include <vector>
#include <set>
#include <optional>

namespace domain {

    enum class BusType {
        REVERSE,
        CIRCULAR
    };

    struct Stop {
        Stop(std::string name, double lat, double lon)
                : name_(std::move(name)), latitude_(lat), longitude_(lon) {}

        std::string name_;
        double latitude_;
        double longitude_;
    };

    struct Bus {
        Bus(std::string name, std::vector<const Stop*> stops, size_t unique_stops, BusType type)
                : name_(std::move(name)), stops_(std::move(stops)), unique_stops_(unique_stops), type_(type) {}

        std::string name_;
        std::vector<const Stop*> stops_;
        size_t unique_stops_;
        BusType type_;
    };

    struct RawBus {
        std::string name_;
        std::vector<std::string> stops_;
        BusType type_;
    };

    struct BusInfo {
        std::string_view name_;
        int stops_on_route_;
        int unique_stops_;
        int route_length_road_;
        double curvature_;
    };

    struct StopInfo {
        std::string_view name_;
        const std::set<std::string_view>* buses_;
    };
}