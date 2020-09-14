#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QThread>

#include "camera.h"

namespace Ui {
class MainWindow;
}

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker(QSharedPointer<Camera> camera);
    ~Worker();

private:
    QSharedPointer<Camera> camera_;

public slots:
    void doWork();

signals:
    void resultReady(uint8_t* frame);
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow* ui;

    QSharedPointer<Camera> camera_;
    bool opened_;
    bool running_;

    /* frame pull */
    Worker* worker_;
    QThread pullThread_;

public slots:
    void handleResults(uint8_t* frame);
signals:
    void operate();

private slots:
    void onStartButtonClicked();
    void onOpenButtonClicked();
};

#endif // MAINWINDOW_H
