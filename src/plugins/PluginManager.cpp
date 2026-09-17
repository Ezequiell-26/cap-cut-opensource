#include "plugins/PluginManager.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace ccos::plugins {
namespace {

constexpr qint64 kMaxManifestBytes = 512 * 1024;
constexpr qint64 kMaxLibraryBytes = 1024LL * 1024LL * 1024LL;

bool isAllowedLibraryExtension(const QString& path) {
#if defined(Q_OS_WIN)
    return path.endsWith(QStringLiteral(".dll"), Qt::CaseInsensitive);
#elif defined(Q_OS_MACOS)
    return path.endsWith(QStringLiteral(".dylib"), Qt::CaseInsensitive) ||
           path.endsWith(QStringLiteral(".so"), Qt::CaseInsensitive);
#else
    return path.endsWith(QStringLiteral(".so"), Qt::CaseInsensitive);
#endif
}

bool isSafeRelativeLibraryPath(const QString& path) {
    const QString candidate = QDir::fromNativeSeparators(path.trimmed());
    if (candidate.isEmpty() || QDir::isAbsolutePath(candidate)) return false;
    if (candidate.contains(QStringLiteral(".."))) return false;
    if (candidate.startsWith(QStringLiteral("/")) || candidate.startsWith(QStringLiteral("\\"))) return false;
    return isAllowedLibraryExtension(candidate);
}

QString calculateSha256(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && !file.atEnd()) return {};
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex());
}

} // namespace

QVector<PluginDescriptor> PluginManager::scan(const QStringList& directories, QStringList* diagnostics) {
    QVector<PluginDescriptor> result;

    for (const QString& directory : directories) {
        QDir dir(directory);
        if (!dir.exists()) {
            if (diagnostics) diagnostics->append(QStringLiteral("Plugin directory missing: %1").arg(directory));
            continue;
        }

        const auto manifests = dir.entryInfoList({QStringLiteral("*.ccosplugin.json")}, QDir::Files | QDir::Readable,
                                                  QDir::Name);
        for (const QFileInfo& fileInfo : manifests) {
            if (fileInfo.size() <= 0 || fileInfo.size() > kMaxManifestBytes) {
                if (diagnostics) diagnostics->append(QStringLiteral("Manifest rejected due to size: %1").arg(fileInfo.fileName()));
                continue;
            }

            QFile file(fileInfo.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly)) {
                if (diagnostics) diagnostics->append(QStringLiteral("Cannot read %1").arg(fileInfo.fileName()));
                continue;
            }

            QJsonParseError parse{};
            const QByteArray bytes = file.readAll();
            const auto doc = QJsonDocument::fromJson(bytes, &parse);
            if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
                if (diagnostics) diagnostics->append(QStringLiteral("Invalid manifest: %1").arg(fileInfo.fileName()));
                continue;
            }

            const QJsonObject object = doc.object();
            PluginDescriptor descriptor;
            descriptor.id = object.value(QStringLiteral("id")).toString().trimmed();
            descriptor.name = object.value(QStringLiteral("name")).toString().trimmed();
            descriptor.version = object.value(QStringLiteral("version")).toString().trimmed();
            descriptor.apiVersion = object.value(QStringLiteral("apiVersion")).toString().trimmed();
            descriptor.library = object.value(QStringLiteral("library")).toString().trimmed();
            descriptor.sha256 = object.value(QStringLiteral("sha256")).toString().trimmed().toLower();
            descriptor.raw = object;

            for (const QJsonValue& value : object.value(QStringLiteral("capabilities")).toArray()) {
                if (value.isString()) descriptor.capabilities.append(value.toString());
            }

            if (descriptor.id.isEmpty() || descriptor.name.isEmpty() || descriptor.version.isEmpty() ||
                descriptor.apiVersion.isEmpty() || descriptor.library.isEmpty()) {
                if (diagnostics) diagnostics->append(QStringLiteral("Manifest missing required fields: %1").arg(fileInfo.fileName()));
                continue;
            }

            if (!isSafeRelativeLibraryPath(descriptor.library)) {
                if (diagnostics) diagnostics->append(QStringLiteral("Unsafe plugin library path: %1").arg(descriptor.library));
                continue;
            }

            const QString resolved = QDir(fileInfo.absolutePath()).filePath(descriptor.library);
            const QFileInfo libraryInfo(resolved);
            const QString canonicalRoot = QDir(fileInfo.absolutePath()).canonicalPath();
            const QString canonicalLibrary = libraryInfo.canonicalFilePath();
            if (canonicalRoot.isEmpty() || canonicalLibrary.isEmpty() ||
                !canonicalLibrary.startsWith(canonicalRoot + QDir::separator())) {
                if (diagnostics) diagnostics->append(QStringLiteral("Plugin library escapes plugin directory: %1").arg(descriptor.id));
                continue;
            }

            descriptor.resolvedLibraryPath = canonicalLibrary;
            descriptor.libraryExists = libraryInfo.exists() && libraryInfo.isFile();
            if (!descriptor.libraryExists) {
                if (diagnostics) diagnostics->append(QStringLiteral("Plugin library missing: %1").arg(descriptor.id));
                continue;
            }

            if (libraryInfo.size() <= 0 || libraryInfo.size() > kMaxLibraryBytes) {
                if (diagnostics) diagnostics->append(QStringLiteral("Plugin library rejected due to size: %1").arg(descriptor.id));
                continue;
            }

            if (!descriptor.sha256.isEmpty()) {
                static const QRegularExpression sha256Pattern(QStringLiteral("^[0-9a-f]{64}$"));
                if (!sha256Pattern.match(descriptor.sha256).hasMatch()) {
                    if (diagnostics) diagnostics->append(QStringLiteral("Invalid SHA-256 in plugin manifest: %1").arg(descriptor.id));
                    continue;
                }

                const QString actualHash = calculateSha256(canonicalLibrary);
                descriptor.hashVerified = !actualHash.isEmpty() && actualHash == descriptor.sha256;
                if (!descriptor.hashVerified) {
                    if (diagnostics) diagnostics->append(QStringLiteral("Plugin hash mismatch: %1").arg(descriptor.id));
                    continue;
                }
            }

            result.append(std::move(descriptor));
        }
    }
    return result;
}

bool PluginManager::isCompatible(const PluginDescriptor& descriptor, const QString& apiVersion) {
    if (descriptor.apiVersion != apiVersion || descriptor.library.isEmpty()) return false;
    if (!descriptor.libraryExists || descriptor.resolvedLibraryPath.isEmpty()) return false;
    if (!isAllowedLibraryExtension(descriptor.resolvedLibraryPath)) return false;
    if (!descriptor.sha256.isEmpty() && !descriptor.hashVerified) return false;
    return true;
}

} // namespace ccos::plugins
