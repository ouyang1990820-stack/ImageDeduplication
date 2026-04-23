/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/MainWindow.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[26];
    char stringdata0[11];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[24];
    char stringdata4[17];
    char stringdata5[10];
    char stringdata6[15];
    char stringdata7[10];
    char stringdata8[11];
    char stringdata9[21];
    char stringdata10[15];
    char stringdata11[18];
    char stringdata12[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 12),  // "addDirectory"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 23),  // "removeSelectedDirectory"
        QT_MOC_LITERAL(49, 16),  // "clearDirectories"
        QT_MOC_LITERAL(66, 9),  // "startScan"
        QT_MOC_LITERAL(76, 14),  // "refreshPreview"
        QT_MOC_LITERAL(91, 9),  // "exportCsv"
        QT_MOC_LITERAL(101, 10),  // "exportHtml"
        QT_MOC_LITERAL(112, 20),  // "deleteSelectedImages"
        QT_MOC_LITERAL(133, 14),  // "cleanupMissing"
        QT_MOC_LITERAL(148, 17),  // "openPreviewDialog"
        QT_MOC_LITERAL(166, 14)   // "openRecycleBin"
    },
    "MainWindow",
    "addDirectory",
    "",
    "removeSelectedDirectory",
    "clearDirectories",
    "startScan",
    "refreshPreview",
    "exportCsv",
    "exportHtml",
    "deleteSelectedImages",
    "cleanupMissing",
    "openPreviewDialog",
    "openRecycleBin"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x08,    1 /* Private */,
       3,    0,   81,    2, 0x08,    2 /* Private */,
       4,    0,   82,    2, 0x08,    3 /* Private */,
       5,    0,   83,    2, 0x08,    4 /* Private */,
       6,    0,   84,    2, 0x08,    5 /* Private */,
       7,    0,   85,    2, 0x08,    6 /* Private */,
       8,    0,   86,    2, 0x08,    7 /* Private */,
       9,    0,   87,    2, 0x08,    8 /* Private */,
      10,    0,   88,    2, 0x08,    9 /* Private */,
      11,    0,   89,    2, 0x08,   10 /* Private */,
      12,    0,   90,    2, 0x08,   11 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'addDirectory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeSelectedDirectory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearDirectories'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startScan'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshPreview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportCsv'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportHtml'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deleteSelectedImages'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cleanupMissing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openPreviewDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openRecycleBin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->addDirectory(); break;
        case 1: _t->removeSelectedDirectory(); break;
        case 2: _t->clearDirectories(); break;
        case 3: _t->startScan(); break;
        case 4: _t->refreshPreview(); break;
        case 5: _t->exportCsv(); break;
        case 6: _t->exportHtml(); break;
        case 7: _t->deleteSelectedImages(); break;
        case 8: _t->cleanupMissing(); break;
        case 9: _t->openPreviewDialog(); break;
        case 10: _t->openRecycleBin(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
