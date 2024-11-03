#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <unordered_map>

namespace serious::toml
{

using Value = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::unordered_map<std::string, struct Node>
>;

struct Node
{
    std::unordered_map<std::string, Value> values;
};

class Parser
{
public:
    std::optional<Node> Parse(const std::string& toml);

private:
    bool ParseLine(std::string_view line);
    void ParseTable(std::string_view table);
    void ParseKeyValue(std::string_view kv);

    Node m_Root;
};

}