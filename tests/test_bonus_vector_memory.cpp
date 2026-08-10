#include "tools/vector_memory_tool.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>

using namespace agent;

void test_cosine_similarity_math() {
    // 1. Identical vectors -> score 1.0
    std::vector<float> v1 = {1.0f, 2.0f, 3.0f};
    std::vector<float> v2 = {1.0f, 2.0f, 3.0f};
    double score1 = VectorMemoryTool::cosine_similarity(v1, v2);
    assert(std::abs(score1 - 1.0) < 1e-5);

    // 2. Orthogonal vectors -> score 0.0
    std::vector<float> v_ortho1 = {1.0f, 0.0f};
    std::vector<float> v_ortho2 = {0.0f, 1.0f};
    double score2 = VectorMemoryTool::cosine_similarity(v_ortho1, v_ortho2);
    assert(std::abs(score2 - 0.0) < 1e-5);

    // 3. Opposite vectors -> score -1.0
    std::vector<float> v_opp1 = {1.0f, 2.0f};
    std::vector<float> v_opp2 = {-1.0f, -2.0f};
    double score3 = VectorMemoryTool::cosine_similarity(v_opp1, v_opp2);
    assert(std::abs(score3 - (-1.0)) < 1e-5);

    // 4. Edge cases: empty or size mismatch
    assert(VectorMemoryTool::cosine_similarity({}, v1) == 0.0);
    assert(VectorMemoryTool::cosine_similarity(v1, {1.0f}) == 0.0);
}

void test_vector_memory_tool_execution() {
    std::string test_db = "vector_memory.db";

    VectorMemoryTool tool;
    assert(tool.name() == "vector_memory");

    // Test argument error handling
    std::string err = tool.execute({});
    assert(err.find("Error") != std::string::npos);

    // Search in empty DB (offline / no embedding server)
    std::string search_res = tool.execute({{"action", "search"}, {"text", "hello"}});
    assert(search_res.find("Error") != std::string::npos || search_res.find("No relevant memory") != std::string::npos);
}

int main() {
    std::cout << "[RUNNING] test_bonus_vector_memory..." << std::endl;
    test_cosine_similarity_math();
    test_vector_memory_tool_execution();
    std::cout << "[PASSED] test_bonus_vector_memory" << std::endl;
    return 0;
}
