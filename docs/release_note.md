# Release Note

按时间顺序追加版本记录，避免覆盖历史发布说明。

## v0.1.0 - 2026-05-12

- 版本级别：中版本（minor）
- Git 提交消息：`feat: 初始化 galay-mail 客户端库`
- Git Tag：`v0.1.0`
- 自述摘要：
  - 初始化 `galay-mail` 独立 CMake package，提供 SMTP/POP3 URL 解析、协议解析和命令编码。
  - 新增 AUTH PLAIN/LOGIN 辅助函数，以及 plain、implicit TLS、STARTTLS/STLS 三种连接模式的异步客户端 API。
  - 补齐 module interface、示例、README 和离线单元测试。
