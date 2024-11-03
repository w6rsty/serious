#include "serious/io/toml.hpp"
#include "serious/io/log.hpp"

#include <fstream>
#include <string>
#include <unordered_map>

namespace serious::toml
{

std::optional<Node> Parser::Parse(const std::string& toml)
{
    std::ifstream file (toml);
    if (!file.is_open()) {
        SEWarn("Failed to open toml file: {}", toml);
        return std::nullopt;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (!ParseLine(line)) {
            SEWarn("Failed to parse line: {}", line);
            return std::nullopt;
        }
    }

    return m_Root;
}

bool Parser::ParseLine(std::string_view line)
{
    if (line.front() == '[' && line.back() == ']') {
        ParseTable(line);
    } else {
        ParseKeyValue(line);
    }
    return true;
}

void Parser::ParseTable(std::string_view table)
{
    // std::string nodeName = std::string(table.substr(1, table.size() - 2));
    // size_t pos = 0;

    // Node* node = &m_Root;

    // while ((pos = nodeName.find('.')) != std::string::npos) {
    //     std::string key = nodeName.substr(0, pos);
    //     if (node->values.find(key) == node->values.end()) {
    //         node->values[key] = Node{ std::unordered_map<std::string, Node>};
    //     }
    //     node 
    // }
}
void Parser::ParseKeyValue(std::string_view kv)
{

}


}