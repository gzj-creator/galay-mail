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

## v0.2.0 - 2026-05-12

- 版本级别：中版本（minor）
- Git 提交消息：`feat: 增强邮件客户端真实环境示例`
- Git Tag：`v0.2.0`
- 自述摘要：
  - 增强真实 SMTP 发送示例，生成带本地时间戳的测试邮件并输出服务端响应。
  - 示例程序统一通过 `RuntimeBuilder` 显式配置 IO scheduler，避免异步网络调用缺少 IO 调度器。
  - README 补充异步网络调用所需 runtime 配置，并加入 163 implicit TLS SMTP URL 示例。
