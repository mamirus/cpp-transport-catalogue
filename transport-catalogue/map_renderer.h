#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

namespace renderer {

    inline const double EPSILON = 1e-6;
    inline bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    class SphereProjector {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding);

        svg::Point operator()(geo::Coordinates coords) const {
            return {
                    (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    struct RenderSettings {
        double width_;
        double height_;
        double padding_;
        double line_width_;
        double stop_radius_;
        int bus_label_font_size_;
        svg::Point bus_label_offset_;
        int stop_label_font_size_;
        svg::Point stop_label_offset_;
        svg::Color underlayer_color_;
        double underlayer_width_;
        std::vector<svg::Color> color_palette_;
    };

    enum class UNDERLAYER_TYPE {
        STOP,
        BUS
    };

    class MapRenderer {
    public:
        explicit MapRenderer(RenderSettings& settings, std::vector<geo::Coordinates> coords, std::vector<const domain::Bus*> buses, std::vector<const domain::Stop*> stops)
        : settings_(settings),
        projector_(coords.begin(), coords.end(), settings.width_, settings.height_,settings.padding_),
        buses_(std::move(buses)),
        stops_(std::move(stops)) {}

        void Render(std::ostream& out) const;

    private:
        RenderSettings settings_;
        SphereProjector projector_;
        std::vector<const domain::Bus*> buses_;
        std::vector<const domain::Stop*> stops_;

        void AddBusesPolylines(svg::Document& doc) const;
        void AddPointsToPolyline(svg::Polyline& polyline, const domain::Bus* const bus) const;
        void AddBusesNames(svg::Document& doc) const;
        void AddStopsCircles(svg::Document& doc) const;
        void AddStopsNames(svg::Document& doc) const;
        svg::Text MakeUnderlayer(const domain::Stop* const stop, std::string_view name, UNDERLAYER_TYPE type) const;
        svg::Text MakeTextBusName(const domain::Stop* const stop, std::string_view name, int& color_count, int color_amount) const;
    };

    template <typename PointInputIt>
    SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
    double max_width, double max_height, double padding)
    : padding_(padding)
            {
                    if (points_begin == points_end) {
                        return;
                    }

                    const auto [left_it, right_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
                    min_lon_ = left_it->lng;
                    const double max_lon = right_it->lng;

                    const auto [bottom_it, top_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
                    const double min_lat = bottom_it->lat;
                    max_lat_ = top_it->lat;

                    std::optional<double> width_zoom;
                    if (!IsZero(max_lon - min_lon_)) {
                        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
                    }

                    std::optional<double> height_zoom;
                    if (!IsZero(max_lat_ - min_lat)) {
                        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
                    }

                    if (width_zoom && height_zoom) {
                        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
                    } else if (width_zoom) {
                        zoom_coeff_ = *width_zoom;
                    } else if (height_zoom) {
                        zoom_coeff_ = *height_zoom;
                    }
            }
}