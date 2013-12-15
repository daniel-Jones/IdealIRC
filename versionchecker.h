#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QTcpSocket>

class VersionChecker : public QTcpSocket
{
    Q_OBJECT
public:
    explicit VersionChecker(QObject *parent = 0);
    QString getVersion() { return version; }
    int getInternVersion() { return internversion; }
    void runChecker(); // Run this once, and signal gotVersion() will emit when finished.

private:
    QString version;
    int internversion;
    bool htmldata;

public slots:
    void socketOpened();
    void dataRead();

signals:
    void gotVersion();
    void unableToCheck(); // When this one is sent, we just want to destroy this object.
};

#endif // VERSIONCHECKER_H
