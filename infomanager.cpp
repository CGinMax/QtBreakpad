#include "infomanager.h"
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QDebug>

namespace ValWell {

class InfoManagerPrivate
{
    Q_DECLARE_PUBLIC(InfoManager)
    Q_DISABLE_COPY(InfoManagerPrivate)
public:
    InfoManagerPrivate(InfoManager* q);
    ~InfoManagerPrivate();

    void insertInfo(const QString& info);
    InfoManager* q_ptr;
    QMutex mutex;
    QList<QString> infoList;
};

InfoManagerPrivate::InfoManagerPrivate(InfoManager* q)
    : q_ptr(q)
{}

InfoManagerPrivate::~InfoManagerPrivate()
{
}

void InfoManagerPrivate::insertInfo(const QString &info)
{
    QMutexLocker locker(&mutex);
    infoList.append(info);
}

InfoManager::InfoManager(QObject* parent)
    : QObject (parent)
    , d_ptr(new InfoManagerPrivate(this))
{
}

InfoManager::~InfoManager()
{}

void InfoManager::start(std::function<void()> &&worker)
{
    worker();
}

void InfoManager::pushDumpPath(const QString &path)
{
    Q_D(InfoManager);
    QString info = QString("Dump path: %1").arg(path);
    qDebug() << info;
    d->insertInfo(info);
}

void InfoManager::pushReportPath(const QString &path)
{
    Q_D(InfoManager);
    QString info = QString("Crash report path: %1").arg(path);
    qDebug() << info;
    d->insertInfo(info);
}

void InfoManager::pushInfo(const QString &info)
{
    Q_D(InfoManager);
    qDebug() << info;
    d->insertInfo(info);
}

void InfoManager::finished()
{
    qDebug() << "finished";
}
} // namespace ValWell
