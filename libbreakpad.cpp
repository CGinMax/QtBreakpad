#include "libbreakpad.h"
#include "platformhelper.h"
#include "infomanager.h"

class LibBreakpadPrivate final
{
public:
    LibBreakpadPrivate();
    ~LibBreakpadPrivate();

    void init();
    QSharedPointer<ValWell::InfoManager> infoManager;
    QScopedPointer<ValWell::PlatformHelper> platformHelper;
};

LibBreakpadPrivate::LibBreakpadPrivate()
    : infoManager(new ValWell::InfoManager())
    , platformHelper(new ValWell::PlatformHelper(infoManager))
{

}

LibBreakpadPrivate::~LibBreakpadPrivate()
{
}

void LibBreakpadPrivate::init()
{
}

LibBreakpad *LibBreakpad::instance()
{
    static LibBreakpad ins;
    return &ins;
}

LibBreakpad::LibBreakpad()
    : _private(new LibBreakpadPrivate())
{
    _private->init();
}

LibBreakpad::~LibBreakpad()
{

}

void LibBreakpad::initBreakpad()
{
    _private->platformHelper->initCrashHandler();
}

void LibBreakpad::setCrashReportFilePath(const QString &filePath)
{
    _private->platformHelper->setCrashReportFilePath(filePath);
}

