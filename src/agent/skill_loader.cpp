#include "skill_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace agent {

static std::string to_lower(std::string_view str) {
    std::string result;
    result.reserve(str.length());
    for (char c : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

static std::vector<std::string> split_comma(const std::string& str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim leading/trailing whitespace
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            tokens.push_back(token.substr(start, end - start + 1));
        }
    }
    return tokens;
}

Skill SkillLoader::parse_skill_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    Skill skill;
    skill.file_path = path;
    skill.name = path.stem().string();

    std::string line;
    bool in_frontmatter = false;
    int delimiter_count = 0;
    std::stringstream content_ss;

    while (std::getline(file, line)) {
        if (line.rfind("---", 0) == 0) {
            delimiter_count++;
            if (delimiter_count == 1) {
                in_frontmatter = true;
                continue;
            } else if (delimiter_count == 2) {
                in_frontmatter = false;
                continue;
            }
        }

        if (in_frontmatter) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string val = line.substr(colon_pos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of(" \t") + 1);

                if (key == "name") {
                    skill.name = val;
                } else if (key == "description") {
                    skill.description = val;
                } else if (key == "keywords") {
                    if (!val.empty() && val.front() == '[') val.erase(0, 1);
                    if (!val.empty() && val.back() == ']') val.pop_back();
                    skill.keywords = split_comma(val);
                }
            }
        } else {
            content_ss << line << "\n";
        }
    }

    skill.content = content_ss.str();
    return skill;
}

void SkillLoader::load_skills(const std::filesystem::path& skills_dir) {
    skills_.clear();
    if (!std::filesystem::exists(skills_dir) || !std::filesystem::is_directory(skills_dir)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(skills_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            Skill skill = parse_skill_file(entry.path());
            if (!skill.name.empty()) {
                skills_.push_back(skill);
            }
        }
    }
}

std::vector<Skill> SkillLoader::match_skills(const std::string& user_prompt) const {
    std::string lower_prompt = to_lower(user_prompt);
    std::vector<Skill> matched;

    for (const auto& skill : skills_) {
        bool match_found = false;
        for (const auto& kw : skill.keywords) {
            std::string lower_kw = to_lower(kw);
            // C++23 std::string::contains
            if (lower_prompt.contains(lower_kw)) {
                match_found = true;
                break;
            }
        }
        if (match_found) {
            matched.push_back(skill);
        }
    }
    return matched;
}

std::string SkillLoader::inject_skills(const std::string& base_prompt, const std::vector<Skill>& matched_skills) const {
    if (matched_skills.empty()) {
        return base_prompt;
    }

    std::stringstream ss;
    ss << base_prompt << "\n\n=== ACTIVATED SKILLS & GUIDELINES ===\n";

    for (const auto& skill : matched_skills) {
        ss << "\n--- Skill: " << skill.name << " ---\n";
        ss << skill.content;
    }
    ss << "\n====================================\n";

    return ss.str();
}

std::string SkillLoader::auto_inject(const std::string& base_prompt, const std::string& user_prompt) const {
    auto matched = match_skills(user_prompt);
    return inject_skills(base_prompt, matched);
}

} // namespace agent
