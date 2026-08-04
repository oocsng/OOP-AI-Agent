# OOP AI AGENT FRAMEWORK WITH OLLAMA API IN C++ (2026)

Dự án Xây dựng Framework ReAct AI Agent bằng C++26 tích hợp với Ollama API (`qwen2.5:7b`), SQLite3 Persistent Vector Memory, Win32 GUI Agent Automation và Đa luồng Multi-Agent Coordination.

---

## ⚙️ 1. HƯỚNG DẪN CÀI ĐẶT OLLAMA & CẤU HÌNH MÔ HÌNH (OLLAMA SETUP GUIDE)

Để dự án hoạt động chính xác trên máy mới, cần cài đặt dịch vụ **Ollama** và tải mô hình AI `qwen2.5:7b`.

### Bước 1: Tải và cài đặt Ollama
* Tải bản cài đặt cho Windows/Linux/macOS từ trang chủ: [https://ollama.com](https://ollama.com)
* Cài đặt Ollama và khởi chạy dịch vụ (Ollama Service sẽ tự động chạy tại giao thức HTTP endpoint `http://localhost:11434`).

### Bước 2: Tải mô hình LLM chính (`qwen2.5:7b`)
Mở PowerShell / Command Prompt và chạy lệnh:
```powershell
ollama pull qwen2.5:7b
```
*(Nếu muốn chạy thử nghiệm nhẹ hơn, có thể dùng `ollama pull qwen2.5:3b` hoặc `ollama pull qwen2.5:1.5b`)*.

### Bước 3: (Tùy chọn) Tải mô hình Multimodal VLM cho GUI Agent
Để chạy các tác vụ GUI Agent chụp màn hình phân tích hình ảnh:
```powershell
ollama pull nomic-embed-text    # Dùng cho Vector Memory Embeddings
```

### Bước 4: Kiểm tra kết nối Ollama API
Chạy lệnh kiểm tra Ollama service đã hoạt động:
```powershell
curl http://localhost:11434/api/tags
```
Nếu trả về danh sách JSON chứa `"name": "qwen2.5:7b"`, Ollama đã sẵn sàng!

---

### 🌐 CẤU HÌNH OLLAMA TỪ XA (REMOTE AI ON GOOGLE COLAB / CLOUD GPU)

Nếu máy tính cá nhân không đủ GPU/RAM để chạy mô hình `qwen2.5:7b` tại máy cục bộ (localhost), bạn có thể chạy Ollama miễn phí trên **Google Colab (T4 GPU)** và kết nối dự án C++ tới Colab:

#### 1. Chạy Ollama trên Google Colab Notebook:
Tạo 1 notebook Colab mới (chọn runtime GPU T4) và chạy các ô mã sau:

```python
# Ô 1: Cài đặt và khởi chạy Ollama Server trên Colab
!curl -fsSL https://ollama.com/install.sh | sh
import subprocess
subprocess.Popen(["ollama", "serve"])

# Ô 2: Tải mô hình Qwen2.5 7B về Colab GPU
!ollama pull qwen2.5:7b

# Ô 3: Mở cổng 11434 ra Internet bằng Ngrok (hoặc Cloudflare Tunnel)
!pip install pyngrok
from pyngrok import ngrok
# Đăng ký token miễn phí tại ngrok.com nếu chưa có: ngrok.set_auth_token("YOUR_NGROK_AUTHTOKEN")
public_url = ngrok.connect(11434)
print(">>> REMOTE OLLAMA URL của bạn là:", public_url.public_url)
```

#### 2. Kết nối C++ AI Agent Framework tới Remote Ollama Colab:
Thay vì sửa code C++, bạn có thể cấu hình URL từ xa qua 2 cách:

* **Cách A: Thiết lập Biến môi trường (PowerShell)**:
  ```powershell
  $env:OLLAMA_BASE_URL = "https://xxxx.ngrok-free.app"
  $env:OLLAMA_MODEL = "qwen2.5:7b"
  ```
* **Cách B: Chạy tham số CLI `--base-url`**:
  ```powershell
  cmd /c "set PATH=C:\msys64\ucrt64\bin;%PATH% && build\run_eval.exe --base-url=https://xxxx.ngrok-free.app --model=qwen2.5:7b --enable-gui"
  ```

---

## 📋 2. YÊU CẦU MÔI TRƯỜNG & THƯ VIỆN BÊN THỨ BA (REQUIREMENTS)

* **Hệ điều hành**: Windows 10/11 (Hỗ trợ Win32 API), Linux, macOS
* **Trình biên dịch**: GCC 15+ (MSYS2 UCRT64 hỗ trợ `-std=c++26`) / MSVC / Clang
* **Công cụ build**: CMake >= 3.21, CTest, MinGW Makefiles
* **Thư viện phụ thuộc**:
  * `libcurl`: Gửi HTTP POST JSON Request tới Ollama API.
  * `nlohmann/json`: Parse & serialize JSON (payload, tool args, trajectory).
  * `sqlite3`: CSDL bộ nhớ persistent Key-Value & Vector Storage.

---

### 🛠️ 3. HƯỚNG DẪN BIÊN DỊCH BẰNG LỆNH THỦ CÔNG (MANUAL STEP-BY-STEP BUILD)

> [!NOTE]
> **Lưu ý quan trọng khi chạy thủ công**: Trong môi trường MSYS2 UCRT64, thư mục `C:\msys64\ucrt64\bin` có chứa một file `cmake.exe` của MSYS2 (yêu cầu môi trường POSIX shell). Khi thêm `C:\msys64\ucrt64\bin` vào đầu `$env:PATH`, lệnh `cmake` mặc định có thể bị gọi nhầm vào `cmake.exe` của MSYS2 gây lỗi không thực thi trong PowerShell thông thường.
> 
> Vì vậy, khi thực hiện từng bước thủ công trong PowerShell, cần lưu lại đường dẫn `cmake` hệ thống (Kitware/Python standalone CMake) trước khi thiết lập PATH:

```powershell
# Bước 0: Mở PowerShell và di chuyển vào thư mục gốc dự án
cd "D:\vsc projects\OOP AI Agents 2026"

# Bước 1: Lưu vị trí CMake hệ thống & Thêm compiler g++ UCRT64 vào PATH
$Cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $Cmake -or $Cmake -like "*msys64*") {
    $Cmake = "C:\Users\segou\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe"
}
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH

# Bước 2: Khởi tạo cấu hình CMake MinGW
& $Cmake -B build -S . -G "MinGW Makefiles"

# Bước 3: Biên dịch toàn bộ 12 mục tiêu (Song song 8 luồng)
& $Cmake --build build --parallel 8

# Bước 4: Chạy kiểm thử tự động CTest
ctest --test-dir build --output-on-failure
```

### 🚀 Cách nhanh nhất & An toàn nhất (Khuyên dùng):
Chạy script tự động đã xử lý sẵn xung đột đường dẫn:
```powershell
.\build.ps1
```

---

## 🧪 4. HƯỚNG DẪN KIỂM THỬ VÀ XÁC NHẬN TOÀN BỘ DỰ ÁN (TESTING GUIDE)

### 4.1. Chạy Tự động 9 Unit Tests (CTest Suite)

#### Cách 1: Sử dụng Kịch bản 1-Click PowerShell
```powershell
.\build.ps1
```

#### Cách 2: Chạy qua lệnh CTest
```powershell
ctest --test-dir build --output-on-failure
```

**Danh sách 9 Unit Test đạt 100% PASS**:
1. `test_client.exe`: OllamaClient JSON payload, base URL & HTTP exception handling.
2. `test_tools.exe`: ToolRegistry allow/deny policy & 5 core + 3 custom tools.
3. `test_loop_detector.exe`: Thuật toán phát hiện vòng lặp Generic Repeat & Ping-Pong.
4. `test_skill_loader.exe`: Parse header markdown skill files & keyword auto-injection.
5. `test_agent_loop.exe`: ReAct loop parsing & Observer step_hook interface.
6. `test_evaluators.exe`: KeywordEvaluator & FunctionalEvaluator (Strategy pattern).
7. `test_bonus_gui.exe`: GUI action parsing, screenshot Base64 encoding & Win32 events.
8. `test_bonus_vector_memory.exe`: SQLite vector embedding & Cosine Similarity math.
9. `test_bonus_multi_agent.exe`: Thread-safe MessageQueue & sub-agent thread spawning.

Chạy từng unit test riêng lẻ nếu cần:
```powershell
.\build\test_client.exe
.\build\test_tools.exe
.\build\test_loop_detector.exe
```

---

### 4.2. Chạy Bộ Đánh Giá Benchmark (`run_eval.exe`)

Đánh giá tự động trên 12 nhiệm vụ benchmark (`benchmark/tasks.json`):

```powershell
cmd /c "set PATH=C:\msys64\ucrt64\bin;%PATH% && build\run_eval.exe --enable-gui"
```

**Kết quả xác nhận**:
* **Success Rate**: 100.00% (12/12 tasks passed).
* **Trajectory Output**: Xuất 12 file nhật ký JSON tại `trajectories/trajectory_*.json`.

---

### 4.3. Chạy Demo Tự Động Hóa Trình Duyệt (GUI Agent Browser Demo)

Chạy kịch bản GUI Agent tự động điều khiển trình duyệt Edge/Chrome:

```powershell
cmd /c "set PATH=C:\msys64\ucrt64\bin;%PATH% && build\run_browser_demo.exe"
```

**Các thao tác thực hiện**:
1. Khởi chạy trình duyệt web.
2. Tự động di chuyển chuột & click ô tìm kiếm.
3. Nhập từ khóa tìm kiếm và nhấn Enter.
4. Chụp ảnh màn hình `screenshot.bmp` mã hóa Base64.
5. Trích xuất văn bản kết quả vào tệp `browser_result.txt`.

---

### 4.4. Chạy Ứng Dụng AI Agent Chính (CLI Interactive App)

Khởi chạy CLI Agent tương tác dòng lệnh ReAct:

```powershell
cmd /c "set PATH=C:\msys64\ucrt64\bin;%PATH% && build\agent_app.exe"
```

---

## 🗂️ 5. CẤU TRÚC THƯ MỤC DỰ ÁN (DIRECTORY STRUCTURE)

```text
OOP AI Agents 2026/
├── CMakeLists.txt
├── README.md
├── GIT_COMMIT_PLAN.md
├── AI_Agent_Coding_Tasks.md
├── build.ps1
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
│   ├── test_client.cpp
│   ├── test_tools.cpp
│   ├── test_loop_detector.cpp
│   ├── test_skill_loader.cpp
│   ├── test_agent_loop.cpp
│   ├── test_evaluators.cpp
│   ├── test_bonus_gui.cpp
│   ├── test_bonus_vector_memory.cpp
│   └── test_bonus_multi_agent.cpp
├── scripts/
│   └── push_commits.ps1
└── docs/
    ├── class_diagram.mmd / .png
    ├── sequence_diagram_agent.mmd / .png
    └── component_diagram.mmd / .png
```
