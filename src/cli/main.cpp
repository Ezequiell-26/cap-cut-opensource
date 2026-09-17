#include "api/EditorApi.hpp"
#include "project/ProjectSerializer.hpp"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ccos-cli"));
    app.setApplicationVersion(QStringLiteral("0.7.0"));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption project({QStringLiteral("p"), QStringLiteral("project")}, QStringLiteral("CCOS project path"), QStringLiteral("file"));
    QCommandLineOption operation({QStringLiteral("o"), QStringLiteral("op")}, QStringLiteral("inspect, validate, export, hardware or doctor"), QStringLiteral("operation"), QStringLiteral("inspect"));
    QCommandLineOption output(QStringLiteral("out"), QStringLiteral("Export output path"), QStringLiteral("file"));
    QCommandLineOption request(QStringLiteral("request"), QStringLiteral("JSON request object"), QStringLiteral("json"));
    QCommandLineOption query(QStringLiteral("query"), QStringLiteral("Location search query for geocode"), QStringLiteral("text"));
    QCommandLineOption ffmpeg(QStringLiteral("ffmpeg"), QStringLiteral("FFmpeg executable"), QStringLiteral("path"), QStringLiteral("ffmpeg"));
    parser.addOption(project);
    parser.addOption(operation);
    parser.addOption(output);
    parser.addOption(request);
    parser.addOption(query);
    parser.addOption(ffmpeg);
    parser.process(app);

    QTextStream out(stdout);

    QJsonObject req;
    if (parser.isSet(request)) {
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(parser.value(request).toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            out << "Invalid JSON request\n";
            return 2;
        }
        req = doc.object();
    } else {
        req.insert(QStringLiteral("op"), parser.value(operation));
        if (parser.isSet(output)) req.insert(QStringLiteral("output"), parser.value(output));
        if (parser.isSet(query)) req.insert(QStringLiteral("query"), parser.value(query));
    }

    const QString requestedOperation = req.value(QStringLiteral("op")).toString().trimmed().toLower();
    if (requestedOperation == QStringLiteral("geocode") || requestedOperation == QStringLiteral("location_search")) {
        ccos::project::Project emptyProject;
        const auto result = ccos::api::EditorApi::command(emptyProject, req, parser.value(ffmpeg));
        out << QJsonDocument(result).toJson(QJsonDocument::Indented) << '\n';
        return result.value(QStringLiteral("ok")).toBool(false) ? 0 : 1;
    }

    if (requestedOperation == QStringLiteral("doctor") || requestedOperation == QStringLiteral("hardware") ||
        requestedOperation == QStringLiteral("hardware_capabilities")) {
        out << QJsonDocument(ccos::api::EditorApi::hardwareCapabilities(parser.value(ffmpeg)))
                   .toJson(QJsonDocument::Indented)
            << '\n';
        return 0;
    }

    if (!parser.isSet(project)) {
        out << "--project is required for operation: " << requestedOperation << "\n";
        return 2;
    }

    ccos::project::Project model;
    QString error;
    if (!ccos::project::ProjectSerializer::load(model, parser.value(project), &error)) {
        out << QJsonDocument(QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}}).toJson(QJsonDocument::Compact) << '\n';
        return 1;
    }

    const auto result = ccos::api::EditorApi::command(model, req, parser.value(ffmpeg));
    out << QJsonDocument(result).toJson(QJsonDocument::Indented) << '\n';
    return result.value(QStringLiteral("ok")).toBool(false) ? 0 : 1;
}
