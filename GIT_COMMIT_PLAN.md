# GITHUB COMMIT STRATEGY & WORKFLOW PLAN (`GIT_COMMIT_PLAN.md`)
**Project**: OOP AI Agent Framework with Ollama API in C++ (2026)  
**Target Compliance**: 18+ Clean Conventional Commits (Supports 1, 2, or 3 Group Members)

---

## QUY ĐỊNH COMMIT & COMPLIANCE RULES

1. **Quy định số lượng Commit (Grading Rules)**:
   * **Nhóm 1 thành viên**: $\ge 6$ commits.
   * **Nhóm 2 thành viên**: $\ge 12$ commits (mỗi người $\ge 6$ commits).
   * **Nhóm 3 thành viên**: $\ge 18$ commits (mỗi người $\ge 6$ commits).
2. **Tiêu chuẩn Commit (Conventional Commits)**:
   * `feat(<scope>)`: Tính năng mới (tools, loop, client, harness...).
   * `fix(<scope>)`: Sửa lỗi (syntax, edge-case, policy logic...).
   * `test(<scope>)`: Thêm hoặc cập nhật unit tests.
   * `docs(<scope>)`: Cập nhật tài liệu README, UML, task list.
   * `chore(<scope>)`: Cấu hình CMake, gitignore, script build.
3. **Quy định đẩy mã nguồn**:
   * Chỉ stage & commit các file mã nguồn, cấu hình và test.
   * KHÔNG commit thư mục tạm `build/`, `.codegraph/`, `.vscode/`, file binary `.exe`, `.db` hoặc screenshot.

---

## END-TO-END GIT INITIALIZATION & WORKFLOW COMMANDS

### 1. Initial Remote & Local Git Setup
```bash
# Set user credentials (if not configured)
git config user.name "Your Name"
git config user.email "your_email@student.hcmus.edu.vn"

# Ensure main branch name
git branch -M main

# Configure Personal Access Token (PAT) remote URL (Replace <PAT> and <REPO_URL>)
# Example: git remote set-url origin https://<TOKEN>@github.com/<USER>/<REPO>.git
git remote -v
```

---

## DETAILED STEP-BY-STEP COMMIT STRATEGY (18 COMMITS)

### Commit 1: Project Setup & CMake Infrastructure
* **Message**: `chore(setup): initialize C++26 project structure CMake and build environment`
* **Target Files**: `CMakeLists.txt`, `README.md`, `.gitignore`, `build.ps1`, `run_tests.ps1`
* **Git Commands**:
  ```bash
  git add CMakeLists.txt README.md .gitignore build.ps1 run_tests.ps1
  git commit -m "chore(setup): initialize C++26 project structure CMake and build environment"
  ```

### Commit 2: Core Data Transfer Objects (DTO)
* **Message**: `feat(dto): implement Message, Step, and Trajectory data structures`
* **Target Files**: `src/dto/message.h`, `src/dto/step.h`, `src/dto/trajectory.h`, `src/dto/trajectory.cpp`
* **Git Commands**:
  ```bash
  git add src/dto/
  git commit -m "feat(dto): implement Message, Step, and Trajectory data structures"
  ```

### Commit 3: LLM Client Interface & Ollama HTTP Client
* **Message**: `feat(client): add abstract LLMClient and OllamaClient HTTP integration via libcurl`
* **Target Files**: `src/client/llm_client.h`, `src/client/ollama_client.h`, `src/client/ollama_client.cpp`
* **Git Commands**:
  ```bash
  git add src/client/
  git commit -m "feat(client): add abstract LLMClient and OllamaClient HTTP integration via libcurl"
  ```

### Commit 4: Abstract Tool Interface & Tool Registry Factory
* **Message**: `feat(tools): add Tool abstract interface and ToolRegistry with allow/deny policies`
* **Target Files**: `src/tools/tool.h`, `src/tools/tool_registry.h`, `src/tools/tool_registry.cpp`
* **Git Commands**:
  ```bash
  git add src/tools/tool.h src/tools/tool_registry.h src/tools/tool_registry.cpp
  git commit -m "feat(tools): add Tool abstract interface and ToolRegistry with allow/deny policies"
  ```

### Commit 5: Implementation of 5 Mandatory Core Tools
* **Message**: `feat(tools): implement exec, file read/write, web search, sqlite memory, and calculator tools`
* **Target Files**: `src/tools/exec_tool.*`, `src/tools/file_tool.*`, `src/tools/web_tool.*`, `src/tools/memory_tool.*`, `src/tools/calculator_tool.*`
* **Git Commands**:
  ```bash
  git add src/tools/exec_tool.* src/tools/file_tool.* src/tools/web_tool.* src/tools/memory_tool.* src/tools/calculator_tool.*
  git commit -m "feat(tools): implement exec, file read/write, web search, sqlite memory, and calculator tools"
  ```

### Commit 6: Implementation of 3 Custom Extension Tools
* **Message**: `feat(tools): implement DateTimeTool, SystemInfoTool, and RegexExtractorTool`
* **Target Files**: `src/tools/custom_tool1.*`, `src/tools/custom_tool2.*`, `src/tools/custom_tool3.*`
* **Git Commands**:
  ```bash
  git add src/tools/custom_tool1.* src/tools/custom_tool2.* src/tools/custom_tool3.*
  git commit -m "feat(tools): implement DateTimeTool, SystemInfoTool, and RegexExtractorTool"
  ```

### Commit 7: Skill Loader & Prompt Auto-Injection System
* **Message**: `feat(skills): implement SkillLoader markdown parser and prompt auto-injection`
* **Target Files**: `src/agent/skill_loader.h`, `src/agent/skill_loader.cpp`, `skills/task_planner.md`, `skills/error_recovery.md`, `skills/code_fixer.md`
* **Git Commands**:
  ```bash
  git add src/agent/skill_loader.* skills/
  git commit -m "feat(skills): implement SkillLoader markdown parser and prompt auto-injection"
  ```

### Commit 8: ReAct Loop Detector Engine
* **Message**: `feat(agent): implement LoopDetector for generic repeat and ping-pong loop detection`
* **Target Files**: `src/agent/loop_detector.h`, `src/agent/loop_detector.cpp`
* **Git Commands**:
  ```bash
  git add src/agent/loop_detector.*
  git commit -m "feat(agent): implement LoopDetector for generic repeat and ping-pong loop detection"
  ```

### Commit 9: ReAct Agent Loop Template Method
* **Message**: `feat(agent): implement AgentLoop template method with Observer step_hook interface`
* **Target Files**: `src/agent/agent_loop.h`, `src/agent/agent_loop.cpp`
* **Git Commands**:
  ```bash
  git add src/agent/agent_loop.*
  git commit -m "feat(agent): implement AgentLoop template method with Observer step_hook interface"
  ```

### Commit 10: Evaluators (Strategy Pattern)
* **Message**: `feat(harness): implement Evaluator strategy interface, KeywordEvaluator, and FunctionalEvaluator`
* **Target Files**: `src/harness/evaluator.h`, `src/harness/keyword_evaluator.h`, `src/harness/keyword_evaluator.cpp`, `src/harness/functional_evaluator.h`, `src/harness/functional_evaluator.cpp`
* **Git Commands**:
  ```bash
  git add src/harness/evaluator.h src/harness/keyword_evaluator.* src/harness/functional_evaluator.*
  git commit -m "feat(harness): implement Evaluator strategy interface, KeywordEvaluator, and FunctionalEvaluator"
  ```

### Commit 11: Harness Runner & Trajectory JSON Logger
* **Message**: `feat(harness): implement HarnessRunner observer binding and trajectory exporter`
* **Target Files**: `src/harness/harness_runner.h`, `src/harness/harness_runner.cpp`
* **Git Commands**:
  ```bash
  git add src/harness/harness_runner.*
  git commit -m "feat(harness): implement HarnessRunner observer binding and trajectory exporter"
  ```

### Commit 12: Benchmark Task Suite & Batch Runner
* **Message**: `feat(benchmark): add 13 benchmark tasks in tasks.json and batch evaluation runner`
* **Target Files**: `benchmark/tasks.json`, `benchmark/run_eval.cpp`
* **Git Commands**:
  ```bash
  git add benchmark/tasks.json benchmark/run_eval.cpp
  git commit -m "feat(benchmark): add 13 benchmark tasks in tasks.json and batch evaluation runner"
  ```

### Commit 13: Bonus Feature 1 - GUI Agent & Win32 Automation
* **Message**: `feat(gui): implement GuiTool Win32 automation Base64 screenshot and browser demo`
* **Target Files**: `src/tools/gui_tool.h`, `src/tools/gui_tool.cpp`, `benchmark/run_browser_demo.cpp`
* **Git Commands**:
  ```bash
  git add src/tools/gui_tool.* benchmark/run_browser_demo.cpp
  git commit -m "feat(gui): implement GuiTool Win32 automation Base64 screenshot and browser demo"
  ```

### Commit 14: Bonus Feature 2 - Vector Memory & Cosine Similarity
* **Message**: `feat(vector-memory): implement VectorMemoryTool with sqlite3 and C++ cosine similarity`
* **Target Files**: `src/tools/vector_memory_tool.h`, `src/tools/vector_memory_tool.cpp`
* **Git Commands**:
  ```bash
  git add src/tools/vector_memory_tool.*
  git commit -m "feat(vector-memory): implement VectorMemoryTool with sqlite3 and C++ cosine similarity"
  ```

### Commit 15: Bonus Feature 3 - Multi-Agent Coordination
* **Message**: `feat(multi-agent): implement SubAgentManager thread pool and thread-safe MessageQueue`
* **Target Files**: `src/agent/sub_agent_manager.h`, `src/agent/sub_agent_manager.cpp`, `src/tools/sub_agent_tool.h`, `src/tools/sub_agent_tool.cpp`
* **Git Commands**:
  ```bash
  git add src/agent/sub_agent_manager.* src/tools/sub_agent_tool.*
  git commit -m "feat(multi-agent): implement SubAgentManager thread pool and thread-safe MessageQueue"
  ```

### Commit 16: Main Application Entry Point
* **Message**: `feat(main): implement CLI entry point main.cpp for interactive Agent execution`
* **Target Files**: `src/main.cpp`
* **Git Commands**:
  ```bash
  git add src/main.cpp
  git commit -m "feat(main): implement CLI entry point main.cpp for interactive Agent execution"
  ```

### Commit 17: CTest Unit Testing Suite (100% PASS)
* **Message**: `test(unit): add unit test suites covering client, tools, loop, evaluators, and bonus features`
* **Target Files**: `tests/test_framework.h`, `tests/test_client.cpp`, `tests/test_tools.cpp`, `tests/test_loop_detector.cpp`, `tests/test_skill_loader.cpp`, `tests/test_agent_loop.cpp`, `tests/test_evaluators.cpp`, `tests/test_bonus_features.cpp`, `tests/test_bonus_gui.cpp`, `tests/test_bonus_vector_memory.cpp`, `tests/test_bonus_multi_agent.cpp`
* **Git Commands**:
  ```bash
  git add tests/
  git commit -m "test(unit): add unit test suites covering client, tools, loop, evaluators, and bonus features"
  ```

### Commit 18: UML Architecture Diagrams & Documentation
* **Message**: `docs(architecture): add Class, Sequence, Component Mermaid UML diagrams`
* **Target Files**: `docs/`, `GIT_COMMIT_PLAN.md`
* **Git Commands**:
  ```bash
  git add docs/ GIT_COMMIT_PLAN.md
  git commit -m "docs(architecture): add Class, Sequence, Component Mermaid UML diagrams"
  ```

---

## VERIFICATION & PUSH COMMANDS

```bash
# Verify all commits are formatted correctly
git log --oneline -n 20

# Check working tree is clean
git status

# Push all commits to GitHub remote main branch
git push origin main
```
