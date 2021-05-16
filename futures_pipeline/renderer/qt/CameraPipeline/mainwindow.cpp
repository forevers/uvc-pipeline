#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    full_screen_{true},
    camera_{nullptr},
    opened_{false},
    running_{false},
    camera_types_validated_{false},
    camera_dev_nodes_size_{0},
    camera_dev_node_index_{0},
    camera_dev_node_{"none"},
    camera_formats_size_{0},
    camera_format_index_{0},
    camera_format_{"none"},
    camera_format_resolution_index_{0},
    camera_format_resolution_{"none"},
    width_{-1},
    height_{-1},
    camera_format_resolution_rate_index_{0},
    camera_format_resolution_rate_{"none"},
    camera_config_{"none", "none", -1, -1, -1, -1, -1, -1, {0, 0}}
{
    qInfo() << QString(__func__) << "() entry" << endl;

    if (full_screen_) this->showFullScreen();

    ui->setupUi(this);
    
    ui->startStopButton->setVisible(false);
    ui->cameraComboBox->setVisible(false);
    ui->formatComboBox->setVisible(false);
    ui->resolutionComboBox->setVisible(false);
    ui->rateComboBox->setVisible(false);

    connect(ui->startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStopButtonClicked);
    connect(ui->openCloseCameraButton, &QPushButton::clicked, this, &MainWindow::onOpenCloseCameraButtonClicked);
    connect(ui->fullScreenButton, &QPushButton::clicked, this, &MainWindow::onFullScreenButtonClicked);


    /* camera parameters */
    connect(ui->cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){ onCameraComboBoxIndexChanged(idx);});
    connect(ui->formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){ onFormatComboBoxIndexChanged(idx);});
    connect(ui->resolutionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){ onResolutionComboBoxIndexChanged(idx);});
    connect(ui->rateComboBox,  QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx){ onRateComboBoxIndexChanged(idx);});

    qInfo() << QString(__func__) << "() exit" << endl;
}


MainWindow::~MainWindow()
{
    delete ui;
}


QStringList MainWindow::CamaraParameterSelectionList(QString selection_type)
{
    QStringList camera_selection_list;

    if (selection_type == "cameras") {
        for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
            camera_selection_list.append(camera["device node"].GetString());
        }
    } else if (selection_type == "formats") {
        for (auto& format : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"].GetArray()) {
            camera_selection_list.append(format["name"].GetString());
        }
    } else if (selection_type == "res rates") {
        for (auto& resolution : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"].GetArray()) {
            camera_selection_list.append(resolution["resolution"].GetString());
        }
    } else if (selection_type == "rates") {
        for (auto& rate : camera_modes_doc_["cameras"][camera_dev_node_index_]["formats"][camera_format_index_]["res rates"][camera_format_resolution_index_]["rates"].GetArray()) {
            camera_selection_list.append(rate.GetString());
        }
    }

    return camera_selection_list;
}


void MainWindow::onOpenCloseCameraButtonClicked()
{
    qInfo() << metaObject()->className() << QString(__func__) + QString("()") << " entry" << endl;

    if (opened_) {
        opened_ = false;

        /* depopulate and hide camera configuration and control */
        ui->startStopButton->setVisible(false);
        ui->cameraComboBox->clear();
        ui->cameraComboBox->setVisible(false);
        ui->formatComboBox->clear();
        ui->formatComboBox->setVisible(false);
        ui->resolutionComboBox->clear();
        ui->resolutionComboBox->setVisible(false);
        ui->rateComboBox->clear();
        ui->rateComboBox->setVisible(false);
        ui->startStopButton->setEnabled(false);
        ui->openCloseCameraButton->setText("open camera");
        qInfo() << "closing camera" << endl;

        camera_modes_doc_.Parse("{}");

        /* release camera resources */
        camera_.reset();
    } else {
       opened_ = true;

       /* render camera configuration and control */
       ui->startStopButton->setVisible(true);
       ui->cameraComboBox->setVisible(true);
       ui->formatComboBox->setVisible(true);
       ui->resolutionComboBox->setVisible(true);
       ui->rateComboBox->setVisible(true);

       ui->startStopButton->setEnabled(true);
       ui->openCloseCameraButton->setText("close camera");
       qInfo() << "opening camera";
       camera_ = QSharedPointer<Camera>(new Camera());

       // TODO modal dialog?

       QStringList cameraStringList;

       /* present camera capabilities */
       /* validate schema */
       // TODO client side api construct camera_modes object for iteration and GUI render
       {
           rapidjson::Document document;
           document.Parse(g_schema);

           rapidjson::SchemaDocument schemaDocument(document);
           rapidjson::SchemaValidator validator(schemaDocument);

           /* parse JSON string */
           string camera_modes = camera_->GetSupportedVideoModes();
           camera_modes_doc_.Parse(camera_modes.c_str());

           if (camera_modes_doc_.Accept(validator)) {
               camera_types_validated_ = true;
               cout<<"schema validated"<<endl;
               size_t align = 0;
               camera_dev_nodes_size_ = camera_modes_doc_["cameras"].Size();
               cout<<string(align, ' ')<<"num cameras: "<<camera_dev_nodes_size_<<endl;
               for (auto& camera : camera_modes_doc_["cameras"].GetArray()) {
                   align = 2;
                   cout<<string(align, ' ')<<camera["device node"].GetString()<<endl;
                   cameraStringList.append(camera["device node"].GetString());
                   camera_formats_size_ = camera["formats"].Size();
                   cout<<string(align, ' ')<<"num formats: "<<camera_formats_size_<<endl;
                   for (auto& format : camera["formats"].GetArray()) {
                       align += 4;
                       cout<<string(align, ' ')<<format["index"].GetInt()<<endl;
                       cout<<string(align, ' ')<<format["type"].GetString()<<endl;
                       cout<<string(align, ' ')<<format["format"].GetString()<<endl;
                       cout<<string(align, ' ')<<format["name"].GetString()<<endl;
                       cout<<string(align, ' ')<<"num res rates: "<<format["res rates"].Size()<<endl;
                       for (auto& res_rate : format["res rates"].GetArray()) {
                           align = 6;
                           cout<<string(align, ' ')<<res_rate["resolution"].GetString()<<endl;
                           cout<<string(align, ' ')<<"num rates: "<<res_rate["rates"].Size()<<endl;
                           for (auto& rate : res_rate["rates"].GetArray()) {
                               align = 8;
                               cout<<string(align, ' ')<<rate.GetString()<<endl;
                           }
                       }
                   }
               }
           } else {
               cout<<"schema invalidated"<<endl;
           }
        }

        /* present inital camera configuration options */
        cameraStringList = CamaraParameterSelectionList("cameras");
        ui->cameraComboBox->insertItems(0, cameraStringList);
        ui->cameraComboBox->setCurrentIndex(0);
    }

    qInfo() << QString(__func__) << "() exit" << endl;
}


void MainWindow::onFullScreenButtonClicked()
{
    if (full_screen_) {
        this->showNormal();
        full_screen_ = false;
    } else {
        this->showFullScreen();
        full_screen_ = true;
    }
}


void MainWindow::onCameraComboBoxIndexChanged(int idx)
{
    qInfo() << QString(__func__) << "() entry";

    /* update selected camera */
    if (-1 != (camera_dev_node_index_ = idx)) {
        camera_dev_node_ = ui->cameraComboBox->currentText();
        camera_config_.camera_dev_node_ = camera_dev_node_.toStdString();
        qInfo()<<"camera["<<camera_dev_node_index_<<"]: "<<camera_dev_node_<<" selected"<<endl;

        /* reset formats */
        ui->formatComboBox->clear();
        QStringList formats = CamaraParameterSelectionList("formats");
        ui->formatComboBox->insertItems(0, formats);
        ui->formatComboBox->setCurrentIndex(0);
    } else {
        /* clear all sub formats */
        ui->formatComboBox->clear();
    }

    qInfo() << QString(__func__) << "() exit";
}


void MainWindow::onFormatComboBoxIndexChanged(int idx)
{
    qInfo() << QString(__func__) << "() entry";

    if (-1 != (camera_format_index_ = idx)) {
        camera_format_ = ui->formatComboBox->currentText();
        qInfo()<<"format["<<camera_format_index_<<"]: "<<camera_format_<<" selected"<<endl;
        camera_config_.format_ = camera_format_.toStdString();

        /* update dependent formats */
        ui->resolutionComboBox->clear();
        QStringList resolutions = CamaraParameterSelectionList("res rates");
        cout<<"num resolutions: "<<resolutions.size()<<endl;
        ui->resolutionComboBox->insertItems(0, resolutions);
        ui->resolutionComboBox->setCurrentIndex(0);
    } else {
        /* clear all sub formats */
        ui->resolutionComboBox->clear();
    }

    qInfo() << QString(__func__) << "() exit";
}


void MainWindow::onResolutionComboBoxIndexChanged(int idx)
{
    qInfo() << QString(__func__) << "() entry";

    if (-1 != (camera_format_resolution_index_ = idx)) {

        camera_format_resolution_ = ui->resolutionComboBox->currentText();
        qInfo()<<"resolution["<<camera_format_resolution_index_<<"]: "<<camera_format_resolution_<<" selected"<<endl;

        /* extract w/h from resolution string */
        int start = 0;
        string delim = "x";
        string resolution = camera_format_resolution_.toStdString();
        int end = resolution.find(delim);
        width_ = stoi(resolution.substr(start, end - start));
        start = end + delim.size();
        end = resolution.find(delim, start);
        height_ = stoi(resolution.substr(start, end - start));
        camera_config_.width_enumerated_ = width_;
        camera_config_.width_actual_ = width_;
        camera_config_.height_ = height_;

        /* update dependent formats */
        ui->rateComboBox->clear();
        QStringList rates = CamaraParameterSelectionList("rates");
        ui->rateComboBox->insertItems(0, rates);
        ui->rateComboBox->setCurrentIndex(0);
    } else {
        /* clear all sub formats */
        ui->rateComboBox->clear();
    }

    qInfo() << QString(__func__) << "() exit";
}


void MainWindow::onRateComboBoxIndexChanged(int idx)
{
    if (-1 != (camera_format_resolution_rate_index_ = idx)) {
        camera_format_resolution_rate_ = ui->rateComboBox->currentText();
        qInfo()<<"rate["<<camera_format_resolution_rate_index_<<"]: "<<camera_format_resolution_rate_<<" selected"<<endl;
        // TODO format error handling
        camera_config_.time_per_frame_.denominator = camera_format_resolution_rate_.left(camera_format_resolution_rate_.size() - 4).toFloat();
        camera_config_.time_per_frame_.numerator = 1;
    }

    qInfo() << QString(__func__) << "() exit";
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

           cout<<"width: "<<width_<<"height: "<<height_<<endl;
           camera_->Start(camera_config_);

           running_ = true;
           ui->openCloseCameraButton->setEnabled(false);
           ui->startStopButton->setText("stop stream");

           /* puller thread */
           pullThread_ = new QThread;
           worker_ = new Worker(camera_);
           worker_->moveToThread(pullThread_);

           /* start thread worker when thread starts */
           connect(pullThread_, &QThread::started, worker_, &Worker::doWork);           /* thread finishes will delete the thread */
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
   int COLOR_COMPONENTS = 3;

   /* create and render pixmap */
   QImage image(frame->data, width_, height_, width_*COLOR_COMPONENTS, QImage::Format_RGB888);

   QPixmap pixmap = QPixmap::fromImage(image);

//   QRect geometry = ui->previewLabel->geometry();
//   geometry.setWidth(width_);
//   geometry.setHeight(height_);
//   ui->previewLabel->setGeometry(geometry);
//   ui->previewLabel->setPixmap(pixmap);
//   ui->previewLabel->setPixmap(pixmap.scaled(width_, height_, Qt::KeepAspectRatio));
   ui->previewLabel->setPixmap(pixmap.scaled(ui->previewLabel->width(), ui->previewLabel->height(), Qt::KeepAspectRatio));

   camera_->ReturnFrame(frame);
}


void MainWindow::blank()
{
   qInfo() << QString(__func__) << "() entry" << endl;

   int COLOR_COMPONENTS = 3;

   /* create and render pixmap */
   uint8_t blank[COLOR_COMPONENTS*width_*height_];
   memset(blank, 0x00, sizeof(blank));
   QImage image(blank, width_, height_, width_*COLOR_COMPONENTS, QImage::Format_RGB888);
   QPixmap pixmap = QPixmap::fromImage(image);
   ui->previewLabel->setPixmap(pixmap);

   qInfo() << QString(__func__) << "() exit" << endl;
}
