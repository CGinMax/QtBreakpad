﻿#include "platformhelper.h"
#include <QCoreApplication>
#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSharedPointer>
#include <QtConcurrent>
#include <QEventLoop>
#include <functional>

#if defined (Q_OS_WIN32) && defined (Q_CC_MSVC)
#include "client/windows/handler/exception_handler.h"
bool callback(const wchar_t *dump_path, const wchar_t *id, void *context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded)
{
    auto platform = reinterpret_cast<ValWell::PlatformHelper *>(context);
    platform->updateBreakpadPath();
    platform->prepareReportFilePath();

    QString newDumpFilePath(platform->archiveDumpFile(QString::fromWCharArray(dump_path)));
    if (newDumpFilePath.isEmpty()) {
        qDebug("move dump file path error");
        return succeeded;
    }

    QEventLoop loop;
    QtConcurrent::run([=](QEventLoop* eventLoop) {
        if (!platform->archiveSym(QCoreApplication::applicationName(), QCoreApplication::applicationDirPath())) {
            qDebug("build sym failed!");
            return ;
        }

        auto sharedNameList = platform->prepareSharedLib(QCoreApplication::applicationDirPath(), QString("dll"));
        std::function<bool(const QString&)> functor = [=](const QString& sharedName) {
            return platform->archiveSym(sharedName, QCoreApplication::applicationDirPath());
        };
        auto mappedResult = QtConcurrent::mapped(sharedNameList, functor);
        mappedResult.waitForFinished();

        QString pluginDirPath = QCoreApplication::applicationDirPath() + "/plugin";
        auto pluginNameList = platform->prepareSharedLib(pluginDirPath, QString("dll"));
        auto pluginResult = QtConcurrent::mapped(pluginNameList, functor);
        pluginResult.waitForFinished();

        if (!platform->reportCrash(newDumpFilePath)) {
            qDebug("report crash failed!");
            return ;
        }
        platform->removeSymbolDir();
        platform->infoFinished();
        while (!eventLoop->isRunning()) {
            QThread::msleep(100);
        }
        eventLoop->quit();
    }, &loop);

    loop.exec();

    return succeeded;
}

namespace ValWell {
class PlatformHelperPrivate final
{
public:
    PlatformHelperPrivate();
    ~PlatformHelperPrivate() = default;

    QSharedPointer<google_breakpad::ExceptionHandler> _excepHandler;
};

PlatformHelperPrivate::PlatformHelperPrivate()
{}

PlatformHelper::PlatformHelper(const QSharedPointer<InfoManager>& info)
    : infoManager(info)
    , _private(new PlatformHelperPrivate())
    , outputPath(QString())
    , dumpSymsPath(QString())
    , minidumpStackwalkPath(QString())
    , crashReportFilePath(QString())
{
    dumpSymsPath = QDir::toNativeSeparators(QString("%1/dump_syms.exe").arg(QCoreApplication::applicationDirPath()));
    minidumpStackwalkPath = QDir::toNativeSeparators(QString("%1/minidump_stackwalk.exe").arg(QCoreApplication::applicationDirPath()));

}
PlatformHelper::~PlatformHelper()
{}

void PlatformHelper::initCrashHandler()
{
    outputPath = QCoreApplication::applicationDirPath() + QLatin1String("/breakpad");
    outputPath = QDir::toNativeSeparators(outputPath);

//    auto eh = new google_breakpad::ExceptionHandler(outputPath.toStdWString(), nullptr, callback, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
    _private->_excepHandler.reset(new google_breakpad::ExceptionHandler(/*outputPath.toStdWString()*/L".", nullptr, callback, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL));
}

bool PlatformHelper::archiveSym(const QString &appName, const QString &appDirPath)
{
    QFileInfo appInfo(appDirPath, appName);
    QString newAppName = appName;
    if (QFile::exists(QDir::toNativeSeparators(appDirPath + "/" + appInfo.completeBaseName() + ".pdb"))) {
        newAppName = appInfo.completeBaseName() + ".pdb";
    }
    QSharedPointer<QProcess> dumpSymProcess(new QProcess());
    QSharedPointer<QByteArray> readData(new QByteArray());

    QObject::connect(dumpSymProcess.data(), &QProcess::readyReadStandardOutput, [=](){
        readData->append(dumpSymProcess->readAllStandardOutput());
    });

    QString appNamePath = QDir::toNativeSeparators(appDirPath + "/" + newAppName);
    dumpSymProcess->start(dumpSymsPath, {appNamePath});
    dumpSymProcess->waitForFinished();
    dumpSymProcess->close();
    if (dumpSymProcess->exitCode() != 0) {
        return false;
    }
    if (readData->isEmpty()) {
        return false;
    }

    QString symId(readFirstLineId(readData));
    if (symId.isEmpty()) {
        return false;
    }
    QString symDirPath = QDir::toNativeSeparators(QString("%1/symbols/%2/%3").arg(breakpadPath()).arg(newAppName).arg(symId));
    QDir symDir(symDirPath);
    if (!symDir.mkpath(symDirPath)) {
        return false;
    }
    // xxx.sym，不需要.pdb或.exe
    QString newSymFilePath = QDir::toNativeSeparators(QString("%1/%2.sym").arg(symDirPath).arg(appInfo.completeBaseName()));
    QFile newSymFile(newSymFilePath);
    if (!newSymFile.open(QFile::WriteOnly)) {
        return false;
    }
    if (newSymFile.write(readData->data()) <= 0) {
        newSymFile.close();
        return false;
    }
    newSymFile.close();
    return true;
}
} // namespace ValWell
#endif