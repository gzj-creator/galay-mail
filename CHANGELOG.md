# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `## [Unreleased]`。
- 发布时将累计变更整理到 `## [vX.Y.Z] - YYYY-MM-DD`。
- 版本号遵循语义化版本：大改动升主版本，新功能升次版本，修复和维护升修订版本。

## [Unreleased]

## [v0.2.0] - 2026-05-12

### Added
- 增强真实 SMTP 发送示例，生成带本地时间戳的测试邮件并输出服务端响应。

### Changed
- 示例程序统一通过 `RuntimeBuilder` 显式配置 IO scheduler，避免异步网络调用缺少 IO 调度器。
- SMTP 示例返回明确进程状态码，并补充负响应、异常发送结果和连接关闭处理。
- README 补充异步网络调用所需 runtime 配置，并加入 163 implicit TLS SMTP URL 示例。

## [v0.1.0] - 2026-05-12

### Added
- 初始化 `galay-mail` CMake package，目录结构对齐 `galay-redis`。
- 新增 SMTP/POP3 URL 解析，支持 `smtp`、`smtp+starttls`、`smtps`、`pop3`、`pop3+stls` 与 `pop3s`。
- 新增 SMTP/POP3 协议解析器、命令编码器和 AUTH PLAIN/LOGIN 辅助函数。
- 新增异步 SMTP/POP3 客户端，支持 plain、implicit TLS 和 STARTTLS/STLS 连接模式。
- 新增 C++ module interface、示例和离线单元测试。
