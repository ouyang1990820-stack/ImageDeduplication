#pragma once

#include "Models.h"

#include <QDialog>

class QLabel;
class QTextEdit;

class PreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreviewDialog(QWidget* parent = nullptr);
    void setImage(const ImageRecord& image);

private:
    QLabel* imageLabel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};
