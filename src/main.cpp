#include "mainwindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <cstdio>
#include <QSurfaceFormat>

namespace
{
// Must be unique per application; the local server uses this name.
const char *kServerName = "cs3-map-edit-single-instance";

// Returns true if this process is the primary instance and owns the server name.
// If another running instance answers on the name, it is asked to show itself
// and this function returns false.
bool ensureSingleInstance(QLocalServer &server)
{
    if (server.listen(kServerName))
        return true;

    // listen() failed: either another live instance owns the name, or a stale
    // socket is left over from a crashed instance.
    QLocalSocket probe;
    probe.connectToServer(kServerName);
    if (probe.waitForConnected(1000))
    {
        // A running instance answered - it will bring its window to the front.
        probe.disconnectFromServer();
        return false;
    }

    // No live instance behind the name: clean up the stale socket and retry.
    probe.abort();
    QLocalServer::removeServer(kServerName);
    server.listen(kServerName);
    return true;
}
}

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setMajorVersion( 2 );
    format.setMinorVersion( 0 );
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);

    QLocalServer server;
    if (!ensureSingleInstance(server))
    {
        fprintf(stderr, "SECOND_INSTANCE_DETECTED\n");
        QMessageBox::warning(nullptr, QObject::tr("cs3-map-edit"),
                             QObject::tr("The map editor is already running.\n"
                                         "A second instance cannot be started."));
        return 0;
    }

    MainWindow w;
    w.show();

    // A second instance's connection attempt is our signal to come to the front.
    QObject::connect(&server, &QLocalServer::newConnection, [&w, &server]()
                     {
        while (QLocalSocket *socket = server.nextPendingConnection())
        {
            w.show();
            w.raise();
            w.activateWindow();
            socket->disconnectFromServer();
        }
    });

    return a.exec();
}
