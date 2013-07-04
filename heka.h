/*****************************************************************************************************************

    HEAK Batch Communication / c++ Interace

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

#ifndef HEKA_H
#define HEKA_H


#include <QString>
#include <QMainWindow>
#include <vector>
#include <fstream>



/*! HEKA class that defines interface to HEKA PAtchmaster via Batch control
 */
class Heka {

public:
    //the batch communication files
    std::fstream batch_command_file;
    std::fstream batch_message_file;
    QString batch_command_file_name;
    QString batch_message_file_name;

    //batch message counter
    int batch_id;

    //time to wait for response messages
    double batch_wait;


    //messages / errors
    QMainWindow* mainWindow;   //todo: generalize to io class here
    void batch_error(const QString& routine, const QString& text);
    void batch_message(const QString& routine, const QString& text);


public:

    // Data Types

    typedef float TemplateTYPE;
    typedef std::vector<TemplateTYPE> TemplateVECTOR;

    /*! Container to store parameter of a HEKA segment
    */
    struct Segment {
        double dur; // segment duration
        double amp; // segment amplitude

        QString toString() const;
    };


    /*! Container to store parameter of a HEKA sequence needed when creatig a new sequence via NewSequence
    */
    struct Sequence {
        QString source;  //source from which to copy sequence
        QString name;    //sequence name
        double interval; //sweep interval
        int trigger , sweepno;
        std::vector<Segment> segment;

        QString toString() const;
    };

    /*! Container to store Sweep Information obtained from HEKA via SweepInfoExt
    */
    struct SweepInfo {
        QString query_status;
        int group, series, sweep;
        //trace info -> could be multiple traces here! -> see HEKA patchmaster manual appendix
        int trace;
        int points;
        double dx;
        double y_factor;
        double y_range;
        double zero;
        int byte_offset;
        int byte_interleave;
        int byte_skip;
        int data_type;
        int endian_type;
        int temp_file;

        QString toString();
    };


    /*! Container to store target information obtained from HEKA via GetTarget
    */
    struct TargetInfo {
        int group, series, sweep, trace, level;

        QString toString();
        QString to_String();
    };


public:

    //Formats and Files
    QString trimm_quotes(const QString& str);
    QString sequence_from_file_name(const QString& file_name);

    QString sequence_to_template_file_name(const QString& sequence, const QString& pgfpath, int channel = 1);
    QString sequence_to_template_file_name(const QString& sequence, const QString& pgfpath, int sweep, int channel);


    /*! read a binary HEKA template file \param fname to vector \param d
    */
    bool read_template_file(const QString& fname, TemplateVECTOR& d);


    /*! write a binary HEKA template file \param fname to vector \param d
    */
    bool write_template_file(const QString& fname, const TemplateVECTOR& d);


    //Heka Batch Communication

    bool open_batch_command_file();
    void close_batch_command_file();
    bool open_batch_message_file();
    void close_batch_message_file();

    bool write_to_batch_command_file(const QString& command);
    bool open_write_to_batch_command_file(const QString& command);
    bool poll_from_batch_message_file(QString& message);
    bool wait_for_batch_message_file(QString& message);
    bool open_wait_for_batch_message_file(QString& message);

    bool check_done(const QString& name);
    bool check_reply(const QString& reply);
    bool check_reply(const QString& name, const QString& reply);

    bool get_target(TargetInfo& tg);
    bool set_target(const TargetInfo& tg);
    bool get_label(QString& label);
    bool get_label(const TargetInfo& tg, QString& label);

    bool execute_sequence(const QString& name);
    bool check_idle();
    bool wait_for_idle(double time, double off = 0);
    bool wait_for_idle(double time, double off, bool* break_execution);

    bool set_comment(const QString& comment);
    bool set_comment(const TargetInfo& tg, const QString& comment);
    bool get_comment(QString& comment);
    bool get_comment(const TargetInfo& tg, QString& comment);

    bool get_data_file_name(QString& filename);
    bool parse_sweep_info(const QString& msg, SweepInfo& sw);
    bool get_sweep_info(SweepInfo& sw);
    bool get_last_recorded_data(std::vector<float>& data);

    bool open_data_file(const QString& file_name, std::fstream& file);
    void close_data_file(std::fstream& file);

    bool delete_sequence(const QString& sequence);
    bool new_sequence(const Sequence& sequence);

    bool break_execution();
    bool terminate();

};

#endif // HEKA_H
