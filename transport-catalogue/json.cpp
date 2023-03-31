#include "json.h"
#include <exception>

using namespace std;

namespace json {

    namespace {

        inline const size_t SKIP_NUM = 4;
        inline const char SKIP_SYMBOLS[SKIP_NUM] = {' ', '\t', '\r' ,'\n'};

        Node LoadNode(istream& input);

        bool IsSkipSymbol(char c) {
            for (size_t i = 0; i < SKIP_NUM; ++i) {
                if (c == SKIP_SYMBOLS[i]) {
                    return true;
                }
            }
            return false;
        }

        void FlushSkipSymbols(std::istream& strm) {
            char c;

            while ( strm >> c ) {
                if ( !IsSkipSymbol(c) ){
                    strm.putback(c);
                    return;
                }
            }
        }


        Node LoadArray(istream& input) {
            Array result;

            char c;
            while (input >> c && c != ']') {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input)); // LoadNode will take care of its skips
                FlushSkipSymbols(input); // Flush after the node, till the ',' most likely
            }
            if (c != ']') {
                throw ParsingError("Error loading array. Unexpected EOF.");
            }

            return Node(move(result));
        }


        Node LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                } else if (ch == '\\') {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end) {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char) {
                        case 'n':
                            s.push_back('\n');
                            break;
                        case 't':
                            s.push_back('\t');
                            break;
                        case 'r':
                            s.push_back('\r');
                            break;
                        case '"':
                            s.push_back('"');
                            break;
                        case '\\':
                            s.push_back('\\');
                            break;
                        default:
                            // Встретили неизвестную escape-последовательность
                            throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                } else if (ch == '\n' || ch == '\r') {
                    // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                } else {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }

            return Node(std::move(s));
        }

        Node LoadDict(istream& input) {
            Dict result;

            FlushSkipSymbols(input);
            char c;
            while (input >> c && c != '}') {
                if (c == ',') {
                    FlushSkipSymbols(input); // flush skips after ','
                    input >> c;
                }

                string key = LoadString(input).AsString();
                FlushSkipSymbols(input); // flush skips, then the next symbol must be ':'
                input >> c; // eat it up
                result.insert({move(key), LoadNode(input)}); // LoadNode will take care of its own skips
                FlushSkipSymbols(input); // flush skips after the value
            }
            if (c != '}') {
                throw ParsingError("Error wile loading dictionary. Unexpected EOF.");
            }

            return Node(move(result));
        }


        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            } else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return Node(std::stoi(parsed_num));
                    } catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node(std::stod(parsed_num));
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadBool(std::istream& input) {
            std::string check;
            check.reserve(5);
            char c;

            while(check.size()<4 && input >> c) {
                check += c;
            }

            if(check.size() < 4) {
                throw ParsingError("Failed to read bool value. EOF!"s);
            }

            bool result;
            if (check == "true"s) {
                result = true;
            } else {
                if ( !(input >> c) ) {
                    throw ParsingError("Failed to read bool value. EOF!"s);
                }
                check += c;
                if (check != "false"s) {
                    cerr << "Data>>"s << check << "<<"s << endl;
                    throw ParsingError("Failed to read bool value. Incorrect symbols presented."s);
                }
                result = false;
            }

            return Node(result);
        }

        Node LoadNull(std::istream& input) {
            std::string check;
            check.reserve(5);
            char c;

            while(check.size()<4 && input >> c) {
                check += c;
            }

            if (check.size() < 4) {
                throw ParsingError("Failed to read null value. EOF!"s);
            }
            if (check != "null"s) {
                throw ParsingError("Failed to read null value. Incorrect symbols presented."s);
            }

            return Node(nullptr);
        }



        Node LoadNode(std::istream& input) {
            char c;

            FlushSkipSymbols(input); // start of node reading

            input >> c;
            if (c == '[') {
                return LoadArray(input);
            } else if (c == '{') {
                return LoadDict(input);
            } else if (c == '"') {
                return LoadString(input);
            } else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            } else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            } else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    }  // namespace


    const Array& Node::AsArray() const {
        if (! IsArray()) {
            throw std::logic_error("Node value is not array.");
        }
        return std::get<Array>(*this);
    }

    Array& Node::AsArray() {
        if (! IsArray()) {
            throw std::logic_error("Node value is not array.");
        }
        return std::get<Array>(*this);
    }



    const Dict& Node::AsDict() const {
        if (!IsDict()) {
            throw std::logic_error("Node value is not map.");
        }
        return std::get<Dict>(*this);
    }

    Dict& Node::AsDict() {
        if (!IsDict()) {
            throw std::logic_error("Node value is not map.");
        }
        return std::get<Dict>(*this);
    }



    int Node::AsInt() const {
        if (! IsInt()) {
            throw std::logic_error("Node value is not int.");
        }
        return std::get<int>(*this);
    }

    const string& Node::AsString() const {
        if (! IsString()) {
            throw std::logic_error("Node value is not string.");
        }
        return std::get<std::string>(*this);
    }

    bool Node::AsBool() const {
        if (! IsBool()) {
            throw std::logic_error("Node value is not bool.");
        }
        return std::get<bool>(*this);
    }

    double Node::AsDouble() const {
        if (!IsDouble()) {
            throw std::logic_error("Node value is not double and not int.");
        }
        if ( IsInt() ) {
            int res = std::get<int>(*this);
            return static_cast<double>(res);
        } else {
            return std::get<double>(*this);
        }
    }


    bool Node::IsInt() const {
        return std::holds_alternative<int>(*this);
    }

    bool Node::IsDouble() const {
        return IsPureDouble() || IsInt();
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(*this);
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(*this);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(*this);
    }

    bool Node::IsNull() const {
        return std::holds_alternative<std::nullptr_t>(*this);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(*this);
    }

    bool Node::IsDict() const {
        return std::holds_alternative<Dict>(*this);
    }

    Document::Document(Node root)
            : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document &other) const {
        return this->GetRoot() == other.GetRoot();
    }

    bool Document::operator!=(const Document &other) const {
        return this->GetRoot() != other.GetRoot();
    }


    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), output);
    }

    void PrintValue(std::nullptr_t, ostream &out) {
        out << "null"sv;
    }


    void PrintValue(const string &str, ostream &out) {
        out << "\""sv;

        for (char c : str) {
            switch (c) {
                case '\"':
                    out << "\\\""sv;
                    break;
                case '\r':
                    out << "\\r"sv;
                    break;
                case '\n':
                    out << "\\n"sv;
                    break;
                case '\t':
                    out << "\t"sv;
                    break;
                case '\\':
                    out << "\\\\"sv;
                    break;
                default:
                    out << c;
            }
        }

        out << "\""sv;
    }

    void PrintValue(bool val, std::ostream& out) {
        if (val) {
            out << "true"sv;
        } else {
            out << "false"sv;
        }
    }

    void PrintValue(const Array &arr, ostream &out) {
        out << "["sv;
        for (auto iter = arr.begin(); iter != arr.end(); ++iter) {
            PrintNode(*iter, out);
            if (std::next(iter) != arr.end()) {
                out << ", "sv;
            }
        }
        out << "]"sv;
    }

    void PrintValue(const Dict &dict, ostream &out) {
        out << "{"sv;
        for (auto iter = dict.begin(); iter != dict.end(); ++iter) {
            out << "\""sv << iter->first << "\":"sv;
            PrintNode(iter->second, out);
            if (std::next(iter) != dict.end() ) {
                out << ", "sv;
            }
        }
        out << "}";
    }

    void PrintNode(const Node &node, ostream &out) {
        std::visit( [&out](const auto& value){
            PrintValue(value, out);
        }, node.GetValue() );
    }


}  // namespace json