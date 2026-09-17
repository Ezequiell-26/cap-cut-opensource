# External APIs and Services

CCOS integrates with multiple free and open-source APIs to provide enhanced functionality without requiring paid subscriptions.

## Stock Media APIs

### Pexels API
- **Endpoint**: `https://www.pexels.com/api/`
- **License**: Free to use (Pexels License)
- **Features**: Search videos and photos, free commercial use, no attribution required
- **API Key**: Required (free at https://www.pexels.com/api/)
- **Usage**: Browse and import stock footage directly into your project

### Pixabay API
- **Endpoint**: `https://pixabay.com/api/`
- **License**: Free to use (Pixabay License)
- **Features**: Search videos, images, illustrations, vectors
- **API Key**: Required (free at https://pixabay.com/api/docs/#api_search_videos)
- **Usage**: Alternative stock media source with diverse content

## Translation APIs

### LibreTranslate
- **Endpoint**: deployment-specific (public or self-hosted)
- **Features**: Machine translation for subtitles and text overlays
- **Authentication/limits**: depend on the selected instance
- **API Key**: Not required for public instance, rate limited
- **Usage**: Translate subtitles and text overlays automatically

## Color Tools

### Built-in Color API
- **License**: MIT (part of CCOS)
- **Features**: 
  - Generate color palettes (complementary, analogous, triadic)
  - Color lightening/darkening
  - Gradient generation
- **Usage**: Create consistent color schemes for titles and graphics

## Weather Data

### Open-Meteo
- **Forecast endpoint**: `https://api.open-meteo.com/v1/forecast`
- **Geocoding endpoint**: `https://geocoding-api.open-meteo.com/v1/search`
- **Features**: Current weather, forecasts and location search
- **Usage**: Check current service/commercial terms before production redistribution or high-volume automation
- **API Key**: Not required
- **Usage**: Add weather overlays and location-based effects

## AI Providers

### Ollama (Local)
- **Endpoint**: `http://localhost:11434/v1`
- **License**: MIT
- **Features**: Local LLM inference, OpenAI-compatible API
- **API Key**: Not required
- **Usage**: Text generation, script assistance, metadata tagging

### LM Studio (Local)
- **Endpoint**: `http://localhost:1234/v1`
- **License**: MIT
- **Features**: Local model serving, OpenAI-compatible
- **API Key**: Not required
- **Usage**: Alternative local AI provider

### Whisper (Local Transcription)
- **License**: MIT
- **Features**: Speech-to-text transcription
- **Usage**: Automatic subtitle generation

## Configuration

API keys can be configured in the editor settings or via environment variables:

```bash
export CCOS_PEXELS_API_KEY="your_pexels_key"
export CCOS_PIXABAY_API_KEY="your_pixabay_key"
export CCOS_LIBRETRANSLATE_ENDPOINT="https://your-libretranslate-instance.com"
```

## Usage Examples

### Searching Stock Video (C++)
```cpp
#include "api/StockMediaApi.hpp"

ccos::api::StockMediaApi stockApi("your_pexels_api_key");
auto results = stockApi.searchVideos("nature landscape", 1, 15);
for (const auto& result : results) {
    qDebug() << "Found:" << result.url;
    qDebug() << "Download:" << result.downloadUrl;
}
```

### Translating Subtitles (C++)
```cpp
#include "api/LibreTranslateApi.hpp"

ccos::api::LibreTranslateApi translateApi;
auto result = translateApi.translate("Hello world", "en", "es");
qDebug() << "Translation:" << result.translatedText;
```

### Generating Color Palette (C++)
```cpp
#include "api/ColorApi.hpp"

auto palette = ccos::api::ColorApi::complementaryPalette("#3498db");
for (const auto& color : palette.colors) {
    qDebug() << "Color:" << color;
}
```

### Getting Weather Data (C++)
```cpp
#include "api/OpenMeteoApi.hpp"

auto weather = ccos::api::OpenMeteoApi::getCurrentWeather(40.7128, -74.0060); // NYC
qDebug() << "Temperature:" << weather.temperature << "°C";
qDebug() << "Condition:" << weather.condition;
```

## Rate Limits

| Service | Free Tier Limit | Notes |
|---------|----------------|-------|
| Pexels | ~20,000 requests/month | Commercial use allowed |
| Pixabay | ~500 requests/hour | Commercial use allowed |
| LibreTranslate | Varies by instance | Self-host for unlimited |
| Open-Meteo | Fair use | No API key required |

## Privacy Notes

- All API calls are made directly from the client
- No user data is stored on CCOS servers
- Consider using self-hosted instances for sensitive projects
- API keys are stored locally in Qt settings


## Unsplash compatibility note

Unsplash Source has been sunset and is intentionally not referenced by CCOS. The application uses the official Unsplash API and should preserve the direct URLs returned by the API; download-like actions should use the API's documented `links.download_location` flow.
