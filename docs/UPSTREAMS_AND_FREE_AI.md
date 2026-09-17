# Upstreams, MIT components and free/local AI

CCOS uses external projects as references or optional integrations. Source code is not copied unless the exact license and component are compatible.

## OpenCut

- OpenCut: https://github.com/OpenCut-app/OpenCut — MIT.
- OpenCut Classic: https://github.com/OpenCut-app/opencut-classic — MIT, archived.

The current OpenCut rewrite explicitly targets an Editor API, plugin-first architecture, desktop/mobile/browser sharing, MCP, headless mode and scripting. CCOS is adapting those goals to its existing C++20/Qt/FFmpeg core. 

## Vetted MIT/permissive projects

- nlohmann/json — MIT: JSON tooling.
- Dear ImGui — MIT: optional developer/diagnostic UI.
- LM Studio `lms` — MIT: local model tooling.
- Ollama — MIT: local model serving.
- rembg — MIT: optional local background removal. Model weights may have different licenses.

## Local AI

CCOS includes `OpenAICompatibleProvider`. It can target local OpenAI-compatible servers without coupling the editor to a paid cloud provider.

Typical endpoints:

- Ollama: `http://localhost:11434/v1`
- LM Studio: `http://localhost:1234/v1`

The adapter accepts `model`, `system`, `temperature`, `max_tokens` and `timeoutMs` in `AIRequest.options`.

## Headless API

`ccos_cli` provides automation entry points:

```text
ccos_cli --project myproject.ccos --op inspect
ccos_cli --project myproject.ccos --op validate
ccos_cli --project myproject.ccos --op export --out final.mp4
ccos_cli --project myproject.ccos --request '{"op":"set_project_name","name":"Campaign 01"}'
```

This is the base for a future MCP server, scripting layer and batch-render pipeline.

## Licensing policy

Shipped dependencies must retain their required notices. Model weights, fonts, codecs and media assets require separate license checks even when their host software is MIT. Cloud free tiers are never treated as guaranteed-free infrastructure; local/offline integrations are preferred.
