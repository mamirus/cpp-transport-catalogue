#pragma once

#include "json.h"
#include <optional>
#include <string>

namespace json {

    // Комметарий по "Красному" замечанияю:
    // п.1 - сделано.
    // п.2 - хотел бы оставить как есть. Я изначально планировал сделать так. Однако, количество кода получается больше.
    //
    //      К тому же, некоторые одинаковые методы в разных классах должны вернуть объект разных классов. В этом нет проблемы, можно
    //      реализовать шаблонный класс и метод, наподобие, как это реализовано в svg::PathProps.
    //
    //      И ещё, главное здесь - публичное наследование от род.класса со всеми методами невозможно,
    //      т.к. тогда все методы будут доступны, а это недопустимо и противоречит самой идее этих классов.
    //      Нужно будет делать приватное наследование и потом всё равно повторять те-же методы
    //      и обращаться к методам род.класса - кода будет больше и дублирования тоже!
    //
    //      По сути, таким общим классом сейчас служит Builder, эти классы пользуются его методами!


    class Builder {
    private:
        std::optional<Node> root_;
        std::vector<Node*> nodes_stack_;
        std::optional<std::string> curr_key_;

        bool IsStarted();
        bool IsFinished();
        bool IsBuilt();

//        bool started_ = false;
//        bool finished_ = false;
        bool built_ = false;


    public:
        class DictItemContext;
        class DictKeyContext;
        class ArrayItemContext;

        class DictItemContext {
        public:
            explicit DictItemContext(Builder& builder) : builder_(builder) {
            }
            DictKeyContext Key(std::string key);
            Builder& EndDict();
        private:
            Builder& builder_;
        };

        class DictKeyContext {
        public:
            explicit DictKeyContext(Builder& builder) : builder_(builder) {
            }
            DictItemContext Value(Node val);
            DictItemContext StartDict();
            ArrayItemContext StartArray();
        private:
            Builder& builder_;
        };

        class ArrayItemContext {
        public:
            explicit ArrayItemContext(Builder& builder) : builder_(builder) {
            }

            ArrayItemContext Value(Node val);
            ArrayItemContext StartArray();
            DictItemContext StartDict();
            Builder& EndArray();

        private:
            Builder& builder_;
        };
        Builder() = default;

        Builder& Value(Node val);

        DictKeyContext Key(std::string key);

        DictItemContext StartDict();
        Builder& EndDict();

        ArrayItemContext StartArray();
        Builder& EndArray();

        Node Build();

    private:
        [[nodiscard]] bool DoingDict() const;
        [[nodiscard]] bool DoingArray() const;

    };
}