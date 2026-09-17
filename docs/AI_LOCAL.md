# CCOS local AI backends

CCOS keeps local AI optional. Editing, rendering and project persistence must remain functional without a hosted AI credential.

## llama.cpp

`ccos::ai::LlamaCppProvider` targets a local `llama-server` instance through the OpenAI-compatible `/v1/chat/completions` contract. The default endpoint is:

`http://127.0.0.1:8080/v1`

A caller supplies the model name through the existing `AIRequest.options["model"]` field. Optional `system`, `temperature`, `max_tokens` and `timeoutMs` settings are forwarded through the existing provider transport.

The CCOS integration does not download or manage model weights. Users can run the server independently and keep model files under their own control.

## Example request

```json
{
  "op": "ai",
  "provider": "llama-cpp",
  "model": "local-model",
  "input": "Create three short title ideas for this video"
}
```

The exact automation route depends on the active `AI`/agent layer; the provider itself accepts the same `AIRequest` contract as the OpenAI-compatible backend.

## Safety

- Bind local AI by loopback unless the user explicitly configures otherwise.
- Never serialize API keys or provider credentials into `.ccos` projects.
- Bound request timeouts and response sizes before parsing.
- Do not let model output directly execute arbitrary filesystem or process commands.
- Route any state-mutating AI action through the existing command/tool safety layer.
- Keep deterministic validation between AI intent and timeline/project mutation.

## Other local AI building blocks

CCOS already has a whisper.cpp adapter and an optional ONNX Runtime boundary. The intended long-term model is:

```text
AI request
   -> provider selection
   -> bounded transport/inference
   -> structured intent
   -> validation
   -> explicit editor command
   -> project/timeline mutation
```

This keeps the model replaceable and prevents a model response from becoming an implicit privileged action.
