#include "custom_tool3.h"
#include <regex>

namespace agent {

std::string RegexExtractorTool::execute(const nlohmann::json& args) {
    std::string text;
    std::string pattern_str;

    if (args.contains("text") && args["text"].is_string()) {
        text = args["text"].get<std::string>();
    } else if (args.contains("input") && args["input"].is_string()) {
        text = args["input"].get<std::string>();
    } else if (args.contains("content") && args["content"].is_string()) {
        text = args["content"].get<std::string>();
    }

    if (args.contains("pattern") && args["pattern"].is_string()) {
        pattern_str = args["pattern"].get<std::string>();
    } else if (args.contains("regex") && args["regex"].is_string()) {
        pattern_str = args["regex"].get<std::string>();
    }

    if (text.empty() || pattern_str.empty()) {
        return "Error: Missing string arguments 'text' and 'pattern'.";
    }

    try {
        std::regex regex_pattern(pattern_str);
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), regex_pattern);
        auto words_end = std::sregex_iterator();

        nlohmann::json matches = nlohmann::json::array();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            matches.push_back(match.str());
        }

        nlohmann::json result;
        result["match_count"] = matches.size();
        result["matches"] = matches;
        return result.dump(2);
    } catch (const std::regex_error& e) {
        return "Error: Invalid regular expression -> " + std::string(e.what());
    }
}

} // namespace agent
