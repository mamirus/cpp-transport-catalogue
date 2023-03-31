#include "json_builder.h"

namespace json {


    Builder& Builder::Value(Node val) {
        using namespace std::literals;
        if ( !started_ ) {
            root_ = std::move(val);
            started_ = true;
            finished_ = true;

            return *this;
        }

        if ( DoingDict() ) {
            if ( !curr_key_ ) {
                throw std::logic_error("Error building a dictionary, attempt to enter value before the key."s);
            }
            Dict& dict = nodes_stack_.back()->AsDict();
            dict.emplace(curr_key_.value(), std::move(val));
            curr_key_.reset();

            return *this;
        }

        if ( DoingArray() ) {
            Array& arr = nodes_stack_.back()->AsArray();
            arr.push_back(std::move(val));

            return *this;
        }

        if ( finished_ ) {
            throw std::logic_error("Error adding a value, Node is built already."s);
        }

        return *this;
    }

    DictKeyContext Builder::Key(std::string key) {
        using namespace std::literals;
        if ( !DoingDict()) {
            throw std::logic_error("Error: no dictionary started. Key defenition is out of scope."s);
        }
        if ( curr_key_ ) {
            throw std::logic_error("Error in dictionary: key is already declared. Please enter value"s);
        }

        curr_key_ = std::move(key);

        return DictKeyContext(*this);
    }

    DictItemContext Builder::StartDict() {
        using namespace std::literals;

        if (finished_ || built_) {
            throw std::logic_error("Error: node build is finished already"s);
        }

        if (!started_) {
            started_ = true;
            nodes_stack_.emplace_back(new Node(Dict{}));
            return DictItemContext(*this);
        }

        if (DoingArray()) {
            nodes_stack_.emplace_back(new Node(Dict{}));
            return DictItemContext(*this);
        }

        if (DoingDict()) {
            if (!curr_key_){
                throw std::logic_error("Error in dictionary: key is not declared, can not start another dictionary as a value."s);
            }
            nodes_stack_.emplace_back(new Node(curr_key_.value()));
            curr_key_.reset();
            nodes_stack_.emplace_back(new Node(Dict{}));
            return DictItemContext(*this);
        }

        throw std::logic_error("Error in dictionary: we should not be here, StartDict method."s);
    }

    Builder& Builder::EndDict() {
        using namespace std::literals;
        if (!started_ || finished_ || built_ || !DoingDict()) {
            throw std::logic_error("Error in dictionary: no active dictionary."s);
        }
        Node* dict = nodes_stack_.back();
        nodes_stack_.pop_back();

        if (nodes_stack_.empty()) { // root node, finish up
            root_ = std::move(*dict);
            delete dict;
            finished_ = true;
            return *this;
        }

        if ( Node* node = nodes_stack_.back(); node->IsString() ) { // this dict is a value in previously declared Dictionary.
            std::string key = node->AsString();
            nodes_stack_.pop_back();
            delete node;
            node = nodes_stack_.empty() ? nullptr : nodes_stack_.back();
            if (!node || !node->IsDict()){
                throw std::logic_error("Error in dictionary: structural error in constructing previous dictionary."s);
            }
            node->AsDict().emplace(key, std::move(*dict));
            delete dict;
            return *this;
        }

        if ( Node* node = nodes_stack_.back(); node->IsArray() ) { // this Dict is a value in array.
            node->AsArray().emplace_back(std::move(*dict));
            delete dict;
            return *this;
        }
        throw std::logic_error("Error in dictionary: we should not be here, EndDict method."s);
    }

    ArrayItemContext Builder::StartArray() {
        using namespace std::literals;

        if (finished_ || built_) {
            throw std::logic_error("Error: node build is finished already"s);
        }

        if (!started_) {
            started_ = true;
            nodes_stack_.emplace_back(new Node(Array{}));
            return ArrayItemContext(*this);
        }

        if (DoingArray()) {
            nodes_stack_.emplace_back(new Node(Array{}));
            return ArrayItemContext(*this);
        }

        if (DoingDict()) {
            if (!curr_key_){
                throw std::logic_error("Error in dictionary: key is not declared, can not start array as a value."s);
            }
            nodes_stack_.emplace_back(new Node(curr_key_.value()));
            curr_key_.reset();
            nodes_stack_.emplace_back(new Node(Array{}));
            return ArrayItemContext(*this);
        }

        throw std::logic_error("Error in array: we should not be here, StartArray method."s);
    }

    Builder& Builder::EndArray() {
        using namespace std::literals;
        if (!started_ || finished_ || built_ || !DoingArray()) {
            throw std::logic_error("Error in array: no active array."s);
        }
        Node* array = nodes_stack_.back();
        nodes_stack_.pop_back();

        if (nodes_stack_.empty()) { // root node, finish up
            root_ = std::move(*array);
            delete array;
            finished_ = true;
            return *this;
        }

        if ( Node* node = nodes_stack_.back(); node->IsString() ) { // this array is a value in previously declared Dictionary.
            std::string key = node->AsString();
            nodes_stack_.pop_back();
            delete node;
            node = nodes_stack_.empty() ? nullptr : nodes_stack_.back();
            if (!node || !node->IsDict()){
                throw std::logic_error("Error in dictionary: structural error in constructing previous dictionary."s);
            }
            node->AsDict().emplace(key, std::move(*array));
            delete array;
            return *this;
        }

        if ( Node* node = nodes_stack_.back(); node->IsArray() ) { // this array is a value in previously declared array.
            node->AsArray().emplace_back(std::move(*array));
            delete array;
            return *this;
        }
        throw std::logic_error("Error in dictionary: we should not be here, EndArray method."s);
    }

    Node Builder::Build() {
        using namespace std::literals;
        if (!started_ || !finished_ || built_) {
            throw std::logic_error("Error in Build method: object is not ready."s);
        }
        built_ = true;

        return root_;
    }

    bool Builder::DoingDict() const {
        if (nodes_stack_.empty()) return false;

        return nodes_stack_.back()->IsDict();
    }

    bool Builder::DoingArray() const {
        if (nodes_stack_.empty()) return false;

        return nodes_stack_.back()->IsArray();
    }


    DictKeyContext DictItemContext::Key(std::string key) {
        builder_.Key(std::move(key));
        return DictKeyContext(builder_);
    }

    Builder& DictItemContext::EndDict() {
        return builder_.EndDict();
    }


    DictItemContext DictKeyContext::Value(Node val) {
        builder_.Value(std::move(val));
        return DictItemContext(builder_);
    }

    DictItemContext DictKeyContext::StartDict() {
        builder_.StartDict();
        return DictItemContext(builder_);
    }

    ArrayItemContext DictKeyContext::StartArray() {
        builder_.StartArray();
        return ArrayItemContext(builder_);
    }


    ArrayItemContext ArrayItemContext::Value(Node val) {
        builder_.Value(std::move(val));
        return ArrayItemContext(builder_);
    }

    ArrayItemContext ArrayItemContext::StartArray() {
        builder_.StartArray();
        return ArrayItemContext(builder_);
    }

    DictItemContext ArrayItemContext::StartDict() {
        builder_.StartDict();
        return DictItemContext(builder_);
    }

    Builder& ArrayItemContext::EndArray() {
        return builder_.EndArray();
    }


} // namespace json