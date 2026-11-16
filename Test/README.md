# Regression tests

From an x64 or x86 Visual Studio developer PowerShell with the v143 C++
toolset and Windows SDK installed:

```powershell
./Test/Run.ps1 -Platform x64
# In the x86 developer shell:
./Test/Run.ps1 -Platform Win32
```

The tests build the DLL, load a temporary copy with an isolated INI, and
use loopback TCP servers. No card reader or BonCasServer is required.
They cover send/receive timeout settings, synchronous and asynchronous
connections, refused connections, independent endpoint backoff, legacy
`[Server]` / `[Server:Port]` enumeration, ANSI/Unicode list buffer guards,
automatic allocation ownership, invalid command sizes, response buffer
guards, and context release while a transmit is in flight.

The workflow runs this suite for both architectures. Hardware/decoder
compatibility still needs to be checked with real applications.

The transport fixes and endpoint backoff are adapted from stuayu/BonCasClient:

- https://github.com/stuayu/BonCasClient/commit/68e722ad5db4699785c754d08cad08edb62c33f1
- https://github.com/stuayu/BonCasClient/commit/7494b45d4ac2b4e5734c85d791c39a7f90cb1a26
- https://github.com/stuayu/BonCasClient/commit/3094d9887e1c0644778bf00b7be1810e7e426ddd

The API guards and registry are implemented against this fork's existing
endpoint-name configuration. Rust, the `[Readers]` format, local reader
forwarding, and additional WinSCard exports are not included. `SetConfig`
retains this fork's existing no-op behavior.
