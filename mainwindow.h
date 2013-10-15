/*
    TemplateCreator: create stimulus templates for HEKA Patchmaster

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich
*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QSettings>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QMessageBox>

#include "qcustomplot.h"
#include "numerics.h"

#include "heka.h"

// basic class to store/handle all parameter information
class Parameter {
public:
    enum Type  {Bool, Integer, Double, String, Text, Set};

public:
    QString name;
    QVariant value;
    QVariant default_value;
    Type type;

    QObject* widget; //assoiated input field

public:
    Parameter();
    Parameter(const Parameter& p);
    Parameter(const QString& n, const QVariant& v, const QVariant& d, const Type& t, QWidget* w);
    ~Parameter();

    bool operator== (const Parameter& p) const;
    bool operator!= (const Parameter& p) const;

    void to_widget();
    void from_widget();

    void from_string(const QString& str, bool* ok);
    QString toString();

    void write_settings(QSettings& settings);
    void read_settings(QSettings& settings);
};

class ParameterSet {
public:
    QString name;
    std::vector<Parameter> parameter;

public:
    ParameterSet();
    ParameterSet(const ParameterSet& p);
    ~ParameterSet();

    bool operator== (const ParameterSet& p) const;
    bool operator!= (const ParameterSet& p) const;

    void to_widgets();
    void from_widgets();

    QString toString();

    void write_settings(QSettings& settings);
    void read_settings(QSettings& settings);

    Parameter& operator[] (const char* name);
    Parameter& operator[] (const QString& name);
};




namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    //Parameter
    void setupParameter();

    // Template Handling
    void message(const QString& msg);
    void message(const QString& msg, const QColor& color);
    void message(const QString& routine, const QString& msg);
    void message(const QString& routine, const QString& msg, const QColor& color);
    void error_message(const QString& msg);
    void error_message(const QString& routine, const QString& msg);
    void clear_messages();

    // Functionality
    bool createZap();
    bool createNoise();
    bool createSin();
    void createData();
    void plotData();
    void plotData(double dt);
    void saveData();
    void saveData(const QString& file_name);
    void copyData(DataVECTOR& v);
    void setData(const DataVECTOR& v);

    void update();
    void updateTemplateInfo();

    int index();
    int HEKAindex();

    //HEKA scripts
    void updateHEKA();
    void updateHEKABatchId();

    bool zap_parameter_from_comment(const QString& comment);
    void zap_parameter_to_comment(QString& comment);
    bool noise_parameter_from_comment(const QString& comment);
    void noise_parameter_to_comment(QString& comment);
    bool sin_parameter_from_comment(const QString& comment);
    void sin_parameter_to_comment(QString& comment);


    bool string_list_to_values(const QString& string, QVector<double>& v);

    void writeManualBatchFile();
    void readManualBatchFile();

    bool createTemplateSequence(const QString& name, double dur, int nsweep =1);
    bool runHEKA(const QString& sequence, const QString& comment, double off, double dur, bool plot, int nrep = 1);
    void breakHEKA();
    void runRun();
    void runZap();
    void runNoise();
    void runSin();
    void runResonance();



private slots:
    //GUI
    void on_actionExit_triggered();

    void on_browseButton_clicked();
    void on_saveopenButton_clicked();
    void on_loadButton_clicked();

    void on_dur_SpinBox_editingFinished();
    void on_off_SpinBox_editingFinished();
    void on_left_SpinBox_editingFinished();
    void on_right_SpinBox_editingFinished();
    void on_sample_SpinBox_editingFinished();
    void on_filename_Edit_editingFinished();

    void on_createButton_clicked();
    void on_f0_SpinBox_editingFinished();
    void on_f1_SpinBox_editingFinished();
    void on_amp_SpinBox_editingFinished();
    void on_reverseCheckBox_clicked();

    void on_createButton_2_clicked();
    void on_omega_SpinBox_2_editingFinished();
    void on_phase_SpinBox_2_editingFinished();
    void on_amp_SpinBox_2_editingFinished();
    void on_f0_SpinBox_2_editingFinished();
    void on_f1_SpinBox_2_editingFinished();
    void on_sigma_SpinBox_2_editingFinished();
    void on_seed_SpinBox_2_editingFinished();

    void on_tabWidget_currentChanged(int index);

    void on_actionAbout_Template_Creator_triggered();

    void on_writeButton_clicked();

    void on_hekaTabWidget_currentChanged(int index);

    void on_browseData_pushButton_clicked();

    void on_readButton_clicked();

    void on_browseTempalate_pushButton_clicked();

    void on_runZap_pushButton_clicked();

    void on_runNoise_pushButton_clicked();

    void on_run_pushButton_clicked();

    void on_runResonance_pushButton_clicked();

    void on_runAll_pushButton_clicked();

    void on_runSin_pushButton_clicked();

    void on_runBreak_pushButton_clicked();

    void on_zapBreak_pushButton_clicked();

    void on_noiseBreak_pushButton_clicked();

    void on_allBreak_pushButton_clicked();

    void on_sinBreak_pushButton_clicked();

    void on_sin_f_SpinBox_editingFinished();

    void on_sin_phase_SpinBox_editingFinished();

    void on_sin_amp_SpinBox_editingFinished();

    void on_sin_peaks_doubleSpinBox_editingFinished();

    void on_sin_peaks_checkBox_clicked();

    void on_sin_f_SpinBox_2_editingFinished();

    void on_sin_phase_SpinBox_2_editingFinished();

    void on_sin_amp_SpinBox_2_editingFinished();

    void on_sin_peaks_doubleSpinBox_2_editingFinished();

    void on_sin_peaks_checkBox_2_clicked();

    void on_sin_positive_checkBox_clicked();

    void on_test_pushButton_clicked();

    void on_test_pushButton_2_clicked();

    void on_noiselist_pushButton_clicked();

    void on_zap_sqr_radioButton_clicked();

    void on_zap_exp_radioButton_clicked();

    void on_zap_lin_radioButton_clicked();

private:
    Ui::MainWindow *ui;
    QCustomPlot * graphWidget;

public:
    int message_counter;

    //plotting
    Heka::TemplateVECTOR data;
    QVector<double> x,y;

    //parameter handling
    enum CreatorTabs {ZapTab = 0, NoiseTab, SinTab, NCreatorTabs};
    std::vector<ParameterSet> parameter;
    ParameterSet last_parameter;
    int last_index;


    enum HEKATabs {Manual = 0, Run, Zap, Resonance, Noise, All, Sin, Settings, NHekaTabs};
    std::vector<ParameterSet> HEKAparameter;

    //Heka communication
    Heka heka;

    bool break_execution;
};

#endif // MAINWINDOW_H
