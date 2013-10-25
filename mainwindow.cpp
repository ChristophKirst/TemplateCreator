/*****************************************************************************************************************

    TemplateCreator: create stimulus templates for HEKA Patchmaster

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QtGui>
#include <QSettings>
#include <QMessageBox>
#include <QTime>

#include "qcustomplot.h"

//#include <gsl/gsl_errno.h>
//#include <gsl/gsl_fft_complex.h>
//#include <gsl/gsl_fft_real.h>
//#include <gsl/gsl_fft_halfcomplex.h>
//#include <fftw3.h>

#include "debug.h"
#include "heka.h"
#include "numerics.h"
#include "fft.h"



/*****************************************************************************************************************
 *
 *      Parameter Handling
 *
 *****************************************************************************************************************/

Parameter::Parameter() {}
Parameter::Parameter(const Parameter& p) : name(p.name), value(p.value), default_value(p.default_value), type(p.type), widget(p.widget) {}
Parameter::Parameter(const QString& n, const QVariant& v, const QVariant& d, const Type& t, QWidget* w)
        : name(n), value(v), default_value(d), type(t), widget(w) {}
Parameter::~Parameter() {}

bool Parameter::operator== (const Parameter& p) const {
        if (name != p.name) return false;
        if (type != p.type) return false;
        if (value != p.value) return false;
        //if (*widget != *p.widget) return false;
        return true;
    }

bool Parameter::operator!= (const Parameter& p) const {
        return !(operator==(p));
    }

void Parameter::to_widget() {
        DEBUG("to_widget") DEBUG(name.toStdString())
        if (widget == NULL) return;
        QVector<QRadioButton*> * rdbs;
        int p;

        switch (type) {
            case Bool:
                dynamic_cast<QCheckBox*>(widget)->setChecked(value.toBool());
                break;

            case Integer:
                dynamic_cast<QSpinBox*>(widget)->setValue(value.toInt());
                break;

            case Double:
                dynamic_cast<QDoubleSpinBox*>(widget)->setValue(value.toDouble());
                break;

            case String:
                dynamic_cast<QLineEdit*>(widget)->setText(value.toString());
                break;

            case Text:
                dynamic_cast<QTextEdit*>(widget)->setPlainText(value.toString());
                break;

            case Set:
                rdbs = (QVector<QRadioButton*>*)(widget);
                for (int i = 0; i < rdbs->size(); i++) { (*rdbs)[i]->setChecked(false); }
                p = value.toInt();
                if (p>= 0 && p < rdbs->size()) (*rdbs)[p]->setChecked(true);
                break;

            default:
                QMessageBox msgBox;
                msgBox.setText("This should never happen! Internal Bug !");
                msgBox.setWindowTitle("Error");
                msgBox.exec();
                DEBUG("error to_widget")
                DEBUG(type)
                break;
        }
    }

void Parameter::from_widget() {
                DEBUG("from_widget") DEBUG(name.toStdString())
        if (widget == NULL) return;

        QVector<QRadioButton*> * rdbs;
        switch (type) {
            case Bool:
                value = ((QCheckBox*) widget)->isChecked();
                break;
            case Integer:
                value = ((QSpinBox*) widget)->value();
                break;
            case Double:
                value = ((QDoubleSpinBox*) widget)->value();
                break;
            case String:
                value = ((QLineEdit*) widget)->text();
                break;
            case Text:
                value = ((QTextEdit*) widget)->toPlainText();
                break;
            case Set:
                rdbs = (QVector<QRadioButton*>*)(widget);
                value = -1;
                for (int i = 0; i < rdbs->size(); i++) {
                    if ((*rdbs)[i]->isChecked()) {
                        value = i;
                        break;
                    }
                }
                break;

            default:
                QMessageBox msgBox;
                msgBox.setText("This should never happen! Internal Bug !");
                msgBox.setWindowTitle("Error");
                msgBox.exec();
                DEBUG("error from_widget")
                DEBUG(type)
                break;
        }
    }

void Parameter::from_string(const QString& str, bool* ok) {
                DEBUG("from_String") DEBUG(name.toStdString())
        if (widget == 0) {*ok = false; return;};

        switch (type) {
            case Bool:
                value = (0 != str.toDouble(ok));
                break;
            case Integer:
                value = str.toInt(ok);
                break;
            case Double:
                value = str.toDouble(ok);
                break;
            case String:
                value = str; *ok = true;
                break;
            case Text:
                value = str; *ok = true;
                break;
            case Set:
                value = str.toInt(ok);
                break;

            default:
                QMessageBox msgBox;
                msgBox.setText("This should never happen! Internal Bug !");
                msgBox.setWindowTitle("Error");
                msgBox.exec();
                DEBUG("error from_string")
                DEBUG(type)
                break;
        }
    }

QString Parameter::toString() {
        DEBUG("to_String") DEBUG(name.toStdString())
        QString str;

        switch (type) {
            case Bool:
                str = QString("%1").arg(value.toBool());
                break;
            case Integer:
            DEBUG("to_String")
                str = QString("%1").arg(value.toInt());
                break;
            case Double:
                str = QString("%1").arg(value.toDouble());
                break;
            case String:
                str = QString("\"%1\"").arg(value.toString());
                break;
            case Text:
                str = QString("\"%1\"").arg(value.toString());
                break;
            case Set:
                str = QString("%1").arg(value.toInt());;
                break;

            default:
                QMessageBox msgBox;
                msgBox.setText("This should never happen! Internal Bug !");
                msgBox.setWindowTitle("Error");
                msgBox.exec();
                DEBUG("error from_string")
                DEBUG(type)
                break;
        }

        return str;
        //return QString("%1=%2").arg(name).arg(str);
    }

void Parameter::write_settings(QSettings& settings) {
        settings.setValue(name, value);
    }

void Parameter::read_settings(QSettings& settings)  {
        value = settings.value(name, default_value);
    }



// ParameterSet

ParameterSet::ParameterSet() {}
ParameterSet::ParameterSet(const ParameterSet& p) : name(p.name), parameter(p.parameter) {}
ParameterSet::~ParameterSet() {}

bool ParameterSet::operator== (const ParameterSet& p) const {
        if (name != p.name) return false;
        if (parameter != p.parameter) return false;
        return true;
    }

bool ParameterSet::operator!= (const ParameterSet& p) const {
        return !(operator==(p));
    }

void ParameterSet::to_widgets() {
        for ( std::vector<Parameter>::iterator it = parameter.begin(); it < parameter.end(); it++){
            it->to_widget();
        }
    }

void ParameterSet::from_widgets() {
        for ( std::vector<Parameter>::iterator it = parameter.begin(); it < parameter.end(); it++){
            it->from_widget();
        }
    }

QString ParameterSet::toString() {
        QString str(name);
        for (int i = 0; i < int(parameter.size()); i++) {
            str = QString("%1,%2").arg(str).arg(parameter[i].toString());
        }
        return str;
    }

void ParameterSet::write_settings(QSettings& settings) {
        settings.beginGroup(name);
        for ( std::vector<Parameter>::iterator it = parameter.begin(); it < parameter.end(); it++){
            it->write_settings(settings);
        }
        settings.endGroup();
    }

void ParameterSet::read_settings(QSettings& settings) {
        settings.beginGroup(name);
        for ( std::vector<Parameter>::iterator it = parameter.begin(); it < parameter.end(); it++){
            it->read_settings(settings);
        }
        settings.endGroup();
    }


Parameter& ParameterSet::operator[] (const char* name) {
        return operator[] (QString("%1").arg(name));
    }

Parameter& ParameterSet::operator[] (const QString& name)
    {
        for (uint i = 0; i < parameter.size(); i++){
            if (parameter[i].name == name) return parameter[i];
        }

        QMessageBox msgBox;
        msgBox.setText("This should never happen! Internal Bug !");
        msgBox.setWindowTitle("Error");
        msgBox.exec();

        DEBUG("error operator[]")
        DEBUG(name.toStdString())

        return parameter[0];
    }




void MainWindow::setupParameter() {

    //setup parameters
    ParameterSet p;
    p.name = "ZapParameter";
    p.parameter.push_back(Parameter("f0", 0.0, 0.0, Parameter::Double, ui->f0_SpinBox));
    p.parameter.push_back(Parameter("f1", 50.0, 50.0, Parameter::Double, ui->f1_SpinBox));
    p.parameter.push_back(Parameter("amp", 1.0, 1.0, Parameter::Double,ui->amp_SpinBox));
    p.parameter.push_back(Parameter("reverse", false, false, Parameter::Bool, (QWidget*) ui->reverseCheckBox));

    p.parameter.push_back(Parameter("dur", 30.0, 30.0, Parameter::Double, ui->dur_SpinBox));
    p.parameter.push_back(Parameter("sample", 20.0, 20.0, Parameter::Double, ui->sample_SpinBox));
    p.parameter.push_back(Parameter("off", 0.0, 0.0, Parameter::Double, ui->off_SpinBox));
    p.parameter.push_back(Parameter("left", 0.0, 0.0, Parameter::Double, ui->left_SpinBox));
    p.parameter.push_back(Parameter("right", 0.0, 0.0, Parameter::Double, ui->right_SpinBox));
    QVector<QRadioButton*> * rdbuttons = new QVector<QRadioButton*>();
    rdbuttons->push_back(ui->zap_lin_radioButton);
    rdbuttons->push_back(ui->zap_sqr_radioButton);
    rdbuttons->push_back(ui->zap_exp_radioButton);
    p.parameter.push_back(Parameter("type", 0, 0, Parameter::Set, (QWidget*) rdbuttons));
    p.parameter.push_back(Parameter("file", "C:\\zap.tpl", "C:\\zap.tpl", Parameter::String,  ui->filename_Edit));

    parameter.push_back(p);
    p.parameter.clear();


    p.name = "LFPNoiseParameter";
    p.parameter.push_back(Parameter("f", 0.0, 0.0, Parameter::Double, ui->omega_SpinBox_2));
    p.parameter.push_back(Parameter("phase", 0.0, 0.0, Parameter::Double, ui->phase_SpinBox_2));
    p.parameter.push_back(Parameter("amp", 0.0, 0.0 ,Parameter::Double, ui->amp_SpinBox_2));
    p.parameter.push_back(Parameter("f0", 0.0, 0.0, Parameter::Double, ui->f0_SpinBox_2));
    p.parameter.push_back(Parameter("f1", 0.0, 0.0, Parameter::Double, ui->f1_SpinBox_2));
    p.parameter.push_back(Parameter("sigma", 1.0, 1.0, Parameter::Double, ui->sigma_SpinBox_2));
    p.parameter.push_back(Parameter("seed", 0, 0, Parameter::Double, ui->seed_SpinBox_2));

    p.parameter.push_back(Parameter("dur", 10.0, 10.0, Parameter::Double, ui->dur_SpinBox));
    p.parameter.push_back(Parameter("sample", 20.0, 20.0, Parameter::Double, ui->sample_SpinBox));
    p.parameter.push_back(Parameter("off", 0.0, 0.0, Parameter::Double, ui->off_SpinBox));
    p.parameter.push_back(Parameter("left", 0.0, 0.0, Parameter::Double, ui->left_SpinBox));
    p.parameter.push_back(Parameter("right", 0.0, 0.0, Parameter::Double, ui->right_SpinBox));
    p.parameter.push_back(Parameter("file", "C:\\lfp.tpl", "C:\\lpf.tpl", Parameter::String, ui->filename_Edit));

    parameter.push_back(p);
    p.parameter.clear();


    p.name = "SinParameter";
    p.parameter.push_back(Parameter("f", 0.0, 0.0, Parameter::Double, ui->sin_f_SpinBox));
    p.parameter.push_back(Parameter("phase", 0.0, 0.0, Parameter::Double, ui->sin_phase_SpinBox));
    p.parameter.push_back(Parameter("amp", 1.0, 1.0, Parameter::Double,ui->sin_amp_SpinBox));
    p.parameter.push_back(Parameter("peaks", false, false, Parameter::Bool,ui->sin_peaks_checkBox));
    p.parameter.push_back(Parameter("npeaks", 3.0, 3.0, Parameter::Double,ui->sin_peaks_doubleSpinBox));

    p.parameter.push_back(Parameter("f2", 0.0, 0.0, Parameter::Double, ui->sin_f_SpinBox_2));
    p.parameter.push_back(Parameter("phase2", 0.0, 0.0, Parameter::Double, ui->sin_phase_SpinBox_2));
    p.parameter.push_back(Parameter("amp2", 1.0, 1.0, Parameter::Double,ui->sin_amp_SpinBox_2));
    p.parameter.push_back(Parameter("peaks2", false, false, Parameter::Bool,ui->sin_peaks_checkBox_2));
    p.parameter.push_back(Parameter("npeaks2", 3.0, 3.0, Parameter::Double,ui->sin_peaks_doubleSpinBox_2));

    p.parameter.push_back(Parameter("positive", false, false, Parameter::Bool,ui->sin_positive_checkBox));

    p.parameter.push_back(Parameter("dur", 30.0, 30.0, Parameter::Double, ui->dur_SpinBox));
    p.parameter.push_back(Parameter("sample", 20.0, 20.0, Parameter::Double, ui->sample_SpinBox));
    p.parameter.push_back(Parameter("off", 0.0, 0.0, Parameter::Double, ui->off_SpinBox));
    p.parameter.push_back(Parameter("left", 0.0, 0.0, Parameter::Double, ui->left_SpinBox));
    p.parameter.push_back(Parameter("right", 0.0, 0.0, Parameter::Double, ui->right_SpinBox));
    p.parameter.push_back(Parameter("file", "C:\\zap.tpl", "C:\\zap.tpl", Parameter::String,  ui->filename_Edit));

    parameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Manual";
    p.parameter.push_back(Parameter("in", "", "", Parameter::Text, ui->textIn));
    p.parameter.push_back(Parameter("out", "", "", Parameter::Text, ui->textOut));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Run";
    p.parameter.push_back(Parameter("sequence", "run", "run", Parameter::String, ui->runSequence_lineEdit));
    p.parameter.push_back(Parameter("repeat", 1, 1, Parameter::Integer, ui->runRepeat_spinBox));
    p.parameter.push_back(Parameter("plot", true, true, Parameter::Bool, ui->runPlot_checkBox));
    p.parameter.push_back(Parameter("off", 10.0, 10.0, Parameter::Double, ui->runOff_doubleSpinBox));
    p.parameter.push_back(Parameter("time", 30.0, 30.0, Parameter::Double, ui->runTime_doubleSpinBox));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Zap";
    p.parameter.push_back(Parameter("sequence", "zap", "zap", Parameter::String, ui->zapSequence_lineEdit));
    p.parameter.push_back(Parameter("repeat", 1, 1, Parameter::Integer, ui->zapRepeat_spinBox));
    p.parameter.push_back(Parameter("plot", true, true, Parameter::Bool, ui->zapPlot_checkBox));
    p.parameter.push_back(Parameter("update", true, true, Parameter::Bool, ui->zapUpdate_checkBox));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Resonance";
    p.parameter.push_back(Parameter("sequence", "zap", "zap", Parameter::String, ui->resonanceSequence_lineEdit));
    p.parameter.push_back(Parameter("template", "zap", "zap", Parameter::String, ui->resonanceTemplate_lineEdit));
    rdbuttons = new QVector<QRadioButton*>();
    rdbuttons->push_back(ui->resonanceLoad_radioButton);
    rdbuttons->push_back(ui->resonanceZap_radioButton);
    p.parameter.push_back(Parameter("load", 0, 0, Parameter::Set, (QWidget*) rdbuttons));
    p.parameter.push_back(Parameter("plot", true, true, Parameter::Bool, ui->resonancePlot_checkBox));
    p.parameter.push_back(Parameter("dur", 30, 30, Parameter::Double, ui->resonanceDur_doubleSpinBox));
    p.parameter.push_back(Parameter("fmax", 50.0, 50.0, Parameter::Double, ui->resonanceFmax_doubleSpinBox));
    p.parameter.push_back(Parameter("peak", 1.0, 1.0, Parameter::Double, ui->resonancePeak_doubleSpinBox));
    p.parameter.push_back(Parameter("smooth", 1, 1, Parameter::Integer, ui->resonanceSmooth_spinBox));
    p.parameter.push_back(Parameter("zero", 0.0, 0.0, Parameter::Double, ui->resonanceZero_doubleSpinBox));
    p.parameter.push_back(Parameter("update", true, true, Parameter::Bool, ui->resonanceUpdate_checkBox));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA LFPNoise";
    p.parameter.push_back(Parameter("sequence", "noise", "noise", Parameter::String, ui->noiseSequence_lineEdit));
    p.parameter.push_back(Parameter("repeat", 1, 1, Parameter::Integer, ui->noiseRepeat_spinBox));
    p.parameter.push_back(Parameter("plot", true, true, Parameter::Bool, ui->noisePlot_checkBox));
    rdbuttons = new QVector<QRadioButton*>();
    rdbuttons->push_back(ui->noiseFrozen_radioButton);
    rdbuttons->push_back(ui->noiseRandom_radioButton);
    rdbuttons->push_back(ui->noiseSeedRandom_radioButton);
    p.parameter.push_back(Parameter("type", 0, 0, Parameter::Set, (QWidget*) rdbuttons));
    p.parameter.push_back(Parameter("seed", 0, 0, Parameter::Integer, ui->noiseSeed_spinBox));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA All";
    p.parameter.push_back(Parameter("runZap", true, true, Parameter::Bool, ui->runZap_checkBox));
    p.parameter.push_back(Parameter("runResonance", true, true, Parameter::Bool, ui->runResonance_checkBox));
    p.parameter.push_back(Parameter("runNoise", true, true, Parameter::Bool, ui->runNoise_checkBox));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Sin";
    p.parameter.push_back(Parameter("sequence", "sin", "sin", Parameter::String, ui->sinSequence_lineEdit));
    p.parameter.push_back(Parameter("repeat", 1, 1, Parameter::Integer, ui->sinRepeat_spinBox));
    p.parameter.push_back(Parameter("plot", true, true, Parameter::Bool, ui->sinPlot_checkBox));

    p.parameter.push_back(Parameter("frequencies", "1", "1", Parameter::String, ui->sinFrequencies_lineEdit));
    p.parameter.push_back(Parameter("amplitudes", "1", "1", Parameter::String, ui->sinAmplitudes_lineEdit));

    HEKAparameter.push_back(p);
    p.parameter.clear();


    p.name = "HEKA Settings";
    p.parameter.push_back(Parameter("file_in", "C:\\E9Batch.In", "C:\\E9Batch.In", Parameter::String, ui->lineEdit_In));
    p.parameter.push_back(Parameter("file_out", "C:\\E9Batch.Out", "C:\\E9Batch.Out", Parameter::String, ui->lineEdit_Out));
    p.parameter.push_back(Parameter("id", 1, 1, Parameter::Integer, ui->id_spinBox));
    p.parameter.push_back(Parameter("wait", 1.0, 1.0, Parameter::Double, ui->wait_SpinBox));
    p.parameter.push_back(Parameter("HekaTemplatePath", "C:\\", "C:\\", Parameter::String, ui->hekaTemplatePath_lineEdit));
    p.parameter.push_back(Parameter("HekaDataPath", "C:\\", "C:\\", Parameter::String, ui->hekaDataPath_lineEdit));
    p.parameter.push_back(Parameter("template", "TemplateCreator", "TemplateCreator", Parameter::String, ui->template_lineEdit));

    HEKAparameter.push_back(p);
    p.parameter.clear();

    DEBUG("parameter set done !")
}






/*****************************************************************************************************************
 *
 *      GUI
 *
 *****************************************************************************************************************/


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    addDockWidget(Qt::BottomDockWidgetArea, ui->creatorDockWidget);
    addDockWidget(Qt::BottomDockWidgetArea, ui->tplDockWidget);
    addDockWidget(Qt::TopDockWidgetArea, ui->viewDockWidget);


    ui->menuView->addAction(ui->tplDockWidget->toggleViewAction());
    ui->menuView->addAction(ui->creatorDockWidget->toggleViewAction());
    ui->menuView->addAction(ui->viewDockWidget->toggleViewAction());
    ui->menuView->addAction(ui->hekaDockWidget->toggleViewAction());
    ui->menuView->addAction(ui->messageDockWidget->toggleViewAction());


    graphWidget = new QCustomPlot();
    ui->graphLayout->addWidget(graphWidget, 0, 0, 1, 1);


    //configure graphics
    graphWidget->addGraph();

    // configure right and top axis to show ticks but no labels (could've also just called graphWidget->setupFullAxesBox):
    graphWidget->xAxis2->setVisible(true);
    graphWidget->xAxis2->setTickLabels(false);
    graphWidget->yAxis2->setVisible(true);
    graphWidget->yAxis2->setTickLabels(false);

    // make left and bottom axes always transfer their ranges to right and top axes:
    connect(graphWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), graphWidget->xAxis2, SLOT(setRange(QCPRange)));
    connect(graphWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), graphWidget->yAxis2, SLOT(setRange(QCPRange)));

    setupParameter();

    // read settings
    QSettings settings("CKSoftware", "TemplateCreator");

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->tabWidget->setCurrentIndex(settings.value("tab", 0).toInt());
    ui->hekaTabWidget->setCurrentIndex(settings.value("HEKAtab", 0).toInt());
    settings.endGroup();

    for (int i=0; i < NCreatorTabs; i++) {
        parameter[i].read_settings(settings);
        parameter[i].to_widgets();
    }
    last_index = 0;

    for (int i=0; i < NHekaTabs; i++) {
        HEKAparameter[i].read_settings(settings);
        HEKAparameter[i].to_widgets();
    }

    DEBUG("reading settings done!")

    //heka setup
    heka.mainWindow  = this;
    updateHEKA();
    heka.batch_id = 1;
    updateHEKABatchId();


    //message window
    message_counter = 0;

    update();



    DEBUG("init done!")
}


MainWindow::~MainWindow()
{
    DEBUG("destroy!")

    //write settings
    QSettings settings("CKSoftware", "TemplateCreator");

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("tab", index());
    settings.setValue("HEKAtab", HEKAindex());
    settings.endGroup();

    DEBUG("destroy: write parameter!")
    for (int i=0; i<NCreatorTabs; i++) {
        parameter[i].from_widgets();
        parameter[i].write_settings(settings);
    }

    DEBUG("destroy: write HEKA parameter")
    for (int i=0; i<NHekaTabs; i++) {
        HEKAparameter[i].from_widgets();
        HEKAparameter[i].write_settings(settings);
    }

    delete ui;
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionAbout_Template_Creator_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("The awsome Template Creator by Christoph Kirst\nckirst@nld.ds.mpg.de");
    msgBox.setWindowTitle("Info");
    //msgBox.setIcon();
    msgBox.exec();
}

void MainWindow::on_browseButton_clicked()
{
    //QFileDialog::setFileMode(QFileDialog::AnyFile);
    QString newfile = QFileDialog::getSaveFileName(this,
                                                   tr("Select Template File"), ui->filename_Edit->text() , tr("Template File (*.tpl);;All (*.*)"));
    if (newfile != "") {

        // change active fileName
        parameter[index()].from_widgets();

        ui->filename_Edit->setText(newfile);
        on_saveopenButton_clicked();
    }
}



void MainWindow::on_browseData_pushButton_clicked()
{
    //QFileDialog::setFileMode(QFileDialog::AnyFile);
    QString newfile = QFileDialog::getExistingDirectory(this, tr("Select Data Directory"),
                                ui->hekaDataPath_lineEdit->text() , QFileDialog::ShowDirsOnly);
    if (newfile != "") {

        // change active fileName
        ui->hekaDataPath_lineEdit->setText(newfile);
    }
}


void MainWindow::on_browseTempalate_pushButton_clicked()
{
    //QFileDialog::setFileMode(QFileDialog::AnyFile);
    QString newfile = QFileDialog::getExistingDirectory(this, tr("Select Template / pgf Directory"),
                                ui->hekaTemplatePath_lineEdit->text() , QFileDialog::ShowDirsOnly);
    if (newfile != "") {
        // change active fileName
        ui->hekaTemplatePath_lineEdit->setText(newfile);
    }
}


void MainWindow::on_loadButton_clicked()
{
    //try to read data
    QString fileName = ui->filename_Edit->text();

    if (heka.read_template_file(fileName, data)) {
        plotData();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Cannot load file: " + fileName);
        msgBox.exec();
    }
}


// general template parameter
void MainWindow::on_dur_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_off_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_left_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_right_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_sample_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_filename_Edit_editingFinished()
{
    update();
}


//zap parameter editing
void MainWindow::on_createButton_clicked()
{
    update();
}

void MainWindow::on_f0_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_f1_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_amp_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_reverseCheckBox_clicked()
{
    update();
}


//LPFnoise
void MainWindow::on_createButton_2_clicked()
{
    update();
}

void MainWindow::on_omega_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_phase_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_amp_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_f0_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_f1_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_sigma_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_seed_SpinBox_2_editingFinished()
{
    update();
}

//tab change
void MainWindow::on_tabWidget_currentChanged(int id)
{
    if (id<0 || id > NCreatorTabs-1) {
        id = last_index;
    } else {
        last_index = id;
    }

    //int id = index();
    //ui->statusBar->showMessage(QString("index is %1").arg(id), 2000 );

    parameter[id].to_widgets();

    update();
}


void MainWindow::on_hekaTabWidget_currentChanged(int)
{
}


void MainWindow::on_saveopenButton_clicked()
{
    update();
    saveData();
}


// Heka scripting
void MainWindow::on_writeButton_clicked()
{
    writeManualBatchFile();
}


void MainWindow::on_readButton_clicked()
{
    readManualBatchFile();
}


//runs
void MainWindow::on_run_pushButton_clicked()
{
    runRun();
}

void MainWindow::on_runZap_pushButton_clicked()
{
    runZap();
}

void MainWindow::on_runResonance_pushButton_clicked()
{
   runResonance();
}

void MainWindow::on_runNoise_pushButton_clicked()
{
   runNoise();
}

void MainWindow::on_runSin_pushButton_clicked()
{
       runSin();
}













/*****************************************************************************************************************
 *
 *      Template GUI Handling
 *
 *****************************************************************************************************************/


// IO
void MainWindow::message(const QString& msg){
    message_counter++;
    if (message_counter > 1000000) {clear_messages(); message_counter = 0;}

    ui->message_textEdit->append(msg);
    ui->statusBar->showMessage(msg, 2000 );

    DEBUG(msg.toStdString())
}

void MainWindow::message(const QString& routine, const QString& msg){
     message(routine + ": " + msg);
}

void MainWindow::message(const QString& msg, const QColor& color){
    QColor tc = ui->message_textEdit->textColor();
    ui->message_textEdit->setTextColor( color );
    message(msg);
    ui->message_textEdit->setTextColor( tc );
}

void MainWindow::message(const QString& routine, const QString& msg, const QColor& color){
     message(routine + ": " + msg, color);
}


void MainWindow::error_message(const QString& msg){
    message_counter++;
    if (message_counter > 1000000) {clear_messages(); message_counter = 0;}

    // save
    //int fw = ui->message_textEdit->fontWeight();
    QColor tc = ui->message_textEdit->textColor();

    // append
    //ui->message_textEdit->setFontWeight( QFont::DemiBold );
    ui->message_textEdit->setTextColor( QColor( "red" ) );
    ui->message_textEdit->append( msg );

    // restore
    //ui->message_textEdit->setFontWeight( fw );
    ui->message_textEdit->setTextColor( tc );

    ui->statusBar->showMessage(msg, 2000 );

    DEBUG("error")
    DEBUG(msg.toStdString())
}

void MainWindow::error_message(const QString& routine, const QString& msg){
    error_message(routine + ": error: " + msg);
}

void MainWindow::clear_messages() {
    ui->message_textEdit->clear();
}




// indices
int MainWindow::index() {
    int id = ui->tabWidget->currentIndex();
    if (id >= NCreatorTabs || id <0) {
        id = last_index;
    }
    return id;
}

int MainWindow::HEKAindex() {
    int id = ui->hekaTabWidget->currentIndex();
    if (id >= NHekaTabs || id <0) {
        id = 0;
    }
    return id;
}



//updating
void MainWindow::update() {
    //read parameter from widgets
    //for (int i= 0; i < NCreatorTabs; i++) {
    //    parameter[i].from_widgets();
    //}

    parameter[index()].from_widgets();


    //check if we have to do something
    if (index() < 3 && last_parameter != parameter[index()]){
        createData();
        plotData();
        updateTemplateInfo();

        last_parameter = parameter[index()];
    }

}

void MainWindow::updateTemplateInfo() {
    if (index() < 3) {
        double dt = 1 / parameter[index()]["sample"].value.toDouble() / 1000;
        double length = parameter[index()]["dur"].value.toDouble() + parameter[index()]["left"].value.toDouble() + parameter[index()]["right"].value.toDouble();
        int points = ceil(length /dt);
        ui->info_label->setText(QString("length = %1 s, dt = %2 s, points = %3").arg(length).arg(dt).arg(points));
    }
}



// data
void MainWindow::copyData(DataVECTOR& v){
    v.resize(data.size());
    copy (data.begin(), data.end(), v.begin());
}

void MainWindow::setData(const DataVECTOR& v){
    data.resize(v.size());
    copy (v.begin(), v.end(), data.begin());
}


void MainWindow::saveData() {
    saveData(ui->filename_Edit->text());
}

void MainWindow::saveData(const QString& file_name) {
    //create directory if not existent
    QDir().mkdir(QFileInfo(file_name).path());

    bool res = heka.write_template_file(file_name, data);

    if (res)
        ui->statusBar->showMessage("Saved Template to " + file_name, 2000 );
    else
        ui->statusBar->showMessage("Could not save Template to " + file_name, 2000 );
}


void MainWindow::createData() {
    // create data

    DEBUG("create data")

    int id = index();

    if (id==0) {
        createZap();
    } else if (id ==1) {
        createNoise();
    } else { //id==2
        createSin();
    }


    DEBUG("create data")
}

bool MainWindow::createZap() {
    DEBUG("create zap")

    int type = parameter[ZapTab]["type"].value.toInt();

    bool suc;
    if (type == 0) { // linear zap
        suc = create_zap( parameter[ZapTab]["dur"].value.toDouble(),  parameter[ZapTab]["sample"].value.toDouble(),
                           parameter[ZapTab]["f0"].value.toDouble(),  parameter[ZapTab]["f1"].value.toDouble(),
                           parameter[ZapTab]["amp"].value.toDouble(),  parameter[ZapTab]["reverse"].value.toBool(),
                           data );

    } else if (type == 1) { // t^2 zap
        suc = create_zap_2( parameter[ZapTab]["dur"].value.toDouble(),  parameter[ZapTab]["sample"].value.toDouble(),
                               parameter[ZapTab]["f0"].value.toDouble(),  parameter[ZapTab]["f1"].value.toDouble(),
                               parameter[ZapTab]["amp"].value.toDouble(),  parameter[ZapTab]["reverse"].value.toBool(),
                               data );
    } else { // exp zap
        suc = create_zap_exp( parameter[ZapTab]["dur"].value.toDouble(),  parameter[ZapTab]["sample"].value.toDouble(),
                               parameter[ZapTab]["f0"].value.toDouble(),  parameter[ZapTab]["f1"].value.toDouble(),
                               parameter[ZapTab]["amp"].value.toDouble(),  parameter[ZapTab]["reverse"].value.toBool(),
                               data );
    }

    postprocess_template( parameter[ZapTab]["sample"].value.toDouble(), parameter[ZapTab]["off"].value.toDouble(),
                          parameter[ZapTab]["left"].value.toDouble(), parameter[ZapTab]["right"].value.toDouble(),
                          data );


    //add some additional offset at end for rounding problems
    double off=parameter[ZapTab]["off"].value.toDouble();
    for (int i =0; i<100; i++) {
        data.push_back(off);
    }

    /*
    bool zap = create_zap(ui-> dur_SpinBox->value(),  ui->sample_SpinBox->value(),
                          ui->f0_SpinBox->value(), ui->f1_SpinBox->value(), ui->amp_SpinBox->value(), ui->reverseCheckBox->isChecked(),
                          data);
    postprocess_template(ui->sample_SpinBox->value(), ui->off_SpinBox->value(), ui->left_SpinBox->value(), ui->right_SpinBox->value(),
                             data);
    */



    DEBUG("create zap done !")




    if (suc)
        ui->statusBar->showMessage("Zap created", 2000 );
    else
        ui->statusBar->showMessage("Could not create Zap!", 2000 );

    return suc;
}

bool MainWindow::createNoise() {

    DEBUG("create noise !")
    bool suc = create_noise(parameter[NoiseTab]["dur"].value.toDouble(), parameter[NoiseTab]["sample"].value.toDouble(),
                     parameter[NoiseTab]["f"].value.toDouble(), parameter[NoiseTab]["phase"].value.toDouble(), parameter[NoiseTab]["amp"].value.toDouble(),
                     parameter[NoiseTab]["f0"].value.toDouble(), parameter[NoiseTab]["f1"].value.toDouble(), parameter[NoiseTab]["sigma"].value.toDouble(), parameter[NoiseTab]["seed"].value.toInt(),
                     data);


    postprocess_template(parameter[NoiseTab]["sample"].value.toDouble(), parameter[NoiseTab]["off"].value.toDouble(),
                         parameter[NoiseTab]["left"].value.toDouble(), parameter[NoiseTab]["right"].value.toDouble(),
                         data );

    double off = parameter[NoiseTab]["off"].value.toDouble();
    for (int i =0; i<100; i++) {
        data.push_back(off);
    }



    /*
    bool noise = create_noise(ui->dur_SpinBox->value(), ui->sample_SpinBox->value(),
                     ui->omega_SpinBox_2->value(), ui->phase_SpinBox_2->value(), ui->amp_SpinBox_2->value(),
                     ui->f0_SpinBox_2->value(), ui->f1_SpinBox_2->value(), ui->sigma_SpinBox_2->value(), ui->seed_SpinBox_2->value(),
                     data);
    postprocess_template(ui->sample_SpinBox->value(), ui->off_SpinBox->value(), ui->left_SpinBox->value(), ui->right_SpinBox->value(),
                             data);
    */
    DEBUG("create noise done !")

    if (suc) {
        ui->statusBar->showMessage("Noise created", 2000 );
    } else {
        ui->statusBar->showMessage("Could not create Noise!", 2000 );
    }

    return suc;
}


bool MainWindow::createSin() {


    if (parameter[SinTab]["peaks"].value.toBool() && parameter[SinTab]["f"].value.toDouble() !=0) {
        parameter[SinTab]["dur"].value = parameter[SinTab]["npeaks"].value.toDouble() / parameter[SinTab]["f"].value.toDouble();
        parameter[SinTab]["dur"].to_widget();
    }
    if (parameter[SinTab]["peaks2"].value.toBool() && parameter[SinTab]["f2"].value.toDouble() !=0) {
        parameter[SinTab]["dur"].value = parameter[SinTab]["npeaks2"].value.toDouble() / parameter[SinTab]["f2"].value.toDouble();
        parameter[SinTab]["dur"].to_widget();
    }

    DEBUG("create sin !")
    bool suc = create_sin(parameter[SinTab]["dur"].value.toDouble(), parameter[SinTab]["sample"].value.toDouble(),
                    parameter[SinTab]["f"].value.toDouble(), parameter[SinTab]["phase"].value.toDouble(), parameter[SinTab]["amp"].value.toDouble(),
                    parameter[SinTab]["f2"].value.toDouble(), parameter[SinTab]["phase2"].value.toDouble(), parameter[SinTab]["amp2"].value.toDouble(),
                    parameter[SinTab]["positive"].value.toBool(),
                     data);


    postprocess_template(parameter[SinTab]["sample"].value.toDouble(), parameter[SinTab]["off"].value.toDouble(),
                         parameter[SinTab]["left"].value.toDouble(), parameter[SinTab]["right"].value.toDouble(),
                         data );

    double off=parameter[SinTab]["off"].value.toDouble();
    for (int i =0; i<100; i++) {
        data.push_back(off);
    }

    DEBUG("sin noise done !")

    if (suc) {
        ui->statusBar->showMessage("Sin created", 2000 );
    } else {
        ui->statusBar->showMessage("Could not create Sin!", 2000 );
    }

    return suc;
}





// a simple plotting routine

void MainWindow::plotData() {
    plotData(1.0 / ui->sample_SpinBox->value() / 1000.0);
}

void MainWindow::plotData(double dt)
{
    DEBUG("plotData")

    // reduce data
    //graphWidget->width();
    int plotpoints = data.size() < 10000 ?  data.size() : 10000;
    double dindex = double(data.size()) / plotpoints;
    double index= 0;

    int n = floor(double(data.size()) / dindex);

    x.resize(n);
    y.resize(n);

    for (int i = 0; i < n; i++) {
        x[i] = floor(index) * dt;
        y[i] = data[int(floor(index))];
        index += dindex;
    }

    // add two new graphs and set their look:
    graphWidget->graph(0)->setPen(QPen(Qt::blue)); // line color blue for first graph
    //graphWidget->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue

    // pass data points to graphs:
    graphWidget->graph(0)->setData(x, y);

    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    graphWidget->graph(0)->rescaleAxes();

    // make range moveable by mouse interaction (click and drag):
    graphWidget->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    graphWidget->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    graphWidget->setInteraction(QCustomPlot::iSelectPlottables); // allow selection of graphs via mouse click

    graphWidget->replot();

    DEBUG("plotData!")
}







/*****************************************************************************************************************
 *
 *      GUI HEKA interface
 *
 *****************************************************************************************************************/


// helpers

void MainWindow::updateHEKA(){
    for (int i= 0; i < NHekaTabs; i++) {
        HEKAparameter[i].from_widgets();
    }

    //set batch file names etc...
    heka.batch_command_file_name = HEKAparameter[Settings]["file_in"].value.toString();
    heka.batch_message_file_name = HEKAparameter[Settings]["file_out"].value.toString();
    heka.batch_id =  HEKAparameter[Settings]["id"].value.toInt();
    heka.batch_wait =  HEKAparameter[Settings]["wait"].value.toDouble();
}

void MainWindow::updateHEKABatchId(){
    HEKAparameter[Settings]["id"].value = heka.batch_id;
    HEKAparameter[Settings]["id"].to_widget();
}


void MainWindow::zap_parameter_to_comment(QString& comment) {
    //we only need the first 10 parameters really !
    QString c("Zap");
    for (int i = 0; i < 10; i++) {
        c = QString("%1; %2").arg(c).arg(parameter[ZapTab].parameter[i].toString());
    }

    comment = c;
}

bool MainWindow::zap_parameter_from_comment(const QString& comment) {
    QStringList list = comment.split(QRegExp("(;\\s)(\\s)*"), QString::SkipEmptyParts);
    if (list.size() != 11) {
        error_message("zap_parameter_from_comment", QString("Cannot parse comment to Zap: %1").arg(comment));
        return false;
    }

    if (list[0] != "Zap") {
        error_message("zap_parameter_from_comment", QString("Cannot parse comment to Zap: %1").arg(comment));
        return false;
    }

    bool ok;
    for (int i = 0; i < 9; i++) {
        parameter[ZapTab].parameter[i].from_string(list[i+1], &ok);

        if (!ok) {
            error_message("zap_parameter_from_comment", QString("Cannot parse comment to Zap: %1").arg(comment));
            return false;
        }
    }

    return true;
}


void MainWindow::noise_parameter_to_comment(QString& comment) {

    QString c("Noise");
    for (int i = 0; i < 12; i++) {
        c = QString("%1; %2").arg(c).arg(parameter[NoiseTab].parameter[i].toString());
    }

    comment = c;
}


bool MainWindow::noise_parameter_from_comment(const QString& comment) {
    QStringList list = comment.split(QRegExp("(;\\s)(\\s)*"), QString::SkipEmptyParts);

    if (list.size() != 13) {
        error_message("noise_parameter_from_comment", QString("Cannot parse comment to Noise: %1").arg(comment));
        return false;
    }

    if (list[0] != "Noise") {
        error_message("noise_parameter_from_comment", QString("Cannot parse comment to Noise: %1").arg(comment));
        return false;
    }

    bool ok;
    for (int i = 0; i < 12; i++) {
        parameter[NoiseTab].parameter[i].from_string(list[i+1], &ok);

        if (!ok) {
            error_message("noise_parameter_from_comment", QString("Cannot parse comment to Noise: %1").arg(comment));
            return false;
        }
    }

    return true;
}


void MainWindow::sin_parameter_to_comment(QString& comment) {

    QString c("Sin");
    for (int i = 0; i < 14; i++) {
        c = QString("%1; %2").arg(c).arg(parameter[SinTab].parameter[i].toString());
    }

    comment = c;
}


bool MainWindow::sin_parameter_from_comment(const QString& comment) {
    QStringList list = comment.split(QRegExp("(;\\s)(\\s)*"), QString::SkipEmptyParts);

    if (list.size() != 15) {
        error_message("sin_parameter_from_comment", QString("Cannot parse comment to Sin: %1").arg(comment));
        return false;
    }

    if (list[0] != "Sin") {
        error_message("sin_parameter_from_comment", QString("Cannot parse comment to Sin: %1").arg(comment));
        return false;
    }

    bool ok;
    for (int i = 0; i < 14; i++) {
        parameter[SinTab].parameter[i].from_string(list[i+1], &ok);

        if (!ok) {
            error_message("noise_parameter_from_comment", QString("Cannot parse comment to Sin: %1").arg(comment));
            return false;
        }
    }

    return true;
}




// parsing lists of values

bool MainWindow::string_list_to_values(const QString& string, QVector<double>& v) {
    QStringList list = string.split(QRegExp("(;)(\\s)*"), QString::SkipEmptyParts);

    /*
    DEBUG(string.toStdString())
    DEBUG(list.size())
     for (int i = 0; i < list.size(); i++) {
         DEBUG(list[i].toStdString())
     }*/

    v.clear();
    bool ok;

    for (int i = 0; i < list.size(); i++) {
        double x = list[i].toDouble(&ok);
        if (!ok) {
            error_message("string_list_to_values", QString("Cannot parse list to values: %1").arg(string));
            return false;
        }
        v.push_back(x);
    }

    return true;
}





// HEKA scripts

void MainWindow::writeManualBatchFile() {
    DEBUG("writeManualBatchFile")

    //update parameter
    updateHEKA();

    bool suc = heka.open_write_to_batch_command_file(ui->textIn->toPlainText());

    if (!suc) {
        ui->textOut->setPlainText(QString("Error issuing command!"));
        return;
    }

    // check for answer
    QString msg;
    suc = heka.open_wait_for_batch_message_file(msg);

    if (!suc) {
        ui->textOut->setPlainText(QString("Error reading from message file!"));
    } else {
        ui->textOut->setPlainText(msg);
    }

    updateHEKABatchId();
}


void MainWindow::readManualBatchFile() {
    //read the raw batch file
    QFile file(ui->lineEdit_Out->text());
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!file.isOpen()) {
        ui->textOut->setText(QString("Error: Cannot open file: %1").arg(file.fileName()));
        error_message("readManualBatchFile", QString("bvCannot open file: %1").arg(file.fileName()));
        file.close();
        return;
    }

    QTextStream in(&file);
    ui->textOut->clear();
    while (!in.atEnd()) {
        ui->textOut->append(in.readLine());
    }
    file.close();
}

//create a standard sequence to play a template in HEKA
//make sure there is a pgf sequence from which to copy.
bool MainWindow::createTemplateSequence(const QString& name, double dur, int nsweep) {
        updateHEKA();

        Heka::Sequence seq;
        seq.source = HEKAparameter[Settings]["template"].value.toString();
        seq.name = name;
        seq.interval = 0.0;
        seq.sweepno = nsweep;
        seq.trigger = 0;

        Heka::Segment seg;
        seg.dur = dur;
        seg.amp = 0;
        seq.segment.push_back(seg);

        // delete old segment
        heka.delete_sequence(name);

        //create new one
        bool suc = heka.new_sequence(seq);
        if (!suc) {
            error_message("createTemplateSequence", QString("could not create: %1").arg(seq.toString()));
        } else {
            message("createTemplateSequence", QString("created: %1").arg(seq.toString()));
        }
        return suc;
}





//a full run
bool MainWindow::runHEKA(const QString& sequence, const QString& comment, double off, double time, bool plot, int nrep){
    // run HEKA

    break_execution = false;
    for (int i = 0; i < nrep; i++){ //usually we should not need this extrax loop -> use sweep number!

    bool suc = heka.execute_sequence(sequence);

    if (!suc) {
        error_message("run", QString("Could not execute sequence: %1").arg(sequence));
        return false;
    }
    message("run", QString("Succesfully executed sequence: %1").arg(sequence));

    //write comment
    if (comment != "") {
        suc = heka.set_comment(comment);
        if(!suc) {
            error_message("run", QString("Could not set comment: %1 in sequence: %2").arg(comment).arg(sequence));
            //return false;
        }
    }

    suc = heka.wait_for_idle(time, off, &break_execution);
    if (break_execution) return false;
    if (!suc) {
        error_message("run", QString("did not finish in time: %1").arg(sequence));
        return false;
    }

    //plot data
    if (plot) {
        //read zap
        suc = heka.get_last_recorded_data(data);
        if (!suc) {
            error_message("run", "Could not read data for last sequence!");
            return false;
        }

        plotData();
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    if (break_execution) {
        break;
    }


    } //for over repeats

    return true;
}


void MainWindow::runRun() {
    updateHEKA();

    QString sequence = HEKAparameter[Run]["sequence"].value.toString();
    int nrep = HEKAparameter[Run]["repeat"].value.toInt();
    bool plot = HEKAparameter[Run]["plot"].value.toBool();
    double off = HEKAparameter[Run]["off"].value.toDouble();
    double time = HEKAparameter[Run]["time"].value.toDouble();

    runHEKA(sequence, "", off, time, plot, nrep);

    updateHEKABatchId();
}


void MainWindow::runZap() {
    //update parameter
    updateHEKA();

    QString path = HEKAparameter[Settings]["HekaTemplatePath"].value.toString();
    QString sequence = HEKAparameter[Zap]["sequence"].value.toString();
    QString filename = heka.sequence_to_template_file_name(sequence, path);

    int nrep = HEKAparameter[Zap]["repeat"].value.toInt();
    bool plot = HEKAparameter[Zap]["plot"].value.toBool();
    double dur = parameter[ZapTab]["dur"].value.toDouble();
    double time =  dur + parameter[ZapTab]["left"].value.toDouble()
                  + parameter[ZapTab]["right"].value.toDouble();
    bool update = HEKAparameter[Zap]["update"].value.toBool();

    QString comment;
    zap_parameter_to_comment(comment);

    //create new sequence
    createTemplateSequence(sequence, time, nrep);

    //create and write zap
    createZap();
    plotData();
    saveData(filename);

    // run HEKA
    runHEKA(sequence, comment, nrep * time, nrep * (time + 10), plot);

    //update parameter for runResonance
    if (update) {
        HEKAparameter[Resonance]["sequence"].value = sequence;
        HEKAparameter[Resonance]["template"].value = sequence;
        HEKAparameter[Resonance]["dur"].value = dur;
        HEKAparameter[Resonance]["fmax"].value = parameter[ZapTab]["f1"].value;
        HEKAparameter[Resonance]["load"].value = 0;
        HEKAparameter[Resonance]["zero"].value = parameter[ZapTab]["off"].value;

        HEKAparameter[Resonance].to_widgets();
    }

    updateHEKABatchId();
}


//search for resonance in Zap response
void MainWindow::runResonance() {

    //update parameter
    updateHEKA();


    QString sequence = HEKAparameter[Resonance]["sequence"].value.toString();
    bool plot = HEKAparameter[Resonance]["plot"].value.toBool();

    QString path = HEKAparameter[Settings]["HekaTemplatePath"].value.toString();

    //get template file

    DataVECTOR stim, resp;


    //get template
    int load = HEKAparameter[Resonance]["load"].value.toInt();

    if (load ==0) { // load tpl file
        QString filename = heka.sequence_to_template_file_name(sequence, path);
        bool suc = heka.read_template_file(filename, stim);
        if (!suc){
            error_message("runResonance", QString("could not read template file: %1").arg(filename));
            return;
        }
    } else { // take last zap
        createZap();
        copyData(stim);
        HEKAparameter[Resonance]["dur"].value = parameter[ZapTab]["dur"].value;
        HEKAparameter[Resonance]["fmax"].value = parameter[ZapTab]["f1"].value;
        HEKAparameter[Resonance]["zero"].value = parameter[ZapTab]["off"].value;

        HEKAparameter[Resonance].to_widgets();
    }

    //get response data
    bool suc = heka.get_last_recorded_data(resp);
    if (!suc) {
        error_message("runResonance", "Could not read data for last sequence!");
        return;
    }


    // testing with predefined data -> works
    //read_template_file("/home/ckirst/Science/Projects/LFPSpikes/Experiment/Patch/Test/zapin.tpl", stim);
    //read_template_file("/home/ckirst/Science/Projects/LFPSpikes/Experiment/Patch/Test/zapout.tpl", resp);

    //read_template_file("E:/Science/Projects/LFPSpikes/Experiment/Patch/Test/zapin.tpl", stim);
    //read_template_file("E:/Science/Projects/LFPSpikes/Experiment/Patch/Test/zapout.tpl", resp);


    int n1 = stim.size();
    int n2 = resp.size();
    if (n1 != n2) {
        error_message("runResonance", QString("array sizes: %1, %2").arg(n1).arg(n2));
    } else {
        message("runResonance", QString("array sizes: %1, %2").arg(n1).arg(n2));
    }

    if (n1 < n2) {
        remove_ends(resp,0, n2-n1);
    }
    if (n1 > n2) {
        remove_ends(stim,0, n1-n2);
    }

    n1 = stim.size();
    n2 = resp.size();
    message("runResonance", QString("array sizes after equalizing: %1, %2").arg(n1).arg(n2));


    //check for on and offsets in stimulus and remove
    double zero = HEKAparameter[Resonance]["zero"].value.toDouble();
    int pos1 = 0;
    first_non_zero(stim, pos1, zero);
    int pos2 = 0;
    last_non_zero(stim, pos2, zero);

    message("runResonance", QString("offsets: %1, %2").arg(pos1).arg(pos2));

    //remove offsets
    remove_ends(stim, pos1, pos2);
    remove_ends(resp, pos1, pos2);

    n1 = stim.size();
    n2 = resp.size();
    message("runResonance", QString("array sizes after removing offsets: %1, %2").arg(n1).arg(n2));

    //find good fft size
    n2 = find_good_smaller_fft_size(n1);
    remove_ends(stim, 0, n1-n2);
    remove_ends(resp, 0, n1-n2);

    n1 = stim.size();
    n2 = resp.size();
    message("runResonance", QString("array sizes after truncating to good fft size: %1, %2").arg(n1).arg(n2));


    // calulate impedance
    DataVECTOR imp;
    impedance(stim, resp, imp);


    //number of relevant data points: maxf * dur
    double dur = HEKAparameter[Resonance]["dur"].value.toDouble();
    double maxf = HEKAparameter[Resonance]["fmax"].value.toDouble();

    //zoom into relevant regime
    int np = int(dur * maxf);
    if (np > n1-5) np = n1-5;
    remove_ends(imp, 5, n1-np-5);

    double df = 1.0/dur;

    //if (plot) plotData();
    message("runResonance Info:", QString("np = %1, df = %2").arg(np).arg(df));

    //now detect resonance peaks:
    double peak = HEKAparameter[Resonance]["peak"].value.toDouble();
    double smooth = HEKAparameter[Resonance]["smooth"].value.toDouble();

    DataVECTOR imp_smooth;
    smooth_data(imp, smooth, imp_smooth);
    setData(imp_smooth);
    df = df * smooth;
    if (plot) plotData(df);

    //find
    std::vector<int> pos;
    find_peaks(imp_smooth, peak, pos, 100);

    for (int i = 0; i < int(pos.size()); i++) message("runResonance", QString("found resonance peak at: %1").arg(pos[i]*df), QColor("green"));
    if ( int(pos.size()) == 0) {
        message("runResonance", QString("found resonance peak at: 0.0"), QColor("green"));
    }
    if (int(pos.size() ==100)) {
        message("runResonance", QString("warning: found more than 100 peaks! Try to modify parameters."), QColor("red"));
    } else {
        message("runResonance", QString("in total: %1 peak(s)").arg(pos.size()), QColor("green"));
    }

    bool update = HEKAparameter[Resonance]["update"].value.toBool();
    double f_guess = 0.0;
    if (pos.size() > 0) {
        f_guess = (*(pos.end()-1)) * df; // take last - alternatively we could average here!!
    }

    if (update){  //update noise parameter
        parameter[NoiseTab]["f"].value = f_guess;
        parameter[NoiseTab]["f"].to_widget();
    }

    updateHEKABatchId();
}




void MainWindow::runNoise() {
    //update parameter
    updateHEKA();
    update();

    QString path = HEKAparameter[Settings]["HekaTemplatePath"].value.toString();
    QString sequence = HEKAparameter[Noise]["sequence"].value.toString();

    int nrep = HEKAparameter[Noise]["repeat"].value.toInt();
    int type = HEKAparameter[Noise]["type"].value.toInt();
    bool plot = HEKAparameter[Noise]["plot"].value.toBool();
    QString comment;
    double dur = parameter[NoiseTab]["dur"].value.toDouble();
    double time = dur + parameter[NoiseTab]["left"].value.toDouble()
                  + parameter[NoiseTab]["right"].value.toDouble();


    //create new sequence
    createTemplateSequence(sequence, time, nrep);

    //switch frozen or random noise
    if (type == 0) {// frozen noise

        //create and write zap
        createNoise();
        plotData();

        QString filename = heka.sequence_to_template_file_name(sequence, path);
        saveData(filename);
        // run HEKA
        noise_parameter_to_comment(comment);
        runHEKA(sequence, comment, nrep * time, nrep * (time + 10), plot);

    } else {// noise repititions

        int seed = 0;
        if (type == 1) { // random seed
            QTime t = QTime::currentTime();
            qsrand((uint)t.msec());
            int seed = qrand();
            HEKAparameter[Noise]["seed"].value = seed;
            HEKAparameter[Noise]["seed"].to_widget();
        } else {  // noise repitiontiions with fixed seed for seeds
            seed = HEKAparameter[Noise]["seed"].value.toInt();
        }
        qsrand(seed);


        //we cannot save all seeds -> so save first intial seed from which we draw all sucessive ones
        QVector<int> seeds;
        for (int i = 0; i < nrep; i++) {
            seeds.push_back(qrand());
        }

        parameter[NoiseTab]["seed"].value = seed;
        noise_parameter_to_comment(comment);
        comment +=";"+QString("%1").arg(type);


        //create different noise templates
        for (int i =0; i< nrep; i++) {
            message("runNoise", QString("set seed to %1").arg(seeds[i]));
            parameter[NoiseTab]["seed"].value = seeds[i];
            parameter[NoiseTab]["seed"].to_widget();

            //create and write noise
            createNoise();
            plotData();

            QString filename = heka.sequence_to_template_file_name(sequence, path, i, 1);
            saveData(filename);
        }


        runHEKA(sequence, comment, nrep * time, nrep * (time + 10), plot);
    }


    updateHEKABatchId();
}


void MainWindow::on_runAll_pushButton_clicked()
{
    updateHEKA();

    break_execution = false;

    if (!break_execution && HEKAparameter[All]["runZap"].value.toBool()) runZap();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    if (!break_execution && HEKAparameter[All]["runResonance"].value.toBool()) runResonance();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    if (!break_execution && HEKAparameter[All]["runNoise"].value.toBool()) runNoise();

    break_execution= false;
}


void MainWindow::runSin() {
    //update parameter
    updateHEKA();
    update();

    QString path = HEKAparameter[Settings]["HekaTemplatePath"].value.toString();
    QString sequence = HEKAparameter[Sin]["sequence"].value.toString();

    int nrep = HEKAparameter[Sin]["repeat"].value.toInt();
    bool plot = HEKAparameter[Sin]["plot"].value.toBool();

    QVector<double> f, amp;
    QString list = HEKAparameter[Sin]["frequencies"].value.toString();
    bool ok = string_list_to_values(list, f);
    if (!ok) return;
    list = HEKAparameter[Sin]["amplitudes"].value.toString();
    ok = string_list_to_values(list, amp);
    if (!ok) return;

    //int ntot = nrep * f.size() * amp.size();

    for (int i = 0; i < f.size(); i++) {
        for (int j = 0; j < amp.size(); j++) {

            double f0 = f[i];
            double amp0 = amp[j];

            //zero hz onlz makes sense for a single amplitude
            if (f0==0 && j>0) {
                message(QString("runZap"), QString("skipping zero amplitudes..."));
            } else {

                parameter[SinTab]["f"].value = f0;
                parameter[SinTab]["f"].to_widget();
                parameter[SinTab]["amp"].value = amp0;
                parameter[SinTab]["amp"].to_widget();

                createSin();
                plotData();

                double dur = parameter[SinTab]["dur"].value.toDouble();
                double time = dur + parameter[SinTab]["left"].value.toDouble()
                           + parameter[SinTab]["right"].value.toDouble();

                //create new sequence
                createTemplateSequence(sequence, time, nrep);

                QString comment;
                sin_parameter_to_comment(comment);

                QString filename = heka.sequence_to_template_file_name(sequence, path);
                saveData(filename);

                ok = runHEKA(sequence, comment, nrep * time, nrep * (time + 10), plot);

                if (break_execution || !ok) break;
                updateHEKABatchId();
            }
        }
        if (break_execution || !ok) break;
     }

     updateHEKABatchId();
}






void MainWindow::on_runBreak_pushButton_clicked()
{
    breakHEKA();
}

void MainWindow::on_zapBreak_pushButton_clicked()
{
    breakHEKA();
}

void MainWindow::on_noiseBreak_pushButton_clicked()
{
    breakHEKA();
}

void MainWindow::on_allBreak_pushButton_clicked()
{
    breakHEKA();
}

void MainWindow::on_sinBreak_pushButton_clicked()
{
    breakHEKA();
}

void MainWindow::breakHEKA() {
    break_execution = true;
    bool suc = heka.break_execution();
    if (!suc) {
        error_message("breakHEKA", "could not break HEKA execution!");
    }
}




void MainWindow::on_sin_f_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_sin_phase_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_sin_amp_SpinBox_editingFinished()
{
    update();
}

void MainWindow::on_sin_peaks_doubleSpinBox_editingFinished()
{
    update();
}

void MainWindow::on_sin_peaks_checkBox_clicked()
{
    update();
}

void MainWindow::on_sin_f_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_sin_phase_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_sin_amp_SpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_sin_peaks_doubleSpinBox_2_editingFinished()
{
    update();
}

void MainWindow::on_sin_peaks_checkBox_2_clicked()
{
    update();
}

void MainWindow::on_sin_positive_checkBox_clicked()
{
    update();
}




void MainWindow::on_zap_sqr_radioButton_clicked()
{
    update();
}

void MainWindow::on_zap_exp_radioButton_clicked()
{
    update();
}

void MainWindow::on_zap_lin_radioButton_clicked()
{
    update();
}



void MainWindow::on_noiselist_pushButton_clicked()
{
    // read list of nosie parameter and create tpl files
    QFile file(ui->noiselist_lineEdit->text());
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!file.isOpen()) {
        error_message("Noise list:", QString("Cannot open file: %1").arg(file.fileName()));
        file.close();
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString comment = in.readLine();
        bool ok = noise_parameter_from_comment(comment);
        if (!ok) {
            error_message("Noise list:", QString("Cannot interpret noise parameter: %1").arg(comment));
        } else {
            message("Noise list:", QString("creating noise stimulus: %1").arg(comment));
            createNoise();

            QString outname(QString("%1-%2.tpl").arg(ui->noiselist_filename_lineEdit->text()).arg(comment));
            message("Noise list:", QString("and saving to file: %1").arg(outname));
            saveData(outname);
        }

    }

    file.close();
}



//test


void MainWindow::on_test_pushButton_clicked()
{
    srand(ui->test_spinBox->value());
    ui->test_textEdit->clear();
    ui->test_textEdit->append(QString("seed: %1, randmax: %2").arg(ui->test_spinBox->value()).arg(RAND_MAX));

    for (int i=0; i < 20; i++){
        ui->test_textEdit->append(QString("%1 ").arg(rand()));
    }
}

void MainWindow::on_test_pushButton_2_clicked()
{
    createNoise();
    saveData(ui->test_lineEdit->text());
}



