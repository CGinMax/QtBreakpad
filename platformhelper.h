#ifndef PLATFORMHELPER_H
#define PLATFORMHELPER_H
#include <QString>
#include <QSharedPointer>
#include <QScopedPointer>
#include "infomanager.h"

namespace ValWell {
class PlatformHelperPrivate;
class PlatformHelper
{
public:
    explicit PlatformHelper(const QSharedPointer<InfoManager>& info);

    ~PlatformHelper();

    void initCrashHandler();

    QSharedPointer<InfoManager> getInfoManager();
    void infoFinished();

    void updateBreakpadPath();
    void prepareReportFilePath();

    QStringList prepareSharedLib(const QString& dirPath, const QString suffix);

    QString breakpadPath() const;

    void setCrashReportFilePath(const QString& filePath);
    QString getCrashReportFilePath() const;

    QString archiveDumpFile(const QString &oldDumpFilePath);

    bool archiveSym(const QString& appName, const QString& appDirPath);
    bool reportCrash(const QString& dumpFilePath);

    bool removeSymbolDir();
private:
    QString readFirstLineId(const QSharedPointer<QByteArray>& bufferData);
    QSharedPointer<InfoManager> infoManager;
    QScopedPointer<PlatformHelperPrivate> _private;
    QString outputPath;
    QString dumpSymsPath;
    QString minidumpStackwalkPath;
    QString crashReportFilePath;
};


} // namespace ValWell
#endif // PLATFORMHELPER_H
