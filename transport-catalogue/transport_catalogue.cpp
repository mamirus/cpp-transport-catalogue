#include "transport_catalogue.h"
#include "domain.h"
#include "geo.h"


namespace transport_catalogue {
    using namespace std::string_literals;

    void TransportCatalogue::AddStop(domain::Stop stop) {
        stops_.push_back(std::move(stop));
        stop_indexes_.insert({std::string_view(stops_.back().name_), &stops_.back()});
    }

    void TransportCatalogue::AddBus(domain::RawBus raw_bus) {
        std::vector<const domain::Stop*> stops_set;
        std::unordered_set<std::string_view> unique_stops;
        for (const std::string_view str : raw_bus.stops_) {
            stops_set.push_back(FindStop(str));
            unique_stops.insert(str);
        }
        buses_.push_back({std::move(raw_bus.name_), std::move(stops_set), unique_stops.size(), raw_bus.type_});
        buses_indexes_.insert({std::string_view(buses_.back().name_), &buses_.back()});
        for (const std::string_view str : unique_stops) {
            buses_through_the_stop_indexes_[FindStop(str)].insert(buses_.back().name_);
        }
    }

    void TransportCatalogue::AddStopsDistances(const std::pair<std::string, std::unordered_map<std::string, int>>& distances) {
        for (auto& [key , value] : distances.second) {
            distances_between_stops_.insert({{FindStop(distances.first), FindStop(key)}, value});
        }
    }

    const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
        if (buses_indexes_.count(name) == 0) return nullptr;
        return buses_indexes_.at(name);
    }

    const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
        if (stop_indexes_.count(name) == 0) return nullptr;
        return stop_indexes_.at(name);
    }

    std::optional<domain::BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
        domain::BusInfo info;
        const domain::Bus* bus = FindBus(name);
        if (bus == nullptr) return std::nullopt;
        info.name_ = bus->name_;
        info.unique_stops_ = static_cast<int>(bus->unique_stops_);
        if (bus->type_ == domain::BusType::REVERSE) {
            info.stops_on_route_ = static_cast<int>(bus->stops_.size()) * 2 - 1;
        } else if (bus->type_ == domain::BusType::CIRCULAR) {
            info.stops_on_route_ = static_cast<int>(bus->stops_.size());
        }
        info.route_length_road_ = ComputeRoadDistance(*bus);
        info.curvature_ =  info.route_length_road_ / ComputeGeographicalDistance(*bus);
        return info;
    }

    std::optional<domain::StopInfo> TransportCatalogue::GetStopInfo(std::string_view name) const {
        const domain::Stop* stop = FindStop(name);
        if (stop == nullptr) return std::nullopt;
        if (buses_through_the_stop_indexes_.count(stop) == 0) {
            domain::StopInfo empty{stop->name_, nullptr};
            return empty;
        }
        domain::StopInfo info{stop->name_, &buses_through_the_stop_indexes_.at(stop)};
        return info;
    }

    std::vector<const domain::Bus*> TransportCatalogue::GetSortedBuses() const {
        std::vector<const domain::Bus*> buses;
        for (auto [name, bus] : buses_indexes_) {
            if (!bus->stops_.empty()) {
                buses.push_back(bus);
            }
        }
        std::sort(buses.begin(), buses.end(), [](const domain::Bus* lhs, const domain::Bus* rhs){
            return lhs->name_ < rhs->name_;
        });
        return buses;
    }

    std::vector<const domain::Stop*> TransportCatalogue::GetSortedStops() const {
        std::vector<const domain::Stop*> stops;
        for (auto [stop, buses] : buses_through_the_stop_indexes_) {
            if (!buses.empty()) {
                stops.push_back(stop);
            }
        }
        std::sort(stops.begin(), stops.end(), [](const domain::Stop* lhs, const domain::Stop* rhs){
            return lhs->name_ < rhs->name_;
        });
        return stops;
    }

    std::vector<geo::Coordinates> TransportCatalogue::GetValidCoordinates() const {
        std::vector<geo::Coordinates> res;
        res.reserve(buses_through_the_stop_indexes_.size());
        for (auto [stop, buses] : buses_through_the_stop_indexes_) {
            if (!buses.empty()) {
                res.push_back({stop->latitude_, stop->longitude_});
            }
        }
        return res;
    }

    const std::unordered_map<std::string_view, const domain::Bus*>& TransportCatalogue::GetBusIndexes() const {
        return buses_indexes_;
    }

    void TransportCatalogue::SetArrayOfUsedStops() {
        for (auto& buses_through_stop : buses_through_the_stop_indexes_) {
            if (!buses_through_stop.second.empty()) {
                used_stops_cash_.push_back(buses_through_stop.first->name_);
            }
        }
    }

    size_t TransportCatalogue::GetAmountOfUsedStops() const {
        return used_stops_cash_.size();
    }

    const std::vector<std::string_view>& TransportCatalogue::GetUsedStopNames() const {
        return used_stops_cash_;
    }

    int TransportCatalogue::GetDistancesBetweenStops(const domain::Stop* stop_1, const domain::Stop* stop_2) const {
        if (distances_between_stops_.count({stop_1, stop_2}) > 0) {
            return distances_between_stops_.at({stop_1, stop_2});
        } else if (distances_between_stops_.count({stop_2, stop_1})) {
            return distances_between_stops_.at({stop_2, stop_1});
        } else {
            std::string error_message = "No any known distance between stops: "s
                                        .append(stop_1->name_)
                                        .append(" and ")
                                        .append(stop_2->name_)
                                        .append("\n");
            throw std::runtime_error(error_message);
        }
    }

    double TransportCatalogue::ComputeGeographicalDistance(const domain::Bus& bus) const {
        std::vector<geo::Coordinates> coordinates;
        std::vector<double> distances;
        for (const domain::Stop* stop: bus.stops_) {
            coordinates.push_back({stop->latitude_, stop->longitude_});
        }
        for (size_t i = 0; i < coordinates.size() - 1; i++) {
            distances.push_back(geo::ComputeDistance(coordinates[i], coordinates[i + 1]));
        }
        if (bus.type_ == domain::BusType::REVERSE) {
            return std::accumulate(distances.begin(), distances.end(), 0.0) * 2;
        } else {
            distances.push_back(geo::ComputeDistance(coordinates.back(), coordinates[0]));
            return std::accumulate(distances.begin(), distances.end(), 0.0);
        }
    }

    int TransportCatalogue::CountDistanceOnSegmentForward(const domain::Bus& bus, size_t finish) const {
        int distance = 0;
        for (size_t i = 0; i < finish; i++) {
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops(bus.stops_[i], bus.stops_[i + 1]);
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops_reverse(bus.stops_[i + 1], bus.stops_[i]);
            if (distances_between_stops_.count(pair_of_stops) > 0) {
                distance += distances_between_stops_.at(pair_of_stops);
            } else if (distances_between_stops_.count(pair_of_stops_reverse) > 0) {
                distance += distances_between_stops_.at(pair_of_stops_reverse);
            }
        }
        return distance;
    }

    int TransportCatalogue::CountDistanceOnSegmentBackward(const domain::Bus& bus, size_t start) const {
        int distance = 0;
        for (size_t i = start; i > 0; i--) {
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops(bus.stops_[i], bus.stops_[i - 1]);
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops_reverse(bus.stops_[i - 1], bus.stops_[i]);
            if (distances_between_stops_.count(pair_of_stops) > 0) {
                distance += distances_between_stops_.at(pair_of_stops);
            } else if (distances_between_stops_.count(pair_of_stops_reverse) > 0) {
                distance += distances_between_stops_.at(pair_of_stops_reverse);
            }
        }
        return distance;
    }

    int TransportCatalogue::ComputeRoadDistance(const domain::Bus& bus) const {
        int distance = 0;
        if (bus.type_ == domain::BusType::CIRCULAR) {
            distance += CountDistanceOnSegmentForward(bus, bus.stops_.size() - 1);
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops(bus.stops_.back(), bus.stops_[0]);
            std::pair<const domain::Stop*, const domain::Stop*> pair_of_stops_reverse(bus.stops_[0], bus.stops_.back());
            if (distances_between_stops_.count(pair_of_stops) > 0) {
                distance += distances_between_stops_.at(pair_of_stops);
            } else if (distances_between_stops_.count(pair_of_stops_reverse) > 0) {
                distance += distances_between_stops_.at(pair_of_stops_reverse);
            }
        } else {
            distance += CountDistanceOnSegmentForward(bus, bus.stops_.size() - 1);
            distance += CountDistanceOnSegmentBackward(bus,  bus.stops_.size() - 1);
        }
        return distance;
    }
}