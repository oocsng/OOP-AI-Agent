#pragma once
#include "tool.h"
#include <string>
#include <vector>
#include <sqlite3.h>

namespace agent {

class VectorMemoryTool : public Tool {
public:
    VectorMemoryTool();
    ~VectorMemoryTool() override;

    [[nodiscard]] std::string name() const override { return "vector_memory"; }
    [[nodiscard]] std::string description() const override {
        return "Save text with vector embedding (action='save', text='...'), or semantic search (action='search', text='...').";
    }
    std::string execute(const nlohmann::json& args) override;
    [[nodiscard]] nlohmann::json parameters_schema() const override {
        return nlohmann::json::object();
    }

public:
    static double cosine_similarity(const std::vector<float>& a, const std::vector<float>& b);

private:
    sqlite3* db_{nullptr};
    std::vector<float> generate_embedding(const std::string& text) const;
    void init_db();
};

} // namespace agent
