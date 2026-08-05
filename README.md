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
- **Cách thiết lập**: Thay đổi Base URL và Model Name trong mã nguồn khi khởi tạo Client hoặc qua biến môi trường.
  *Ví dụ C++:*
  ```cpp
  // Chạy qua endpoint của Colab Ngrok thay vì localhost
  OllamaClient client("https://your-ngrok-url.ngrok-free.app", "gemma2");
  ```
  *(Lưu ý: Nếu sử dụng API Key cho OpenRouter/Gemini, hãy thêm Header `Authorization: Bearer <KEY>` vào HTTP request của `OllamaClient`).*

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

## 3. HƯỚNG DẪN BIÊN DỊCH TỐC ĐỘ CAO (ACCELERATED BUILD)

Dự án hỗ trợ biên dịch song song (`--parallel`) giúp giảm đáng kể thời gian build. 

- **Cách 1 (Tiêu chuẩn)**: Sử dụng cấu hình MinGW Makefiles. (An toàn cho hệ thống ít RAM).
- **Cách 2 (Siêu tốc - Khuyến nghị)**: Cài đặt Ninja (`pacman -S mingw-w64-ucrt-x86_64-ninja`) và biên dịch bằng Ninja để nhanh gấp 5 lần.

### Cách 1: Biên dịch bằng MinGW Makefiles (Tiêu chuẩn)
Mở PowerShell tại thư mục gốc của dự án và dán toàn bộ đoạn mã sau:

```powershell
# Step 0: Di chuyển vào thư mục dự án (Thay đổi đường dẫn này thành thư mục chứa code của bạn)
cd "Path\To\Project"

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
& $Cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

Write-Host "[OK] Build completed successfully." -ForegroundColor Green
```

### Cách 2: Biên dịch bằng Ninja (Siêu tốc)
Yêu cầu đã cài đặt Ninja (`pacman -S mingw-w64-ucrt-x86_64-ninja`). Dán đoạn mã này vào PowerShell:

```powershell
# Step 0: Di chuyển vào thư mục dự án
cd "Path\To\Project"

# Step 1: Initialize current CMake path
$Cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source

# Step 2: Ignore MSYS2 CMake and find Native Windows CMake
if (-not $Cmake -or $Cmake -like "*msys64*") {
    $CommonPaths = @(
        "$env:ProgramFiles\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\Python\*\Scripts\cmake.exe",
        "$env:APPDATA\Python\*\Scripts\cmake.exe"
    )
    $Cmake = Resolve-Path $CommonPaths -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Path
    
    if (-not $Cmake) {
        Write-Error "[ERROR] Native Windows CMake not found. Please install CMake from cmake.org or via 'pip install cmake'."
        throw "CMake not found"
    }
}

# Step 3: Ensure GCC (MSYS2 UCRT64) and Ninja are in the PATH
if ($env:PATH -notlike "*msys64\ucrt64\bin*") {
    $env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
}

# Step 4: Configure and Build with Ninja
Write-Host "[INFO] Cleaning previous build cache..." -ForegroundColor Cyan
if (Test-Path build) { Remove-Item -Recurse -Force build }

Write-Host "[INFO] Configuring project with Ninja..." -ForegroundColor Cyan
& $Cmake -B build -S . -G "Ninja" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe"
if ($LASTEXITCODE -ne 0) { throw "Configuration failed with exit code $LASTEXITCODE" }

Write-Host "[INFO] Building project with Ninja (limited parallel jobs to prevent Out-Of-Memory)..." -ForegroundColor Cyan
& $Cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

Write-Host "[OK] Build completed successfully." -ForegroundColor Green
```

---

## 4. HƯỚNG DẪN SỬ DỤNG VÀ KIỂM THỬ (TESTING & RUNNING)

### 4.1. Chạy Kiểm Thử Tự Động (Unit Tests)
Sau khi biên dịch thành công, chạy bộ kiểm thử toàn diện của dự án. 

**Cách 1 (Tiêu chuẩn):** Sử dụng CTest:
```powershell
ctest --test-dir build --output-on-failure
```

**Cách 2 (Thay thế nếu CTest bị lỗi):** Sử dụng script kiểm thử độc lập:
```powershell
.\run_tests.ps1
```

**Cách 3 (Thủ công):** Chạy từng file kiểm thử độc lập để dễ dàng theo dõi lỗi:
```powershell
.\build\test_client.exe
.\build\test_tools.exe
.\build\test_agent_loop.exe
.\build\test_evaluators.exe
.\build\test_loop_detector.exe
.\build\test_skill_loader.exe
```
Hệ thống sẽ chạy qua các Unit Test (Ollama Client, Tools, ReAct Loop, v.v.) để đảm bảo 100% tính nguyên vẹn của mã nguồn.

> **QUAN TRỌNG: Kiểm Thử Các Tính Năng Bonus (GUI, Vector Memory, Multi-Agent)**  
> Cả 3 tính năng Bonus được gom chung vào một file executable duy nhất và **tách biệt hoàn toàn** khỏi bộ `ctest` và `unit_tests.exe` tiêu chuẩn.  
> *Lý do*: Việc kiểm thử tự động giao diện (GUI Agent) thao tác trực tiếp lên Win32 API, chiếm quyền điều khiển chuột/bàn phím và mở cửa sổ thực tế. Việc cô lập này giữ cho các test tiêu chuẩn chạy ẩn (headless), nhanh chóng, và không gây loạn focus màn hình.  
> **Cách chạy (Dành cho Graders/Người chấm điểm):**  
> Chạy trực tiếp file thực thi độc lập để test toàn bộ các tính năng Bonus (GUI, Vector Memory, Multi-Agent):  
> ```powershell
> .\build\test_bonus_features.exe
> ```
> *(Hoặc `.\build\Release\test_bonus_features.exe` tùy vào trình biên dịch).*

### 4.2. Chạy Đánh Giá Benchmark (`run_eval`)
Chạy bộ tự động giải quyết 13 nhiệm vụ thực tế đa dạng (Simple, Medium, Hard, Bonus):
```powershell
$env:LLM_BASE_URL="http://localhost:11434"
$env:LLM_MODEL="qwen2.5:7b"
.\build\run_eval.exe --enable-gui
```
Kết quả hành vi của Agent (Thoughts, Actions, Latency) sẽ được xuất dưới dạng tệp JSON tại thư mục `trajectories/`.

### 4.3. Chạy AI Browser Automation Demo
Trình diễn khả năng Agent tương tác trực tiếp với giao diện Windows (GUI) để tự động hóa tìm kiếm trên web:
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
OOP AI Agents 2026/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── agent/
│   │   ├── agent_loop.h / .cpp
│   │   ├── loop_detector.h / .cpp
│   │   ├── skill_loader.h / .cpp
│   │   └── sub_agent_manager.h / .cpp
│   ├── client/
│   │   ├── llm_client.h
│   │   └── ollama_client.h / .cpp
│   ├── tools/
│   │   ├── tool.h
│   │   ├── tool_registry.h / .cpp
│   │   ├── calculator_tool.h / .cpp
│   │   ├── file_tool.h / .cpp
│   │   ├── exec_tool.h / .cpp
│   │   ├── memory_tool.h / .cpp
│   │   ├── web_tool.h / .cpp
│   │   ├── custom_tool1.h / .cpp (DateTimeTool)
│   │   ├── custom_tool2.h / .cpp (SystemInfoTool)
│   │   ├── custom_tool3.h / .cpp (RegexExtractorTool)
│   │   ├── gui_tool.h / .cpp
│   │   └── vector_memory_tool.h / .cpp
│   └── harness/
│       ├── harness_runner.h / .cpp
│       ├── trajectory.h / .cpp
│       ├── evaluator.h
│       ├── keyword_evaluator.h / .cpp
│       └── functional_evaluator.h / .cpp
├── skills/
│   ├── task_planner.md
│   ├── error_recovery.md
│   └── code_fixer.md
├── benchmark/
│   ├── tasks.json
│   ├── run_eval.cpp
│   └── run_browser_demo.cpp
├── tests/
│   └── (Unit tests for all components)
└── docs/
    ├── class_diagram.mmd / .png
    ├── sequence_diagram_agent.mmd / .png
    └── component_diagram.mmd / .png
```
