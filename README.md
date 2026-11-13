# OOP AI AGENT FRAMEWORK WITH OLLAMA API IN C++

Dự án Xây dựng Framework ReAct AI Agent bằng C++26, tích hợp với Ollama API (`qwen2.5:7b`), SQLite3 Persistent Vector Memory, Win32 GUI Agent Automation và Đa luồng Multi-Agent Coordination.

---

## 1. YÊU CẦU HỆ THỐNG & CÔNG CỤ (PREREQUISITES)

Để biên dịch và chạy dự án này trên một máy tính hoàn toàn mới, bạn cần cài đặt các công cụ sau:

* **Hệ điều hành**: Windows 10/11 (Do có sử dụng Win32 API cho GUI Automation), Linux, hoặc macOS.
* **C++ Compiler (GCC 15+)**: 
  * Cài đặt [MSYS2](https://www.msys2.org/).
  * Mở MSYS2 UCRT64 terminal và chạy: `pacman -S mingw-w64-ucrt-x86_64-gcc`
* **Native CMake**:
  * Tải bản cài đặt từ [cmake.org](https://cmake.org/download/) hoặc cài đặt qua Python: `pip install cmake` hoặc qua Winget: `winget install Kitware.CMake`.
  * *Lưu ý quan trọng:* KHÔNG sử dụng `cmake` mặc định của MSYS2 vì sẽ gây lỗi sinh file Makefile. Hãy đảm bảo CMake bản native của Windows được thêm vào biến môi trường `PATH`.
* **Thư viện phụ thuộc** (có thể cài qua MSYS2 UCRT64):
  * `pacman -S mingw-w64-ucrt-x86_64-curl`
  * `pacman -S mingw-w64-ucrt-x86_64-openssl`
  * `pacman -S mingw-w64-ucrt-x86_64-pkgconf`
  * `pacman -S mingw-w64-ucrt-x86_64-sqlite3`
  * `pacman -S mingw-w64-ucrt-x86_64-nlohmann-json`

---

## 2. CÀI ĐẶT OLLAMA HOẶC REMOTE AI BACKEND (LLM SETUP)

Dự án này hỗ trợ cả mô hình chạy cục bộ (Ollama) và các dịch vụ Remote AI/LLM Servers (như Google Colab, NVIDIA NIM, OpenRouter, Gemini API).

### 2.1. Cấu hình Ollama Cục Bộ (Local)
* Tải và cài đặt tại trang chủ: [https://ollama.com](https://ollama.com)
* Dịch vụ Ollama sẽ tự động chạy tại HTTP endpoint `http://localhost:11434`.

### 2.2. Cấu hình Remote AI Backend (Thay thế Ollama Local)
Nếu không chạy được Ollama trên máy cá nhân, bạn có thể kết nối Agent với các máy chủ AI từ xa:
- **Ngrok / Localtunnel (Kaggle/Colab)**: Chạy mô hình (ví dụ `gemma-2-9b-it`) trên đám mây và mở port bằng Ngrok.
- **NVIDIA NIM / OpenRouter / Gemini**: Cấu hình Base URL tương ứng.
- **Cách thiết lập**: Thiết lập qua biến môi trường (`LLM_BASE_URL`, `LLM_MODEL`), cờ CLI (`--base-url`, `--model`), hoặc cấu hình `agent::LLMConfig` truyền vào `OllamaClient::chat()`.
  *Ví dụ PowerShell:*
  ```powershell
  $env:LLM_BASE_URL="https://your-ngrok-url.ngrok-free.app"
  $env:LLM_MODEL="gemma2"
  ```
  *Ví dụ C++:*
  ```cpp
  agent::LLMConfig config;
  config.base_url = "https://your-ngrok-url.ngrok-free.app";
  config.model_name = "gemma2";

  agent::OllamaClient client;
  auto response = client.chat(history, config);
  ```
  *(Lưu ý: Khi sử dụng API Key cho OpenAI-compatible/OpenRouter/Gemini endpoint, thiết lập thêm biến môi trường `LLM_API_KEY`).*

### Bước 2: Tải mô hình LLM & Vector Embedding (Dành cho Local)
Mở PowerShell / Command Prompt và chạy lệnh:
```powershell
ollama pull qwen2.5:7b        # Mô hình suy luận chính (ReAct Agent)
ollama pull nomic-embed-text  # Mô hình nhúng (Vector Memory Embeddings)
```
*(Nếu RAM/VRAM yếu, có thể thay thế bằng `qwen2.5:3b` hoặc `qwen2.5:1.5b`)*.

### Bước 3: Kiểm tra kết nối
```powershell
curl http://localhost:11434/api/tags
```
*(Nếu JSON trả về có tên mô hình bạn vừa tải, hệ thống AI đã sẵn sàng).*

---

## 3. HƯỚNG DẪN BIÊN DỊCH

Dự án hỗ trợ biên dịch song song (`--parallel`) giúp giảm đáng kể thời gian build. 

### Biên dịch bằng MinGW Makefiles
Chuyển đến thư mục dự án: `cd "Path\To\Project"`

Mở PowerShell tại thư mục gốc của dự án và dán toàn bộ đoạn mã sau:

```powershell
# Step 1: Initialize current CMake path
$Cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source

# Step 2: Ignore MSYS2 CMake (causes errors) and find Native Windows CMake
if (-not $Cmake -or $Cmake -like "*msys64*") {
    Write-Host "[INFO] MSYS2 CMake detected or missing. Searching for native Windows CMake..." -ForegroundColor Yellow
    $CommonPaths = @(
        "$env:ProgramFiles\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\Python\*\Scripts\cmake.exe",
        "$env:APPDATA\Python\*\Scripts\cmake.exe"
    )
    $Cmake = Resolve-Path $CommonPaths -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Path
    
    if ($Cmake) {
        Write-Host "[OK] Found native CMake at: $Cmake" -ForegroundColor Green
    } else {
        Write-Error "[ERROR] Native Windows CMake not found. Please install CMake from cmake.org or via 'pip install cmake'."
        throw "CMake not found"
    }
}

# Step 3: Ensure GCC (MSYS2 UCRT64) is in the PATH for compilation
if ($env:PATH -notlike "*msys64\ucrt64\bin*") {
    $env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
}

# Step 4: Configure and Build the project (Forcing MSYS2 GCC to prevent MSVC conflicts)
Write-Host "[INFO] Cleaning previous build cache..." -ForegroundColor Cyan
if (Test-Path build) { Remove-Item -Recurse -Force build }

Write-Host "[INFO] Configuring project..." -ForegroundColor Cyan
& $Cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe"
if ($LASTEXITCODE -ne 0) { throw "Configuration failed with exit code $LASTEXITCODE" }

Write-Host "[INFO] Building project (limited parallel jobs to prevent Out-Of-Memory)..." -ForegroundColor Cyan
& $Cmake --build build --config Release --parallel 8
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

Write-Host "[OK] Build completed successfully." -ForegroundColor Green
```


## 4. HƯỚNG DẪN SỬ DỤNG VÀ KIỂM THỬ (TESTING & RUNNING)

### 4.1. Chạy Kiểm Thử Tự Động (Unit Tests)
Sau khi biên dịch thành công, chạy bộ kiểm thử toàn diện của dự án.

**Cách 1 (Tiêu chuẩn - CTest / PowerShell):** Chạy toàn bộ 9 test targets qua CTest hoặc script PowerShell:
```powershell
ctest --test-dir build --output-on-failure
```
Hoặc:
```powershell
.\run_tests.ps1
```

> **Troubleshooting: PowerShell Script Execution Error**
> Nếu bạn gặp lỗi báo rằng `.\run_tests.ps1 cannot be loaded` hoặc `is not digitally signed` khi chạy lệnh trên một máy mới, vấn đề phát sinh do các chính sách bảo mật mặc định của Windows. Để tạm thời bỏ qua điều này cho session hiện tại, hãy chạy lệnh sau trước khi thực thi các bài kiểm thử:
> ```powershell
> Set-ExecutionPolicy Bypass -Scope Process -Force
> ```

**Cách 2 (Thủ công):** Chạy từng file kiểm thử độc lập để dễ dàng theo dõi lỗi:
```powershell
.\build\test_client.exe
.\build\test_tools.exe
.\build\test_agent_loop.exe
.\build\test_evaluators.exe
.\build\test_loop_detector.exe
.\build\test_skill_loader.exe
```
Hệ thống sẽ chạy qua các Unit Test (Ollama Client, Tools, ReAct Loop, v.v.) để đảm bảo tính toàn vẹn của mã nguồn.

> **Kiểm Thử Các Tính Năng Bonus (GUI, Vector Memory, Multi-Agent)**
> 3 module Bonus được tách thành 3 file executable riêng biệt và đã được đăng ký trong `ctest` (cũng như chạy trong `.\run_tests.ps1`):
> ```powershell
> .\build\test_bonus_gui.exe
> .\build\test_bonus_vector_memory.exe
> .\build\test_bonus_multi_agent.exe
> ```
> *(Lưu ý: `test_bonus_gui.exe` gọi trực tiếp Win32 GUI API trên Windows).*

### 4.2. Chạy Đánh Giá Benchmark (`run_eval`)
**Chế độ Offline (`--mock`, không cần Ollama/Internet):**
Chạy 10/13 task offline bằng `MockLLMClient` để kiểm tra luồng ReAct, tool execution, SQLite và functional assertions (tự động bỏ qua `task-med-02`, `task_bonus_vector`, và `task_bonus_gui`):
```powershell
.\build\run_eval.exe --mock
```
*(Hoặc thiết lập biến môi trường `$env:LLM_MOCK="1"`).*

**Chế độ Live LLM (Ollama / Remote Server):**
Chạy bộ đánh giá đầy đủ với mô hình LLM thực tế (hỗ trợ cờ `--enable-gui`, `--base-url=URL`, `--model=MODEL`):
```powershell
$env:LLM_BASE_URL="http://localhost:11434"
$env:LLM_MODEL="qwen2.5:7b"
$env:EMBEDDING_MODEL="nomic-embed-text"
.\build\run_eval.exe --enable-gui
```
Kết quả hành vi của Agent (Thoughts, Actions, Latency) sẽ được xuất dưới dạng tệp JSON tại thư mục `trajectories/`.

### 4.3. Chạy AI Browser Automation Demo
Trình diễn khả năng Agent tương tác trực tiếp với giao diện Windows (GUI) để tự động hóa tìm kiếm trên web:

> **Lưu ý quan trọng:** Bạn cần bỏ tay khỏi chuột và bàn phím trong khoảng 15 giây để AI có thể tự động thực hiện các thao tác (nhập liệu, phím tắt) từng bước một cách chính xác nhất.

```powershell
$env:LLM_BASE_URL="http://localhost:11434"
$env:LLM_MODEL="qwen2.5:7b"
.\build\run_browser_demo.exe
```

### 4.4. Chạy Ứng Dụng CLI Trực Tiếp
Để tương tác trò chuyện trực tiếp với AI Agent:
```powershell
$env:LLM_BASE_URL="http://localhost:11434"
$env:LLM_MODEL="qwen2.5:7b"
.\build\agent_app.exe
```

---

## 5. CẤU TRÚC THƯ MỤC DỰ ÁN (DIRECTORY STRUCTURE)

```text
OOP-AI-Agent/
├── .github/workflows/ci.yml
├── CMakeLists.txt
├── README.md
├── run_tests.ps1
├── version.txt
├── include/utils/time_utils.hpp
├── src/
│   ├── main.cpp
│   ├── common/
│   │   └── cpp_features.h
│   ├── dto/
│   │   ├── message.h
│   │   ├── step.h
│   │   └── trajectory.h
│   ├── agent/
│   │   ├── agent_loop.h / .cpp
│   │   ├── loop_detector.h / .cpp
│   │   ├── skill_loader.h / .cpp
│   │   └── sub_agent_manager.h / .cpp
│   ├── client/
│   │   ├── llm_client.h
│   │   ├── mock_llm_client.h / .cpp
│   │   └── ollama_client.h / .cpp
│   ├── tools/
│   │   ├── tool.h
│   │   ├── tool_registry.h / .cpp
│   │   ├── calculator_tool.h / .cpp
│   │   ├── file_tool.h / .cpp
│   │   ├── exec_tool.h / .cpp
│   │   ├── memory_tool.h / .cpp
│   │   ├── web_tool.h / .cpp
│   │   ├── sub_agent_tool.h / .cpp
│   │   ├── custom_tool1.h / .cpp (DateTimeTool)
│   │   ├── custom_tool2.h / .cpp (SystemInfoTool)
│   │   ├── custom_tool3.h / .cpp (RegexExtractorTool)
│   │   ├── gui_tool.h / .cpp
│   │   └── vector_memory_tool.h / .cpp
│   └── harness/
│       ├── environment.h
│       ├── evaluator.h
│       ├── harness_runner.h / .cpp
│       ├── trajectory.h / .cpp
│       ├── keyword_evaluator.h / .cpp
│       ├── functional_evaluator.h / .cpp
│       └── vlm_evaluator.h / .cpp
├── skills/
│   ├── task_planner.md
│   ├── error_recovery.md
│   └── code_fixer.md
├── benchmark/
│   ├── tasks.json
│   ├── run_eval.cpp
│   └── run_browser_demo.cpp
├── tests/
│   ├── test_framework.h
│   ├── test_client.cpp
│   ├── test_tools.cpp
│   ├── test_loop_detector.cpp
│   ├── test_skill_loader.cpp
│   ├── test_agent_loop.cpp
│   ├── test_evaluators.cpp
│   ├── test_bonus_gui.cpp
│   ├── test_bonus_vector_memory.cpp
│   └── test_bonus_multi_agent.cpp
└── docs/
    ├── class_diagram.mmd / .png
    ├── sequence_diagram_agent.mmd / .png
    └── component_diagram.mmd / .png
```
