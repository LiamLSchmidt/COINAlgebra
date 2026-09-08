#include <QApplication>
#include <QIcon>
#include "MainWindow.h"
#include <cstdio>

namespace {
QtMessageHandler previousMessageHandler = nullptr;

void startupMessageHandler(QtMsgType type, const QMessageLogContext& context,
                           const QString& message)
{
    // Qt5 emits this informational notice even with an explicit X11 backend.
    // Filter only this exact notice, and only while QApplication initializes.
    if (type == QtInfoMsg && message == QStringLiteral(
            "Warning: Ignoring XDG_SESSION_TYPE=wayland on Gnome. "
            "Use QT_QPA_PLATFORM=wayland to run on Wayland anyway."))
        return;
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    } else {
        const auto text = qFormatLogMessage(type, context, message).toLocal8Bit();
        std::fprintf(stderr, "%s\n", text.constData());
        std::fflush(stderr);
    }
}
}


int main(int argc, char** argv)
{
    previousMessageHandler = qInstallMessageHandler(startupMessageHandler);
    QApplication app(argc, argv);
    qInstallMessageHandler(previousMessageHandler);
    app.setApplicationName("DecayQuiver Studio");
    app.setApplicationDisplayName("DecayQuiver Studio");
    app.setOrganizationName("COINAlgebra");
    app.setDesktopFileName("DQStudio");
    app.setStyle("Fusion");

    MainWindow w;
    app.setWindowIcon(w.windowIcon());
    w.show();

    return app.exec();
}
