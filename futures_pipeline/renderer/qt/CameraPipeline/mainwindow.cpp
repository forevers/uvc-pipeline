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
    qInfo() << QString(__func__) << "() entry";

    ui->setupUi(this);

    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::onOpenButtonClicked);

    qInfo() << QString(__func__) << "() exit";
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onOpenButtonClicked()
{
    // TODO qInfo thread safe?
    qInfo() << metaObject()->className() << QString(__func__) + QString("()") << " entry";

    if (opened_) {
        opened_ = false;
        ui->openButton->setText("open camera");
        qInfo() << "closing camera";

        /* release camera resources */
        camera_.reset();
    } else {
       opened_ = true;
       ui->openButton->setText("close camera");
       qInfo() << "opening camera";
       camera_ = QSharedPointer<Camera>(new Camera());
    }

    qInfo() << QString(__func__) << "() exit";
}


void MainWindow::onStartButtonClicked()
{
     qInfo() << QString(__func__) << "() entry";

    if (opened_) {

        if (running_) {

           camera_->Stop();
           running_ = false;

           ui->startButton->setText("start stream");

           pullThread_.quit();
           pullThread_.wait();

        } else {

           camera_->Start();
           running_ = true;
           ui->startButton->setText("stop stream");

           /* puller thread */
           worker_ = new Worker(camera_);
           worker_->moveToThread(&pullThread_);

           connect(&pullThread_, &QThread::finished, worker_, &QObject::deleteLater);

           connect(this, &MainWindow::operate, worker_, &Worker::doWork);
           /* frame ready signal */
           connect(worker_, &Worker::resultReady, this, &MainWindow::handleResults);

           /* start frame puller thread */
           pullThread_.start();
           emit operate();
        }
    } else {
        qCritical() << "camera not opened";
    }

    // TODO move test gen code to lowest c++ level
//    static unsigned char shift = 0;
//    int _width = 100;
//    int _height = 200;
//    int COLOR_COMPONENTS = 3;
//    unsigned char buffer[_width*_height*COLOR_COMPONENTS];

//    for (int row = 0; row < _height; row++) {
//        for (int col = 0; col < _width; col++) {
//            buffer[row*_width*COLOR_COMPONENTS + col*COLOR_COMPONENTS] = col+shift;
//            buffer[row*_width*COLOR_COMPONENTS + col*COLOR_COMPONENTS + 1] = col+shift;
//            buffer[row*_width*COLOR_COMPONENTS + col*COLOR_COMPONENTS + 2] = col+shift;
//        }
//    }
//    shift++;

//
//    QImage image(buffer, _width, _height, _width*COLOR_COMPONENTS, QImage::Format_RGB888);
//    QPixmap pixmap = QPixmap::fromImage(image);
//    ui->previewLabel->setPixmap(pixmap);

    qInfo() << QString(__func__) << "() exit";
}


Worker::Worker(QSharedPointer<Camera> camera)
{
    qInfo() << QString(__func__) << "() entry";
    camera_ = camera;
    qInfo() << QString(__func__) << "() exit";
}


Worker::~Worker()
{
    qInfo() << QString(__func__) << "() entry";
    qInfo() << QString(__func__) << "() exit";
}


void Worker::doWork()
{
    qInfo() << QString(__func__) << "() entry";

    bool run = true;
    while (run) {
        uint8_t* frame;
        qInfo() << QString(__func__) << "() pre GetFrame()";
        if (camera_->GetFrame(&frame)) {
            emit resultReady(frame);
            if (frame == nullptr) {
                qInfo() << QString(__func__) << "() received nullptr frame";
                run = false;
            }
        } else {
            qInfo() << QString(__func__) << "camera_->GetFrame(&frame) failure";
        }
    }

    qInfo() << QString(__func__) << "() exit";
}


void MainWindow::handleResults(uint8_t* frame)
{
   qInfo() << QString(__func__) << "() entry";

   int _width = 640;
   int _height = 480;
   int COLOR_COMPONENTS = 3;

   /* create and render pixmap */
   QImage image(frame, _width, _height, _width*COLOR_COMPONENTS, QImage::Format_RGB888);
   QPixmap pixmap = QPixmap::fromImage(image);
   ui->previewLabel->setPixmap(pixmap);

   qInfo() << QString(__func__) << "() exit";
}
