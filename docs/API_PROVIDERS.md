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

## Cultural / open-access collections

### Internet Archive

Implemented by `CulturalMediaApi::searchInternetArchive` using the Advanced Search JSON API. Search responses are converted to bounded `CreativeMediaItem` records with an archive details URL and download-directory URL. The item page remains the authoritative place to review the individual item's rights statement.

### Smithsonian Open Access

Implemented by `CulturalMediaApi::searchSmithsonian`. The endpoint uses the Smithsonian public API hosted on api.data.gov and requires the user's API key. CCOS imports available record IDs, titles and media URLs while marking the rights field for verification; Smithsonian states that public-domain objects can provide media, while restricted objects may expose metadata without a media file.

### The Met Open Access

Implemented by `CulturalMediaApi::searchMet`. The search endpoint is followed by a bounded sequence of object lookups (maximum 30) to obtain primary image URLs and artwork metadata. The Met's Open Access program provides corresponding high-resolution images for public-domain works; CCOS still retains the source record URL.

### Europeana

Implemented by `CulturalMediaApi::searchEuropeana` using the Search API JSON endpoint. Europeana requires an API key and provides search, record and IIIF APIs. CCOS preserves the record's rights string, source URL and preview URL.

### Library of Congress

Implemented by `CulturalMediaApi::searchLibraryOfCongress` using the JSON Photos API. CCOS preserves the catalog URL, title/contributor metadata and image URL when present. Rights remain item-specific and must be checked from the authoritative record.

## AI and utility APIs

- OpenAI-compatible endpoints — local/self-hosted model servers and compatible hosted providers.
- `LlamaCppProvider` — local llama.cpp `llama-server` profile using the existing OpenAI-compatible transport, defaulting to loopback `http://127.0.0.1:8080/v1`.
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

## Machine-readable provider catalog

`EditorApi::culturalMediaProviders()` and the command aliases `cultural_media_providers` / `open_media_providers` expose the cultural provider list, expected authentication mode, supported media class and rights reminder to automation agents.

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

The repository's MIT dependency policy applies to software dependencies. Media returned by Openverse, Wikimedia Commons, Freesound, NASA, Internet Archive, Smithsonian, The Met, Europeana, Library of Congress or other sources remains subject to its item-level license and provider terms.


## Location and weather enrichment

### Open-Meteo Geocoding

CCOS now exposes the Open-Meteo geocoding endpoint for resolving city or postal-code queries into bounded latitude/longitude results, country metadata and timezone. The request is limited to 100 results and uses the HTTPS endpoint documented by Open-Meteo.

The CLI and Editor API expose this as `geocode` / `location_search`. Service usage limits and commercial-access terms should be checked against the current Open-Meteo documentation.

## Unsplash integration note

Unsplash Source is not used by CCOS. The adapter now retains the API-provided `links.download_location` action URL for download-like operations and continues using the direct image URLs returned under `photo.urls` for image content.
