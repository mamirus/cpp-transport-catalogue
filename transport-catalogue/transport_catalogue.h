#pragma once

#include <map>
#include <set>
#include <deque>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>

#include "geo.h"
#include "domain.h"
#include "graph.h"


namespace transport_catalogue {

const double MET_MIN_RATIO = 1000.00 / 60.00;



struct StopsPointers {
public:
    const Stop* stop;
    const Stop* other;

    size_t operator()(const StopsPointers& st_pair) const{
        return hasher_(st_pair.stop) + 43*hasher_(st_pair.other);
    }

    bool operator()(const StopsPointers& lhs, const StopsPointers& rhs) const{
        return lhs.stop == rhs.stop && lhs.other == rhs.other;
    }

private:
    std::hash<const Stop*> hasher_;

};


class TransportCatalogue {
public:
    TransportCatalogue() = default;
    void AddStop(const std::string& name, const geo::Coordinates coords);
    void AddStop(const Stop& stop);
    std::pair<bool, const Stop&> FindStop(const std::string_view name) const;
    bool AddBus(const BusRoute& bus_route);
    const BusRoute& FindBus(std::string_view name);
    BusInfo GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>& GetBusesForStop(std::string_view stop) const;
    bool SetDistanceBetweenStops(std::string_view stop, std::string_view other_stop, int dist);
    int GetDistanceBetweenStops(std::string_view stop, std::string_view other_stop) const;
    const std::map<std::string_view, const BusRoute*> GetAllRoutesIndex() const;
    const std::map<std::string_view, const Stop*> GetAllStopsIndex() const;
    const std::unordered_map<std::string_view, const Stop*>& RawStopsIndex() const;
    size_t GetNumberOfStopsOnAllRoutes() const;
    const std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers>& RawDistancesIndex() const;
    const std::unordered_map<std::string_view, std::set<std::string_view>>& GetStopAndBuses() const;

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;

    std::deque<BusRoute> bus_routes_;
    std::unordered_map<std::string_view, const BusRoute*> routes_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stop_and_buses_;
    std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> stops_distance_index_;
};


} // transport_catalogue namespace