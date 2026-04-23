#pragma once

#include "DatabaseManager.h"
#include "PreviewDialog.h"
#include "ScannerService.h"
#include "SimilarityService.h"

#include <QMainWindow>

class QListWidget;
class QTreeWidget;
class QLabel;
class QTextEdit;
class QComboBox;
class QSlider;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QSplitter;
class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;
    void addDirectoryPath(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void addDirectory();
    void removeSelectedDirectory();
    void clearDirectories();
    void startScan();
    void refreshPreview();
    void exportCsv();
    void exportHtml();
    void deleteSelectedImages();
    void cleanupMissing();
    void openPreviewDialog();
    void openRecycleBin();

private:
    void buildUi();
    void createMenus();
    void createToolBar();
    void loadSettings();
    void saveSettings();
    void appendLog(const QString& message);
    void refreshDirectories();
    void refreshGroups(const QList<SimilarGroupData>& groups);
    void updateThresholdLabel(int value);
    ScanOptions currentScanOptions() const;
    QList<ImageRecord> selectedImages() const;
    void revealFile(const QString& path);

    DatabaseManager databaseManager_;
    ScannerService scannerService_;
    SimilarityService similarityService_;
    QList<SimilarGroupData> currentGroups_;
    PreviewDialog* previewDialog_ = nullptr;

    QListWidget* directoryListWidget_ = nullptr;
    QTreeWidget* resultsTreeWidget_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QComboBox* depthComboBox_ = nullptr;
    QSpinBox* depthSpinBox_ = nullptr;
    QCheckBox* skipHiddenCheckBox_ = nullptr;
    QLineEdit* extensionsEdit_ = nullptr;
    QSpinBox* minSizeSpinBox_ = nullptr;
    QSpinBox* maxSizeSpinBox_ = nullptr;
    QComboBox* algorithmComboBox_ = nullptr;
    QSlider* thresholdSlider_ = nullptr;
    QLabel* thresholdValueLabel_ = nullptr;
};
