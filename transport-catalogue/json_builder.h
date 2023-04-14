#pragma once

#include <iostream>
#include <vector>
#include <deque>
#include "json.h"

namespace json {

    class Builder {
    private:
        class DictItemContext;
        class ArrayItemContext;
        class KeyItemContext;
        class ValueAfterArrayContext;
        class ValueAfterKeyContext;
        class ItemContext;

        template<typename T>
        void StartData(T obj);
        void EndData();
        Node root_ = nullptr;
        std::vector<Node*> nodes_stack_;
        std::deque<Node> nodes_;

    public:
        Builder& Key(std::string key);
        Builder& Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        json::Node Build();

    };

    class Builder::ItemContext {
    public:
        ItemContext(Builder& builder) : builder_(builder) {};
        ValueAfterArrayContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
    protected:
        Builder& builder_;
    };

    class Builder::ValueAfterKeyContext : public ItemContext {
    public:
        KeyItemContext Key(std::string key);
        ValueAfterArrayContext Value(Node::Value value) = delete;
        Builder& EndArray() = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
    };

    class Builder::ValueAfterArrayContext : public ItemContext {
    public:
        Builder& EndDict() = delete;
    };

    class Builder::KeyItemContext : public ItemContext {
    public:
        ValueAfterKeyContext Value(Node::Value value);
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
    };

    class Builder::DictItemContext : public ItemContext {
    public:
        KeyItemContext Key(std::string key);
        ValueAfterArrayContext Value(Node::Value value) = delete;
        Builder& EndArray() = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
    };

    class Builder::ArrayItemContext : public ItemContext {
    public:
        Builder& EndDict() = delete;
    };

    template<typename T>
    void Builder::StartData(T obj) {
        std::string str;
        if constexpr (std::is_same<T, Array>::value) {
            str = "Array";
        } else {
            str = "Dict";
        }
        if (root_ != nullptr) throw std::logic_error("calling Start" + str + "-method for ready object");
        if (nodes_stack_.empty() || nodes_stack_.back()->IsArray() || nodes_stack_.back()->IsString()) {
            nodes_.emplace_back(obj);
            nodes_stack_.push_back(&nodes_.back());
        } else {
            throw std::logic_error("calling Start" + str + "-method in wrong place");
        }
    }
}