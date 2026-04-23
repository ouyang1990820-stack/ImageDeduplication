#include "PreviewDialog.h"

#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QTextEdit>
#include <QVBoxLayout>

PreviewDialog::PreviewDialog(QWidget* parent)
    : QDialog(parent) {
    resize(900, 600);
    auto* layout = new QHBoxLayout(this);
    imageLabel_ = new QLabel(this);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setMinimumSize(480, 360);
    imageLabel_->setScaledContents(false);

    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);

    layout->addWidget(imageLabel_, 3);
    layout->addWidget(detailsEdit_, 2);
    setWindowTitle(tr("图片预览"));
}

void PreviewDialog::setImage(const ImageRecord& image) {
    QImageReader reader(image.filePath);
    reader.setAutoTransform(true);
    const QImage loaded = reader.read();
    if (!loaded.isNull()) {
        imageLabel_->setPixmap(QPixmap::fromImage(loaded).scaled(imageLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        imageLabel_->setText(tr("无法加载图片"));
    }

    detailsEdit_->setPlainText(tr("路径：%1\n文件名：%2\n大小：%3 KB\n分辨率：%4 × %5\n修改时间：%6\nMD5：%7\nSHA-256：%8\n感知哈希：%9\n算法：%10")
                                   .arg(image.filePath,
                                        image.fileName,
                                        QString::number(image.fileSize / 1024.0, 'f', 2),
                                        QString::number(image.width),
                                        QString::number(image.height),
                                        image.modifiedDate.toString(Qt::ISODate),
                                        image.md5Hash,
                                        image.sha256Hash,
                                        image.perceptualHashHex,
                                        image.hashAlgorithm));
}
