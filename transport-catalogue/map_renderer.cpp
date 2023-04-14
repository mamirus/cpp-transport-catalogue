#include "map_renderer.h"
#include <stdexcept>

namespace renderer {

    void MapRenderer::Render(std::ostream& out) const {
        svg::Document doc;
        AddBusesPolylines(doc);
        AddBusesNames(doc);
        AddStopsCircles(doc);
        AddStopsNames(doc);
        doc.Render(out);
    }

    void MapRenderer::AddBusesPolylines(svg::Document& doc) const {
        static int color_count_x = settings_.color_palette_.size();
        int color_amount = color_count_x;
        for (const auto bus : buses_) {
            if (bus == nullptr) throw std::invalid_argument("Vector of buses pointers has null pointer(s)");
            if (bus->stops_.empty()) continue;
            svg::Polyline polyline;
            AddPointsToPolyline(polyline, bus);
            polyline.SetFillColor("none")
                    .SetStrokeColor(settings_.color_palette_[color_count_x % color_amount])
                    .SetStrokeWidth(settings_.line_width_)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            color_count_x++;
            doc.Add(polyline);
        }
    }

    void MapRenderer::AddPointsToPolyline(svg::Polyline& polyline, const domain::Bus* const bus) const {
        for (auto stop : bus->stops_) {
            const svg::Point point = projector_({stop->latitude_, stop->longitude_});
            polyline.AddPoint(point);
        }
        if (bus->type_ == domain::BusType::REVERSE) {
            for (int i = bus->stops_.size() - 2; i >= 0; i--) {
                const svg::Point point = projector_({ bus->stops_[i]->latitude_, bus->stops_[i]->longitude_ });
                polyline.AddPoint(point);
            }
        }
    }

    void MapRenderer::AddBusesNames(svg::Document& doc) const {
        static int color_count = settings_.color_palette_.size();
        int color_amount = color_count;
        for (const auto bus : buses_) {
            if (bus == nullptr) throw std::invalid_argument("Vector of buses pointers has null pointer(s)");
            if (bus->stops_.empty()) continue;
            svg::Text underlayer = MakeUnderlayer(bus->stops_.front(), bus->name_, UNDERLAYER_TYPE::BUS);
            doc.Add(underlayer);
            svg::Text text = MakeTextBusName(bus->stops_.front(), bus->name_, color_count, color_amount);
            doc.Add(text);
            if (bus->type_ == domain::BusType::REVERSE && bus->stops_.front() != bus->stops_.back()) {
                svg::Text underlayer = MakeUnderlayer(bus->stops_.back(), bus->name_, UNDERLAYER_TYPE::BUS);
                doc.Add(underlayer);
                svg::Text text = MakeTextBusName(bus->stops_.back(), bus->name_, color_count, color_amount);
                doc.Add(text);
            }
            color_count++;
        }
    }

    void MapRenderer::AddStopsCircles(svg::Document& doc) const {
        for (const auto stop : stops_) {
            if (stop == nullptr) throw std::invalid_argument("Vector of stops pointers has null pointer(s)");
            svg::Circle circle;
            circle.SetCenter(projector_({stop->latitude_, stop->longitude_}))
                  .SetRadius(settings_.stop_radius_)
                  .SetFillColor("white");
            doc.Add(circle);
        }
    }

    void MapRenderer::AddStopsNames(svg::Document& doc) const {
        for (const auto stop : stops_) {
            if (stop == nullptr) throw std::invalid_argument("Vector of stops pointers has null pointer(s)");
            svg::Text underlayer = MakeUnderlayer(stop, stop->name_, UNDERLAYER_TYPE::STOP);
            doc.Add(underlayer);
            svg::Text text;
            text.SetFillColor("black")
                .SetPosition(projector_({stop->latitude_, stop->longitude_}))
                .SetOffset(settings_.stop_label_offset_)
                .SetFontSize(settings_.stop_label_font_size_)
                .SetFontFamily("Verdana")
                .SetData(std::string{stop->name_});
            doc.Add(text);
        }
    }

    svg::Text MapRenderer::MakeUnderlayer(const domain::Stop* const stop, std::string_view name, UNDERLAYER_TYPE type) const {
        svg::Text underlayer;
        underlayer.SetFillColor(settings_.underlayer_color_)
                .SetStrokeColor(settings_.underlayer_color_)
                .SetStrokeWidth(settings_.underlayer_width_)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                .SetPosition(projector_({stop->latitude_, stop->longitude_}))
                .SetOffset(type == UNDERLAYER_TYPE::BUS ? settings_.bus_label_offset_ : settings_.stop_label_offset_)
                .SetFontSize(type == UNDERLAYER_TYPE::BUS ? settings_.bus_label_font_size_ : settings_.stop_label_font_size_)
                .SetFontFamily("Verdana")
                .SetFontWeight(type == UNDERLAYER_TYPE::BUS ? "bold" : "")
                .SetData(std::string{name});
        return underlayer;
    }

    svg::Text MapRenderer::MakeTextBusName(const domain::Stop* const stop, std::string_view name, int& color_count, int color_amount) const {
        svg::Text text;
        text.SetFillColor(settings_.color_palette_[color_count % color_amount])
            .SetPosition(projector_({stop->latitude_, stop->longitude_}))
            .SetOffset(settings_.bus_label_offset_)
            .SetFontSize(settings_.bus_label_font_size_)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(std::string{name});
        return text;
    }
}