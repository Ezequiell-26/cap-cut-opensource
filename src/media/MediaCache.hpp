#pragma once

#include <QByteArray>
#include <QString>
#include <cstddef>

namespace ccos::media {

class MediaCache {
public:
    explicit MediaCache(QString root, std::size_t maxBytes = 1024ULL * 1024ULL * 1024ULL);

    [[nodiscard]] QString keyFor(const QString& source, const QString& variant) const;
    [[nodiscard]] QString pathFor(const QString& source, const QString& variant, const QString& extension) const;

    // Writes are crash-safe: the destination is replaced atomically on commit.
    bool write(const QString& source, const QString& variant, const QByteArray& data, const QString& extension);
    [[nodiscard]] QByteArray read(const QString& source, const QString& variant, const QString& extension) const;
    bool remove(const QString& source, const QString& variant, const QString& extension);
    void clear();

private:
    [[nodiscard]] static QString normalizeExtension(const QString& extension);
    [[nodiscard]] static bool isCacheFileName(const QString& fileName);
    void enforceLimit();

    QString root_;
    std::size_t maxBytes_;
};

} // namespace ccos::media
