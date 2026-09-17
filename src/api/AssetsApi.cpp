#include "AssetsApi.hpp"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrlQuery>
#include <QDebug>

namespace ccos::api {

// ==================== Google Fonts API ====================

GoogleFontsApi::GoogleFontsApi(QString apiKey)
    : apiKey_(std::move(apiKey))
{
}

void GoogleFontsApi::setProvider(const QString& provider) {
    provider_ = provider;
}

QStringList GoogleFontsApi::fontCategories() {
    return {"serif", "sans-serif", "display", "handwriting", "monospace"};
}

QStringList GoogleFontsApi::fontSubsets() {
    return {
        "latin", "latin-ext", "cyrillic", "cyrillic-ext", "greek", 
        "greek-ext", "vietnamese", "arabic", "hebrew", "devanagari",
        "chinese-simplified", "chinese-traditional", "japanese", "korean"
    };
}

QVector<FontInfo> GoogleFontsApi::listFonts(const QString& subset, int maxResults) {
    if (apiKey_.isEmpty()) {
        qWarning() << "Google Fonts API key not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://www.googleapis.com/webfonts/v1/webfonts");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("key", apiKey_);
    queryBuilder.addQueryItem("subset", subset);
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Google Fonts API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseFontsResponse(QJsonDocument::fromJson(data));
}

FontInfo GoogleFontsApi::getFontDetails(const QString& fontFamily) {
    // Simplified - in production would fetch specific font details
    QVector<FontInfo> fonts = listFonts("latin", 1000);
    for (const auto& font : fonts) {
        if (font.family == fontFamily) {
            return font;
        }
    }
    return {};
}

QString GoogleFontsApi::getFontDownloadUrl(const QString& fontFamily, const QString& variant) {
    QString familyParam = fontFamily.replace(" ", "+");
    QString variantParam = variant;
    
    if (variant == "regular") {
        variantParam = "400";
    } else if (variant == "bold") {
        variantParam = "700";
    } else if (variant == "italic") {
        variantParam = "400i";
    }
    
    return QString("https://fonts.googleapis.com/css2?family=%1:wght@%2&display=swap")
        .arg(familyParam, variantParam);
}

QVector<FontInfo> GoogleFontsApi::searchFonts(const QString& category, const QString& subset) {
    QVector<FontInfo> allFonts = listFonts(subset, 1000);
    QVector<FontInfo> filtered;
    
    for (const auto& font : allFonts) {
        if (category.isEmpty() || font.category == category) {
            filtered.append(font);
        }
    }
    
    return filtered;
}

QVector<FontInfo> GoogleFontsApi::parseFontsResponse(const QJsonDocument& doc) {
    QVector<FontInfo> results;
    QJsonObject obj = doc.object();
    QJsonArray itemsArray = obj["items"].toArray();

    for (const auto& itemValue : itemsArray) {
        QJsonObject fontObj = itemValue.toObject();
        
        FontInfo result;
        result.family = fontObj["family"].toString();
        result.variant = fontObj["variants"].toArray().first().toString();
        result.category = fontObj["category"].toString();
        result.license = fontObj["license"].toString();
        
        QJsonArray subsetsArray = fontObj["subsets"].toArray();
        QStringList subsets;
        for (const auto& subset : subsetsArray) {
            subsets.append(subset.toString());
        }
        result.subsets = subsets;
        
        // Build download URL
        result.downloadUrl = getFontDownloadUrl(result.family);

        results.append(result);
    }

    return results;
}

// ==================== IconFinder API ====================

IconFinderApi::IconFinderApi(QString apiKey)
    : apiKey_(std::move(apiKey))
{
}

void IconFinderApi::setProvider(const QString& provider) {
    provider_ = provider;
}

QStringList IconFinderApi::iconCategories() {
    return {
        "business", "technology", "interface", "media", "nature",
        "food", "transport", "sports", "health", "education",
        "shopping", "social", "weather", "arrows", "files"
    };
}

QVector<IconData> IconFinderApi::searchIcons(const QString& query, int page, int limit) {
    if (apiKey_.isEmpty()) {
        qWarning() << "IconFinder API key not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://api.iconfinder.com/v4/icons/search");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("q", query);
    queryBuilder.addQueryItem("count", QString::number(limit));
    queryBuilder.addQueryItem("offset", QString::number((page - 1) * limit));
    queryBuilder.addQueryItem("styles[]", "line");
    queryBuilder.addQueryItem("is_premium", "0"); // Free icons only
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Bearer " + apiKey_).toUtf8());

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "IconFinder API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseIconFinderResponse(QJsonDocument::fromJson(data));
}

IconData IconFinderApi::getIconDetails(const QString& iconId) {
    // Simplified implementation
    return {};
}

QString IconFinderApi::getIconSvgUrl(const QString& iconId) {
    return QString("https://api.iconfinder.com/v4/icons/%1/content/svg").arg(iconId);
}

QVector<IconData> IconFinderApi::listPhosphorIcons(const QString& category) {
    // Phosphor Icons provides a static JSON catalog
    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://raw.githubusercontent.com/phosphor-icons/core/main/catalog.json");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Phosphor Icons error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parsePhosphorIcons(QJsonDocument::fromJson(data));
}

QString IconFinderApi::getPhosphorSvg(const QString& iconName) {
    return QString("https://unpkg.com/@phosphor-icons/core@latest/icons/%1.svg").arg(iconName);
}

QVector<IconData> IconFinderApi::listFeatherIcons() {
    // Feather Icons is a fixed set, could be bundled or fetched from CDN
    QVector<IconData> icons;
    // Common feather icons
    QStringList iconNames = {
        "activity", "airplay", "alert-circle", "alert-octagon", "alert-triangle",
        "align-center", "align-justify", "align-left", "align-right", "anchor",
        "aperture", "archive", "arrow-down", "arrow-down-circle", "arrow-down-left",
        "arrow-down-right", "arrow-left", "arrow-left-circle", "arrow-right",
        "arrow-right-circle", "arrow-up", "arrow-up-circle", "arrow-up-left",
        "arrow-up-right", "at-sign", "award", "bar-chart", "bar-chart-2",
        "battery", "battery-charging", "bell", "bell-off", "bluetooth",
        "bold", "book", "bookmark", "box", "briefcase", "calendar", "camera",
        "camera-off", "cast", "check", "check-circle", "check-square", "chevron-down",
        "chevron-left", "chevron-right", "chevron-up", "chevrons-down", "chevrons-left",
        "chevrons-right", "chevrons-up", "chrome", "circle", "clipboard", "clock",
        "cloud", "cloud-drizzle", "cloud-lightning", "cloud-off", "cloud-rain",
        "cloud-snow", "code", "codepen", "codesandbox", "coffee", "columns",
        "command", "compass", "copy", "corner-down-left", "corner-down-right",
        "corner-left-down", "corner-left-up", "corner-right-down", "corner-right-up",
        "corner-up-left", "corner-up-right", "cpu", "credit-card", "crop",
        "crosshair", "database", "delete", "disc", "dollar-sign", "download",
        "download-cloud", "droplet", "edit", "edit-2", "edit-3", "external-link",
        "eye", "eye-off", "facebook", "fast-forward", "feather", "figma",
        "file", "file-minus", "file-plus", "file-text", "film", "filter",
        "flag", "folder", "folder-minus", "folder-plus", "framer", "frown",
        "gift", "git-branch", "git-commit", "git-merge", "git-pull-request",
        "github", "gitlab", "globe", "grid", "hard-drive", "hash", "headphones",
        "heart", "help-circle", "hexagon", "home", "image", "inbox", "info",
        "instagram", "italic", "layers", "layout", "life-buoy", "link", "link-2",
        "linkedin", "list", "loader", "lock", "log-in", "log-out", "mail",
        "map", "map-pin", "maximize", "maximize-2", "menu", "message-circle",
        "message-square", "mic", "mic-off", "minimize", "minimize-2", "minus",
        "minus-circle", "minus-square", "monitor", "moon", "more-horizontal",
        "more-vertical", "mouse-pointer", "move", "music", "navigation", "navigation-2",
        "octagon", "package", "paperclip", "pause", "pause-circle", "pen-tool",
        "percent", "phone", "phone-call", "phone-forwarded", "phone-incoming",
        "phone-missed", "phone-off", "phone-outgoing", "pie-chart", "play",
        "play-circle", "plus", "plus-circle", "plus-square", "pocket", "power",
        "printer", "radio", "refresh-ccw", "refresh-cw", "repeat", "rewind",
        "rotate-ccw", "rotate-cw", "rss", "save", "scissors", "search", "send",
        "server", "settings", "share", "share-2", "shield", "shield-off",
        "shopping-bag", "shopping-cart", "shuffle", "sidebar", "skip-back",
        "skip-forward", "slack", "slash", "sliders", "smartphone", "smile",
        "speaker", "square", "star", "stop-circle", "sun", "sunrise", "sunset",
        "tablet", "tag", "target", "terminal", "thermometer", "thumbs-down",
        "thumbs-up", "toggle-left", "toggle-right", "tool", "trash", "trash-2",
        "trello", "trending-down", "trending-up", "triangle", "truck", "tv",
        "twitch", "twitter", "type", "umbrella", "underline", "unlock", "upload",
        "upload-cloud", "user", "user-check", "user-minus", "user-plus", "user-x",
        "users", "video", "video-off", "voicemail", "volume", "volume-1",
        "volume-2", "volume-x", "watch", "wifi", "wifi-off", "wind", "x",
        "x-circle", "x-square", "youtube", "zap", "zap-off", "zoom-in", "zoom-out"
    };

    for (const auto& name : iconNames) {
        IconData icon;
        icon.name = name;
        icon.id = "feather-" + name;
        icon.svgPath = getFeatherSvg(name);
        icon.categories = QStringList{"interface"};
        icons.append(icon);
    }

    return icons;
}

QString IconFinderApi::getFeatherSvg(const QString& iconName) {
    return QString("https://unpkg.com/feather-icons@latest/icons/%s.svg").arg(iconName);
}

QVector<IconData> IconFinderApi::parseIconFinderResponse(const QJsonDocument& doc) {
    QVector<IconData> results;
    QJsonObject obj = doc.object();
    QJsonArray iconsArray = obj["icons"].toArray();

    for (const auto& iconValue : iconsArray) {
        QJsonObject iconObj = iconValue.toObject();
        
        IconData result;
        result.id = iconObj["id"].toVariant().toString();
        result.name = iconObj["title"].toString();
        
        // Get SVG preview
        QJsonObject previews = iconObj["previews"].toObject();
        result.svgPath = previews["svg"].toString();
        
        result.unicode = iconObj["unicode"].toString();
        
        QJsonArray categoriesArray = iconObj["categories"].toArray();
        QStringList categories;
        for (const auto& cat : categoriesArray) {
            categories.append(cat.toString());
        }
        result.categories = categories;

        results.append(result);
    }

    return results;
}

QVector<IconData> IconFinderApi::parsePhosphorIcons(const QJsonDocument& doc) {
    QVector<IconData> results;
    QJsonObject obj = doc.object();

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        IconData result;
        result.name = it.key();
        result.id = "phosphor-" + it.key();
        result.svgPath = getPhosphorSvg(it.key());
        result.categories = QStringList{"interface"};
        results.append(result);
    }

    return results;
}

// ==================== Unsplash API ====================

UnsplashApi::UnsplashApi(QString accessKey)
    : accessKey_(std::move(accessKey))
{
}

void UnsplashApi::setProvider(const QString& provider) {
    provider_ = provider;
}

QStringList UnsplashApi::photoSizes() {
    return {"thumb", "small", "medium", "large", "full"};
}

QVector<UnsplashApi::PhotoResult> UnsplashApi::searchPhotos(const QString& query, int page, int perPage) {
    if (accessKey_.isEmpty()) {
        qWarning() << "Unsplash API access key not configured";
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url("https://api.unsplash.com/photos/search");
    QUrlQuery queryBuilder;
    queryBuilder.addQueryItem("query", query);
    queryBuilder.addQueryItem("page", QString::number(page));
    queryBuilder.addQueryItem("per_page", QString::number(perPage));
    queryBuilder.addQueryItem("orientation", "landscape");
    url.setQuery(queryBuilder);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Client-ID " + accessKey_).toUtf8());

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Unsplash API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    return parseUnsplashResponse(QJsonDocument::fromJson(data));
}

UnsplashApi::PhotoResult UnsplashApi::getPhotoDetails(const QString& photoId) {
    if (accessKey_.isEmpty()) {
        return {};
    }

    QNetworkAccessManager manager;
    QEventLoop loop;

    QUrl url(QString("https://api.unsplash.com/photos/%1").arg(photoId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CCOS-Editor/0.5");
    request.setRawHeader("Authorization", ("Client-ID " + accessKey_).toUtf8());

    auto* reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Unsplash API error:" << reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    PhotoResult result;
    result.id = obj["id"].toString();
    result.description = obj["description"].toString();
    result.url = obj["links"].toObject()["html"].toString();
    result.photographer = obj["user"].toObject()["name"].toString();
    result.width = obj["width"].toInt(0);
    result.height = obj["height"].toInt(0);
    
    QJsonObject colorObj = obj["color"].toString();
    // Parse dominant color from hex string
    result.dominantColor = QColor(obj["color"].toString());
    
    result.downloadUrl = obj["links"].toObject()["download"].toString();

    return result;
}

QString UnsplashApi::getPhotoDownloadUrl(const QString& photoId, const QString& size) {
    QString sizeParam;
    if (size == "thumb") sizeParam = "thumb";
    else if (size == "small") sizeParam = "small";
    else if (size == "medium") sizeParam = "med";
    else if (size == "large") sizeParam = "large";
    else sizeParam = "full";

    return QString("https://source.unsplash.com/%1?photo_id=%2").arg(sizeParam, photoId);
}

QVector<UnsplashApi::PhotoResult> UnsplashApi::parseUnsplashResponse(const QJsonDocument& doc) {
    QVector<PhotoResult> results;
    QJsonArray photosArray = doc.array();

    for (const auto& photoValue : photosArray) {
        QJsonObject photoObj = photoValue.toObject();
        
        PhotoResult result;
        result.id = photoObj["id"].toString();
        result.description = photoObj["description"].toString();
        result.url = photoObj["links"].toObject()["html"].toString();
        result.photographer = photoObj["user"].toObject()["name"].toString();
        result.width = photoObj["width"].toInt(0);
        result.height = photoObj["height"].toInt(0);
        result.dominantColor = QColor(photoObj["color"].toString());
        result.downloadUrl = photoObj["links"].toObject()["download"].toString();

        results.append(result);
    }

    return results;
}

} // namespace ccos::api
