#include "MainWindow.h"

#include "DeleteConfirmationDialog.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextStream>
#include <QTime>
#include <QToolBar>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QString readableSize(qint64 bytes) {
    double value = static_cast<double>(bytes);
    QString unit = QStringLiteral("B");
    if (value > 1024.0) {
        value /= 1024.0;
        unit = QStringLiteral("KB");
    }
    if (value > 1024.0) {
        value /= 1024.0;
        unit = QStringLiteral("MB");
    }
    if (value > 1024.0) {
        value /= 1024.0;
        unit = QStringLiteral("GB");
    }
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', 2), unit);
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), scannerService_(&databaseManager_, this) {
    const QString defaultDatabase = QCoreApplication::applicationDirPath() + QStringLiteral("/image_deduplication.sqlite");
    if (!databaseManager_.initialize(defaultDatabase)) {
        QMessageBox::critical(this, tr("数据库初始化失败"), databaseManager_.lastError());
    }

    buildUi();
    createMenus();
    createToolBar();
    loadSettings();
    refreshDirectories();
    refreshGroups(databaseManager_.loadGroups(algorithmComboBox_->currentText()));

    connect(&scannerService_, &ScannerService::logMessage, this, &MainWindow::appendLog);
    connect(resultsTreeWidget_, &QTreeWidget::itemSelectionChanged, this, &MainWindow::refreshPreview);
    connect(resultsTreeWidget_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int) { openPreviewDialog(); });
    connect(thresholdSlider_, &QSlider::valueChanged, this, &MainWindow::updateThresholdLabel);

    statusBar()->showMessage(tr("就绪"));
    updateThresholdLabel(thresholdSlider_->value());
}

void MainWindow::buildUi() {
    resize(1480, 900);
    setWindowTitle(tr("图片相似度检测与清理工具"));

    auto* centralSplitter = new QSplitter(Qt::Horizontal, this);
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);

    auto* directoryGroup = new QGroupBox(tr("扫描目录"), leftPanel);
    auto* directoryLayout = new QVBoxLayout(directoryGroup);
    directoryListWidget_ = new QListWidget(directoryGroup);
    directoryListWidget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    directoryLayout->addWidget(directoryListWidget_);

    auto* directoryButtonsLayout = new QHBoxLayout();
    auto* addButton = new QPushButton(tr("添加目录"), directoryGroup);
    auto* removeButton = new QPushButton(tr("移除所选"), directoryGroup);
    auto* clearButton = new QPushButton(tr("清空"), directoryGroup);
    directoryButtonsLayout->addWidget(addButton);
    directoryButtonsLayout->addWidget(removeButton);
    directoryButtonsLayout->addWidget(clearButton);
    directoryLayout->addLayout(directoryButtonsLayout);
    leftLayout->addWidget(directoryGroup);

    auto* optionsGroup = new QGroupBox(tr("扫描与分析设置"), leftPanel);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    depthComboBox_ = new QComboBox(optionsGroup);
    depthComboBox_->addItems({tr("仅当前目录"), tr("包含子目录"), tr("限制层级")});
    depthComboBox_->setCurrentIndex(1);
    depthSpinBox_ = new QSpinBox(optionsGroup);
    depthSpinBox_->setRange(1, 10);
    depthSpinBox_->setValue(3);
    skipHiddenCheckBox_ = new QCheckBox(tr("跳过隐藏/系统目录"), optionsGroup);
    skipHiddenCheckBox_->setChecked(true);
    extensionsEdit_ = new QLineEdit(QStringLiteral("jpg,jpeg,png,bmp,gif,webp,tiff,tif"), optionsGroup);
    minSizeSpinBox_ = new QSpinBox(optionsGroup);
    minSizeSpinBox_->setRange(0, 1024 * 1024);
    minSizeSpinBox_->setSuffix(tr(" KB"));
    maxSizeSpinBox_ = new QSpinBox(optionsGroup);
    maxSizeSpinBox_->setRange(0, 1024 * 1024);
    maxSizeSpinBox_->setSuffix(tr(" KB"));
    algorithmComboBox_ = new QComboBox(optionsGroup);
    algorithmComboBox_->addItems({QStringLiteral("pHash"), QStringLiteral("aHash"), QStringLiteral("dHash")});
    thresholdSlider_ = new QSlider(Qt::Horizontal, optionsGroup);
    thresholdSlider_->setRange(0, 100);
    thresholdSlider_->setValue(90);
    thresholdValueLabel_ = new QLabel(optionsGroup);

    optionsLayout->addRow(tr("扫描深度"), depthComboBox_);
    optionsLayout->addRow(tr("层级限制"), depthSpinBox_);
    optionsLayout->addRow(QString(), skipHiddenCheckBox_);
    optionsLayout->addRow(tr("扩展名"), extensionsEdit_);
    optionsLayout->addRow(tr("最小尺寸"), minSizeSpinBox_);
    optionsLayout->addRow(tr("最大尺寸"), maxSizeSpinBox_);
    optionsLayout->addRow(tr("哈希算法"), algorithmComboBox_);
    optionsLayout->addRow(tr("相似度阈值"), thresholdSlider_);
    optionsLayout->addRow(tr("当前阈值"), thresholdValueLabel_);
    leftLayout->addWidget(optionsGroup);

    auto* actionLayout = new QHBoxLayout();
    auto* scanButton = new QPushButton(tr("开始扫描"), leftPanel);
    auto* cleanupButton = new QPushButton(tr("清理失效记录"), leftPanel);
    actionLayout->addWidget(scanButton);
    actionLayout->addWidget(cleanupButton);
    leftLayout->addLayout(actionLayout);
    leftLayout->addStretch();

    auto* rightSplitter = new QSplitter(Qt::Vertical, this);
    resultsTreeWidget_ = new QTreeWidget(rightSplitter);
    resultsTreeWidget_->setColumnCount(5);
    resultsTreeWidget_->setHeaderLabels({tr("分组 / 图片"), tr("相似度"), tr("大小"), tr("状态"), tr("路径")});
    resultsTreeWidget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    resultsTreeWidget_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    resultsTreeWidget_->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    resultsTreeWidget_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* previewContainer = new QWidget(rightSplitter);
    auto* previewLayout = new QHBoxLayout(previewContainer);
    previewLabel_ = new QLabel(previewContainer);
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setMinimumSize(420, 320);
    previewLabel_->setText(tr("选择图片查看预览"));
    detailsEdit_ = new QTextEdit(previewContainer);
    detailsEdit_->setReadOnly(true);
    previewLayout->addWidget(previewLabel_, 3);
    previewLayout->addWidget(detailsEdit_, 2);

    rightSplitter->addWidget(resultsTreeWidget_);
    rightSplitter->addWidget(previewContainer);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 2);

    centralSplitter->addWidget(leftPanel);
    centralSplitter->addWidget(rightSplitter);
    centralSplitter->setStretchFactor(0, 0);
    centralSplitter->setStretchFactor(1, 1);
    setCentralWidget(centralSplitter);

    auto* logDockContainer = new QWidget(this);
    auto* logLayout = new QVBoxLayout(logDockContainer);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logEdit_ = new QTextEdit(logDockContainer);
    logEdit_->setReadOnly(true);
    logLayout->addWidget(logEdit_);
    auto* dock = new QDockWidget(tr("扫描日志"), this);
    dock->setObjectName(QStringLiteral("logDock"));
    dock->setWidget(logDockContainer);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::addDirectory);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedDirectory);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearDirectories);
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::startScan);
    connect(cleanupButton, &QPushButton::clicked, this, &MainWindow::cleanupMissing);
    connect(resultsTreeWidget_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        auto* item = resultsTreeWidget_->itemAt(position);
        if (!item || !item->parent()) {
            return;
        }
        const QString filePath = item->data(0, Qt::UserRole).toString();
        QMenu menu(this);
        menu.addAction(tr("打开所在目录"), this, [this, filePath] { revealFile(filePath); });
        menu.addAction(tr("复制路径"), this, [filePath] { QApplication::clipboard()->setText(filePath); });
        menu.addAction(tr("大图预览"), this, &MainWindow::openPreviewDialog);
        menu.exec(resultsTreeWidget_->viewport()->mapToGlobal(position));
    });
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(tr("添加目录"), QKeySequence(tr("Ctrl+O")), this, &MainWindow::addDirectory);
    fileMenu->addAction(tr("导出 CSV"), QKeySequence(tr("Ctrl+E")), this, &MainWindow::exportCsv);
    fileMenu->addAction(tr("导出 HTML"), this, &MainWindow::exportHtml);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), QKeySequence::Quit, this, &QWidget::close);

    auto* actionMenu = menuBar()->addMenu(tr("操作"));
    actionMenu->addAction(tr("开始扫描"), QKeySequence(tr("F5")), this, &MainWindow::startScan);
    actionMenu->addAction(tr("删除到回收站"), QKeySequence::Delete, this, &MainWindow::deleteSelectedImages);
    actionMenu->addAction(tr("清理失效记录"), this, &MainWindow::cleanupMissing);
    actionMenu->addAction(tr("打开回收站"), this, &MainWindow::openRecycleBin);
}

void MainWindow::createToolBar() {
    auto* toolbar = addToolBar(tr("快捷操作"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->addAction(tr("添加目录"), this, &MainWindow::addDirectory);
    toolbar->addAction(tr("扫描"), this, &MainWindow::startScan);
    toolbar->addAction(tr("导出 CSV"), this, &MainWindow::exportCsv);
    toolbar->addAction(tr("导出 HTML"), this, &MainWindow::exportHtml);
    toolbar->addAction(tr("删除"), this, &MainWindow::deleteSelectedImages);
}

void MainWindow::loadSettings() {
    QSettings settings(QStringLiteral("ouyang1990820-stack"), QStringLiteral("ImageDeduplication"));
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    restoreState(settings.value(QStringLiteral("window/state")).toByteArray());
    skipHiddenCheckBox_->setChecked(settings.value(QStringLiteral("scan/skipHidden"), true).toBool());
    extensionsEdit_->setText(settings.value(QStringLiteral("scan/extensions"), extensionsEdit_->text()).toString());
    minSizeSpinBox_->setValue(settings.value(QStringLiteral("scan/minKb"), 0).toInt());
    maxSizeSpinBox_->setValue(settings.value(QStringLiteral("scan/maxKb"), 0).toInt());
    algorithmComboBox_->setCurrentText(settings.value(QStringLiteral("scan/algorithm"), QStringLiteral("pHash")).toString());
    thresholdSlider_->setValue(settings.value(QStringLiteral("scan/threshold"), 90).toInt());
    depthComboBox_->setCurrentIndex(settings.value(QStringLiteral("scan/depthMode"), 1).toInt());
    depthSpinBox_->setValue(settings.value(QStringLiteral("scan/depthLimit"), 3).toInt());
}

void MainWindow::saveSettings() {
    QSettings settings(QStringLiteral("ouyang1990820-stack"), QStringLiteral("ImageDeduplication"));
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
    settings.setValue(QStringLiteral("scan/skipHidden"), skipHiddenCheckBox_->isChecked());
    settings.setValue(QStringLiteral("scan/extensions"), extensionsEdit_->text());
    settings.setValue(QStringLiteral("scan/minKb"), minSizeSpinBox_->value());
    settings.setValue(QStringLiteral("scan/maxKb"), maxSizeSpinBox_->value());
    settings.setValue(QStringLiteral("scan/algorithm"), algorithmComboBox_->currentText());
    settings.setValue(QStringLiteral("scan/threshold"), thresholdSlider_->value());
    settings.setValue(QStringLiteral("scan/depthMode"), depthComboBox_->currentIndex());
    settings.setValue(QStringLiteral("scan/depthLimit"), depthSpinBox_->value());
}

void MainWindow::appendLog(const QString& message) {
    logEdit_->append(QTime::currentTime().toString(QStringLiteral("HH:mm:ss ")) + message);
    statusBar()->showMessage(message, 5000);
}

void MainWindow::refreshDirectories() {
    directoryListWidget_->clear();
    for (const auto& directory : databaseManager_.loadDirectories()) {
        auto* item = new QListWidgetItem(directory.path, directoryListWidget_);
        item->setToolTip(tr("上次扫描：%1\n状态：%2").arg(directory.lastScanTime.toString(Qt::ISODate), directory.status));
        item->setData(Qt::UserRole, directory.path);
    }
}

void MainWindow::refreshGroups(const QList<SimilarGroupData>& groups) {
    currentGroups_ = groups;
    resultsTreeWidget_->clear();

    qint64 reclaimableBytes = 0;
    int groupIndex = 1;
    for (const auto& group : groups) {
        if (group.members.size() < 2) {
            continue;
        }
        auto* groupItem = new QTreeWidgetItem(resultsTreeWidget_);
        groupItem->setText(0, tr("组 #%1 (%2张, %3, 平均相似度 %4%)")
                                  .arg(groupIndex++)
                                  .arg(group.members.size())
                                  .arg(group.exactGroup ? tr("完全相同") : tr("相似"))
                                  .arg(QString::number(group.averageSimilarity, 'f', 1)));
        groupItem->setText(1, QString::number(group.averageSimilarity, 'f', 1) + QStringLiteral("%"));
        groupItem->setText(2, readableSize(group.totalSize));
        groupItem->setText(3, group.exactGroup ? tr("相同") : tr("相似"));
        groupItem->setExpanded(true);

        bool keptRepresentative = false;
        for (const auto& member : group.members) {
            auto* child = new QTreeWidgetItem(groupItem);
            child->setText(0, member.image.fileName);
            child->setText(1, QString::number(member.similarity, 'f', 1) + QStringLiteral("%"));
            child->setText(2, readableSize(member.image.fileSize));
            child->setText(3, member.image.status);
            child->setText(4, member.image.filePath);
            child->setData(0, Qt::UserRole, member.image.filePath);
            child->setData(1, Qt::UserRole, member.image.width);
            child->setData(2, Qt::UserRole, member.image.height);
            if (!keptRepresentative && member.image.filePath == group.representative.filePath) {
                keptRepresentative = true;
            } else {
                reclaimableBytes += member.image.fileSize;
            }
        }
    }
    statusBar()->showMessage(tr("共 %1 组，可回收空间约 %2").arg(groups.size()).arg(readableSize(reclaimableBytes)));
}

void MainWindow::updateThresholdLabel(int value) {
    thresholdValueLabel_->setText(QStringLiteral("%1%").arg(value));
}

ScanOptions MainWindow::currentScanOptions() const {
    ScanOptions options;
    const QStringList rawExtensions = extensionsEdit_->text().split(',', Qt::SkipEmptyParts);
    for (const QString& extension : rawExtensions) {
        options.extensions.append(extension.trimmed().toLower());
    }
    options.minimumSizeBytes = static_cast<qint64>(minSizeSpinBox_->value()) * 1024;
    options.maximumSizeBytes = static_cast<qint64>(maxSizeSpinBox_->value()) * 1024;
    options.skipHiddenAndSystem = skipHiddenCheckBox_->isChecked();
    options.algorithm = algorithmComboBox_->currentText();
    options.similarityThreshold = thresholdSlider_->value();

    switch (depthComboBox_->currentIndex()) {
    case 0:
        options.recursive = false;
        options.maximumDepth = 0;
        break;
    case 1:
        options.recursive = true;
        options.maximumDepth = -1;
        break;
    case 2:
        options.recursive = true;
        options.maximumDepth = depthSpinBox_->value();
        break;
    }
    return options;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::addDirectoryPath(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
    databaseManager_.upsertDirectory(path, true);
    refreshDirectories();
}

void MainWindow::addDirectory() {
    const QString directory = QFileDialog::getExistingDirectory(this, tr("选择目录"));
    if (directory.isEmpty()) {
        return;
    }
    addDirectoryPath(directory);
}

void MainWindow::removeSelectedDirectory() {
    const auto items = directoryListWidget_->selectedItems();
    for (QListWidgetItem* item : items) {
        databaseManager_.removeDirectory(item->text());
    }
    refreshDirectories();
    refreshGroups(databaseManager_.loadGroups(algorithmComboBox_->currentText()));
}

void MainWindow::clearDirectories() {
    databaseManager_.clearDirectories();
    refreshDirectories();
    refreshGroups({});
}

void MainWindow::startScan() {
    QStringList directories;
    for (int row = 0; row < directoryListWidget_->count(); ++row) {
        directories.append(directoryListWidget_->item(row)->text());
    }
    if (directories.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先添加至少一个目录。"));
        return;
    }

    saveSettings();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const ScanOptions options = currentScanOptions();
    scannerService_.scan(directories, options);
    const auto groups = similarityService_.buildGroups(databaseManager_.loadActiveImages(options.algorithm), options.similarityThreshold);
    databaseManager_.saveGroups(groups);
    refreshDirectories();
    refreshGroups(groups);
    QApplication::restoreOverrideCursor();
}

QList<ImageRecord> MainWindow::selectedImages() const {
    QList<ImageRecord> images;
    const auto items = resultsTreeWidget_->selectedItems();
    for (QTreeWidgetItem* item : items) {
        if (!item->parent()) {
            continue;
        }
        const QString filePath = item->data(0, Qt::UserRole).toString();
        for (const auto& group : currentGroups_) {
            for (const auto& member : group.members) {
                if (member.image.filePath == filePath) {
                    images.append(member.image);
                }
            }
        }
    }
    return images;
}

void MainWindow::refreshPreview() {
    const auto images = selectedImages();
    if (images.isEmpty()) {
        previewLabel_->setText(tr("选择图片查看预览"));
        detailsEdit_->clear();
        return;
    }

    QPixmap pixmap(images.first().filePath);
    previewLabel_->setPixmap(pixmap.scaled(previewLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    detailsEdit_->setPlainText(tr("路径：%1\n大小：%2\n分辨率：%3 × %4\n修改时间：%5\nMD5：%6\nSHA-256：%7\n感知哈希：%8")
                                   .arg(images.first().filePath,
                                        readableSize(images.first().fileSize),
                                        QString::number(images.first().width),
                                        QString::number(images.first().height),
                                        images.first().modifiedDate.toString(Qt::ISODate),
                                        images.first().md5Hash,
                                        images.first().sha256Hash,
                                        images.first().perceptualHashHex));
}

void MainWindow::exportCsv() {
    if (currentGroups_.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("当前没有可导出的分组。"));
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(this, tr("导出 CSV"), QStringLiteral("similar_images.csv"), tr("CSV Files (*.csv)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件。"));
        return;
    }

    QTextStream stream(&file);
    stream << "group,type,similarity,file_name,file_path,file_size,width,height,modified_date\n";
    for (int index = 0; index < currentGroups_.size(); ++index) {
        const auto& group = currentGroups_[index];
        for (const auto& member : group.members) {
            stream << index + 1 << ",\""
                   << (group.exactGroup ? "exact" : "similar") << "\",\""
                   << QString::number(member.similarity, 'f', 1) << "\",\""
                   << member.image.fileName << "\",\""
                   << member.image.filePath << "\",\""
                   << member.image.fileSize << "\",\""
                   << member.image.width << "\",\""
                   << member.image.height << "\",\""
                   << member.image.modifiedDate.toString(Qt::ISODate) << "\"\n";
        }
    }
    appendLog(tr("已导出 CSV：%1").arg(filePath));
}

void MainWindow::exportHtml() {
    if (currentGroups_.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("当前没有可导出的分组。"));
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(this, tr("导出 HTML"), QStringLiteral("similar_images.html"), tr("HTML Files (*.html)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件。"));
        return;
    }

    QTextStream stream(&file);
    stream << "<html><head><meta charset='utf-8'><style>body{font-family:Segoe UI,sans-serif;} .group{margin-bottom:24px;} .item{display:flex;gap:12px;margin:8px 0;align-items:center;} img{max-width:160px;max-height:120px;border:1px solid #ccc;object-fit:contain;} </style></head><body>";
    stream << "<h1>图片相似度报告</h1>";
    for (int index = 0; index < currentGroups_.size(); ++index) {
        const auto& group = currentGroups_[index];
        stream << QStringLiteral("<div class='group'><h2>组 #%1 - %2 (%3, 平均相似度 %4%)</h2>")
                      .arg(index + 1)
                      .arg(group.members.size())
                      .arg(group.exactGroup ? tr("完全相同") : tr("相似"))
                      .arg(QString::number(group.averageSimilarity, 'f', 1));
        for (const auto& member : group.members) {
            stream << QStringLiteral("<div class='item'><img src='%1'><div><div><strong>%2</strong></div><div>路径：%3</div><div>相似度：%4%</div><div>大小：%5</div></div></div>")
                          .arg(QUrl::fromLocalFile(member.image.filePath).toString(),
                               member.image.fileName,
                               member.image.filePath,
                               QString::number(member.similarity, 'f', 1),
                               readableSize(member.image.fileSize));
        }
        stream << "</div>";
    }
    stream << "</body></html>";
    appendLog(tr("已导出 HTML：%1").arg(filePath));
}

void MainWindow::deleteSelectedImages() {
    const auto images = selectedImages();
    if (images.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请选择要删除的图片。"));
        return;
    }

    QStringList filePaths;
    for (const auto& image : images) {
        filePaths.append(image.filePath);
    }
    filePaths.removeDuplicates();

    DeleteConfirmationDialog dialog(filePaths, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    int deletedCount = 0;
    for (const QString& filePath : dialog.checkedFiles()) {
        QFile file(filePath);
        if (file.exists() && file.moveToTrash()) {
            ++deletedCount;
            appendLog(tr("已移入回收站：%1").arg(filePath));
        }
    }

    databaseManager_.markMissingRecords();
    const auto groups = similarityService_.buildGroups(databaseManager_.loadActiveImages(algorithmComboBox_->currentText()), thresholdSlider_->value());
    databaseManager_.saveGroups(groups);
    refreshGroups(groups);
    QMessageBox::information(this, tr("完成"), tr("已删除 %1 个文件。").arg(deletedCount));
}

void MainWindow::cleanupMissing() {
    databaseManager_.markMissingRecords();
    databaseManager_.cleanupMissingRecords();
    refreshDirectories();
    const auto groups = similarityService_.buildGroups(databaseManager_.loadActiveImages(algorithmComboBox_->currentText()), thresholdSlider_->value());
    databaseManager_.saveGroups(groups);
    refreshGroups(groups);
    appendLog(tr("已清理失效记录。"));
}

void MainWindow::openPreviewDialog() {
    const auto images = selectedImages();
    if (images.isEmpty()) {
        return;
    }
    if (!previewDialog_) {
        previewDialog_ = new PreviewDialog(this);
    }
    previewDialog_->setImage(images.first());
    previewDialog_->show();
    previewDialog_->raise();
    previewDialog_->activateWindow();
}

void MainWindow::openRecycleBin() {
#ifdef Q_OS_WIN
    QProcess::startDetached(QStringLiteral("explorer.exe"), {QStringLiteral("shell:RecycleBinFolder")});
#else
    QMessageBox::information(this, tr("提示"), tr("当前平台不支持直接打开回收站。"));
#endif
}

void MainWindow::revealFile(const QString& path) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}
