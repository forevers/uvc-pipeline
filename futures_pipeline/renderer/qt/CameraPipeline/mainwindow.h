#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QThread>

#include "camera.h"

namespace Ui {
class MainWindow;
}

/* worker object for frame processing */
class Worker : public QObject
{
    Q_OBJECT

public:
    Worker(QSharedPointer<Camera> camera);
    ~Worker();

private:
    QSharedPointer<Camera> camera_;

public slots:
    /* frame processing */
    void doWork();

signals:
    /* frame ready for ui rendering */
    void resultReady(CameraFrame* frame);
    /* thread termination and self deletion */
    void finished(void);
    /* blank ui */
    void blank();
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
    QThread* pullThread_;

public slots:
    /* frame processing */
    void handleFrame(CameraFrame* frame);
    /* ui frame blank */
    void blank();

private slots:
    void onStartStopButtonClicked();
    void onOpenCloseCameraButtonClicked();
};

#endif // MAINWINDOW_H
