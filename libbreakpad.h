#ifndef LIBBREAKPAD_H
#define LIBBREAKPAD_H

#include <QString>
#include <QScopedPointer>
#include "libbreakpad_global.h"

class LibBreakpadPrivate;
class LIBBREAKPADSHARED_EXPORT LibBreakpad
{

public:
    static LibBreakpad* instance();

    void initBreakpad();

    void setCrashReportFilePath(const QString& filePath);

private:
    LibBreakpad();
    ~LibBreakpad();
    QScopedPointer<LibBreakpadPrivate> _private;
};

#endif // LIBBREAKPAD_H
