#include "agent/skill_loader.h"
#include "test_framework.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace agent;

void test_parse_skill_file() {
    std::filesystem::path temp_skill_path = "temp_test_skill.md";
    {
        std::ofstream ofs(temp_skill_path);
        ofs << "---\n";
        ofs << "name: Test Planner\n";
        ofs << "description: Test skill for planning tasks.\n";
        ofs << "keywords: [plan, task, calculate, error]\n";
        ofs << "---\n\n";
        ofs << "# Test Skill Content\nAlways break down steps carefully.\n";
    }

    Skill skill = SkillLoader::parse_skill_file(temp_skill_path);
    TEST_ASSERT(skill.name == "Test Planner", "Skill name mismatch");
    TEST_ASSERT(skill.description == "Test skill for planning tasks.", "Skill description mismatch");
    TEST_ASSERT(skill.keywords.size() == 4, "Skill keywords size should be 4");
    TEST_ASSERT(skill.keywords[0] == "plan", "First keyword mismatch");
    TEST_ASSERT(skill.content.find("Always break down steps carefully.") != std::string::npos, "Skill content mismatch");

    std::filesystem::remove(temp_skill_path);
}

void test_skill_loader_matching_and_injection() {
    SkillLoader loader;
    if (std::filesystem::exists("skills")) {
        loader.load_skills("skills");
    } else if (std::filesystem::exists("../skills")) {
        loader.load_skills("../skills");
    }

    const auto& all_skills = loader.get_all_skills();
    TEST_ASSERT(!all_skills.empty(), "Skills should not be empty");

    // Test prompt matching with keywords
    std::vector<Skill> matched = loader.match_skills("Please help me plan my tasks and recover from errors.");
    TEST_ASSERT(!matched.empty(), "Matched skills should not be empty");

    std::string base_prompt = "You are a helpful AI Agent.";
    std::string injected = loader.inject_skills(base_prompt, matched);
    TEST_ASSERT(injected.find("You are a helpful AI Agent.") != std::string::npos, "Injected prompt missing base prompt");
    TEST_ASSERT(injected.find("SKILLS SYSTEM") != std::string::npos || injected.find("Skill") != std::string::npos || injected.find(matched[0].name) != std::string::npos, "Injected prompt missing skill content");

    std::string auto_injected = loader.auto_inject(base_prompt, "Fix this code error");
    TEST_ASSERT(!auto_injected.empty(), "Auto injected prompt should not be empty");
}

int main() {
    TestSuite suite("SKILL LOADER MODULE");
    suite.run("Parse Skill File", test_parse_skill_file);
    suite.run("Skill Matching & Injection", test_skill_loader_matching_and_injection);
    return suite.print_summary();
}
