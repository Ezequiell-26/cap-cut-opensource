# CCOS — API provider catalog

This is the maintained provider catalog. API availability, quotas and content rights are separate concerns: a free API does not make every returned asset public-domain or MIT.

## Media discovery

### Openverse

Purpose: search openly licensed media.

Integrated endpoints:
- `GET https://api.openverse.org/v1/images/`
- `GET https://api.openverse.org/v1/audio/`

CCOS preserves provider, asset id, creator, license, license URL, source URL, preview URL, download URL and basic media metadata.

### Wikimedia Commons

Purpose: search Wikimedia Commons media pages through the MediaWiki API.

CCOS uses `generator=search`, namespace `6`, `prop=imageinfo` and `extmetadata` to obtain media URLs and available license/creator metadata.

### Freesound

Purpose: searchable sound effects/audio discovery.

Integrated endpoint:
- `GET https://freesound.org/apiv2/search/`

Freesound requires an application token. CCOS sends it as a `Token` authorization header and does not serialize it. The adapter requests bounded metadata fields and exposes a preview MP3 URL. Original-file access may require stronger OAuth permissions.

### NASA APOD

Purpose: optional astronomy imagery/video source for creative projects, backgrounds and educational edits.

Integrated endpoint:
- `GET https://api.nasa.gov/planetary/apod`

The adapter defaults to NASA's public `DEMO_KEY` and accepts a user-provided key. It supports an optional ISO date and caps response size. NASA material is not automatically treated as unrestricted: third-party material, trademarks and item-specific rights must be checked before redistribution.

### Pexels

Existing provider in `StockMediaApi`. Requires an API key. Follow current API terms and individual asset licensing requirements.

### Pixabay

Existing provider in `StockMediaApi`. Requires an API key. Follow current API terms and individual asset licensing requirements.

### Unsplash

Existing provider in `AssetsApi`. Requires an access key. Follow current API and attribution/hotlinking requirements.

### Existing stock/media source adapters

`PremiumStockApi` contains provider-specific adapters for the project's existing stock/media sources. These are external-provider integrations rather than software dependencies; each provider's API and content terms must be followed.

## AI and utility APIs

- OpenAI-compatible endpoints — local/self-hosted model servers and compatible hosted providers.
- Hugging Face — optional inference services; token/configuration supplied by the user.
- Whisper local adapter — local whisper.cpp executable, invoked through `ProcessRunner` with a bounded timeout.
- LibreTranslate — translation provider; deployment-specific authentication and limits apply.
- Open-Meteo — weather/geocoding data provider.
- Accessibility adapters — optional Google Cloud, Azure and AWS integrations; user-supplied credentials are required.

## Local automation API

`LocalAutomationApi` uses `cpp-httplib` and binds only to `127.0.0.1` when enabled.

Available routes:
- `GET /api/v1/health`
- `GET /api/v1/jobs`
- `GET /api/v1/jobs/{id}`
- `POST /api/v1/jobs/{id}/cancel`

The API is diagnostics/job-control focused and does not expose direct `Project`/`Timeline` mutation. An optional Bearer token can be configured for local automation clients.

## External-provider safety policy

All new HTTP integrations must:

1. use TLS endpoints by default;
2. apply an explicit timeout;
3. cap response size before parsing;
4. validate JSON types and required fields;
5. never trust provider URLs as executable paths;
6. avoid API keys in logs, crash reports and serialized projects;
7. preserve licensing/attribution metadata when importing content;
8. fail closed on malformed responses;
9. expose provider-specific rate-limit failures without retry storms.

## Important distinction

The repository's MIT dependency policy applies to software dependencies. Media returned by Openverse, Wikimedia Commons, Freesound, NASA or other sources remains subject to its item-level license and provider terms.
