#include "web_tool.h"
#include <curl/curl.h>
#include <regex>
#include <sstream>

namespace agent {

static size_t web_curl_write(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total);
    return total;
}

static std::string decode_html_entities(std::string str) {
    size_t pos = 0;
    while ((pos = str.find("&quot;", pos)) != std::string::npos) { str.replace(pos, 6, "\""); }
    pos = 0;
    while ((pos = str.find("&#x27;", pos)) != std::string::npos) { str.replace(pos, 6, "'"); }
    pos = 0;
    while ((pos = str.find("&apos;", pos)) != std::string::npos) { str.replace(pos, 6, "'"); }
    pos = 0;
    while ((pos = str.find("&amp;", pos)) != std::string::npos) { str.replace(pos, 5, "&"); }
    pos = 0;
    while ((pos = str.find("&lt;", pos)) != std::string::npos) { str.replace(pos, 4, "<"); }
    pos = 0;
    while ((pos = str.find("&gt;", pos)) != std::string::npos) { str.replace(pos, 4, ">"); }
    return str;
}

std::string WebSearchTool::execute(const nlohmann::json& args) {
    std::string query;
    if (args.contains("query") && args["query"].is_string()) {
        query = args["query"].get<std::string>();
    } else if (args.contains("q") && args["q"].is_string()) {
        query = args["q"].get<std::string>();
    } else if (args.contains("raw_input") && args["raw_input"].is_string()) {
        query = args["raw_input"].get<std::string>();
    } else if (args.is_string()) {
        query = args.get<std::string>();
    } else {
        return "Error: Missing string argument 'query'.";
    }

    CURL* raw_curl = curl_easy_init();
    if (!raw_curl) {
        return "Error: Failed to initialize libcurl for web search.";
    }
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(raw_curl, curl_easy_cleanup);

    std::unique_ptr<char, decltype(&curl_free)> encoded_query(
        curl_easy_escape(curl.get(), query.c_str(), static_cast<int>(query.length())), curl_free);
        
    std::string postfields = "q=" + std::string(encoded_query.get());

    std::string response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, "https://html.duckduckgo.com/html/");
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postfields.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, web_curl_write);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
        return "Error: Web search HTTP request failed: " + std::string(curl_easy_strerror(res));
    }

    if (response.empty()) {
        return "No results found for query: " + query;
    }

    std::regex snippet_regex(R"(<a class="result__snippet"[^>]*>(.*?)</a>)");
    auto begin = std::sregex_iterator(response.begin(), response.end(), snippet_regex);
    auto end = std::sregex_iterator();

    std::stringstream result_ss;
    int count = 0;
    for (auto i = begin; i != end && count < 5; ++i, ++count) {
        std::string raw_snippet = (*i)[1].str();
        std::regex tag_regex(R"(<[^>]+>)");
        std::string clean = std::regex_replace(raw_snippet, tag_regex, "");
        clean = decode_html_entities(clean);
        clean.erase(0, clean.find_first_not_of(" \t\r\n"));
        clean.erase(clean.find_last_not_of(" \t\r\n") + 1);

        if (!clean.empty()) {
            result_ss << "[" << (count + 1) << "] " << clean << "\n\n";
        }
    }

    std::string result_text = result_ss.str();
    if (result_text.empty()) {
        std::regex tag_regex(R"(<[^>]+>)");
        std::string clean_fallback = std::regex_replace(response, tag_regex, " ");
        clean_fallback = decode_html_entities(clean_fallback);
        if (clean_fallback.length() > 1000) {
            clean_fallback = clean_fallback.substr(0, 1000) + "...";
        }
        return "Web Search Summary for '" + query + "':\n" + clean_fallback;
    }

    return "Web Search Results for '" + query + "':\n" + result_text;
}

} // namespace agent
