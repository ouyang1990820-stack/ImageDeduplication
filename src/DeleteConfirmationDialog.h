#pragma once

#include <QDialog>

class QListWidget;

class DeleteConfirmationDialog : public QDialog {
    Q_OBJECT
public:
    explicit DeleteConfirmationDialog(const QStringList& filePaths, QWidget* parent = nullptr);
    QStringList checkedFiles() const;

private:
    QListWidget* listWidget_ = nullptr;
};
