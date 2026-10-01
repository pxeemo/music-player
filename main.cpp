#include <QGuiApplication>
#include <QQmlApplicationEngine>


int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    // Gives QStandardPaths a stable cache directory for the extracted artwork.
    QGuiApplication::setApplicationName(QStringLiteral("karaoke"));
    QGuiApplication::setOrganizationName(QStringLiteral("karaoke"));

    QQmlApplicationEngine engine;
    engine.loadFromModule("Karaoke", "Main");
    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;
    return app.exec();
}
