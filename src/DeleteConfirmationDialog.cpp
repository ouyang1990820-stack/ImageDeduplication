#include "DeleteConfirmationDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

DeleteConfirmationDialog::DeleteConfirmationDialog(const QStringList& filePaths, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("确认删除到回收站"));
    resize(640, 420);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("请确认要删除的文件，取消勾选可跳过。"), this));

    listWidget_ = new QListWidget(this);
    for (const QString& filePath : filePaths) {
        auto* item = new QListWidgetItem(filePath, listWidget_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
    layout->addWidget(listWidget_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QStringList DeleteConfirmationDialog::checkedFiles() const {
    QStringList files;
    for (int row = 0; row < listWidget_->count(); ++row) {
        auto* item = listWidget_->item(row);
        if (item->checkState() == Qt::Checked) {
            files.append(item->text());
        }
    }
    return files;
}
