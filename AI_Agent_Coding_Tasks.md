# BẢNG CÔNG VIỆC KỸ THUẬT & HƯỚNG DẪN TRIỂN KHAI CODE (AI AGENT C++17+)

Tài liệu này tổng hợp toàn bộ danh sách công việc (Checklist) phần mã nguồn (Coding Task List) cần hoàn thành cho đồ án **Xây dựng AI Agent Framework với Ollama API bằng C++**. Tài liệu được thiết kế dành cho các AI Coding Agent (Cursor, Claude, Windsurf, Copilot...) hoặc các thành viên nhóm dễ dàng theo dõi và triển khai từng bước.

---

## THÔNG TIN CHUNG VỀ DỰ ÁN

* **Ngôn ngữ bắt buộc**: C++17 (tối thiểu 4 tính năng), C++20 (tối thiểu 2 tính năng), C++23 (tối thiểu 2 tính năng), C++26 (tối thiểu 1 tính năng).
* **Thư viện bên thứ ba**:
  * `libcurl`: Xử lý HTTP Request gửi tới Ollama API / SearXNG / DDG API.
  * `nlohmann/json`: Parse và serialize JSON dữ liệu, tool call, trajectory.
  * `sqlite3`: Lưu trữ và truy vấn bộ nhớ persistent.
* **Mô hình Design Patterns bắt buộc**:
  1. **Strategy Pattern**: Dùng cho bộ danh mục đánh giá `Evaluator` (`KeywordEvaluator`, `FunctionalEvaluator`, `VLMEvaluator`).
  2. **Template Method Pattern**: Dùng cho `AgentLoop::run()` định nghĩa bộ khung thuật toán ReAct.
  3. **Registry/Factory Pattern**: Dùng cho `ToolRegistry` quản lý, tạo và tra cứu Tool dynamic.
  4. **Observer/Hook Pattern**: Dùng cho `HarnessRunner` chèn `step_hook` vào `AgentLoop` để ghi nhận nhật ký (trajectory).

---

## CẤU TRÚC THƯ MỤC CẦN KHỞI TẠO

```text
Agent_MSSV1_MSSV2_MSSV3/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── agent/
│   │   ├── agent_loop.h / .cpp
│   │   ├── loop_detector.h / .cpp
│   │   └── skill_loader.h / .cpp
│   ├── client/
│   │   ├── llm_client.h             (Abstract Base Class)
│   │   └── ollama_client.h / .cpp
│   ├── tools/
│   │   ├── tool.h                   (Abstract Base Class)
│   │   ├── tool_registry.h / .cpp
│   │   ├── exec_tool.cpp
│   │   ├── file_tool.cpp
│   │   ├── web_tool.cpp
│   │   ├── memory_tool.cpp
│   │   ├── calculator_tool.cpp
│   │   ├── custom_tool1.cpp         (Tool bổ sung 1)
│   │   ├── custom_tool2.cpp         (Tool bổ sung 2)
│   │   └── custom_tool3.cpp         (Tool bổ sung 3)
│   └── harness/
│       ├── harness_runner.h / .cpp
│       ├── trajectory.h / .cpp
│       ├── evaluator.h              (Abstract Base Class)
│       ├── keyword_evaluator.cpp
│       └── functional_evaluator.cpp
├── skills/
│   ├── task_planner.md
│   ├── error_recovery.md
│   └── [custom_skill].md
├── benchmark/
│   ├── tasks.json
│   └── run_eval.cpp
├── tests/
│   └── [unit_tests].cpp
└── docs/
    ├── class_diagram.png
    ├── sequence_diagram_agent.png
    └── component_diagram.png
```

---

## CHECKLIST CÁC TẦNG MÃ NGUỒN CẦN LẬP TRÌNH

### Phase 1: Khởi tạo Project & Infrastructure
- [x] Setup file `CMakeLists.txt` liên kết thành công `libcurl`, `nlohmann_json`, `sqlite3` và bật flag chuẩn C++23/C++26 (ví dụ: `-std=c++23` hoặc `-std=c++2b`).
- [x] Xây dựng các struct / class chứa dữ liệu dùng chung (DTO):
  - [x] `Message` (`role`: system/user/assistant/tool, `content`: string, `images`: `std::vector<std::string>` chứa Base64).
  - [x] `Step` (step_id, thought, action, tool_result, tokens_used, latency_ms).
  - [x] `Trajectory` (task_id, model, success, total_tokens, total_time_ms, vector<Step>).

---

### Phase 2: Tầng LLM Client (`LLMClient` & `OllamaClient`)
- [x] Define abstract interface `LLMClient`:
  - [x] Pure virtual method `virtual Response chat(const std::vector<Message>& history, const Config& config) = 0;`
- [x] Triển khai `OllamaClient` kế thừa `LLMClient`:
  - [x] Tích hợp `libcurl` gửi HTTP POST tới Ollama Endpoint (`/api/chat` hoặc `/v1/chat/completions`).
  - [x] Hỗ trợ cấu hình `base_url`, `model_name`, `temperature`, `max_tokens`.
  - [x] Hỗ trợ **Multimodal** (chuyển đổi file ảnh thành base64 và gửi kèm payload `images`).
  - [x] Bắt lỗi nâng cao: HTTP status code != 200, Timeout, Connection Refused, JSON Response malformed/corrupted (dùng C++ Exception handling).

---

### Phase 3: Tầng Tool Registry & Triển khai Tools
- [x] Define abstract interface `Tool`:
  - [x] Virtual methods: `name()`, `description()`, `execute(const nlohmann::json& args)`.
- [x] Triển khai `ToolRegistry` (Registry / Factory Pattern):
  - [x] Quản lý các Tool instance qua Smart Pointers (`std::shared_ptr<Tool>` hoặc `std::unique_ptr<Tool>`).
  - [x] Đăng ký tool động ở Runtime qua `register_tool()`.
  - [x] Hỗ trợ cơ chế **Policy Allow / Deny**: cho phép/cấm gọi danh sách tool cụ thể.
  - [x] Xuất ra Schema định dạng JSON/Text mô tả danh sách tool để inject vào System Prompt cho LLM đọc.
- [x] Triển khai **5 Tool bắt buộc**:
  - [x] `exec_tool`: Sử dụng `popen()` / `exec()` trên Linux/Windows để thực thi lệnh shell, trả về stdout/stderr.
  - [x] `read_file` / `write_file` tool: Thao tác đọc/ghi file an toàn bằng `std::filesystem` và C++ `fstream`.
  - [x] `web_search`: Gọi HTTP Request tới SearXNG hoặc DuckDuckGo API để lấy kết quả tìm kiếm web.
  - [x] `memory_save` / `memory_search`: Lưu thông tin và tìm kiếm thông tin bằng từ khóa trong CSDL `SQLite3`.
  - [x] `calculator`: Parser biểu thức số học (ví dụ: cộng, trừ, nhân, chia, lũy thừa).
- [x] Triển khai **ít nhất 3 Tool bổ sung** (thuộc 3 loại khác nhau):
  - [x] Tool 1 (ví dụ: `get_system_time` / `date_time_tool`).
  - [x] Tool 2 (ví dụ: `system_info_tool` - đọc CPU, RAM usage).
  - [x] Tool 3 (ví dụ: `json_parser_tool` / `regex_extractor_tool`).

---

### Phase 4: Tầng Skill System (`SkillLoader`)
- [x] Viết lớp `SkillLoader`:
  - [x] Quản lý đọc các file `.md` trong thư mục `skills/` bằng `std::filesystem`.
  - [x] Giải mã nội dung header / keyword của skill file.
  - [x] Thuật toán **Keyword Matching**: Tự động quét Prompt người dùng để chọn danh sách `.md` phù hợp.
  - [x] Phương thức ghép (inject) nội dung Skill vào System Prompt trước khi khởi tạo lượt chạy Agent.
- [x] Viết nội dung **3 file skill markdown thật**:
  - [x] `skills/task_planner.md`: Hướng dẫn LLM lập kế hoạch chia nhỏ nhiệm vụ phức tạp.
  - [x] `skills/error_recovery.md`: Hướng dẫn LLM cách xử lý khi Tool trả về lỗi.
  - [x] `skills/code_fixer.md`: Hướng dẫn chi tiết cho một tác vụ chuyên biệt (ví dụ: tổng hợp tin tức hoặc sửa lỗi file).

---

### Phase 5: Tầng ReAct Agent Loop & Loop Detection
- [x] Triển khai `LoopDetector`:
  - [x] Phát hiện vòng lặp 1: **Generic Repeat** (LLM gọi liên tục 1 tool với tham số giống hệt nhau $N$ lần).
  - [x] Phát hiện vòng lặp 2: **Ping-Pong Loop** (LLM gọi luân phiên Tool A $\leftrightarrow$ Tool B lặp đi lặp lại).
  - [x] Cấu hình ngưỡng cảnh báo (`Warning`) và ngưỡng ngắt (`Critical Stop`).
- [x] Triển khai `AgentLoop` (Sử dụng Template Method Pattern):
  - [x] Quản lý vòng lặp ReAct: `Observe` $\rightarrow$ `Think` $\rightarrow$ `Act` $\rightarrow$ `Observe`.
  - [x] Parse Tool Calls từ LLM output (dùng Regex hoặc `nlohmann::json`).
  - [x] Giới hạn bước `max_steps` để tránh tràn vô tận.
  - [x] Expose Hook Interface (`step_hook`) để thông báo cho Harness ghi log mà **không được include/phụ thuộc vào Harness**.

---

### Phase 6: Tầng Harness Engineering & Evaluators
- [x] Xây dựng abstract interface `Evaluator` (Strategy Pattern):
  - [x] Interface method: `virtual EvalResult evaluate(const Trajectory& trajectory, const TaskSpec& spec) = 0;`
- [x] Triển khai các lớp Evaluator cụ thể:
  - [x] `KeywordEvaluator`: Kiểm tra từ khóa bắt buộc phải xuất hiện trong câu trả lời cuối hoặc file kết quả.
  - [x] `FunctionalEvaluator`: Chạy file kịch bản/lệnh shell test (ví dụ: `test -f result.txt && grep 255 result.txt`) để kiểm tra kết quả thực tế.
- [x] Triển khai `HarnessRunner`:
  - [x] Thiết lập môi trường chạy (Environment setup).
  - [x] Đăng ký `step_hook` vào `AgentLoop` (Observer Pattern).
  - [x] Thu thập dữ liệu latency (ms), số token đã dùng, danh sách các bước.
  - [x] Ghi xuất Trajectory ra file JSON đúng chuẩn format `trajectory_{task_id}.json`.
- [x] Xây dựng bộ chương trình Batch Benchmark Runner (`benchmark/run_eval.cpp`):
  - [x] Đọc tập nhiệm vụ từ `benchmark/tasks.json`.
  - [x] Chạy tự động tập task, tính toán tỉ lệ thành công (`Success Rate`), tổng thời gian và tổng token tiêu tốn.

---

### Phase 7: Xây dựng Tập Benchmark (`tasks.json`)
- [x] Đảm bảo chuẩn bị đủ **10 Task**:
  - [x] **4 Task Đơn giản**: Tính toán biểu thức, đọc/ghi file đơn, lấy thời gian hệ thống.
  - [x] **4 Task Trung bình**: Kết hợp 2-3 tool liên tiếp (ví dụ: tính toán rồi ghi file, hoặc tìm kiếm rồi tổng hợp).
  - [x] **2 Task Khó**: Multi-step phức tạp đòi hỏi agent tự suy luận thứ tự gọi tool và phục hồi lỗi khi tool thất bại.

---

### Phase 8: Kiểm tra Tính tuân thủ C++ Technical Requirements
- [x] **Tính năng C++17 (Tối thiểu 4)**:
  - [x] `std::unique_ptr` / `std::shared_ptr` quản lý bộ nhớ Tool/Evaluator.
  - [x] `std::function` + Lambda dùng cho `step_hook` và tool callback.
  - [x] `std::variant` / `std::visit` xử lý Action types.
  - [x] `std::filesystem` quét thư mục `skills/` và kiểm tra file.
  - [x] `Structured Bindings` unpack giá trị trả về.
  - [x] `std::optional<T>` trả về kết quả tool / partial score.
- [x] **Tính năng C++20 (Tối thiểu 2)**:
  - [x] `std::format` định dạng chuỗi prompt / log.
  - [x] `Concepts` giới hạn kiểu dữ liệu mẫu (ví dụ: trong Template Registry).
  - [x] `std::string::starts_with` / `ends_with` hoặc `std::span`.
- [x] **Tính năng C++23 (Tối thiểu 2)**:
  - [x] `std::print` / `std::println` thay thế `std::cout`.
  - [x] `std::expected<T, E>` xử lý lỗi minh bạch thay thế exception hoặc trả về `nullopt`.
  - [x] `std::string::contains` kiểm tra chuỗi đơn giản.
- [x] **Tính năng C++26 (Tối thiểu 1)**:
  - [x] Tận dụng tính năng chuẩn C++26 (ví dụ: `static_assert` nâng cao không cần message chuỗi, hoặc các hàm thư viện mới được trình biên dịch hỗ trợ như `std::saturation_arithmetic` / `std::inplace_vector`).

---

## CÁC TÍNH NĂNG ĐIỂM THƯỞNG (TỐI ĐA +15 ĐIỂM)

- [x] **Bonus 1: GUI Agent (+8 điểm)**
  - [x] Viết tool `capture_screenshot()` chụp màn hình desktop.
  - [x] Gửi ảnh chụp màn hình sang VLM (Qwen2-VL / Gemma4 qua Ollama).
  - [x] Thực thi các hành động GUI `click(x, y)`, `type_text()`, `key_press()`, `sleep_ms()`, `get_clipboard_text()` thông qua Win32 API / OS calls.
  - [x] **Browser Automation Demo (`run_browser_demo`)**: Mở trình duyệt web Edge/Chrome, tự động gõ từ khóa tìm kiếm, bấm Enter, đợi kết quả, chụp màn hình và trích xuất ghi tệp `browser_result.txt`.
- [x] **Bonus 2: Persistent Memory với Vector Search (+4 điểm)**
  - [x] Tích hợp mô hình `nomic-embed-text` qua Ollama để sinh Vector Embeddings cho nội dung bộ nhớ.
  - [x] Triển khai thuật toán Cosine Similarity bằng C++ để tìm kiếm bộ nhớ tương đồng theo ngữ nghĩa.
- [x] **Bonus 3: Multi-Agent Coordination (+3 điểm)**
  - [x] Triển khai `HarnessRunner` có khả năng tạo Sub-Agent chạy trên Thread mới (`std::thread` / `std::jthread`).
  - [x] Cho phép các Agent truyền nhận thông điệp qua Hàng đợi an toàn đa luồng (`std::queue` kết hợp `std::mutex` & `std::condition_variable`).

---

### Phase 9: Unit Testing Suite & CTest Integration
- [x] **Core Module Unit Tests (`tests/`)**:
  - [x] `tests/test_client.cpp`: OllamaClient JSON payload formatting, base URL configuration, offline exception handling.
  - [x] `tests/test_tools.cpp`: ToolRegistry allow/deny policies, registration/lookup, 5 core tools & 3 custom tools.
  - [x] `tests/test_loop_detector.cpp`: Detection logic for Generic Repeat & Ping-Pong loops with threshold limits.
  - [x] `tests/test_skill_loader.cpp`: Skill markdown parsing, keyword matching algorithm, prompt injection.
  - [x] `tests/test_agent_loop.cpp`: ReAct parsing, step_hook Observer pattern.
  - [x] `tests/test_evaluators.cpp`: KeywordEvaluator accuracy & FunctionalEvaluator command execution.
- [x] **Bonus Feature Unit Tests (`tests/`)**:
  - [x] `tests/test_bonus_gui.cpp`: GUI action parsing (click/type/key/screenshot) & base64 screenshot conversion.
  - [x] `tests/test_bonus_vector_memory.cpp`: Cosine similarity math calculations & vector embedding storage/query.
  - [x] `tests/test_bonus_multi_agent.cpp`: Thread-safe Message Queue (push/pop synchronization) & sub-agent thread spawning.
- [x] **CMake & CTest Integration**:
  - [x] `enable_testing()` included in `CMakeLists.txt`.
  - [x] Executable targets linking test files with `src/` modules, libcurl, nlohmann_json, sqlite3, gdi32.
  - [x] Registered all 9 tests with CTest via `add_test()`.
  - [x] 100% CTest pass rate verified (`ctest --test-dir build --output-on-failure`).

---

### Phase 10: UML Architecture Diagrams & Documentation (`docs/`)
- [x] **Class Diagram (`docs/class_diagram.mmd` & `class_diagram.png`)**: Complete class hierarchy covering abstract interfaces (`LLMClient`, `Tool`, `Evaluator`), concrete tools, `OllamaClient`, `AgentLoop`, `LoopDetector`, `SkillLoader`, `SubAgentManager`, and `HarnessRunner`.
- [x] **Sequence Diagram (`docs/sequence_diagram_agent.mmd` & `sequence_diagram_agent.png`)**: Detailed step-by-step ReAct loop interaction sequence between User, Harness, AgentLoop, LLMClient, LoopDetector, ToolRegistry, Tool, and StepHook.
- [x] **Component Diagram (`docs/component_diagram.mmd` & `component_diagram.png`)**: System module components and external third-party library dependencies (`libcurl`, `nlohmann_json`, `sqlite3`, `Win32 API`, `Ollama`).

---

## QUY ĐỊNH COMMIT GIT VÀ QUẢN LÝ MÃ NGUỒN

1. **Số lượng Commit**:
   * Nhóm 1 thành viên: $\ge 6$ commits.
   * Nhóm 2 thành viên: $\ge 12$ commits (mỗi người tối thiểu 6 commits).
   * Nhóm 3 thành viên: $\ge 18$ commits (mỗi người tối thiểu 6 commits).
2. **Tần suất Commit**: Không được cách nhau quá **7 ngày** giữa 2 commit liên tiếp.
3. **Phân chia công việc**: Khối lượng dòng code / commit của các thành viên không lệch quá **20%**.
4. **Cấu hình Repo**: Đảm bảo tạo **Personal Access Token (Read-Only)** để cung cấp cho giảng viên chấm bài.

---
*Tài liệu này được tạo tự động nhằm hỗ trợ quá trình lập trình và theo dõi tiến độ cho dự án OOP AI Agent 2026.*
