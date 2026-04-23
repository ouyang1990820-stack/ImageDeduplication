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
        QTimer::singleShot(0, &window, [&window, autoScanDirectory]() {
            window.addDirectoryPath(autoScanDirectory);
            QMetaObject::invokeMethod(&window, "startScan", Qt::QueuedConnection);
        });
    }

    const QString screenshotPath = qEnvironmentVariable("IMAGE_DEDUP_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(2500, &window, [&window, screenshotPath, &application]() {
            window.grab().save(screenshotPath);
            if (qEnvironmentVariableIsSet("IMAGE_DEDUP_AUTOQUIT")) {
                application.quit();
            }
        });
    }
    return application.exec();
}
