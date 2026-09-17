# APIs y Servicios Externos de CCOS

CCOS se integra con múltiples APIs gratuitas y de código abierto para proporcionar funcionalidad mejorada sin requerir suscripciones pagas.

## 📺 Stock Media APIs

### Pexels API
- **Endpoint**: `https://www.pexels.com/api/`
- **Licencia**: Gratis (Pexels License) - Uso comercial permitido
- **Características**: Búsqueda de videos y fotos, sin atribución requerida
- **API Key**: Requerida (gratis en https://www.pexels.com/api/)
- **Uso**: Explorar e importar stock footage directamente al proyecto

### Pixabay API
- **Endpoint**: `https://pixabay.com/api/`
- **Licencia**: Gratis (Pixabay License) - Uso comercial permitido
- **Características**: Videos, imágenes, ilustraciones, vectores
- **API Key**: Requerida (gratis)
- **Límite**: ~500 requests/hora

### Unsplash API *(NUEVO)*
- **Endpoint**: `https://api.unsplash.com/`
- **Licencia**: Unsplash License - Gratis, uso comercial
- **Características**: Fotos de alta resolución, metadatos completos
- **API Key**: Requerida (gratis en https://unsplash.com/developers)
- **Implementación**: `AssetsApi.hpp/cpp` - Clase `UnsplashApi`

## 🎵 Audio & Music APIs

### FreeSound API *(NUEVO)*
- **Endpoint**: `https://freesound.org/apiv2/`
- **Licencia**: Varias (CC0, Creative Commons)
- **Características**: Efectos de sonido, samples de audio
- **API Key**: Requerida (gratis)
- **Implementación**: `AudioApi.hpp/cpp` - Búsqueda por tags, duración, licencia

### Jamendo API *(NUEVO)*
- **Endpoint**: `https://api.jamendo.com/rest/3.0/`
- **Licencia**: Creative Commons - Música libre de regalías
- **Características**: Música para fondo, bandas sonoras, géneros variados
- **API Key**: Requerida (gratis para desarrollo)
- **Géneros**: Ambient, cinematic, corporate, electronic, rock, pop, jazz, etc.

### Internet Archive Audio *(NUEVO)*
- **Endpoint**: `https://archive.org/advancedsearch.php`
- **Licencia**: Varias (dominio público, Creative Commons)
- **Características**: Archivos históricos, música antigua, grabaciones únicas
- **API Key**: No requerida
- **Colecciones**: 78 RPMs, Old Time Radio, Live Music Archive

## 🌐 Traducción

### LibreTranslate
- **Endpoint**: `https://libretranslate.com` o self-hosted
- **Licencia**: AGPL (self-hostable)
- **Idiomas**: 30+ idiomas soportados
- **API Key**: No requerida para instancia pública (rate limited)
- **Uso**: Traducir subtítulos y textos automáticamente

### Hugging Face Translation *(NUEVO)*
- **Endpoint**: `https://api-inference.huggingface.co/`
- **Modelos**: Helsinki-NLP/opus-mt-* (100+ pares de idiomas)
- **Licencia**: MIT/Apache 2.0
- **API Key**: Opcional (rate limit sin key)
- **Implementación**: `AdvancedAI.hpp/cpp` - Clase `HuggingFaceInference`

## 🎨 Color & Design

### Built-in Color API
- **Licencia**: MIT (parte de CCOS)
- **Características**: 
  - Paletas de colores (complementarios, análogos, triádicos)
  - Clarificar/oscurecer colores
  - Generación de gradientes
- **Uso**: Crear esquemas de color consistentes

## 🌤️ Datos Externos

### Open-Meteo Weather API
- **Endpoint**: `https://open-meteo.com/`
- **Licencia**: Gratis para uso no comercial
- **Características**: Clima actual, pronósticos, datos históricos
- **API Key**: No requerida
- **Uso**: Overlays de clima, efectos basados en ubicación

## 🔤 Fuentes & Tipografía

### Google Fonts API *(NUEVO)*
- **Endpoint**: `https://www.googleapis.com/webfonts/v1/`
- **Licencia**: OFL, Apache 2.0, MIT
- **Fuentes**: 1000+ familias de fuentes gratuitas
- **Subsets**: Latin, Cyrillic, Greek, Arabic, Hebrew, Devanagari, CJK, etc.
- **Categorías**: Serif, Sans-serif, Display, Handwriting, Monospace
- **Implementación**: `AssetsApi.hpp/cpp` - Clase `GoogleFontsApi`

## 🎯 Íconos & SVG

### IconFinder API *(NUEVO)*
- **Endpoint**: `https://api.iconfinder.com/v4/`
- **Licencia**: Varias (gratis y premium)
- **Características**: Íconos vectoriales, categorías múltiples
- **API Key**: Requerida (free tier disponible)
- **Filtros**: Estilo (line, solid), premium/free, categorías

### Phosphor Icons *(NUEVO)*
- **Licencia**: MIT - Completamente gratis
- **Íconos**: 6000+ íconos consistentes
- **API Key**: No requerida
- **Formato**: SVG directo desde CDN
- **Implementación**: `AssetsApi.hpp/cpp` - `listPhosphorIcons()`, `getPhosphorSvg()`

### Feather Icons *(NUEVO)*
- **Licencia**: MIT - Completamente gratis
- **Íconos**: 287 íconos esenciales
- **API Key**: No requerida
- **Estilo**: Minimalista, outline
- **Implementación**: `listFeatherIcons()`, `getFeatherSvg()`

## 🤖 Inteligencia Artificial

### Ollama (Local)
- **Endpoint**: `http://localhost:11434/v1`
- **Licencia**: MIT
- **Modelos**: Llama 3.2, Mistral, Phi, Gemma, etc.
- **API Key**: No requerida
- **Uso**: Generación de texto, asistencia de guion, metadata tagging

### LM Studio (Local)
- **Endpoint**: `http://localhost:1234/v1`
- **Licencia**: MIT
- **Características**: Servidor local compatible con OpenAI
- **API Key**: No requerida

### LocalAI (Self-hosted)
- **Endpoint**: `http://localhost:8080/v1`
- **Licencia**: MIT
- **Características**: Múltiples modelos, GPU acceleration

### Whisper.cpp (Transcripción Local)
- **Licencia**: MIT
- **Características**: Speech-to-text offline
- **Idiomas**: 100+ idiomas soportados
- **Modelos**: Tiny, Base, Small, Medium, Large
- **Implementación**: `AdvancedAI.hpp/cpp` - Clase `WhisperLocalProvider`

### Hugging Face Inference API *(NUEVO)*
- **Endpoint**: `https://api-inference.huggingface.co/`
- **Licencia**: Varias (modelos open source)
- **Características**:
  - **Text-to-Image**: Stable Diffusion XL, SD 1.5
  - **Image Classification**: ViT, ResNet
  - **Text Classification**: Sentimiento, temas
  - **NER**: Reconocimiento de entidades
  - **Summarization**: BART, T5
  - **Translation**: OPUS-MT (100+ idiomas)
- **API Key**: Opcional (rate limits más altos con key)
- **Implementación**: `AdvancedAI.hpp/cpp` - Clase `HuggingFaceInference`

## 📊 Límites de Rate

| Servicio | Límite Gratis | Notas |
|----------|--------------|-------|
| Pexels | ~20,000 req/mes | Uso comercial permitido |
| Pixabay | ~500 req/hora | Uso comercial permitido |
| Unsplash | ~50 req/hora | Atribución recomendada |
| FreeSound | ~100 req/día | Requiere API key |
| Jamendo | ~5000 req/día | Desarrollo gratuito |
| LibreTranslate | Variable | Self-host para ilimitado |
| Open-Meteo | Fair use | Sin API key |
| Google Fonts | 1000 req/día | Sin key para uso básico |
| Hugging Face | ~30,000 chars/mes | Sin key, más con key gratis |

## 🔧 Configuración

Las API keys se pueden configurar en:

### Variables de Entorno
```bash
export CCOS_PEXELS_API_KEY="tu_pexels_key"
export CCOS_PIXABAY_API_KEY="tu_pixabay_key"
export CCOS_UNSPLASH_ACCESS_KEY="tu_unsplash_key"
export CCOS_FREESOUND_API_KEY="tu_freesound_key"
export CCOS_JAMENDO_CLIENT_ID="tu_jamendo_client_id"
export CCOS_HUGGINGFACE_TOKEN="tu_hf_token"
export CCOS_LIBRETRANSLATE_ENDPOINT="https://tu-libretranslate.com"
```

### Desde el Editor
1. Ir a **Settings > External APIs**
2. Ingresar las keys correspondientes
3. Las keys se guardan en Qt Settings (encriptadas)

## 💡 Ejemplos de Uso

### Buscar Música de Fondo (C++)
```cpp
#include "api/AudioApi.hpp"

ccos::api::AudioApi audioApi("freesound_key", "jamendo_client_id");

// Buscar música corporativa
auto music = audioApi.searchMusic("corporate uplifting", "corporate", 1, 20);
for (const auto& track : music) {
    qDebug() << "Track:" << track.title << "by" << track.artist;
    qDebug() << "Download:" << track.downloadUrl;
}

// Buscar efectos de sonido
auto sfx = audioApi.searchSoundEffects("whoosh transition", 1, 20);
```

### Buscar Fotos (C++)
```cpp
#include "api/AssetsApi.hpp"

ccos::api::UnsplashApi unsplash("your_access_key");
auto photos = unsplash.searchPhotos("mountain landscape sunset", 1, 20);
for (const auto& photo : photos) {
    qDebug() << "Photo by" << photo.photographer;
    qDebug() << "Download:" << photo.downloadUrl;
}
```

### Descargar Fuentes (C++)
```cpp
ccos::api::GoogleFontsApi fonts("your_google_api_key");
auto fontList = fonts.listFonts("latin", 100);
for (const auto& font : fontList) {
    qDebug() << "Font:" << font.family << "(" << font.category << ")";
    qDebug() << "Download:" << fonts.getFontDownloadUrl(font.family, "regular");
}
```

### Generar Imágenes con IA (C++)
```cpp
#include "ai/AdvancedAI.hpp"

ccos::ai::HuggingFaceInference hf("your_hf_token");

// Generar imagen desde texto
QByteArray imageData = hf.generateImage(
    "cinematic shot of a futuristic city at sunset, photorealistic, 8k",
    "stabilityai/stable-diffusion-xl-base-1.0",
    1024, 1024, 30
);

// Analizar imagen
auto analysis = hf.classifyImage(imageData, "google/vit-base-patch16-224");
qDebug() << "Tags:" << analysis.tags;
```

### Transcribir Audio (C++)
```cpp
ccos::ai::WhisperLocalProvider whisper;

if (whisper.isWhisperAvailable()) {
    auto result = whisper.transcribe("audio.mp3", "es", true);
    qDebug() << "Transcription:" << result.text;
    
    for (const auto& segment : result.segments) {
        qDebug() << QString("[%1 - %2] %3")
            .arg(segment.start, 0, 'f', 2)
            .arg(segment.end, 0, 'f', 2)
            .arg(segment.text);
    }
}
```

### Chat con IA Local (C++)
```cpp
ccos::ai::LocalAIProvider ai("http://localhost:11434/v1");

if (ai.isAvailable()) {
    // Generar texto
    auto result = ai.generateText(
        "Write a short video intro about climate change",
        "You are a professional video editor assistant",
        256,  // max tokens
        0.7   // temperature
    );
    qDebug() << "Generated:" << result.text;
    
    // Chat multi-mensaje
    QVector<QPair<QString, QString>> messages = {
        {"system", "Eres un asistente de edición de video"},
        {"user", "¿Cómo puedo mejorar la transición entre escenas?"}
    };
    auto chatResult = ai.chatCompletion(messages, 512);
}
```

## 🔒 Privacidad & Seguridad

- ✅ Todas las llamadas API son directas desde el cliente
- ✅ No hay servidores intermedios de CCOS
- ✅ Las API keys se almacenan localmente (Qt Settings)
- ✅ Opción de self-hosting para servicios críticos
- ✅ Modo offline disponible para funciones locales (Whisper, Ollama)
- ✅ Sin telemetría ni tracking

## 📚 Recursos Adicionales

- [Documentación de Pexels](https://www.pexels.com/api/documentation/)
- [Documentación de Pixabay](https://pixabay.com/api/docs/)
- [Documentación de Unsplash](https://unsplash.com/documentation)
- [Documentación de FreeSound](https://freesound.org/docs/api/)
- [Documentación de Jamendo](https://developer.jamendo.com/)
- [Documentación de Google Fonts](https://developers.google.com/fonts/docs/developer_api)
- [Documentación de Hugging Face](https://huggingface.co/docs/api-inference/)
- [Ollama Documentation](https://github.com/ollama/ollama/blob/main/docs/api.md)

## 🆕 Próximas Integraciones

- [ ] Giphy API (GIFs y stickers)
- [ ] Tenor API (GIFs)
- [ ] NASA API (imágenes espaciales)
- [ ] Flickr API (fotos Creative Commons)
- [ ] Storyset API (ilustraciones)
- [ ] Coqui TTS (text-to-speech local)
- [ ] Bark TTS (generación de voz neural)
