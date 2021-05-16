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

protected:
//    void resizeEvent(QResizeEvent *event);

private:
    Ui::MainWindow* ui;
    bool full_screen_;

    QSharedPointer<Camera> camera_;
    bool opened_;
    bool running_;

    /* frame pull */
    Worker* worker_;
    QThread* pullThread_;

    rapidjson::Document camera_modes_doc_;
    bool camera_types_validated_;
    /* number of available cameras */
    size_t camera_dev_nodes_size_;
    /* number of available formats for selected camera */
    size_t camera_formats_size_;
    /* index and name of selected camera */
    int camera_dev_node_index_;
    QString camera_dev_node_;
    /* index and name of selected format */
    int camera_format_index_;
    QString camera_format_;
    /* index and name of selected resolution */
    int camera_format_resolution_index_;
    QString camera_format_resolution_;
    int width_;
    int height_;
    /* index and name of selected rate */
    int camera_format_resolution_rate_index_;
    QString camera_format_resolution_rate_;

    /* selected camera configuration */
    CameraConfig camera_config_;

    QStringList CamaraParameterSelectionList(QString selection_type);


public slots:
    /* frame processing */
    void handleFrame(CameraFrame* frame);
    /* ui frame blank */
    void blank();

private slots:
    void onStartStopButtonClicked();
    void onOpenCloseCameraButtonClicked();
    void onFullScreenButtonClicked();

    void onCameraComboBoxIndexChanged(int idx);
    void onFormatComboBoxIndexChanged(int idx);
    void onResolutionComboBoxIndexChanged(int idx);
    void onRateComboBoxIndexChanged(int idx);
};

#endif // MAINWINDOW_H
