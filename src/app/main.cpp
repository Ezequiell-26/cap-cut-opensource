#include "ui/MainWindow.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QFont>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("CCOS"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("CCOS Open Source"));
    app.setStyle(QStringLiteral("Fusion"));

    QFont font = app.font();
    font.setPointSize(10);
    app.setFont(font);

    ccos::ui::MainWindow window;
#ifdef __EMSCRIPTEN__
    // In the browser, the native-style Qt window is mapped to the WebAssembly
    // canvas. Maximizing here makes the existing desktop UI use the full viewport.
    window.showMaximized();
#else
    window.show();
#endif
    return app.exec();
}
