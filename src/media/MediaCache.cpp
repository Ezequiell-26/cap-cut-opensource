#include "media/MediaCache.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <algorithm>

namespace ccos::media {

MediaCache::MediaCache(QString root, std::size_t maxBytes) : root_(std::move(root)), maxBytes_(maxBytes) {
    QDir().mkpath(root_);
}

QString MediaCache::keyFor(const QString& source, const QString& variant) const {
    const QByteArray payload = source.toUtf8() + '\0' + variant.toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

QString MediaCache::pathFor(const QString& source, const QString& variant, const QString& extension) const {
    const QString cleanExtension = extension.startsWith('.') ? extension : QStringLiteral(".") + extension;
    return QDir(root_).filePath(keyFor(source, variant) + cleanExtension);
}

bool MediaCache::write(const QString& source, const QString& variant, const QByteArray& data, const QString& extension) {
    const auto path = pathFor(source, variant, extension);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    if (file.write(data) != data.size()) return false;
    file.close();
    enforceLimit();
    return true;
}

QByteArray MediaCache::read(const QString& source, const QString& variant, const QString& extension) const {
    QFile file(pathFor(source, variant, extension));
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

bool MediaCache::remove(const QString& source, const QString& variant, const QString& extension) {
    return QFile::remove(pathFor(source, variant, extension));
}

void MediaCache::clear() {
    const QDir dir(root_);
    for (const auto& info : dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed)) QFile::remove(info.absoluteFilePath());
}

void MediaCache::enforceLimit() {
    QDir dir(root_);
    auto files = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
    std::size_t total = 0;
    for (const auto& info : files) total += static_cast<std::size_t>(info.size());
    for (const auto& info : files) {
        if (total <= maxBytes_) break;
        const auto size = static_cast<std::size_t>(std::max<qint64>(0, info.size()));
        if (QFile::remove(info.absoluteFilePath())) total = total >= size ? total - size : 0;
    }
}

} // namespace ccos::media
