#include "media/MediaImporter.hpp"
#include "media/MediaProbe.hpp"

#include <QFileInfo>
#include <QSet>
#include <utility>

namespace ccos::media {

std::vector<MediaAsset> MediaImporter::importFiles(const QStringList& paths) {
    return importFiles(paths, 4096, nullptr);
}

std::vector<MediaAsset> MediaImporter::importFiles(const QStringList& paths, const std::size_t maxFiles, QString* error) {
    if (maxFiles == 0) {
        if (error) *error = QStringLiteral("Maximum import file count must be greater than zero");
        return {};
    }

    std::size_t nonEmptyCount = 0;
    for (const QString& path : paths) {
        if (!path.trimmed().isEmpty()) ++nonEmptyCount;
    }
    if (nonEmptyCount > maxFiles) {
        if (error) {
            *error = QStringLiteral("Import contains %1 files; the limit is %2")
                         .arg(static_cast<qulonglong>(nonEmptyCount))
                         .arg(static_cast<qulonglong>(maxFiles));
        }
        return {};
    }

    std::vector<MediaAsset> assets;
    assets.reserve(nonEmptyCount);
    QSet<QString> seenPaths;
    QStringList failures;

    for (const QString& path : paths) {
        const QString normalized = path.trimmed();
        if (normalized.isEmpty()) continue;

        const QFileInfo fileInfo(normalized);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            failures.append(QStringLiteral("Input is not a regular file: %1").arg(normalized));
            continue;
        }

        const QString absolutePath = fileInfo.absoluteFilePath();
        if (seenPaths.contains(absolutePath)) continue;
        seenPaths.insert(absolutePath);

        MediaAsset asset(absolutePath);
        QString probeError;
        if (!MediaProbe::probe(asset, QStringLiteral("ffprobe"), &probeError)) {
            failures.append(QStringLiteral("Unable to probe '%1': %2").arg(absolutePath, probeError));
            continue;
        }
        assets.push_back(std::move(asset));
    }

    if (error && !failures.isEmpty()) {
        *error = failures.join(QStringLiteral("\n"));
    }
    return assets;
}

}
