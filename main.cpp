#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "filemanager.h"
#include "sortfilterproxymodel.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setOrganizationName("hanrai");
    app.setOrganizationDomain("hanrai.com");
    app.setApplicationName("Elf Explorer");

    qmlRegisterType<SortFilterProxyModel>("org.qtproject.example", 1, 0, "SortFilterProxyModel");
    QQmlApplicationEngine engine;
    FileManager fileManager(&engine);
    QQmlContext* rootContext = engine.rootContext();
    rootContext->setContextProperty("fileManager", &fileManager);
    rootContext->setContextProperty("arcModel", (QObject*)fileManager.getModel());
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));

    return app.exec();
}
