#ifndef INFOMANAGER_H
#define INFOMANAGER_H

#include <QObject>
#include <QScopedPointer>
#include <functional>

/**
 * 未完全完成
 */
namespace ValWell {

class InfoManagerPrivate;
class InfoManager : public QObject
{
    Q_OBJECT
public:
    explicit InfoManager(QObject* parent = nullptr);
    ~InfoManager() override;

    void start(std::function<void()>&& worker);
    void pushDumpPath(const QString& path);
    void pushReportPath(const QString& path);
    void pushInfo(const QString& info);
    void finished();
signals:
    void updateDumpPath(const QString& path);
    void updateReportPath(const QString& path);
    void notifyInfo(const QString& info);

private:
    const QScopedPointer<InfoManagerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(InfoManager)
};


} // namespace ValWell
#endif // INFOMANAGER_H
