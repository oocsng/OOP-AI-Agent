#include "file_tool.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "utils/time_utils.hpp"

namespace agent {

std::string ReadFileTool::execute(const nlohmann::json& args) {
    std::string path_str;
    if (args.contains("path") && args["path"].is_string()) {
        path_str = args["path"].get<std::string>();
    } else if (args.contains("filename") && args["filename"].is_string()) {
        path_str = args["filename"].get<std::string>();
    } else if (args.contains("file") && args["file"].is_string()) {
        path_str = args["file"].get<std::string>();
    } else if (args.contains("filepath") && args["filepath"].is_string()) {
        path_str = args["filepath"].get<std::string>();
    } else {
        return "Error: Missing string argument 'path'.";
    }

    try {
        std::filesystem::path file_path = path_str;
        if (!std::filesystem::exists(file_path)) {
            return "Error: File does not exist at path '" + file_path.string() + "'";
        }

        std::ifstream file(file_path);
        if (!file.is_open()) {
            return "Error: Failed to open file for reading: " + file_path.string();
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // Prevent binary files from crashing JSON serialization
        if (content.find('\0') != std::string::npos) {
            return "Error: File appears to be binary. The read_file tool can only read text files.";
        }
        
        return content;
    } catch (const std::exception& e) {
        return "Error reading file: " + std::string(e.what());
    }
}

std::string WriteFileTool::execute(const nlohmann::json& args) {
    std::string path_str;
    std::string content_str;

    if (args.contains("path") && args["path"].is_string()) {
        path_str = args["path"].get<std::string>();
    } else if (args.contains("filename") && args["filename"].is_string()) {
        path_str = args["filename"].get<std::string>();
    } else if (args.contains("file") && args["file"].is_string()) {
        path_str = args["file"].get<std::string>();
    }

    if (args.contains("content") && args["content"].is_string()) {
        content_str = args["content"].get<std::string>();
    } else if (args.contains("text") && args["text"].is_string()) {
        content_str = args["text"].get<std::string>();
    } else if (args.contains("data") && args["data"].is_string()) {
        content_str = args["data"].get<std::string>();
    }

    if (path_str.empty() || content_str.empty()) {
        return "Error: Arguments 'path' and 'content' must be strings.";
    }

    try {
        std::filesystem::path file_path = path_str;
        if (file_path.has_parent_path() && !std::filesystem::exists(file_path.parent_path())) {
            std::filesystem::create_directories(file_path.parent_path());
        }

        std::ofstream file(file_path);
        if (!file.is_open()) {
            return "Error: Failed to open file for writing: " + file_path.string();
        }

        file << content_str;
        file << "\n--- " << utils::get_iso8601_timestamp() << " ---\n";
        return "Successfully wrote content to file: " + file_path.string();
    } catch (const std::exception& e) {
        return "Error writing file: " + std::string(e.what());
    }
}

} // namespace agent
