#include "platformhelper.h"
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QSharedPointer>
#include <QBuffer>
#include <QDateTime>

namespace ValWell {

#if !defined (Q_OS_LINUX) && !(defined (Q_OS_WIN32) && defined (Q_CC_MSVC))
class PlatformHelperPrivate
{
public:
    PlatformHelperPrivate() = default;
    ~PlatformHelperPrivate() = default;
};

PlatformHelper::PlatformHelper(const QSharedPointer<InfoManager>& info)
    : infoManager(info)
    , _private(new PlatformHelperPrivate())
    , outputPath(QString())
    , dumpSymsPath(QString())
    , minidumpStackwalkPath(QString())
    , crashReportFilePath(QString())
{}

PlatformHelper::~PlatformHelper()
{}

void PlatformHelper::initCrashHandler()
{}
#endif

QSharedPointer<InfoManager> PlatformHelper::getInfoManager()
{
    return infoManager;
}

void PlatformHelper::infoFinished()
{
    infoManager->finished();
}

void PlatformHelper::updateBreakpadPath()
{
    outputPath.append(QDir::toNativeSeparators("/") + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss.zzz"));
}

QString PlatformHelper::breakpadPath() const
{
    return outputPath;
}

void PlatformHelper::setCrashReportFilePath(const QString &filePath)
{
    crashReportFilePath = filePath;
}

QString PlatformHelper::getCrashReportFilePath() const
{
    return crashReportFilePath;
}

void PlatformHelper::prepareReportFilePath()
{
    if (crashReportFilePath.isEmpty()) {
        setCrashReportFilePath(QDir::toNativeSeparators(breakpadPath() + QLatin1String("/crash_report.bug")));
    }
}

QStringList PlatformHelper::prepareSharedLib(const QString &dirPath, const QString suffix)
{
    QDir sharedLibDir(QDir::toNativeSeparators(dirPath));
    QStringList filenameList;
    QFileInfoList fileinfoList = sharedLibDir.entryInfoList(QDir::Files);

    for (auto& fileinfo : fileinfoList) {
        if (fileinfo.completeSuffix().contains(suffix) && !fileinfo.fileName().contains("Qt5")
                && !fileinfo.fileName().contains("api-ms-win", Qt::CaseInsensitive)) {
            filenameList.append(fileinfo.fileName());
        }
    }
    return filenameList;
}

QString PlatformHelper::archiveDumpFile(const QString& oldDumpFilePath)
{
    int lastSlashIdx = oldDumpFilePath.lastIndexOf(QDir::toNativeSeparators("/"));
    QString dumpFileName = oldDumpFilePath.right(oldDumpFilePath.size() - lastSlashIdx - 1);

    QString newDumpFilePath = breakpadPath() + QDir::toNativeSeparators("/") + dumpFileName;
    infoManager->pushDumpPath(newDumpFilePath);

    QDir reportDir(breakpadPath());
    if (!reportDir.exists()) {
        if (!reportDir.mkpath(breakpadPath())) {
            infoManager->pushInfo(QObject::tr("Can not make dir %1!").arg(breakpadPath()));
            return QString();
        }
    }
    if (!QFile::exists(oldDumpFilePath)) {
        infoManager->pushInfo(QObject::tr("Not found dmp file!"));
        return QString();
    }
    if (!QFile::copy(oldDumpFilePath, newDumpFilePath)) {
        infoManager->pushInfo(QObject::tr("Move %1 to %2 failed!").arg(oldDumpFilePath).arg(newDumpFilePath));
        return QString();
    }
    QFile::remove(oldDumpFilePath);
    return newDumpFilePath;
}

bool PlatformHelper::reportCrash(const QString &dumpFilePath)
{
    QSharedPointer<QProcess> minidumpProcess(new QProcess());
    QSharedPointer<QByteArray> readData(new QByteArray());
    QString symbolsDirPath(QDir::toNativeSeparators(breakpadPath() + QLatin1String("/symbols")));

    // 将输出写入到 crashReportFilePath
    QObject::connect(minidumpProcess.data(), &QProcess::readyReadStandardOutput, [=](){
        readData->append(minidumpProcess->readAllStandardOutput());
    });
    // 类似命令：minidump_stackwalk ./xxxx.dump ./symbols
    minidumpProcess->start(minidumpStackwalkPath, {dumpFilePath, symbolsDirPath});
    minidumpProcess->waitForFinished();
    minidumpProcess->close();

    infoManager->pushReportPath(crashReportFilePath);

    if (minidumpProcess->exitCode() != 0) {
        infoManager->pushInfo(QObject::tr("%1 execute failed! Exit code=%2").arg(minidumpStackwalkPath).arg(minidumpProcess->exitCode()));
        return false;
    }
    if (readData->isEmpty()) {
        infoManager->pushInfo(QObject::tr("minidump_stackwalk output crash report failed!"));
        return false;
    }
    QFile reportFile(crashReportFilePath);
    if (!reportFile.open(QFile::WriteOnly)) {
        infoManager->pushInfo(QObject::tr("Can not open %1! %2").arg(crashReportFilePath).arg(reportFile.errorString()));
        return false;
    }
    if (reportFile.write(readData->data()) <= 0) {
        infoManager->pushInfo(QObject::tr("Write crash report data to %1 failed!").arg(crashReportFilePath));
        reportFile.flush();
        reportFile.close();
        return false;
    }
    reportFile.flush();
    reportFile.close();
    return true;
}

bool PlatformHelper::removeSymbolDir()
{
    QDir dir(QDir::toNativeSeparators(breakpadPath() + QLatin1String("/symbols")));
    return dir.removeRecursively();
}

QString PlatformHelper::readFirstLineId(const QSharedPointer<QByteArray> &bufferData)
{
    QBuffer buffer(bufferData.data());
    if (!buffer.open(QBuffer::ReadOnly)) {
        return QString();
    }
    QList<QByteArray> splitList = buffer.readLine().split(' ');
    buffer.close();
    if (splitList.size() < 4) {
        return QString();
    }
    return splitList.at(3);
}
} // namespace ValWell
