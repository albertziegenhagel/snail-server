param(
    [String]$VsDiagnosticsPath,
    [String]$AppPath,
    [String]$SessionId,
    [String]$OutputName
)

Start-Process $VsDiagnosticsPath start, $SessionId, "/launch:${AppPath}", "/loadAgent:4EA90761-2248-496C-B854-3C0399A591A4;DiagnosticsHub.CpuAgent.dll" -Wait -NoNewWindow
Start-Process $VsDiagnosticsPath stop, $SessionId, /output:${OutputName} -Wait -NoNewWindow
