# Third-party license inventory

CCOS source code is MIT. Dependencies remain under their own licenses.

| Dependency | Purpose | License policy |
|---|---|---|
| Qt 6 | Desktop UI | Use a compatible LGPL/GPL commercial-free configuration and retain required notices |
| FFmpeg | Demux/decode/encode/filtering | Prefer LGPL components; do not accidentally enable GPL-only components without documenting the distribution impact |
| GoogleTest | Tests only | BSD-3-Clause |

Before introducing a dependency, verify its exact version, SPDX/license text, source repository and redistribution obligations. Do not copy source code solely because a repository is public.
