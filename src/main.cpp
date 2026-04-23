#include "MainWindow.h"

#include <QApplication>
#include <QMetaObject>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("ouyang1990820-stack"));
    application.setApplicationName(QStringLiteral("ImageDeduplication"));

    MainWindow window;
    window.show();

    const QString autoScanDirectory = qEnvironmentVariable("IMAGE_DEDUP_AUTOSCAN_DIR");
    if (!autoScanDirectory.isEmpty()) {
        auto* windowPtr = &window;
        QTimer::singleShot(0, &window, [windowPtr, autoScanDirectory]() {
            windowPtr->addDirectoryPath(autoScanDirectory);
            QMetaObject::invokeMethod(windowPtr, "startScan", Qt::QueuedConnection);
        });
    }

    const QString screenshotPath = qEnvironmentVariable("IMAGE_DEDUP_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        auto* windowPtr = &window;
        auto* applicationPtr = &application;
        QTimer::singleShot(2500, &window, [windowPtr, screenshotPath, applicationPtr]() {
            windowPtr->grab().save(screenshotPath);
            if (qEnvironmentVariableIsSet("IMAGE_DEDUP_AUTOQUIT")) {
                applicationPtr->quit();
            }
        });
    }
    return application.exec();
}
