#include "platformhelper.h"
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QSharedPointer>
#include <QFileInfo>
#include <algorithm>

#include <QtConcurrent>
#include <QEventLoop>
#include <QDebug>
#ifdef Q_OS_LINUX
#include "client/linux/handler/exception_handler.h"

bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void *context, bool succeeded)
{
    auto platform = reinterpret_cast<ValWell::PlatformHelper*>(context);
    platform->updateBreakpadPath();
    platform->prepareReportFilePath();
    QString newDumpFilePath(platform->archiveDumpFile(descriptor.path()));
    if (newDumpFilePath.isEmpty()) {
        qDebug("move dump file failed!");
        return succeeded;
    }

    QEventLoop loop;
    QtConcurrent::run([=](QEventLoop* eventLoop){
        if (!platform->archiveSym(QCoreApplication::applicationName(), QCoreApplication::applicationDirPath())) {
            return ;
        }

        // 似乎对.so.1有效
        auto sharedNameList = platform->prepareSharedLib(QCoreApplication::applicationDirPath(), QString("so"));
        std::function<bool(const QString&)> functor = [=](const QString& sharedName) {
            return platform->archiveSym(sharedName, QCoreApplication::applicationDirPath());
        };
        auto mappedResult = QtConcurrent::mapped(sharedNameList, functor);
        mappedResult.waitForFinished();

        QString pluginDirPath = QCoreApplication::applicationDirPath() + QLatin1String("/plugin");
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

class PlatformHelperPrivate
{
public:
    PlatformHelperPrivate() = default;
    ~PlatformHelperPrivate() = default;

    QSharedPointer<google_breakpad::MinidumpDescriptor> _descriptor;
    QSharedPointer<google_breakpad::ExceptionHandler> _excepHandler;
};

PlatformHelper::PlatformHelper(const QSharedPointer<InfoManager>& info)
    : infoManager(info)
    , _private(new PlatformHelperPrivate())
    , outputPath(QString())
    , dumpSymsPath(QString())
    , minidumpStackwalkPath(QString())
    , crashReportFilePath(QString())
{
    dumpSymsPath = QDir::toNativeSeparators(QString("%1/dump_syms").arg(QCoreApplication::applicationDirPath()));
    minidumpStackwalkPath = QDir::toNativeSeparators(QString("%1/minidump_stackwalk").arg(QCoreApplication::applicationDirPath()));
}

PlatformHelper::~PlatformHelper()
{
}

void PlatformHelper::initCrashHandler()
{
    outputPath = QCoreApplication::applicationDirPath() + QLatin1String("/breakpad");
    outputPath = QDir::toNativeSeparators(outputPath);
    QDir dir(outputPath);
    if (!dir.exists()) {
        dir.mkdir(outputPath);
    }

    _private->_descriptor.reset(new google_breakpad::MinidumpDescriptor(outputPath.toStdString()));
//    auto eh = new google_breakpad::ExceptionHandler(*(_private->_descriptor), nullptr, dumpCallback, this, true, -1);
    _private->_excepHandler.reset(new google_breakpad::ExceptionHandler(*_private->_descriptor, nullptr, dumpCallback, this, true, -1));
}

bool PlatformHelper::archiveSym(const QString &appName, const QString &appDirPath)
{
    QSharedPointer<QProcess> dumpSymProcess(new QProcess());
    QSharedPointer<QByteArray> readData(new QByteArray());

    // 截取输出
    QObject::connect(dumpSymProcess.data(), &QProcess::readyReadStandardOutput, [=](){
        readData->append(dumpSymProcess->readAllStandardOutput());
    });

    QString appNamePath = QDir::toNativeSeparators(appDirPath + "/" + appName);
    infoManager->pushInfo(QObject::tr("building %1 sym file...").arg(appName));

    // 类型命令: dump_sym [appName/sharedLibName]
    dumpSymProcess->start(dumpSymsPath, {appNamePath});
    dumpSymProcess->waitForFinished();
    dumpSymProcess->close();

    if (dumpSymProcess->exitCode() != 0) {
        infoManager->pushInfo(QObject::tr("%1 execute failed! Exit code=%2").arg(dumpSymsPath).arg(dumpSymProcess->exitCode()));
        return false;
    }
    if (readData->isEmpty()) {
        infoManager->pushInfo(QObject::tr("%1 %2 do not output the sym information!").arg(dumpSymsPath).arg(appDirPath));
        return false;
    }

    // 读取symid
    QString symId(readFirstLineId(readData));
    if (symId.isEmpty()) {
        infoManager->pushInfo(QObject::tr("Can not read sym id in dump_sym output data!"));
        return false;
    }
    // 创建symid文件夹
    QString symDirPath = QDir::toNativeSeparators(QString("%1/symbols/%2/%3").arg(breakpadPath()).arg(appName).arg(symId));
    QDir symDir(symDirPath);
    if (!symDir.mkpath(symDirPath)) {
        infoManager->pushInfo(QObject::tr("make path: %1 failed!").arg(symDirPath));
        return false;
    }

    // 写.sym文件
    QString newSymFilePath = QDir::toNativeSeparators(QString("%1/%2.sym").arg(symDirPath).arg(appName));
    QFile newSymFile(newSymFilePath);
    if (!newSymFile.open(QFile::WriteOnly)) {
        infoManager->pushInfo(QObject::tr("Open %1 failed! %2").arg(newSymFilePath).arg(newSymFile.errorString()));
        return false;
    }
    if (newSymFile.write(readData->data()) <= 0) {
        infoManager->pushInfo(QObject::tr("Write %1 failed!").arg(newSymFilePath));
        newSymFile.close();
        return false;
    }
    newSymFile.close();
    return true;
}
} // namespace ValWell
#endif // Q_OS_LINUX
