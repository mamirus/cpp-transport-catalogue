#include "transport_catalogue.h"

#include <iomanip>
#include <numeric>
#include <utility>


namespace transport_catalogue {

    namespace stop_catalogue {

        using namespace detail;

        std::ostream& operator<<(std::ostream& out, const BusesToStopNames& buses) {
            bool first = true;
            for (const std::string_view& bus : buses) {
                if (!first) {
                    out << std::string(" ");
                }
                out << bus;
                first = false;
            }
            return out;
        }

        const Stop* Catalogue::Push(std::string&& name, std::string&& string_coord) {
            stops_.push_back({ std::move(name), Coordinates::ParseFromStringView(string_coord) });
            stop_buses_.insert({ &stops_.back(), {} });
            return &stops_.back();
        }

        void Catalogue::PushBusToStop(const Stop* stop, const std::string_view& bus_name) {
            stop_buses_.at(stop).insert(bus_name);
        }

        void Catalogue::AddDistance(const Stop* stop_1, const Stop* stop_2, double distance) {
            PointerPair<Stop> stop_pair_direct = { stop_1, stop_2 };
            PointerPair<Stop> stop_pair_reverse = { stop_2, stop_1 };

            distances_between_stops_[stop_pair_direct] = distance;

            if (distances_between_stops_.count(stop_pair_reverse) == 0) {
                distances_between_stops_[stop_pair_reverse] = distance;
            }
        }

    }
    namespace bus_catalogue {

        using namespace detail;
        using namespace stop_catalogue;
        Bus::Bus(std::string&& name, std::deque<const Stop*>&& rout, double rout_lenght, double rout_true_lenght, RoutType rout_type)
            : name(name)
            , rout(rout)
            , rout_type(rout_type)
            , rout_lenght(rout_lenght)
            , rout_true_lenght(rout_true_lenght)
            , stops_on_rout(rout.size()) {
            std::unordered_set<std::string_view> unique_stops_names;
            for (const Stop* stop : rout) {
                unique_stops_names.insert(stop->name);
            }
            unique_stops = unique_stops_names.size();

            if (rout_type == RoutType::BackAndForth) {
                stops_on_rout = stops_on_rout * 2 - 1;
            }
        }

        std::ostream& operator<<(std::ostream& out, const Bus& bus) {
            static const char* str_bus = "Bus ";
            static const char* str_sep = ": ";
            static const char* str_comma = ", ";
            static const char* str_space = " ";
            static const char* str_stops_on_rout = "stops on route";
            static const char* str_unique_stops = "unique stops";
            static const char* str_route_lenght = "route length";
            static const char* str_curvature = "curvature";

            double curvature = bus.rout_true_lenght / bus.rout_lenght;

            out << str_bus << bus.name << str_sep;
            out << bus.stops_on_rout << str_space << str_stops_on_rout << str_comma;
            out << bus.unique_stops << str_space << str_unique_stops << str_comma;
            out << std::setprecision(6) << bus.rout_true_lenght << str_space << str_route_lenght << str_comma;
            out << std::setprecision(6) << curvature << str_space << str_curvature;

            return out;
        }
        const Bus* Catalogue::Push(std::string&& name, std::vector<std::string_view>&& string_rout, RoutType type, const VirtualCatalogue<Stop>& stops_catalogue, const DistancesContainer& stops_distances) {
            std::deque<const Stop*> stops;
            for (const std::string_view& stop_name : string_rout) {
                auto [it, res] = stops_catalogue.At(stop_name);
                if (res) {
                    stops.push_back((*it).second);
                }
            }
            double rout_geo_lenght = CalcRoutGeoLenght(stops, type);
            double rout_true_lenght = CalcRoutTrueLenght(stops, stops_distances, type);
            buses_.push_back(Bus(std::move(name), std::move(stops), rout_geo_lenght, rout_true_lenght, type));
            return &buses_.back();
        }

        double Catalogue::CalcRoutGeoLenght(const std::deque<const Stop*>& rout, RoutType rout_type) {
            std::vector<double> distance(rout.size() - 1);
            std::transform(
                rout.begin(), rout.end() - 1,
                rout.begin() + 1, distance.begin(),
                [](const Stop* from, const Stop* to) {
                    return ComputeDistance(from->coord, to->coord);
                });
            double lenght = std::reduce(distance.begin(), distance.end());

            if (rout_type == RoutType::BackAndForth) {
                lenght *= 2.0;
            }

            return lenght;
        }

        double Catalogue::CalcRoutTrueLenght(const std::deque<const Stop*>& rout, const DistancesContainer& stops_distances, RoutType rout_type) {
            std::vector<double> distance(rout.size() - 1);
            std::transform(
                rout.begin(), rout.end() - 1,
                rout.begin() + 1, distance.begin(),
                [&stops_distances](const Stop* from, const Stop* to) {
                    return stops_distances.at(PointerPair<Stop>{ from, to });
                });
            double lenght = std::reduce(distance.begin(), distance.end());

            if (rout_type == RoutType::BackAndForth) {
                std::transform(
                    rout.rbegin(), rout.rend() - 1,
                    rout.rbegin() + 1, distance.begin(),
                    [&stops_distances](const Stop* from, const Stop* to) {
                        return stops_distances.at(PointerPair<Stop>{ from, to });
                    });
                lenght += std::reduce(distance.begin(), distance.end());
            }

            return lenght;
        }
    }
}

