#include "api/EditorApi.hpp"
#include "project/ProjectSerializer.hpp"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ccos-cli"));
    app.setApplicationVersion(QStringLiteral("0.5.0"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption project({QStringLiteral("p"), QStringLiteral("project")}, QStringLiteral("CCOS project path"), QStringLiteral("file"));
    QCommandLineOption operation({QStringLiteral("o"), QStringLiteral("op")}, QStringLiteral("inspect, validate or export"), QStringLiteral("operation"), QStringLiteral("inspect"));
    QCommandLineOption output(QStringLiteral("out"), QStringLiteral("Export output path"), QStringLiteral("file"));
    QCommandLineOption request(QStringLiteral("request"), QStringLiteral("JSON request object"), QStringLiteral("json"));
    QCommandLineOption ffmpeg(QStringLiteral("ffmpeg"), QStringLiteral("FFmpeg executable"), QStringLiteral("path"), QStringLiteral("ffmpeg"));
    parser.addOption(project); parser.addOption(operation); parser.addOption(output); parser.addOption(request); parser.addOption(ffmpeg);
    parser.process(app);

    QTextStream out(stdout);
    if (!parser.isSet(project)) { out << "--project is required\n"; return 2; }
    ccos::project::Project model;
    QString error;
    if (!ccos::project::ProjectSerializer::load(model, parser.value(project), &error)) {
        out << QJsonDocument(QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}}).toJson(QJsonDocument::Compact) << '\n';
        return 1;
    }

    QJsonObject req;
    if (parser.isSet(request)) {
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(parser.value(request).toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) { out << "Invalid JSON request\n"; return 2; }
        req = doc.object();
    } else {
        req.insert(QStringLiteral("op"), parser.value(operation));
        if (parser.isSet(output)) req.insert(QStringLiteral("output"), parser.value(output));
    }

    const auto result = ccos::api::EditorApi::command(model, req, parser.value(ffmpeg));
    out << QJsonDocument(result).toJson(QJsonDocument::Indented) << '\n';
    return result.value(QStringLiteral("ok")).toBool(false) ? 0 : 1;
}
