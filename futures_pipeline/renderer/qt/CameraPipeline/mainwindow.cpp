#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    camera_{nullptr},
    opened_{false},
    running_{false}
{
    qInfo() << QString(__func__) << "() entry" << endl;

    ui->setupUi(this);

    connect(ui->startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopButtonClicked);
    connect(ui->openCloseCameraButton, &QPushButton::clicked, this, &MainWindow::onOpenCloseCameraButtonClicked);

    qInfo() << QString(__func__) << "() exit" << endl;
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onOpenCloseCameraButtonClicked()
{
    qInfo() << metaObject()->className() << QString(__func__) + QString("()") << " entry" << endl;

    if (opened_) {
        opened_ = false;
        ui->startStopButton->setEnabled(false);
        ui->openCloseCameraButton->setText("open camera");
        qInfo() << "closing camera" << endl;

        /* release camera resources */
        camera_.reset();
    } else {
       opened_ = true;
       ui->startStopButton->setEnabled(true);
       ui->openCloseCameraButton->setText("close camera");
       qInfo() << "opening camera";
       camera_ = QSharedPointer<Camera>(new Camera());
    }

    qInfo() << QString(__func__) << "() exit" << endl;
}


void MainWindow::onStartStopButtonClicked()
{
     qInfo() << QString(__func__) << "() entry";

    if (opened_) {

        if (running_) {

           qInfo() << "stop streaming";
           camera_->Stop();
           running_ = false;

           ui->openCloseCameraButton->setEnabled(true);
           ui->startStopButton->setText("start stream");

        } else {

           qInfo() << "enter start streaming section";

           camera_->Start();
           running_ = true;
           ui->openCloseCameraButton->setEnabled(false);
           ui->startStopButton->setText("stop stream");

           /* puller thread */
           pullThread_ = new QThread;
           worker_ = new Worker(camera_);
           worker_->moveToThread(pullThread_);

           /* start thread worker when thread starts */
           connect(pullThread_, &QThread::started, worker_, &Worker::doWork);
           /* thread finishes will delete the thread */
           connect(pullThread_, &QThread::finished, pullThread_, &QThread::deleteLater);
           /* worker finishes will quit the thread it runs on */
           connect(worker_, &Worker::finished, pullThread_, &QThread::quit);
           /* worker finishes will delete the worker */
           connect(worker_, &Worker::finished, worker_, &Worker::deleteLater);
           /* worker frame ready will signal ui thread to render it */
           connect(worker_, &Worker::resultReady, this, &MainWindow::handleFrame);
           /* worker blank request to ui thread */
           connect(worker_, &Worker::blank, this, &MainWindow::blank);

           /* start qt frame puller thread */
           pullThread_->start();

           qInfo() << "exit start streaming section" << endl;
        }
    } else {
        qCritical() << "camera not opened" << endl;
    }

    qInfo() << QString(__func__) << "() exit" << endl;
}


Worker::Worker(QSharedPointer<Camera> camera)
{
    qInfo() << QString(__func__) << "() entry" << endl;
    camera_ = camera;
    qInfo() << QString(__func__) << "() exit" << endl;
}


Worker::~Worker()
{
    qInfo() << QString(__func__) << "() entry" << endl;
    qInfo() << QString(__func__) << "() exit" << endl;
}


void Worker::doWork()
{
    qInfo() << QString(__func__) << "() entry" << endl;

    bool run = true;
    while (run) {
        CameraFrame* frame;
        if (camera_->GetFrame(&frame)) {
            if (frame == nullptr) {
                qInfo() << QString(__func__) << "() **************************************" << endl;
                qInfo() << QString(__func__) << "() ******* received nullptr frame *******" << endl;
                qInfo() << QString(__func__) << "() **************************************" << endl;
                run = false;
            } else {
                emit resultReady(frame);
            }
        } else {
            qInfo() << QString(__func__) << "camera_->GetFrame(&frame) failure" << endl;
        }
    }

    /* signal frame access ifc we no longer require it */
    // TODO change to setting ifc to nullptr and letting ifc destructor generate a signal to thread exit sequence
    camera_->Release();

    qInfo() << QString(__func__) << "post emit finished()" << endl;
    emit finished();
    qInfo() << QString(__func__) << "post emit blank()" << endl;
    emit blank();

    qInfo() << QString(__func__) << "() exit" << endl;
}


void MainWindow::handleFrame(CameraFrame* frame)
{
   int _width = 640;
   int _height = 480;
   int COLOR_COMPONENTS = 3;

   /* create and render pixmap */
   QImage image(frame->data, _width, _height, _width*COLOR_COMPONENTS, QImage::Format_RGB888);
   QPixmap pixmap = QPixmap::fromImage(image);
   ui->previewLabel->setPixmap(pixmap);

   camera_->ReturnFrame(frame);
}


void MainWindow::blank()
{
   qInfo() << QString(__func__) << "() entry" << endl;

   int _width = 640;
   int _height = 480;
   int COLOR_COMPONENTS = 3;

   /* create and render pixmap */
   uint8_t blank[COLOR_COMPONENTS*_width*_height];
   memset(blank, 0xFF, sizeof(blank));
   QImage image(blank, _width, _height, _width*COLOR_COMPONENTS, QImage::Format_RGB888);
   QPixmap pixmap = QPixmap::fromImage(image);
   ui->previewLabel->setPixmap(pixmap);

   qInfo() << QString(__func__) << "() exit" << endl;
}
