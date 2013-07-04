/*****************************************************************************************************************

    HEAK Batch Communication / c++ Interace

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich

 *****************************************************************************************************************/

#include "heka.h"

#include "debug.h"

#include <QCoreApplication>
#include <QString>
#include <QFileInfo>
#include <QDir>


/*****************************************************************************************************************
 *
 *      Message / Error handling: here we simply retroute messages to our MainWindow
 *
 *****************************************************************************************************************/

#include "mainwindow.h"

//error handling
void Heka::batch_error(const QString& routine, const QString& text){
    DEBUG("batch_error()")
    //DEBUG(routine.toStdString())
    //DEBUG(text.toStdString())

    dynamic_cast<MainWindow*>(mainWindow)->error_message("HEKA Batch Communication Error: " + routine + " error: " + text);
}

void Heka::batch_message(const QString& routine, const QString& text){
    DEBUG("batch_message()")
    //DEBUG(routine.toStdString())
    //DEBUG(text.toStdString())

    dynamic_cast<MainWindow*>(mainWindow)->message("HEKA Batch Communication: " + routine + ": " + text);
}



/*****************************************************************************************************************
 *
 *      Containers
 *
 *****************************************************************************************************************/

QString Heka::Segment::toString() const {
        return QString("%1, %2").arg(dur).arg(amp);
 }


QString Heka::Sequence::toString() const {
        int nseg = segment.size();
        QString str = QString("\"%1\", \"%2\", %3, %4, %5, %6")
                .arg(source).arg(name).arg(interval).arg(trigger)
                .arg(sweepno).arg(nseg);

        for (int i = 0; i < nseg; i++){
            str = QString("%1, %2").arg(str).arg(segment[i].toString());
        }
        return str;
}


QString Heka::SweepInfo::toString() {
        return QString("SweepInfoExt %1, %2_%3_%4; %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16")
                .arg(query_status).arg(group).arg(series).arg(sweep).arg(trace).arg(points).arg(dx).arg(y_factor).arg(y_range)
                .arg(zero).arg(byte_offset).arg(byte_interleave).arg(byte_skip).arg(data_type).arg(endian_type).arg(temp_file);
}


QString Heka::TargetInfo::toString() {
        return QString("%1, %2, %3, %4, %5").arg(group).arg(series).arg(sweep).arg(trace).arg(level);
}

QString Heka::TargetInfo::to_String()  {
        return QString("%1_%2%_%3").arg(group).arg(series).arg(sweep);
}


/*****************************************************************************************************************
 *
 *      HEKA Formats and Files
 *
 *****************************************************************************************************************/

//some simple formatting
QString Heka::trimm_quotes(const QString &str) {
    QString result(str);
    result.chop(1);
    result.remove(0,1);
    return result;
}

QString Heka::sequence_from_file_name(const QString& file_name){
    QFileInfo fi(file_name);
    return fi.baseName();
}

/*
QString sequence_to_file_name(const QString& sequence, const QString& path){
    QFileInfo fi(path, QString("%1.tpl").arg(sequence));
    return fi.absoluteFilePath();
}
*/

QString Heka::sequence_to_template_file_name(const QString& sequence, const QString& path, int channel){
    //QFileInfo fi(path, QString("%1_1_1.tpl").arg(sequence));
    QFileInfo fi(QDir(path), QString("%1/%1_%2.tpl").arg(sequence).arg(channel));
    return fi.absoluteFilePath();
}

QString Heka::sequence_to_template_file_name(const QString& sequence, const QString& path, int sweep, int channel){
    return sequence_to_template_file_name(QString("%1_%2").arg(sequence).arg(sweep), path, channel);
}


//read write template files
bool Heka::read_template_file(const QString& fname, TemplateVECTOR& d) {

   /* open file as binary */

   std::fstream file;
   file.open( fname.toStdString().c_str(), std::ios::in | std::ios::binary );

   if (!file.good()) return false;

   std::streampos pos = file.tellg();
   file.seekg(0, std::ios_base::end);
   std::streampos end = file.tellg();
   file.seekg(pos, std::ios_base::beg);
   int data_size = (end-pos)/sizeof(TemplateTYPE);

   //data.resize(size);
   d.resize(data_size);
   file.read((char *) & d[0] , data_size*sizeof(TemplateTYPE));
   file.close();

   return true;
}


bool Heka::write_template_file(const QString& fname, const TemplateVECTOR& d) {

   /* open file as binary */
   std::fstream file;
   file.open( fname.toStdString().c_str(), std::ios::out | std::ios::binary );
   if (!file.good()) return false;

   /* write data */
   DEBUG("writinig data")
   file.write( (char *) &d[0], d.size() * sizeof(TemplateTYPE) );

   file.close();

   return true;
}



/*****************************************************************************************************************
 *
 *      HEKA patch master batch communication interface
 *
 *****************************************************************************************************************/


//read write from batch file
bool Heka::open_batch_command_file() {
    DEBUG("open_batch_command_file()")
    if (batch_command_file.is_open()) {
        batch_error("open_batch_command_file", "file was open!");
        close_batch_command_file();
    }

    batch_command_file.open(batch_command_file_name.toStdString().c_str(),  std::ios::out);

    return batch_command_file.is_open();
}

void Heka::close_batch_command_file() {
    DEBUG("close_batch_command_file()")
    batch_command_file.close();
}


bool Heka::open_batch_message_file() {
    DEBUG("open_batch_message_file()")
    if (batch_message_file.is_open()) {
        batch_error("open_batch_message_file()", "file was open!");
        close_batch_message_file();
    }

    batch_message_file.open(batch_message_file_name.toStdString().c_str(),  std::ios::in);

    return batch_message_file.is_open();
}

void Heka::close_batch_message_file() {
    DEBUG("close_batch_message_file()")
    batch_message_file.close();
}



bool Heka::open_write_to_batch_command_file(const QString& command) {
    DEBUG("open_write_to_batch_command_file()")
    open_batch_command_file();
    bool suc = write_to_batch_command_file(command);
    close_batch_command_file();
    return suc;
}


bool Heka::write_to_batch_command_file(const QString& command) {
    DEBUG("write_to_batch_command_file")
    if (!batch_command_file.is_open() || !batch_command_file.good()) {
        batch_error("write_to_batch_command_file", "Command batch file not ready !");
        return false;
    }

    //first write full text with first byte = '-'
    QString out = QString("-%1").arg(batch_id);
    batch_command_file.seekp(0);
    batch_command_file.write(out.toStdString().c_str(), out.size());
    if (!batch_command_file.good()) {
        batch_error("write_to_batch_command_file", "Cannot write signature to batch input file!");
        return false;
    }

    out = QString("\n%1\n").arg(command);
    batch_command_file.write( out.toStdString().c_str(), out.size());
    if (!batch_command_file.good()) {
        batch_error("write_to_batch_command_file", "Cannot write text to batch input file!");
        return false;
    }

    //now set first char to '+'
    batch_command_file.seekp(0, std::ios_base::beg);
    batch_command_file.write("+",1);
    if (!batch_command_file.good()) {
        batch_error("write_to_batch_command_file", "Cannot write signature to batch input file!");
        return false;
    }

    // next command id
    batch_id++;

    return true;
}

//single poll
bool Heka::poll_from_batch_message_file(QString& message) {
    DEBUG("poll_from_batch_message_file()")
    // check if batch file is ready for reading

    if (!batch_message_file.good() || !batch_message_file.is_open()) {
        batch_error("poll_from_batch_message_file", "Message batch file not ready!");
        return false;
    }

    //read id
    batch_message_file.sync();
    batch_message_file.seekg(0, std::ios_base::beg);
    char line[256];
    batch_message_file.getline(line, 256);
    if (!batch_message_file.good()) {
        batch_error("poll_from_batch_message_file", "Cannot read signature from batch output file!");
        return false;
    }

    //check id
    QString str(line);
    int bid = str.toInt();
    if (bid < 0 || bid  < batch_id - 1) {
        //this is typical error and anyoing when waiting for response!!
        //batch_error("poll_from_batch_message_file", QString("Bad signature form batch output file! batch_id=%1 found_id=%2").arg(batch_id-1).arg(bid));
        return false;
    }

    // read text until \n
    message = "";
    char c;
    batch_message_file.get(c);
    while (c != '\n') {
        message.append(c);
        batch_message_file.get(c);
    }

    /*
    batch_message_file.seekg (0, std::ios::end);
    int size = batch_message_file.tellg();
    char in[size];
    batch_message_file.seekg (0, std::ios::beg);
    batch_message_file.read(in, size);
    message = QString().fromAscii(in, size);
    */

    batch_message("poll_from_batch_message_file", QString("success reading message id = %1\n message is:\n%2").arg(bid).arg(message));

    if (bid > batch_id - 1) {
        batch_message("poll_from_batch_message_file", QString("synchronizing message ids %1 > %2").arg(bid).arg(batch_id-1));
        batch_id = bid + 1; //bid should be batch_id -1 as batch_id is increased after writing !!

        //try to resynchronize ids
        write_to_batch_command_file(QString("acknowledged"));
    }

    return true;
}

bool Heka::wait_for_batch_message_file(QString& msg){
    DEBUG("wait_for_batch_message_file()")
    bool suc = false;
    QTime t;
    t.start();
    int i =0;
    while (!suc && t.elapsed() <  1000 * batch_wait ) {
            suc = poll_from_batch_message_file(msg);
            i++;
    }

    //batch_message("wait_for_batch_message_file", QString("time elapsed: %1 i=%2").arg(t.elapsed()).arg(i));

    if (!suc) {
        batch_error("wait_for_batch_message_file", "waiting to read from message file failed!");
    }

    return suc;
}

bool Heka::open_wait_for_batch_message_file(QString& msg){
    DEBUG("open_wait_for_batch_message_file")
    bool suc = false;
    QTime t;
    t.start();
    int i =0;
    while (!suc && t.elapsed() <  1000 * batch_wait ) {
            open_batch_message_file();
            suc = poll_from_batch_message_file(msg);
            close_batch_message_file();
            i++;
    }

    //batch_message("open_wait_for_batch_message_file", QString("time elapsed: %1 i=%2").arg(t.elapsed()).arg(i));

    if (!suc) {
        batch_error("open_wait_for_batch_message_file", "waiting to read from message file failed!");
    }

    return suc;
}



bool Heka::check_reply(const QString& reply){
    QString msg;
    bool suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    if (msg != reply) {
        batch_error("check_done", QString("Invalid reply: %1 != %2").arg(msg).arg(reply));
        return false;
    }

    return true;
}

bool Heka::check_reply(const QString& name, const QString& reply){
    QString msg;
    bool suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;
    QStringList list = msg.split(QRegExp("(\\n|\\s)"), QString::SkipEmptyParts);

    if (list.size() != 2) {
        batch_error("check_done", QString("Inconsistent size: 2 != %1").arg(list.size()));
        return false;
    }

    if (list[0] != name) {
        batch_error("check_done", QString("Invalid name: %1 != %2").arg(name).arg(list[0]));
        return false;
    }

    if (list[1] != reply) {
        batch_error("check_done", QString("Did not find done but: %1").arg(list[1]));
        return false;
    }

    return true;
}



bool Heka::check_done(const QString& name) {
    return check_reply(name, "Done");
}


bool Heka::check_idle(){
    //DEBUG("check_idle()")

    bool suc = open_write_to_batch_command_file("Query");
    if (!suc) return false;

    QString msg;
    suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    QStringList list = msg.split(QRegExp("(\\n|_|\\s)"), QString::SkipEmptyParts);

    if (list.size() != 2) {
        batch_error("wait_for_idle", QString("Inconsistent size: 2 != %1").arg(list.size()));
        return false;
    }

    if (list[0] != "Query") {
        batch_error("wait_for_idle", QString("Invalid name: Query != %2").arg(list[0]));
        return false;
    }

    if (list[1] != "Idle") {
        //batch_error("wait_for_idle", QString("HEKA busy: %1").arg(list[1]));
        return false;
    }

    return true;
}

bool Heka::wait_for_idle(double time, double off) {
    bool break_execution = false;
    return wait_for_idle(time, off, &break_execution);
}

bool Heka::wait_for_idle(double time, double off, bool* break_execution) {
    DEBUG("wait_for_idle")
    QTime t;
    t.start();
    while (!(*break_execution) && t.elapsed() < off * 1000) {
         QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    bool suc = false;
    while (!suc && !(*break_execution) && t.elapsed() < time * 1000){
        suc = check_idle();
        if (!suc) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    return suc;
}




//direct implementation of some commands

bool Heka::execute_sequence(const QString& name){
    bool suc = open_write_to_batch_command_file(QString("ExecuteSequence %1").arg(name));
    if (!suc) return false;
    return check_done("ExecuteSequence");
}


bool Heka::get_target(TargetInfo& tg){
    DEBUG("get_target()")
    bool suc = open_write_to_batch_command_file("GetTarget");
    if (!suc) return false;

    //read respone
    QString msg;
    suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    //parse response
    QStringList list = msg.split(QRegExp("(\\n|\\s|,)"), QString::SkipEmptyParts);
    if (list.size() != 6) {
        batch_error("get_target", QString("inconsistent target size: 6 != %1").arg(list.size()));
        batch_error("get_target", QString("list is: %1").arg(list.join(" ~ ")));
        return false;
    }

    if (list[0] != "GetTarget") {
        batch_error("get_target", QString("Invalid Response: %1").arg(list[0]));
        return false;
    }

    bool ok;
    tg.group = list[1].toInt(&ok);
    if (!ok) { batch_error("get_target", QString("cannot read group: %1").arg(list[1])); return false; }

    tg.series = list[2].toInt(&ok);
    if (!ok) { batch_error("get_target", QString("cannot read series: %1").arg(list[2])); return false; }

    tg.sweep = list[3].toInt(&ok);
    if (!ok) { batch_error("get_target", QString("cannot sweep group: %1").arg(list[3])); return false; }

    tg.trace = list[4].toInt(&ok);
    if (!ok) { batch_error("get_target", QString("cannot read trace: %1").arg(list[4])); return false; }

    tg.level = list[5].toInt(&ok);
    if (!ok) { batch_error("get_target", QString("cannot read level: %1").arg(list[5])); return false; }

    return true;
}


bool Heka::set_target(const TargetInfo& tg) {
    DEBUG("set_trace()")
    bool suc = open_write_to_batch_command_file(QString("SetTarget %1,%2,%3,%4,%5,FALSE,FALSE").arg(tg.group).arg(tg.series).arg(tg.sweep).arg(tg.trace).arg(tg.level));
    if (!suc) return false;
    return check_done("SetTarget");
}


bool Heka::get_label(QString& label){
    DEBUG("get_label()")

    //get actual target
    TargetInfo tg;
    bool suc = get_target(tg);
    if (!suc) return false;

    //label is typically only accessible on series level -> for automatic retrieval choose level=2
    tg.level = 2;

    return get_label(tg, label);
}

bool Heka::get_label(const TargetInfo& tg, QString& label){
    DEBUG("get_label(ints)")
    bool suc = open_write_to_batch_command_file(QString("GetLabel %1, %2, %3, %4, %5")
                                                .arg(tg.group).arg(tg.series).arg(tg.sweep).arg(tg.trace).arg(tg.level));
    if (!suc) return false;

    QString msg;
    suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    QStringList list = msg.split(QRegExp("(\\n|\\s)"), QString::SkipEmptyParts);

    if (list.size() != 2) {
        batch_error("get_label", QString("Inconsistent size: 2 != %1").arg(list.size()));
        return false;
    }

    if (list[0] != "GetLabel") {
        batch_error("get_label", QString("Invalid name: %1").arg(list[0]));
        return false;
    }

    //remove "'s
    label = trimm_quotes(list[1]);
    return true;
}




bool Heka::set_comment(const QString& comment) {
    TargetInfo tg;
    bool suc = get_target(tg);
    if (!suc) {
        batch_error("set_comment", QString("target not found! could not set comment to %1").arg(comment));
        return false;
    }
    tg.level = 2; //this level is only working !!!
    return set_comment(tg, comment);
}

bool Heka::set_comment(const TargetInfo& tg, const QString& comment) {
    bool suc = open_write_to_batch_command_file(QString("SetComment %1,%2,%3,%4,%5, \"%6\"").arg(tg.group).arg(tg.series).arg(tg.sweep).arg(tg.trace).arg(tg.level).arg(comment));
    if (!suc) {
        batch_error("set_comment", QString("could not set comment to %1").arg(comment));
        return false;
    }
    batch_message("set_comment", QString("set comment to %1").arg(comment));
    return check_done("SetComment");
}

bool Heka::get_comment(QString& comment) {
    TargetInfo tg;
    bool suc = get_target(tg);
    if (!suc) return false;

    tg.level = 2; //this level is wonly working !!!
    return get_comment(tg, comment);
}

bool Heka::get_comment(const TargetInfo& tg, QString& comment) {
    bool suc = open_write_to_batch_command_file(QString("GetComment %1,%2,%3,%4,%5, \"%6\"").arg(tg.group).arg(tg.series).arg(tg.sweep).arg(tg.trace).arg(comment));
    if (!suc) return false;

    //parse comment
    QString msg;
    suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    msg = msg.trimmed();

    if (msg.left(10) != "GetComment") {
        batch_error("get_comment", QString("Invalid Response: %1").arg(msg.left(10)));
        return false;
    }

    msg = msg.remove(0,10).trimmed();
    comment = trimm_quotes(msg);
    return true;
}


//given message parse sweep info
bool Heka::parse_sweep_info(const QString& msg, SweepInfo& sw) {

    QStringList list = msg.split(QRegExp("(\\n|\\_|;|,|\\s)(\\s)*"), QString::SkipEmptyParts);
    DEBUG(list.join(" ~ ").toStdString())

    //length should be: 18
    if (list.size() != 18) {
        batch_error("parse_sweep_info", QString("Invalid SweepInfo has invalid size: 18 != %1").arg(list.size()));
        return false;
    }

    if (list[0] != "SweepInfoExt") {
        batch_error("parse_sweep_info", QString("Invalid Response: %1").arg(list[0]));
        return false;
    }

    //query status is split into two due to "_"
    sw.query_status = list[1] + "_" + list[2];

    // now convert to numbers
    bool ok;
    sw.group = list[3].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read group: %1").arg(list[3])); return false; }

    sw.series = list[4].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read series: %1").arg(list[4])); return false; }

    sw.sweep = list[5].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read sweep: %1").arg(list[5])); return false; }

    sw.trace = list[6].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read trace: %1").arg(list[6])); return false; }

    sw.points = list[7].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read points: %1").arg(list[7])); return false; }

    sw.dx = list[8].toDouble(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read dx: %1").arg(list[8])); return false; }

    sw.y_factor = list[9].toDouble(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read y_factor: %1").arg(list[9])); return false; }

    sw.y_range = list[10].toDouble(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read y_range: %1").arg(list[10])); return false; }

    sw.zero = list[11].toDouble(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read zero: %1").arg(list[11])); return false; }

    sw.byte_offset = list[12].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read byte_offset: %1").arg(list[12])); return false; }

    sw.byte_interleave = list[13].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read byte_interleave: %1").arg(list[13])); return false; }

    sw.byte_skip = list[14].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read byte_skip: %1").arg(list[14])); return false; }

    sw.data_type = list[15].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read data_type: %1").arg(list[15])); return false; }

    sw.endian_type = list[16].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read endian_type: %1").arg(list[16])); return false; }

    sw.temp_file = list[17].toInt(&ok);
    if (!ok) {batch_error("parse_sweep_info", QString("cannot read temp_file: %1").arg(list[17])); return false; }

    return true;
}


bool Heka::get_sweep_info(SweepInfo& sw) {
    DEBUG("get_sweep_info")
    bool suc = open_write_to_batch_command_file("SweepInfoExt");
    if (!suc) return false;

    QString msg;
    suc = open_wait_for_batch_message_file(msg);
    if (!suc) return false;

    //msg = "SweepInfoExt Query_Idle, 1_8_3; 1, 1360400, 2.500000E-05, 3.125000E-05, 1.023969E+00, 0.000000E+00, 17981056, 0, 0, 0, 1, 0";

    //parse answer
    suc = parse_sweep_info(msg, sw);

    return suc;
}


// get actual data file name
bool Heka::get_data_file_name(QString& filename){
    DEBUG("get_data_file_name")
    //querry file name from HEKA
    bool suc = open_write_to_batch_command_file("GetParameters DataFile");
    if (!suc) return false;

    //wait for response
    QString resp;
    suc = open_wait_for_batch_message_file(resp);
    if (!suc) return false;

    //extract message
    QTextStream str(resp.toStdString().c_str());

    //read response
    str >> resp;
    if (resp != "GetParameters") {
        batch_error("get_file_name", QString("unexspected response: %1").arg(resp));
        return false;
    }

    //now read file name
    str >> resp;
    batch_message("get_file_name", QString("data file name is: %1").arg(resp));

    //remove "'s
    filename = trimm_quotes(resp);

    return true;
}


bool Heka::open_data_file(const QString& file_name, std::fstream& file) {
    file.open(file_name.toStdString().c_str(), std::ios::in | std::ios::binary );
    if (!file.is_open()) {
        batch_error("open_data_file", QString("cannot open data file: %1").arg(file_name));
        file.close();
        return false;
    }
    return true;
}

void Heka::close_data_file(std::fstream& file) {
    file.close();
}

bool Heka::get_last_recorded_data(DataVECTOR& data) {
    QString data_file_name;
    bool suc = get_data_file_name(data_file_name);
    batch_message("get_last_recorded_data", QString("data file name is: %1").arg(data_file_name));
    if (!suc) return false;


    //get sweep info
    SweepInfo sw;
    suc = get_sweep_info(sw);
    if (!suc) return false;

    //check status
    if (sw.query_status != "Query_Idle") {
        batch_error("access_last_recorded_data", QString("HEKA is not idle: %1").arg(sw.query_status));
        return false;
    }

    //check all other conditions
    if (sw.temp_file != 0 || (sw.byte_interleave != 0) || (sw.byte_skip != 0) || (sw.endian_type != 1) || sw.data_type != 0) {
        batch_error("access_last_recorded_data", QString("inconsistent data: %1").arg(sw.toString()));
        return false;
    }

    //now we try to acces data
    std::fstream file;
    suc = open_data_file(data_file_name, file);
    if (!suc) return false;

    //read data
    file.seekg(sw.byte_offset, std::ios_base::beg);

    //data is in form of int32 so we have to convert
    std::vector<short> d;
    d.resize(sw.points);
    file.read((char*) & d[0], sw.points * sizeof(short));

    data.resize(sw.points);
    for (int i=0; i < int(d.size()); i++){
        //data[i] = d[i] * sw.y_factor
        data[i] = d[i]; // for our analysis it does not matter -> skip mult here !!
    }

    close_data_file(file);
    return true;
}


bool Heka::delete_sequence(const QString& sequence){
    bool suc = open_write_to_batch_command_file(QString("DeleteSequence %1").arg(sequence));
    if (!suc) return false;
    return check_done("DeleteSequence");
}


bool Heka::new_sequence(const Sequence& sequence){
    bool suc = open_write_to_batch_command_file(QString("NewSequence %1").arg(sequence.toString()));
    if (!suc) {
        batch_error("new_sequence", QString("could not create new sequence %1").arg(sequence.toString()));
        return false;
    }
    return check_done("NewSequence");
}




bool Heka::break_execution() {
    bool suc = open_write_to_batch_command_file("Set N Break True");
    if (!suc) {
        batch_error("break", QString("could not set break to true!"));
        return false;
    }

    suc = open_write_to_batch_command_file("Set N Break False");
    if (!suc) {
        batch_error("break", QString("could not set break to false!"));
        return false;
    }

    return true;
}


//unclear what this does
bool Heka::terminate() {
    bool suc = open_write_to_batch_command_file("Terminate");
    if (!suc) {
        batch_error("terminate", QString("could not termiante!"));
        return false;
    }
    return check_reply("Terminated");
}

