#include "media/MediaCache.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <utility>

namespace ccos::media {
namespace {

constexpr int kMaxExtensionLength = 16;
constexpr int kCacheKeyLength = 64;

bool isHex(const QChar c) noexcept {
    return (c >= QLatin1Char('0') && c <= QLatin1Char('9')) ||
           (c >= QLatin1Char('a') && c <= QLatin1Char('f')) ||
           (c >= QLatin1Char('A') && c <= QLatin1Char('F'));
}

} // namespace

MediaCache::MediaCache(QString root, std::size_t maxBytes)
    : root_(QDir::cleanPath(std::move(root))), maxBytes_(maxBytes) {
    if (!root_.isEmpty()) QDir().mkpath(root_);
}

QString MediaCache::normalizeExtension(const QString& extension) {
    QString normalized = extension.trimmed();
    while (normalized.startsWith(QLatin1Char('.'))) normalized.remove(0, 1);

    if (normalized.isEmpty() || normalized.size() > kMaxExtensionLength) return {};
    for (const QChar c : normalized) {
        const bool valid = (c >= QLatin1Char('a') && c <= QLatin1Char('z')) ||
                           (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) ||
                           (c >= QLatin1Char('0') && c <= QLatin1Char('9')) ||
                           c == QLatin1Char('_') || c == QLatin1Char('-');
        if (!valid) return {};
    }
    return normalized.toLower();
}

bool MediaCache::isCacheFileName(const QString& fileName) {
    const qsizetype dot = fileName.indexOf(QLatin1Char('.'));
    if (dot != kCacheKeyLength || dot <= 0 || dot == fileName.size() - 1) return false;
    if (fileName.lastIndexOf(QLatin1Char('.')) != dot) return false;
    for (int i = 0; i < dot; ++i) {
        if (!isHex(fileName.at(i))) return false;
    }
    const QString extension = normalizeExtension(fileName.mid(dot + 1));
    return !extension.isEmpty();
}

QString MediaCache::keyFor(const QString& source, const QString& variant) const {
    QString normalizedSource = source.trimmed();
    const QFileInfo info(normalizedSource);
    if (info.exists() && info.isFile()) {
        normalizedSource = info.absoluteFilePath();
        const QByteArray fingerprint = QByteArray::number(info.size()) + '\0' +
                                       QByteArray::number(info.lastModified().toMSecsSinceEpoch());
        normalizedSource += QStringLiteral("#file=") + QString::fromLatin1(fingerprint.toHex());
    }

    const QByteArray payload = normalizedSource.toUtf8() + '\0' + variant.toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

QString MediaCache::pathFor(const QString& source, const QString& variant, const QString& extension) const {
    const QString normalized = normalizeExtension(extension);
    if (root_.isEmpty() || normalized.isEmpty()) return {};
    return QDir(root_).filePath(keyFor(source, variant) + QLatin1Char('.') + normalized);
}

bool MediaCache::write(const QString& source, const QString& variant, const QByteArray& data, const QString& extension) {
    const QString path = pathFor(source, variant, extension);
    if (path.isEmpty()) return false;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) return false;

    enforceLimit();
    return true;
}

QByteArray MediaCache::read(const QString& source, const QString& variant, const QString& extension) const {
    const QString path = pathFor(source, variant, extension);
    if (path.isEmpty()) return {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

bool MediaCache::remove(const QString& source, const QString& variant, const QString& extension) {
    const QString path = pathFor(source, variant, extension);
    return !path.isEmpty() && QFile::remove(path);
}

void MediaCache::clear() {
    if (root_.isEmpty()) return;

    const QDir dir(root_);
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& info : files) {
        if (isCacheFileName(info.fileName())) QFile::remove(info.absoluteFilePath());
    }
}

void MediaCache::enforceLimit() {
    if (root_.isEmpty()) return;

    QDir dir(root_);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    files.erase(std::remove_if(files.begin(), files.end(), [](const QFileInfo& info) {
        return !isCacheFileName(info.fileName());
    }), files.end());

    std::size_t total = 0;
    for (const QFileInfo& info : files) {
        total += static_cast<std::size_t>(std::max<qint64>(0, info.size()));
    }

    for (const QFileInfo& info : files) {
        if (total <= maxBytes_) break;
        const std::size_t size = static_cast<std::size_t>(std::max<qint64>(0, info.size()));
        if (QFile::remove(info.absoluteFilePath())) total = total >= size ? total - size : 0;
    }
}

} // namespace ccos::media
