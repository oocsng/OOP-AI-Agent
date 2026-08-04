#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace agent {

struct Skill {
    std::string name;
    std::string description;
    std::vector<std::string> keywords;
    std::string content;
    std::filesystem::path file_path;
};

class SkillLoader {
private:
    std::vector<Skill> skills_;

public:
    SkillLoader() = default;

    void load_skills(const std::filesystem::path& skills_dir = "skills");
    
    [[nodiscard]] const std::vector<Skill>& get_all_skills() const noexcept { return skills_; }

    [[nodiscard]] std::vector<Skill> match_skills(const std::string& user_prompt) const;

    [[nodiscard]] std::string inject_skills(const std::string& base_prompt, const std::vector<Skill>& matched_skills) const;

    [[nodiscard]] std::string auto_inject(const std::string& base_prompt, const std::string& user_prompt) const;

    static Skill parse_skill_file(const std::filesystem::path& path);
};

} // namespace agent
