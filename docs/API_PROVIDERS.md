# CCOS — API provider catalog

This file describes optional external APIs used for media discovery, translation, accessibility and AI. API access and content licenses are separate concerns: an API being free does not make every returned asset public-domain or MIT.

## Media discovery

### Openverse

Purpose: search for openly licensed media.

Integrated endpoints:

- `GET https://api.openverse.org/v1/images/`
- `GET https://api.openverse.org/v1/audio/`

CCOS stores provider, asset id, creator, license, license URL, source URL, preview URL, download URL and basic media metadata so the UI can show attribution/licensing information before import.

### Wikimedia Commons

Purpose: broad search across Wikimedia Commons media pages.

CCOS uses the MediaWiki API with `generator=search`, `gsrnamespace=6` and `prop=imageinfo` for image/audio/video media pages. License metadata is taken from the media page's `extmetadata` when available.

### Pexels

Existing provider in `StockMediaApi`. Requires an API key. Use its API terms and the individual asset's licensing/attribution requirements.

### Pixabay

Existing provider in `StockMediaApi`. Requires an API key. Use Pixabay's API and content-license terms.

### Unsplash

Existing provider in `AssetsApi`. Requires an access key. Use Unsplash API requirements, including attribution/hotlinking requirements where applicable.

## Existing utility APIs

- Open-Meteo — weather/geocoding data without a normal application API key for standard public usage.
- LibreTranslate — translation provider; deployment-specific limits and authentication apply.
- Accessibility providers — Google Cloud, Azure and AWS adapters are optional and require user-provided credentials.
- Hugging Face — optional inference provider for AI tasks; token/configuration is user supplied.
- OpenAI-compatible providers — configurable endpoint support for local/self-hosted model servers.

## External-provider safety policy

All new HTTP integrations must:

1. use TLS endpoints by default;
2. apply an explicit timeout;
3. cap response size before parsing;
4. validate JSON types and required fields;
5. never trust provider URLs as executable paths;
6. avoid API keys in logs, crash reports and serialized projects;
7. preserve the provider's licensing metadata when importing content;
8. fail closed on malformed responses.

## Important distinction

The repository's MIT dependency policy applies to software dependencies. Media returned by Openverse, Wikimedia Commons, Pexels, Pixabay, Unsplash or other providers remains subject to its own item-level license and provider terms.
