> **IMPORTANT CONTEXT**: 
> Bắt buộc đọc và tuân thủ các quy tắc bổ sung dành cho Gemini API / Gemma Model tại file: [GEMINI.md](./GEMINI.md).
# AGENT BEHAVIOR RULES

1. Luôn đọc `AI_Agent_Coding_Tasks.md` trước khi làm bất kỳ việc gì.
2. Thực hiện công việc theo đúng thứ tự từ Phase 1 -> Phase 8.
3. Nguyên tắc hoàn thành 1 Task:
   - Viết code C++17/20/23/26 đúng thiết kế OOP.
   - Biên dịch thử bằng `cmake --build build`.
   - Nếu bị lỗi -> tự sửa đến khi build thành công.
   - Khi build thành công -> Đổi `[ ]` thành `[x]` trong `AI_Agent_Coding_Tasks.md`.
4. Khi làm xong 1 Task, tự động chuyển sang Task tiếp theo chưa được tick.