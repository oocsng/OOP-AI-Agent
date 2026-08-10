# OOP AI Agents 2026 - Sequential Git Commit & Push Automation Helper

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " 🚀 OOP AI AGENTS 2026 - GIT COMMIT AUTOMATION SCRIPT" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($ScriptDir) {
    Set-Location (Join-Path $ScriptDir "..")
}

Write-Host "[1/3] Current Directory: $(Get-Location)" -ForegroundColor Yellow

$commits = @(
    @{
        Files = @("CMakeLists.txt", "README.md", ".gitignore", "build.ps1", "AGENTS.md", "GEMINI.md")
        Msg   = "chore(setup): initialize C++26 project structure CMake and build environment"
    },
    @{
        Files = @("src/dto/")
        Msg   = "feat(dto): implement Message, Step, and Trajectory data structures"
    },
    @{
        Files = @("src/client/")
        Msg   = "feat(client): add abstract LLMClient and OllamaClient HTTP integration via libcurl"
    },
    @{
        Files = @("src/tools/tool.h", "src/tools/tool_registry.h", "src/tools/tool_registry.cpp")
        Msg   = "feat(tools): add Tool abstract interface and ToolRegistry with allow/deny policies"
    },
    @{
        Files = @("src/tools/exec_tool.h", "src/tools/exec_tool.cpp", "src/tools/file_tool.h", "src/tools/file_tool.cpp", "src/tools/web_tool.h", "src/tools/web_tool.cpp", "src/tools/memory_tool.h", "src/tools/memory_tool.cpp", "src/tools/calculator_tool.h", "src/tools/calculator_tool.cpp")
        Msg   = "feat(tools): implement exec, file read/write, web search, sqlite memory, and calculator tools"
    },
    @{
        Files = @("src/tools/custom_tool1.h", "src/tools/custom_tool1.cpp", "src/tools/custom_tool2.h", "src/tools/custom_tool2.cpp", "src/tools/custom_tool3.h", "src/tools/custom_tool3.cpp")
        Msg   = "feat(tools): implement DateTimeTool, SystemInfoTool, and RegexExtractorTool"
    },
    @{
        Files = @("src/agent/skill_loader.h", "src/agent/skill_loader.cpp", "skills/")
        Msg   = "feat(skills): implement SkillLoader markdown parser and prompt auto-injection"
    },
    @{
        Files = @("src/agent/loop_detector.h", "src/agent/loop_detector.cpp")
        Msg   = "feat(agent): implement LoopDetector for generic repeat and ping-pong loop detection"
    },
    @{
        Files = @("src/agent/agent_loop.h", "src/agent/agent_loop.cpp")
        Msg   = "feat(agent): implement AgentLoop template method with Observer step_hook interface"
    },
    @{
        Files = @("src/harness/evaluator.h", "src/harness/keyword_evaluator.h", "src/harness/keyword_evaluator.cpp", "src/harness/functional_evaluator.h", "src/harness/functional_evaluator.cpp")
        Msg   = "feat(harness): implement Evaluator strategy interface, KeywordEvaluator, and FunctionalEvaluator"
    },
    @{
        Files = @("src/harness/harness_runner.h", "src/harness/harness_runner.cpp")
        Msg   = "feat(harness): implement HarnessRunner observer binding and trajectory exporter"
    },
    @{
        Files = @("benchmark/tasks.json", "benchmark/run_eval.cpp")
        Msg   = "feat(benchmark): add 12 benchmark tasks in tasks.json and batch evaluation runner"
    },
    @{
        Files = @("src/tools/gui_tool.h", "src/tools/gui_tool.cpp", "benchmark/run_browser_demo.cpp")
        Msg   = "feat(gui): implement GuiTool Win32 automation Base64 screenshot and browser demo"
    },
    @{
        Files = @("src/tools/vector_memory_tool.h", "src/tools/vector_memory_tool.cpp")
        Msg   = "feat(vector-memory): implement VectorMemoryTool with sqlite3 and C++ cosine similarity"
    },
    @{
        Files = @("src/agent/sub_agent_manager.h", "src/agent/sub_agent_manager.cpp", "src/tools/sub_agent_tool.h", "src/tools/sub_agent_tool.cpp")
        Msg   = "feat(multi-agent): implement SubAgentManager thread pool and thread-safe MessageQueue"
    },
    @{
        Files = @("src/main.cpp")
        Msg   = "feat(main): implement CLI entry point main.cpp for interactive Agent execution"
    },
    @{
        Files = @("tests/")
        Msg   = "test(unit): add 9 unit test suites covering client, tools, loop, evaluators, and bonus features"
    },
    @{
        Files = @("docs/", "AI_Agent_Coding_Tasks.md", "GIT_COMMIT_PLAN.md", "scripts/")
        Msg   = "docs(architecture): add Class, Sequence, Component Mermaid UML diagrams and complete task checklist"
    }
)

Write-Host "[2/3] Executing 18 Commits sequentially..." -ForegroundColor Yellow

$stepIdx = 1
foreach ($c in $commits) {
    Write-Host "  -> [Step $stepIdx/18] Staging files: $($c.Files -join ' ')" -ForegroundColor Cyan
    git add $c.Files
    git commit -m $c.Msg
    if ($LASTEXITCODE -ne 0) {
        Write-Host "⚠️ Step $stepIdx did not produce a new commit (or no changes to commit)." -ForegroundColor DarkYellow
    } else {
        Write-Host "✔ Step $stepIdx committed successfully." -ForegroundColor Green
    }
    $stepIdx++
}

Write-Host "`n[3/3] Displaying recent git log summary..." -ForegroundColor Yellow
git log --oneline -n 20

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " To push commits to remote repository, run:" -ForegroundColor Yellow
Write-Host "   git push origin main" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
