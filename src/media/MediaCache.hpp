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
    bool write(const QString& source, const QString& variant, const QByteArray& data, const QString& extension);
    [[nodiscard]] QByteArray read(const QString& source, const QString& variant, const QString& extension) const;
    bool remove(const QString& source, const QString& variant, const QString& extension);
    void clear();
private:
    void enforceLimit();
    QString root_;
    std::size_t maxBytes_;
};
}
