#include "AssetsApi.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace ccos::api {
namespace {

QJsonDocument getJson(const QUrl& url, const QByteArray& authorization = {}) {
    constexpr qint64 kMaxResponseBytes = 8 * 1024 * 1024;
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CCOS-Editor/0.9"));
    request.setRawHeader("Accept", "application/json");
    if (!authorization.isEmpty()) request.setRawHeader("Authorization", authorization);

    QNetworkReply* reply = manager.get(request);
    const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isValid() && contentLength.toLongLong() > kMaxResponseBytes) {
        reply->abort();
    }

    QByteArray payload;
    bool oversized = false;
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [&]() {
        if (oversized) return;
        payload.append(reply->readAll());
        if (payload.size() > kMaxResponseBytes) {
            oversized = true;
            reply->abort();
        }
    });

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(20'000);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&loop, reply]() {
        if (reply->isRunning()) reply->abort();
        loop.quit();
    });
    loop.exec();
    if (timer.isActive()) timer.stop();

    payload.append(reply->readAll());
    const bool ok = !oversized && payload.size() <= kMaxResponseBytes &&
                    reply->error() == QNetworkReply::NoError;
    reply->deleteLater();
    if (!ok) return {};

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    return parseError.error == QJsonParseError::NoError ? document : QJsonDocument{};
}

}

GoogleFontsApi::GoogleFontsApi(QString apiKey) : apiKey_(std::move(apiKey)) {}
void GoogleFontsApi::setProvider(const QString& provider) { provider_ = provider; }

QStringList GoogleFontsApi::fontCategories() {
    return {QStringLiteral("serif"), QStringLiteral("sans-serif"), QStringLiteral("display"),
            QStringLiteral("handwriting"), QStringLiteral("monospace")};
}

QStringList GoogleFontsApi::fontSubsets() {
    return {QStringLiteral("latin"), QStringLiteral("latin-ext"), QStringLiteral("cyrillic"),
            QStringLiteral("cyrillic-ext"), QStringLiteral("greek"), QStringLiteral("greek-ext"),
            QStringLiteral("vietnamese"), QStringLiteral("arabic"), QStringLiteral("hebrew"),
            QStringLiteral("devanagari"), QStringLiteral("chinese-simplified"),
            QStringLiteral("chinese-traditional"), QStringLiteral("japanese"), QStringLiteral("korean")};
}

QVector<FontInfo> GoogleFontsApi::listFonts(const QString& subset, int maxResults) {
    if (apiKey_.isEmpty() || maxResults <= 0) return {};
    QUrl url(QStringLiteral("https://www.googleapis.com/webfonts/v1/webfonts"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), apiKey_);
    if (!subset.isEmpty()) query.addQueryItem(QStringLiteral("subset"), subset);
    query.addQueryItem(QStringLiteral("sort"), QStringLiteral("popularity"));
    url.setQuery(query);
    QVector<FontInfo> results = parseFontsResponse(getJson(url));
    if (maxResults < results.size()) results.resize(maxResults);
    return results;
}

FontInfo GoogleFontsApi::getFontDetails(const QString& fontFamily) {
    const auto fonts = listFonts(QStringLiteral("latin"), 1000);
    for (const auto& font : fonts) if (font.family == fontFamily) return font;
    return {};
}

QString GoogleFontsApi::getFontDownloadUrl(const QString& fontFamily, const QString& variant) {
    QString familyParam = fontFamily;
    familyParam.replace(QLatin1Char(' '), QLatin1Char('+'));
    QString cssVariant = variant.toLower();
    if (cssVariant == QStringLiteral("bold")) cssVariant = QStringLiteral("700");
    else if (cssVariant == QStringLiteral("regular")) cssVariant = QStringLiteral("400");
    else if (cssVariant == QStringLiteral("italic")) {
        return QStringLiteral("https://fonts.googleapis.com/css2?family=%1:ital,wght@1,400&display=swap").arg(familyParam);
    }
    return QStringLiteral("https://fonts.googleapis.com/css2?family=%1:wght@%2&display=swap").arg(familyParam, cssVariant);
}

QVector<FontInfo> GoogleFontsApi::searchFonts(const QString& category, const QString& subset) {
    QVector<FontInfo> filtered;
    for (const auto& font : listFonts(subset, 1000)) {
        if (category.isEmpty() || font.category == category) filtered.append(font);
    }
    return filtered;
}

QVector<FontInfo> GoogleFontsApi::parseFontsResponse(const QJsonDocument& doc) {
    QVector<FontInfo> results;
    if (!doc.isObject()) return results;
    for (const auto& value : doc.object().value(QStringLiteral("items")).toArray()) {
        const QJsonObject object = value.toObject();
        FontInfo result;
        result.family = object.value(QStringLiteral("family")).toString();
        result.variant = object.value(QStringLiteral("variants")).toArray().value(0).toString();
        result.category = object.value(QStringLiteral("category")).toString();
        result.license = object.value(QStringLiteral("license")).toString();
        result.downloadUrl = getFontDownloadUrl(result.family, result.variant.isEmpty() ? QStringLiteral("regular") : result.variant);
        for (const auto& subset : object.value(QStringLiteral("subsets")).toArray()) result.subsets.append(subset.toString());
        if (!result.family.isEmpty()) results.append(result);
    }
    return results;
}

IconFinderApi::IconFinderApi(QString apiKey) : apiKey_(std::move(apiKey)) {}
void IconFinderApi::setProvider(const QString& provider) { provider_ = provider; }

QStringList IconFinderApi::iconCategories() {
    return {QStringLiteral("business"), QStringLiteral("technology"), QStringLiteral("interface"),
            QStringLiteral("media"), QStringLiteral("nature"), QStringLiteral("sports"),
            QStringLiteral("health"), QStringLiteral("social"), QStringLiteral("weather"),
            QStringLiteral("arrows"), QStringLiteral("files")};
}

QVector<IconData> IconFinderApi::searchIcons(const QString& query, int page, int limit) {
    if (apiKey_.isEmpty() || page < 1 || limit <= 0) return {};
    limit = std::clamp(limit, 1, 100);
    QUrl url(QStringLiteral("https://api.iconfinder.com/v4/icons/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query);
    params.addQueryItem(QStringLiteral("count"), QString::number(limit));
    params.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * limit));
    params.addQueryItem(QStringLiteral("is_premium"), QStringLiteral("0"));
    url.setQuery(params);
    return parseIconFinderResponse(getJson(url, QByteArray("Bearer ") + apiKey_.toUtf8()));
}

IconData IconFinderApi::getIconDetails(const QString& iconId) {
    IconData icon;
    icon.id = iconId;
    icon.name = iconId;
    icon.svgPath = getIconSvgUrl(iconId);
    icon.categories = {QStringLiteral("interface")};
    return icon;
}

QString IconFinderApi::getIconSvgUrl(const QString& iconId) {
    return QStringLiteral("https://api.iconfinder.com/v4/icons/%1/content/svg").arg(iconId);
}

QVector<IconData> IconFinderApi::listPhosphorIcons(const QString& category) {
    Q_UNUSED(category);
    static const QStringList names = {
        QStringLiteral("play"), QStringLiteral("pause"), QStringLiteral("scissors"), QStringLiteral("film-strip"),
        QStringLiteral("music-note"), QStringLiteral("text-t"), QStringLiteral("image"), QStringLiteral("microphone"),
        QStringLiteral("gear"), QStringLiteral("download"), QStringLiteral("upload"), QStringLiteral("trash"),
        QStringLiteral("plus"), QStringLiteral("minus"), QStringLiteral("check"), QStringLiteral("x")};
    QVector<IconData> icons;
    icons.reserve(names.size());
    for (const auto& name : names) {
        IconData icon;
        icon.name = name;
        icon.id = QStringLiteral("phosphor-") + name;
        icon.svgPath = getPhosphorSvg(name);
        icon.categories = {QStringLiteral("interface")};
        icons.append(icon);
    }
    return icons;
}

QString IconFinderApi::getPhosphorSvg(const QString& iconName) {
    return QStringLiteral("https://unpkg.com/@phosphor-icons/core@latest/assets/regular/%1.svg").arg(iconName);
}

QVector<IconData> IconFinderApi::listFeatherIcons() {
    QVector<IconData> icons;
    for (const auto& name : {QStringLiteral("activity"), QStringLiteral("camera"), QStringLiteral("film"), QStringLiteral("music"), QStringLiteral("play"), QStringLiteral("scissors")}) {
        IconData icon;
        icon.name = name;
        icon.id = QStringLiteral("feather-") + name;
        icon.svgPath = getFeatherSvg(name);
        icon.categories = {QStringLiteral("interface")};
        icons.append(icon);
    }
    return icons;
}

QString IconFinderApi::getFeatherSvg(const QString& iconName) {
    return QStringLiteral("https://unpkg.com/feather-icons@4.29.2/icons/%1.svg").arg(iconName);
}

QVector<IconData> IconFinderApi::parseIconFinderResponse(const QJsonDocument& doc) {
    QVector<IconData> results;
    if (!doc.isObject()) return results;
    for (const auto& value : doc.object().value(QStringLiteral("icons")).toArray()) {
        const QJsonObject object = value.toObject();
        IconData result;
        result.id = object.value(QStringLiteral("id")).toVariant().toString();
        if (result.id.isEmpty()) result.id = QString::number(object.value(QStringLiteral("icon_id")).toInteger());
        result.name = object.value(QStringLiteral("title")).toString();
        result.unicode = object.value(QStringLiteral("unicode")).toString();
        result.svgPath = object.value(QStringLiteral("previews")).toObject().value(QStringLiteral("svg")).toString();
        for (const auto& category : object.value(QStringLiteral("categories")).toArray()) {
            result.categories.append(category.toObject().value(QStringLiteral("name")).toString());
        }
        if (!result.id.isEmpty()) results.append(result);
    }
    return results;
}

QVector<IconData> IconFinderApi::parsePhosphorIcons(const QJsonDocument& doc) {
    QVector<IconData> results;
    if (!doc.isObject()) return results;
    for (auto it = doc.object().cbegin(); it != doc.object().cend(); ++it) {
        IconData icon;
        icon.name = it.key();
        icon.id = QStringLiteral("phosphor-") + it.key();
        icon.svgPath = getPhosphorSvg(it.key());
        results.append(icon);
    }
    return results;
}

UnsplashApi::UnsplashApi(QString accessKey) : accessKey_(std::move(accessKey)) {}
void UnsplashApi::setProvider(const QString& provider) { provider_ = provider; }
QStringList UnsplashApi::photoSizes() { return {QStringLiteral("thumb"), QStringLiteral("small"), QStringLiteral("medium"), QStringLiteral("large"), QStringLiteral("full")}; }

QVector<UnsplashApi::PhotoResult> UnsplashApi::searchPhotos(const QString& query, int page, int perPage) {
    if (accessKey_.isEmpty() || page < 1 || perPage <= 0) return {};
    QUrl url(QStringLiteral("https://api.unsplash.com/search/photos"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("query"), query);
    params.addQueryItem(QStringLiteral("page"), QString::number(page));
    params.addQueryItem(QStringLiteral("per_page"), QString::number(std::clamp(perPage, 1, 30)));
    url.setQuery(params);
    return parseUnsplashResponse(getJson(url, QByteArray("Client-ID ") + accessKey_.toUtf8()));
}

UnsplashApi::PhotoResult UnsplashApi::getPhotoDetails(const QString& photoId) {
    if (accessKey_.isEmpty() || photoId.isEmpty()) return {};
    const QUrl url(QStringLiteral("https://api.unsplash.com/photos/%1").arg(photoId));
    const QJsonDocument doc = getJson(url, QByteArray("Client-ID ") + accessKey_.toUtf8());
    if (!doc.isObject()) return {};
    const QJsonObject object = doc.object();
    PhotoResult result;
    result.id = object.value(QStringLiteral("id")).toString();
    result.description = object.value(QStringLiteral("description")).toString();
    result.url = object.value(QStringLiteral("links")).toObject().value(QStringLiteral("html")).toString();
    result.photographer = object.value(QStringLiteral("user")).toObject().value(QStringLiteral("name")).toString();
    result.width = object.value(QStringLiteral("width")).toInt();
    result.height = object.value(QStringLiteral("height")).toInt();
    result.dominantColor = QColor(object.value(QStringLiteral("color")).toString());
    const auto links = object.value(QStringLiteral("links")).toObject();
    result.downloadUrl = object.value(QStringLiteral("urls")).toObject().value(QStringLiteral("full")).toString();
    result.downloadLocationUrl = links.value(QStringLiteral("download_location")).toString();
    return result;
}

QString UnsplashApi::getPhotoDownloadUrl(const QString& photoId, const QString& size) {
    Q_UNUSED(size);
    if (accessKey_.isEmpty() || photoId.trimmed().isEmpty()) return {};
    const PhotoResult details = getPhotoDetails(photoId.trimmed());
    return details.downloadLocationUrl;
}

QVector<UnsplashApi::PhotoResult> UnsplashApi::parseUnsplashResponse(const QJsonDocument& doc) {
    QVector<PhotoResult> results;
    if (!doc.isObject()) return results;
    for (const auto& value : doc.object().value(QStringLiteral("results")).toArray()) {
        const QJsonObject object = value.toObject();
        PhotoResult result;
        result.id = object.value(QStringLiteral("id")).toString();
        result.description = object.value(QStringLiteral("description")).toString();
        result.url = object.value(QStringLiteral("links")).toObject().value(QStringLiteral("html")).toString();
        result.photographer = object.value(QStringLiteral("user")).toObject().value(QStringLiteral("name")).toString();
        result.width = object.value(QStringLiteral("width")).toInt();
        result.height = object.value(QStringLiteral("height")).toInt();
        result.dominantColor = QColor(object.value(QStringLiteral("color")).toString());
        result.downloadUrl = object.value(QStringLiteral("urls")).toObject().value(QStringLiteral("full")).toString();
        result.downloadLocationUrl = object.value(QStringLiteral("links")).toObject().value(QStringLiteral("download_location")).toString();
        if (!result.id.isEmpty()) results.append(result);
    }
    return results;
}

} // namespace ccos::api
