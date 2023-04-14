#include "json_builder.h"

namespace json {

    Builder& Builder::Key(std::string key) {
        if (root_ != nullptr) throw std::logic_error("calling Key-method for ready object");
        if (nodes_stack_.back()->IsDict()) {
            Node::Value str{std::move(key)};
            nodes_.emplace_back(std::move(str));
            nodes_stack_.push_back(&nodes_.back());
        } else {
            throw std::logic_error("calling Key-method in wrong place");
        }
        return *this;
    }

    Builder& Builder::Value(Node::Value value) {
        if (root_ != nullptr) throw std::logic_error("calling Value-method for ready object");
        if (root_ == nullptr && nodes_stack_.empty()) {
            root_ = std::move(value);
        } else if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(std::move(value));
        } else if (nodes_stack_.back()->IsString()) {
            Node& node_ref =*nodes_stack_.back();
            nodes_stack_.pop_back();
            nodes_stack_.back()->AsDict().insert({node_ref.AsString(), std::move(value)});
        } else {
            throw std::logic_error("calling Value-method in wrong place");
        }
        return *this;
    }

    Builder::DictItemContext Builder::StartDict() {
        StartData(Dict{});
        return {*this};
    }

    Builder::ArrayItemContext Builder::StartArray() {
        StartData(Array{});
        return {*this};
    }

    Builder& Builder::EndDict() {
        if (nodes_stack_.empty()) throw std::logic_error("calling EndDict-method for ready or empty object");
        if (nodes_stack_.back()->IsDict()) {
            EndData();
            return *this;
        } else {
            throw std::logic_error("calling EndDict-method in wrong place");
        }
    }

    Builder& Builder::EndArray() {
        if (nodes_stack_.empty()) throw std::logic_error("calling EndArray-method for ready or empty object");
        if (nodes_stack_.back()->IsArray()) {
            EndData();
            return *this;
        } else {
            throw std::logic_error("calling EndArray-method in wrong place");
        }
    }

    json::Node Builder::Build() {
        if (nodes_stack_.empty() && root_ != nullptr) {
            return root_;
        } else {
            throw std::logic_error("calling build when object is not ready");
        }
    }

    void Builder::EndData() {
        Node& node_ref = *nodes_stack_.back();
        nodes_stack_.pop_back();
        if (nodes_stack_.empty()) {
            root_ = std::move(node_ref);
        } else if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(std::move(node_ref));
        } else if (nodes_stack_.back()->IsString()) {
            Node& str_node_ref = *nodes_stack_.back();
            nodes_stack_.pop_back();
            nodes_stack_.back()->AsDict().insert({str_node_ref.AsString(), std::move(node_ref)});
        }
    }

    Builder& Builder::ItemContext::EndDict() {
        builder_.EndDict();
        return builder_;
    }

    Builder& Builder::ItemContext::EndArray() {
        builder_.EndArray();
        return builder_;
    }

    Builder::DictItemContext Builder::ItemContext::StartDict() {
        builder_.StartDict();
        return {*this};
    }

    Builder::ArrayItemContext Builder::ItemContext::StartArray() {
        builder_.StartArray();
        return {*this};
    }

    Builder::ValueAfterArrayContext Builder::ItemContext::Value(Node::Value value) {
        builder_.Value(std::move(value));
        return {*this};
    }

    Builder::ValueAfterKeyContext Builder::KeyItemContext::Value(Node::Value value) {
        builder_.Value(std::move(value));
        return {*this};
    }

    Builder::KeyItemContext Builder::ValueAfterKeyContext::Key(std::string key) {
        builder_.Key(std::move(key));
        return {*this};
    }

    Builder::KeyItemContext Builder::DictItemContext::Key(std::string key) {
        builder_.Key(std::move(key));
        return {*this};
    }
}