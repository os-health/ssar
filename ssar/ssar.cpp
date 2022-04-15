/*
 *
 *  Copyright (c) 2021 Alibaba Inc.
 *  SRESAR is licensed under Mulan PSL v2.
 *  You can use this software according to the terms and conditions of the Mulan PSL v2.
 *  You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 *  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 *  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 *  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *  See the Mulan PSL v2 for more details.
 *
 *  Part of SRESAR (Site Reliability Engineering System Activity Reporter)
 *  
 *  Author:  Miles Wen <mileswen@linux.alibaba.com>
 *           Xlpang   <xlpang@linux.alibaba.com>
 *
 * 
 */



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <dirent.h>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/utsname.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utmpx.h>
#include <vector>

#include "gzstream.hpp"
#include "CLI11.hpp"
#include "json.hpp"
#include "cpptoml.h"
#include "ssar.h"

using namespace cpptoml;
using namespace std;
using json = nlohmann::json;

#define VERSION "1.0.2"
#define AREA_LINE 20
const string sresar_suffix = "sresar25";
const string cmdline_suffix = "cmdline";

#define SSTREAM_IGNORE(ss,n) for(int ss_i = 0;ss_i < (n); ss_i++){  \
            if((ss).peek() == ' '){            \
                (ss).ignore();                 \
                while((ss).peek() != ' '){     \
                    (ss).ignore();             \
                }                              \
            }                                  \
        }                                      

#define KEY_COMPARE(field)                                   \
    }else if (#field == key.first && "-" == key.second ) {   \
        if(itema.field > itemb.field){                       \
            ret = 1;                                         \
        }else if(itema.field == itemb.field){                \
            ret = 0;                                         \
        }else{                                               \
            ret = -1;                                        \
        }                                                    \
    }else if (#field == key.first && "+" == key.second) {    \
        if(itema.field < itemb.field){                       \
            ret = 1;                                         \
        }else if(itema.field == itemb.field){                \
            ret = 0;                                         \
        }else{                                               \
            ret = -1;                                        \
        }

#define TIME_KEY_COMPARE(field1, field2)                                        \
    }else if (#field1 == key.first && "-" == key.second ) {                     \
        if(itema.field2 > itemb.field2){                                        \
            ret = 1;                                                            \
        }else if(itema.field2 == itemb.field2){                                 \
            ret = 0;                                                            \
        }else{                                                                  \
            ret = -1;                                                           \
        }                                                                       \
    }else if (#field1 == key.first && "+" == key.second) {                      \
        if(itema.field2 < itemb.field2){                                        \
            ret = 1;                                                            \
        }else if(itema.field2 == itemb.field2){                                 \
            ret = 0;                                                            \
        }else{                                                                  \
            ret = -1;                                                           \
        }

#define FORMAT2JSON(field)                                                      \
    }else if (#field == (*j_element)) {                                         \
        stringstream ss_json_field;                                             \
        if(1012 == (*i_element).record_type){                                   \
            pollute = true;                                                     \
            ss_json_field << "<";                                               \
        }else if(1013 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            ss_json_field << ">";                                               \
        }else if(1003 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            ss_json_field << "-";                                               \
        }else if(1004 == (*i_element).record_type){                             \
            if("scope" == field_attribute.at((*j_element)).at("unit")){         \
                pollute = true;                                                 \
                ss_json_field << "-";                                           \
            }else{                                                              \
                ss_json_field << (*i_element).field;                            \
            }                                                                   \
        }else{                                                                  \
            ss_json_field << (*i_element).field;                                \
        }                                                                       \
        obj_json[#field] = ss_json_field.str(); 

#define FORMAT_PERCENT2JSON(field)                                              \
    }else if (#field == (*j_element)) {                                         \
        stringstream ss_json_field;                                             \
        if(1012 == (*i_element).record_type){                                   \
            pollute = true;                                                     \
            ss_json_field << "<";                                               \
        }else if(1013 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            ss_json_field << ">";                                               \
        }else if(1003 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            ss_json_field << "-";                                               \
        }else if(1004 == (*i_element).record_type){                             \
            if("scope" == field_attribute.at((*j_element)).at("unit")){         \
                pollute = true;                                                 \
                ss_json_field << "-";                                           \
            }else{                                                              \
                ss_json_field << fixed;                                         \
                ss_json_field << setprecision(1);                               \
                ss_json_field << 100 * (*i_element).field;                      \
            }                                                                   \
        }else{                                                                  \
            ss_json_field << fixed;                                             \
            ss_json_field << setprecision(1);                                   \
            ss_json_field << 100 * (*i_element).field;                          \
        }                                                                       \
        obj_json[#field] = ss_json_field.str();

#define FORMAT2SHELL(field)                                                     \
    }else if (#field == (*j_element)){                                          \
        it_shell << setw(stoi(field_attribute.at((*j_element)).at("f_width"))); \
        string it_adjust = field_attribute.at((*j_element)).at("adjust");       \
        it_shell << resetiosflags(ios::adjustfield);                            \
        if("left" == it_adjust){                                                \
            it_shell << setiosflags(ios::left);                                 \
        }else{                                                                  \
            it_shell << setiosflags(ios::right);                                \
        }                                                                       \
        if(1012 == (*i_element).record_type){                                   \
            pollute = true;                                                     \
            it_shell << "<";                                                    \
        }else if(1013 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            it_shell << ">";                                                    \
        }else if(1003 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            it_shell << "-";                                                    \
        }else if(1004 == (*i_element).record_type){                             \
            if("scope" == field_attribute.at((*j_element)).at("unit")){         \
                pollute = true;                                                 \
                it_shell << "-";                                                \
            }else{                                                              \
                it_shell << (*i_element).field;                                 \
            }                                                                   \
        }else{                                                                  \
            it_shell << (*i_element).field;                                     \
        }

#define FORMAT_PERCENT2SHELL(field)                                             \
    }else if (#field == (*j_element)){                                          \
        string it_adjust = field_attribute.at((*j_element)).at("adjust");       \
        it_shell << resetiosflags(ios::adjustfield);                            \
        if("left" == it_adjust){                                                \
            it_shell << setiosflags(ios::left);                                 \
        }else{                                                                  \
            it_shell << setiosflags(ios::right);                                \
        }                                                                       \
        if(1012 == (*i_element).record_type){                                   \
            pollute = true;                                                     \
            it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
            it_shell << "<";                                                    \
        }else if(1013 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
            it_shell << ">";                                                    \
        }else if(1003 == (*i_element).record_type){                             \
            pollute = true;                                                     \
            it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
            it_shell << "-";                                                    \
        }else if(1004 == (*i_element).record_type){                             \
            if("scope" == field_attribute.at((*j_element)).at("unit")){         \
                pollute = true;                                                 \
                it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
                it_shell << "-";                                                \
            }else{                                                              \
                it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
                it_shell << fixed;                                              \
                it_shell << setprecision(1);                                    \
                it_shell << 100 * (*i_element).field;                           \
            }                                                                   \
        }else{                                                                  \
            it_shell <<setw(stoi(field_attribute.at((*j_element)).at("f_width")));\
            it_shell << fixed;                                                  \
            it_shell << setprecision(1);                                        \
            it_shell << 100 * (*i_element).field;                               \
        }

#define FORMAT_LOAD5S2SHELL(field)                                                     \
    }else if (#field == (*j_element)){                                                 \
        it_shell << setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width"))); \
        string it_adjust = load5s_field_attribute.at((*j_element)).at("adjust");       \
        it_shell << resetiosflags(ios::adjustfield);                                   \
        if("left" == it_adjust){                                                       \
            it_shell << setiosflags(ios::left);                                        \
        }else{                                                                         \
            it_shell << setiosflags(ios::right);                                       \
        }                                                                              \
        if(load5s_field_attribute.at((*j_element)).at("zoom_state") == "0" && (*i_element).zoom_state == 0){ \
            it_shell << "-";                                                           \
        }else{                                                                         \
            it_shell << (*i_element).field;                                            \
        }

#define FORMAT_LOAD5S_PERCENT2SHELL(field)                                                           \
    }else if (#field == (*j_element)){                                                               \
        string it_adjust = load5s_field_attribute.at((*j_element)).at("adjust");                     \
        it_shell << resetiosflags(ios::adjustfield);                                                 \
        if("left" == it_adjust){                                                                     \
            it_shell << setiosflags(ios::left);                                                      \
        }else{                                                                                       \
            it_shell << setiosflags(ios::right);                                                     \
        }                                                                                            \
        if(load5s_field_attribute.at((*j_element)).at("zoom_state") == "0" && (*i_element).zoom_state == 0){ \
            it_shell <<setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width")));            \
            it_shell << "-";                                                                         \
        }else{                                                                                       \
            it_shell <<setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width"))-1);          \
            it_shell << fixed;                                                                       \
            it_shell << setprecision(1);                                                             \
            it_shell << 100 * (*i_element).field << "%";                                             \
        }

#define FORMAT_LOAD5S2JSON(field)                                                                    \
    }else if (#field == (*j_element)){                                                               \
        stringstream ss_json_field;                                                                  \
        if(load5s_field_attribute.at((*j_element)).at("zoom_state") == "0" && (*i_element).zoom_state == 0){ \
            ss_json_field << "-";                                                                    \
        }else{                                                                                       \
            ss_json_field << (*i_element).field;                                                     \
        }                                                                                            \
        it_json[i][#field] = ss_json_field.str();

#define FORMAT_LOAD5S_PERCENT2JSON(field)                                                            \
    }else if (#field == (*j_element)){                                                               \
        stringstream ss_json_field;                                                                  \
        if(load5s_field_attribute.at((*j_element)).at("zoom_state") == "0" && (*i_element).zoom_state == 0){ \
            ss_json_field << "-";                                                                    \
        }else{                                                                                       \
            ss_json_field << fixed;                                                                  \
            ss_json_field << setprecision(1);                                                        \
            ss_json_field << 100 * (*i_element).field << "%";                                        \
        }                                                                                            \
        it_json[i][#field] = ss_json_field.str();

#define FORMAT_LOAD2P2SHELL(field)                                                                   \
    }else if (#field == (*j_element)){                                                               \
        it_shell << setw(stoi(load2p_field_attribute.at((*j_element)).at("f_width")));               \
        string it_adjust = load2p_field_attribute.at((*j_element)).at("adjust");                     \
        it_shell << resetiosflags(ios::adjustfield);                                                 \
        if("left" == it_adjust){                                                                     \
            it_shell << setiosflags(ios::left);                                                      \
        }else{                                                                                       \
            it_shell << setiosflags(ios::right);                                                     \
        }                                                                                            \
        it_shell << (*i_element).field;

#define FORMAT_LOAD2P2JSON(field)                                                                    \
    }else if (#field == (*j_element)){                                                               \
        stringstream ss_json_field;                                                                  \
        ss_json_field << (*i_element).field;                                                         \
        it_json[i][#field] = ss_json_field.str();


/**********************************************************************************
 *                                                                                *
 *                               common part                                      * 
 *                                                                                *
 **********************************************************************************/

// validate  1234
bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// validate 1234 -1234 12.34 -12.34 0.34 12/34 12cd 
bool is_float(const std::string &s) {
    if(s.empty()){
        return 0;
    }

    long double ld;
    try {
        ld = stold(s);                                                 // string to long double
    }catch (std::invalid_argument&){
        //cout << "Invalid_argument" << endl;
        return 0;
    }catch (std::out_of_range&){
        cout << "Out of range" << endl;
        return 0;
    }catch (...) {
        cout << "Something else" << endl;
        return 0;
    }

    return 1;
}

int ParserDatetime(string &fmt_datetime, chrono::system_clock::time_point &time_point, const string &format = "%Y-%m-%dT%H:%M:%S"){
    struct tm it_tm;
    int ret;
    memset(&it_tm, 0, sizeof(struct tm));
    if(NULL == strptime(fmt_datetime.c_str(), format.c_str(), &it_tm)){
        return -1;
    }
    time_point = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(mktime(&it_tm)));
    return 0; 
}

int DetectDatetime(string &fmt_datetime, chrono::system_clock::time_point &time_point){
    int it_size = fmt_datetime.size();
    string it_format;

    if(19 == it_size){
       it_format = "%Y-%m-%dT%H:%M:%S";
    }else if(14 == it_size){
       it_format = "%Y%m%d%H%M%S";
    }else if(12 == it_size){
       it_format = "%Y%m%d%H%M";
    }else if(10 == it_size){
       it_format = "%Y%m%d%H";
    }else if(8 == it_size){
       it_format = "%Y%m%d";
    }else{
        return -1;
    }

    return ParserDatetime(fmt_datetime, time_point, it_format); 
}

string FmtDatetime(const chrono::system_clock::time_point &time_point, const string &format = "%Y-%m-%dT%H:%M:%S"){
    string     fmt_datetime;
    struct tm *tm;
    char       it_datetime[80];

    time_t tt = chrono::system_clock::to_time_t(time_point);
    tm = localtime(&tt);
    strftime(it_datetime, 80, format.c_str(), tm);              // %Y-%m-%dT%H:%M:%S
    fmt_datetime = it_datetime;

    return fmt_datetime;
}

string FmtTime(unsigned long long time){
    int dd,hh,mm,ss;
    stringstream fmt_time;

    ss = time%60;
    time /= 60;
    mm = time%60;
    time /= 60;
    hh = time%24;
    time /= 24;
    dd = time;
   
    if(dd){
        fmt_time << to_string(dd) << "T" << setw(2) << setfill('0') << to_string(hh) << ":" << setw(2) << to_string(mm) << ":" << setw(2) << to_string(ss);
    }else{
        fmt_time << to_string(hh) << ":" << setw(2) << setfill('0') << to_string(mm) << ":" << setw(2) << to_string(ss);
    }

    return fmt_time.str();
}

void RemoveDuplicateListElement(list<string> &input){
    set<string> local_set = {};
    list<string> local_input = {};
    list<string>::const_iterator i_element = input.cbegin();
    for(;i_element != input.cend(); ++i_element){
        local_set.insert(*i_element);
    }
    i_element = input.cbegin();
    for(;i_element != input.cend(); ++i_element){
        if(local_set.cend() != local_set.find(*i_element)){
            local_input.push_back(*i_element);
            local_set.erase(*i_element);
        }
    }
    input.assign(local_input.begin(),local_input.end());
}

void RemoveDuplicateVectorElement(vector<string> &input){
    set<string> local_set = {};
    vector<string> local_input = {};
    vector<string>::const_iterator i_element = input.cbegin();
    for(;i_element != input.cend(); ++i_element){
        local_set.insert(*i_element);
    }
    i_element = input.cbegin();
    for(;i_element != input.cend(); ++i_element){
        if(local_set.cend() != local_set.find(*i_element)){
            local_input.push_back(*i_element);
            local_set.erase(*i_element);
        }
    }
    input.assign(local_input.begin(),local_input.end());
}

int ReadRebootTimes(vector<chrono::system_clock::time_point>* &reboot_times){
    struct utmpx *ut;

    if(utmpxname("/var/log/wtmp") == -1){
        return -1;
    }
    setutxent();
    while((ut = getutxent()) != NULL){
        if(ut->ut_type != BOOT_TIME){
            continue;
        }
        reboot_times->push_back(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(ut->ut_tv.tv_sec)));
    }
    endutxent();
    sort(reboot_times->begin(), reboot_times->end());

    return 0;
}
 
string GetFilePath(chrono::system_clock::time_point it_time_point, string data_path, string suffix){
    string it_datetime_hour  = FmtDatetime(it_time_point, "%Y%m%d%H");
    string it_datetime_point = FmtDatetime(it_time_point, "%Y%m%d%H%M%S");
    string it_path = data_path + it_datetime_hour + "/" + it_datetime_point + "_" + suffix;  
    return it_path;
}

list<int> ParseParam(string &param){
    string delimiter = ",";
    size_t pos_start = 0, pos_end;
    string token;
    list<string> res;

    while((pos_end = param.find(delimiter, pos_start)) != string::npos) {
        token = param.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + 1;
        res.push_back(token);
    }
    res.push_back(param.substr(pos_start));

    list<int> ret;
    string it_part;
    string begin_part,end_part;
    size_t it_end;
    int num;
    int i;
    auto i_element = res.cbegin();
    for(;i_element != res.cend(); ++i_element){
        it_part = (*i_element);
        if(it_part.empty()){
            continue;
        }
        num = count(it_part.begin(), it_part.end(), '-');
        if(num > 1){
            continue;
        }else if(num == 1){
            it_end     = it_part.find("-", 0);
            begin_part = it_part.substr(0, it_end);
            end_part   = it_part.substr(it_end + 1);

            if (!is_number(begin_part) || !is_number(end_part)){
                continue;
            }
            if(stoi(begin_part) >= stoi(end_part)){
                continue;
            }
            for(i = stoi(begin_part); i < stoi(end_part) + 1; i++){
                ret.push_back(i);
            }
        }else{
            if(is_number(it_part)){
                ret.push_back(stoi(it_part));
            }else{
                continue;
            }
        }
    }   
    ret.sort(); 
    ret.unique(); 

    return ret;
}

/**********************************************************************************
 *                                                                                *
 *                               class part                                       * 
 *                                                                                *
 **********************************************************************************/

/////////////////////////////////////////// sreproc class ////////////////////////////////////////////

int SreProc::GetDirLists(string &path, set<string> &dirs){
    DIR           *dirp;
    struct dirent *dp;
    string dp_d_name;
    dirp = opendir(path.c_str());
    if(dirp == NULL){ 
        return -1; 
    }
    while((dp = readdir(dirp)) != NULL){
        dp_d_name = dp->d_name;
        if(!(dp_d_name == "." || dp_d_name == "..")){
            dirs.insert(dp_d_name);
        }
    }
    closedir(dirp);
    if(dirs.empty()){
        return -2; 
    }

    return 0;
}

int SreProc::GetDataDir(){
    string base_path = this->data_path;
    bool geted = false;

    this->data_dir = FmtDatetime(this->time_point, this->dir_format);
    for(;;){
        set<string>::iterator it_find_set = this->dirs.find(this->data_dir);
        if(it_find_set == this->dirs.end()){
            if(!geted && 0 == this->GetDirLists(base_path, this->dirs)){
                geted = true;
                continue;
            }else{
                return -1; 
            }
        }else{
            break;
        }
    }

    return 0;
}

int SreProc::GetDataFile(){
    string it_minute;
    string sub_dir;
    string sub_file = "";
    string it_file  = "";
    bool   geted    = false;
    time_t tt;

    it_minute = FmtDatetime(this->time_point, this->file_format_minute);
    sub_dir  += this->data_path;
    sub_dir  += this->data_dir;
    for(;;){
        set<string>::const_iterator i_element = this->files.cbegin();
        for(;i_element != this->files.cend(); ++i_element){
            sub_file = (*i_element);
            if((sub_file.size() == this->suffix.size() + 15) && 0 == sub_file.find(it_minute, 0) && 14 == sub_file.rfind("_" + this->suffix, 14)){
                it_file = sub_file;
                break;
            }
        }
        if(it_file.empty()){
            if(!geted && 0 == this->GetDirLists(sub_dir, this->files)){
                geted = true;
                continue;
            }else{
                return -1; 
            }
        }else{
            break;
        }
    }
    this->data_datetime = it_file.substr(0, 14);
    if(0 != ParserDatetime(this->data_datetime, this->time_point_real, this->file_format_second)){
        return 1;
    }
    tt = chrono::system_clock::to_time_t(this->time_point_real);
    this->data_timestamp = tt;

    return 0;
}

int SreProc::_MakeDataHourPath(const chrono::system_clock::time_point &it_time_point){
    this->time_point = it_time_point;

    if(0 > this->GetDataDir()){
        return 2;
    }
    if(this->GetDataFile() < 0){
        return 1;
    }
    this->data_hour_path  = this->data_path;
    this->data_hour_path += this->data_dir;
    this->data_hour_path += "/";
    this->data_hour_path += this->data_datetime;
    this->data_hour_path += "_";
    this->data_hour_path += this->suffix;

    return 0;
}

int SreProc::MakeDataHourPath(const chrono::system_clock::time_point &it_time_point){
    int ret_data_hour_path = 0;
    ret_data_hour_path = this->_MakeDataHourPath(it_time_point);
    if((0 != ret_data_hour_path)){
        if(!this->adjust){
            return ret_data_hour_path;
        }
        chrono::system_clock::time_point it_time_point_adjust;
        if(this->adjust_forward){
            it_time_point_adjust = it_time_point + chrono::duration<int>(this->adjust_unit);
        }else{
            it_time_point_adjust = it_time_point - chrono::duration<int>(this->adjust_unit);
        }
        ret_data_hour_path = this->_MakeDataHourPath(it_time_point_adjust);
        if(0 != ret_data_hour_path){
            return ret_data_hour_path;
        }
        this->adjusted = true;
    }
    return 0;
}

int SreProc::MakeDataHours(string data_path){
    if(0 != this->GetDirLists(data_path, this->dirs)){
        return -1;
    }

    return 0;
}

/////////////////////////////////////////// load5s class ////////////////////////////////////////////

int Load5s::GetActiveCounts(int collect_time, active_t &act){
    chrono::system_clock::time_point it_time_point = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(collect_time));
    
    string it_path = GetFilePath(it_time_point, this->data_path, this->suffix); 
    ifstream it_file;
    it_file.open(it_path, ios_base::in);
    if(!it_file.is_open()){
        chrono::system_clock::time_point adjust_time_point = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(collect_time + 1));        // adjust 1s.
        it_path = GetFilePath(adjust_time_point, this->data_path, this->suffix);
        it_file.open(it_path, ios_base::in);
        if(!it_file.is_open()){
            return -1;
        }
    }
    string file_contents;
    act.actr = 0;
    act.actd = 0;
    string it_state;
    while(it_file.good()){
        getline(it_file, file_contents);
        if(file_contents.empty()){
            break;
        }
        istringstream ifile_contents(file_contents);
        ifile_contents >> it_state;
        if("R" == it_state){
            act.actr++;
        }else if("D" == it_state){
            act.actd++;
        }else{
            return -1;
        }
    }
    it_file.close();
  
    return 0;
}

/**********************************************************************************
 *                                                                                *
 *                               init part                                        * 
 *                                                                                *
 **********************************************************************************/

void InitOptions(SeqOptions &seq_option){
    seq_option.ssar_path = seq_option.work_path + "/sre_proc/";
    seq_option.data_path = seq_option.ssar_path + "data/";

    // init time_point_collect
    if(!seq_option.collect.empty()){
        chrono::system_clock::time_point it_time_point_collect;
        if(-1 == DetectDatetime(seq_option.collect, it_time_point_collect)){
            string exception_info = "collect datetime is not correct.";
            throw exception_info;
        }
        seq_option.time_point_collect = chrono::time_point_cast<chrono::seconds>(it_time_point_collect);
    }

    // init time_point_finish
    chrono::system_clock::time_point it_time_point_finish;
    if(seq_option.finish.empty()){
        it_time_point_finish = seq_option.time_point_finish;
    }else{
        if(0 == seq_option.finish.find("-", 0)){
            string finish_str = seq_option.finish.substr(1);
            int finish_int = 0;
            if((finish_str.size()-1) == finish_str.rfind("d", (finish_str.size()-1))){
                finish_int = int(24 * 60 * stof(finish_str.substr(0,(finish_str.size()-1))));
            }else if((finish_str.size()-1) == finish_str.rfind("h", (finish_str.size()-1))){
                finish_int = int(60 * stof(finish_str.substr(0,(finish_str.size()-1))));
            }else if((finish_str.size()-1) == finish_str.rfind("m", (finish_str.size()-1))){
                finish_int = int(stof(finish_str.substr(0,(finish_str.size()-1))));
            }else{
                finish_int = int(stof(finish_str));
            }
            it_time_point_finish = chrono::system_clock::now() - chrono::duration<int>(60 * finish_int);
        }else if(0 == seq_option.finish.find("+", 0) && "sys" == seq_option.src){
            if("sys" == seq_option.src){
                string finish_str = seq_option.finish.substr(1);
                int finish_int = 0;
                if(finish_str.empty()){
                    finish_int = 1440;
                }else if((finish_str.size()-1) == finish_str.rfind("d", (finish_str.size()-1))){
                    finish_int = int(24 * 60 * stof(finish_str.substr(0,(finish_str.size()-1))));
                }else if((finish_str.size()-1) == finish_str.rfind("h", (finish_str.size()-1))){
                    finish_int = int(60 * stof(finish_str.substr(0,(finish_str.size()-1))));
                }else if((finish_str.size()-1) == finish_str.rfind("m", (finish_str.size()-1))){
                    finish_int = int(stof(finish_str.substr(0,(finish_str.size()-1))));
                }else{
                    finish_int = int(stof(finish_str));
                }
                it_time_point_finish = chrono::system_clock::now() + chrono::duration<int>(60 * finish_int);
                seq_option.live_mode = true;
            }else{
                string exception_info = "finish datetime is not correct.";
                throw exception_info;
            }
        }else{
            if(-1 == DetectDatetime(seq_option.finish, it_time_point_finish)){
                throw ssar_cli::THROW_FINISH_DATETIME;
            }
            if(it_time_point_finish > chrono::system_clock::now()){
                throw ssar_cli::THROW_FINISH_DATETIME;
            }
        }
    }
    if("load5s" == seq_option.src || ("sys" == seq_option.src && seq_option.live_mode)){
        seq_option.time_point_finish = chrono::time_point_cast<chrono::seconds>(it_time_point_finish);
    }else{
        seq_option.time_point_finish = chrono::time_point_cast<chrono::minutes>(it_time_point_finish);
    }

    // init range
    if(seq_option.range){
        if(seq_option.live_mode){
            string exception_info = "when live mode, --range is needless.";
            throw exception_info;
        }
        if(seq_option.range < 0){
            string exception_info = "range must more than 0.";
            throw exception_info;
        }
        seq_option.duration_range = chrono::duration<int>(60 * seq_option.range);
    }else{
        if("proc" == seq_option.src || "sys" == seq_option.src){
            seq_option.duration_range = chrono::duration<int>(60 * 60 * 5);    // 5 hour
        }
    }

    // init begin
    chrono::system_clock::time_point it_time_point_begin;
    if(seq_option.begin.empty()){
        if(seq_option.live_mode){
            it_time_point_begin = chrono::system_clock::now();
        }else{
            it_time_point_begin = seq_option.time_point_finish - seq_option.duration_range;
        }
    }else{
        if(seq_option.live_mode){
            string exception_info = "when live mode, --begin is needless.";
            throw exception_info;
        }
        if(-1 == DetectDatetime(seq_option.begin, it_time_point_begin)){
            throw ssar_cli::THROW_BEGIN_DATETIME;
        }
    }
    if("load5s" == seq_option.src){
        seq_option.time_point_begin = chrono::time_point_cast<chrono::seconds>(it_time_point_begin);
    }else{
        seq_option.time_point_begin = chrono::time_point_cast<chrono::minutes>(it_time_point_begin);
    }
    if(seq_option.time_point_begin >= seq_option.time_point_finish){
        throw ssar_cli::THROW_BEGIN_COMPARE_FINISH;
    }

    if(!seq_option.live_mode){
        SreProc it_sre_proc;
        if(0 != it_sre_proc.MakeDataHours(seq_option.data_path)){
            string exception_info = "";             // no live mode must have config file.
            if(seq_option.ssar_conf){
                exception_info = "Data Paths in config file is not exist: " + seq_option.data_path;
            }else{
                exception_info = "Data Paths in default config is not exist: " + seq_option.data_path;
            }
            throw exception_info;
        }
        string time_begin_hard = it_sre_proc.GetFirstHour() + "0000";
        if(-1 == ParserDatetime(time_begin_hard, seq_option.time_point_begin_hard, "%Y%m%d%H%M%S")){
            string exception_info = "parser time_begin_hard datetime is fail.";
            throw exception_info;
        }
        if(seq_option.time_point_finish <= seq_option.time_point_begin_hard){
            string exception_info = "time_point_finish " + FmtDatetime(seq_option.time_point_finish) + " can't lower than time_begin_hard " + FmtDatetime(seq_option.time_point_begin_hard) + ".";
            throw exception_info;
        }
        if(seq_option.time_point_begin < seq_option.time_point_begin_hard){
            seq_option.time_point_begin = seq_option.time_point_begin_hard;
        }
    }

    // init key
    if(!seq_option.key.empty()){
        istringstream ss_key(seq_option.key);
        seq_option.keys.clear();
        string it_key_str;
        string it_order;
        string it_key;
        while(getline(ss_key, it_key_str, ',')){
            if(0 == it_key_str.find("+", 0)){
                it_key = it_key_str.substr(1);
                it_order = "+";            
            }else if(0 == it_key_str.find("-", 0)){
                it_key = it_key_str.substr(1);
                it_order = "-";            
            }else{
                it_key = it_key_str;
                auto j_field_found = field_attribute.find(it_key);
                if(j_field_found == field_attribute.cend()){
                    throw ssar_cli::THROW_NO_KEY + it_key;
                }
                it_order = field_attribute.at(it_key).at("k_order");
            }
            seq_option.keys.push_back(make_pair(it_key, it_order));
        }
    }else if(seq_option.cpu || seq_option.mem || seq_option.job || seq_option.sched){
        if(seq_option.cpu){
            seq_option.keys = {{"pcpu","-"}};
        }
        if(seq_option.mem){
            seq_option.keys = {{"rss_dlt","-"}};
        }
        if(seq_option.job){
            seq_option.keys = {{"pid","+"}};
        }
        if(seq_option.sched){
            seq_option.keys = {{"prio","+"}};
        }
    }

    // init lines
    if(seq_option.lines < 0){
        string exception_info = "lines must more than 0.";
        throw exception_info;
    }

    // init interval
    if(!seq_option.interval.empty()){
        if((seq_option.interval.size()-1) == seq_option.interval.rfind("d", (seq_option.interval.size()-1))){
            seq_option.intervals = int(24 * 60 * 60 * stof(seq_option.interval.substr(0,(seq_option.interval.size()-1))));
        }else if((seq_option.interval.size()-1) == seq_option.interval.rfind("h", (seq_option.interval.size()-1))){
            seq_option.intervals = int(60 * 60 * stof(seq_option.interval.substr(0,(seq_option.interval.size()-1))));
        }else if((seq_option.interval.size()-1) == seq_option.interval.rfind("m", (seq_option.interval.size()-1))){
            seq_option.intervals = int(60 * stof(seq_option.interval.substr(0,(seq_option.interval.size()-1))));
        }else if((seq_option.interval.size()-1) == seq_option.interval.rfind("s", (seq_option.interval.size()-1))){
            if("sys"==seq_option.src){
                if(seq_option.live_mode){
                    seq_option.intervals = int(stof(seq_option.interval.substr(0,(seq_option.interval.size()-1))));
                }else{
                    string exception_info = "interval not support Second level precision.";
                    throw exception_info;
                }
            }else if("proc"==seq_option.src){
                string exception_info = "interval not support Second level precision.";
                throw exception_info;
            }
        }else{
            seq_option.intervals = int(60 * stoi(seq_option.interval));
        }
    }else{
        if("sys" == seq_option.src && seq_option.live_mode){
            seq_option.intervals = 1;
        }
    }
    if(("proc" == seq_option.src || "sys" == seq_option.src) && seq_option.intervals <= 0){
        string exception_info = "interval must more than 0.";
        throw exception_info;
    }

    // init pid
    if("proc" == seq_option.src && seq_option.pid <= 0){
        string exception_info = "pid must more than 0.";
        throw exception_info;
    }

    // init formats
    if(seq_option.format.empty()){
        if("load5s" == seq_option.src){
            seq_option.formats.splice(seq_option.formats.end(), load5s_default_formats);
        }else if("load2p" == seq_option.src){
            if(seq_option.loadr){
                seq_option.formats.push_back("loadr");
            }
            if(seq_option.loadd){
                seq_option.formats.push_back("loadd");
            }
            if(seq_option.psr){
                seq_option.formats.push_back("psr");
            }
            if(seq_option.stackinfo){
                seq_option.formats.push_back("stackinfo");
            }
            if(seq_option.loadrd){
                seq_option.formats.push_back("loadrd");
            }
            if(seq_option.stack){
                seq_option.formats.push_back("stack");
            }
            if(seq_option.formats.empty()){
                seq_option.formats.splice(seq_option.formats.end(), load2p_default_formats);
            }
        }else if("proc" == seq_option.src){
            if(seq_option.cpu){
                seq_option.formats.splice(seq_option.formats.end(), proc_cpu_formats);
            }
            if(seq_option.mem){
                seq_option.formats.splice(seq_option.formats.end(), proc_mem_formats);
            }
            if(seq_option.formats.empty()){
                seq_option.formats.splice(seq_option.formats.end(), proc_default_formats);
            }
        }else if("sys" == seq_option.src){
            for(auto it_view : seq_option.views) {
                if(it_view.second){
                    seq_option.formats.splice(seq_option.formats.end(), (*(seq_option.sys_view))[it_view.first]);
                }
            }
            if(seq_option.formats.empty()){
                if(seq_option.sys_view->find("default") == seq_option.sys_view->end()){                    // if not success parse sys.conf file
                    seq_option.formats.splice(seq_option.formats.end(), sys_default_formats);
                }else{
                    seq_option.formats.splice(seq_option.formats.end(), (*(seq_option.sys_view))["default"]);
                }
            }
        }else{                                                                                             // seq_option.src == procs
            if(seq_option.cpu){
                seq_option.formats.splice(seq_option.formats.end(), procs_cpu_formats);
            }
            if(seq_option.mem){
                seq_option.formats.splice(seq_option.formats.end(), procs_mem_formats);
            }
            if(seq_option.job){
                seq_option.formats.splice(seq_option.formats.end(), procs_job_formats);
            }
            if(seq_option.sched){
                seq_option.formats.splice(seq_option.formats.end(), procs_sched_formats);
            }
            if(seq_option.formats.empty()){
                seq_option.formats.splice(seq_option.formats.end(), procs_default_formats);
            }
        }
        if(!seq_option.extra.empty()){
            char extra_delim = ',';
            if(seq_option.extra.find(';') != string::npos){
                extra_delim = ';';
            }
            istringstream ss_format(seq_option.extra);
            string it_format;
            while(getline(ss_format, it_format, extra_delim)){
                seq_option.formats.push_back(it_format);
            }
        }
    }else{
        char format_delim = ',';
        if(seq_option.format.find(';') != string::npos){
            format_delim = ';';
        }
        istringstream ss_format(seq_option.format);
        string it_format;
        while(getline(ss_format, it_format, format_delim)){
            seq_option.formats.push_back(it_format);
        }
    }

    RemoveDuplicateListElement(seq_option.formats);
    for(auto it_format:seq_option.formats) {  
        if(it_format == "cmdline" || it_format == "fullcmd"){
            seq_option.has_cmdline = true;
        }
    }
}

void InitEnv(SeqOptions &seq_option){
    bool cross_reboot = false;
    if(-1 == ReadRebootTimes(seq_option.reboot_times)){
        string exception_info = "set utmpx name failed.";
        throw exception_info;
    }
    stringstream exception_info;
    auto i_element = (*seq_option.reboot_times).cbegin();
    for(;i_element != (*seq_option.reboot_times).cend(); ++i_element){
        if(seq_option.time_point_begin < (*i_element) && seq_option.time_point_finish > (*i_element)){
            chrono::system_clock::time_point it_time_point = (*i_element);
            exception_info << "begin_datetime "; 
            exception_info << FmtDatetime(seq_option.time_point_begin); 
            exception_info << " and finish_datetime "; 
            exception_info << FmtDatetime(seq_option.time_point_finish);
            exception_info << " cross over reboot_datetime "; 
            exception_info << FmtDatetime(it_time_point);
            cross_reboot = true;
            break;
        }
    }
    if("procs" == seq_option.src){
        if(cross_reboot){
            throw exception_info.str();
        }
    }

    struct utsname sysinfo;
    if(uname(&sysinfo) == -1){
        string exception_info = "uname() failed.";
        throw exception_info;
    }
    seq_option.uts_release = sysinfo.release;
}

void InitIndicator(SeqOptions &seq_option, indicator_t it_indicator_t, unordered_map<string, vector<string>> &file_infos){
    string it_line_content;
    int j = 0;
    int k = 0;
    int line_begin_pos;
    string first_cell;
    int word_count = 0;
    string it_word;
    string combine_delim;
    string begin_combine;
    string line_combine;
    string full_combine;

    if(it_indicator_t.line){
        if(it_indicator_t.line > file_infos[it_indicator_t.cfile].size()){
            string exception_info = "line " + to_string(it_indicator_t.line) + " out of range " + to_string(file_infos[it_indicator_t.cfile].size());
            throw exception_info;
        }
        it_line_content = file_infos[it_indicator_t.cfile].at(it_indicator_t.line - 1);  // if line not find, throw out_of_range.
    }else{
        j = 1;
        auto j_element = file_infos[it_indicator_t.cfile].cbegin();
        for(;j_element != file_infos[it_indicator_t.cfile].cend(); ++j_element){
            it_line_content = (*j_element);
            line_begin_pos = it_line_content.find(it_indicator_t.line_begin + " ", 0);  // must plus a space
            if(line_begin_pos != string::npos){
                if("string" == it_indicator_t.dtype){
                    it_indicator_t.line = j;
                    break;
                }else{
                    istringstream ss_first_cell(it_line_content);
                    k = 1;
                    while(ss_first_cell >> first_cell){
                        if(k == it_indicator_t.column){
                            if(is_float(first_cell)){
                                it_indicator_t.line = j;
                                break;
                            }
                        }
                        k++;
                    }
                    if(it_indicator_t.line){
                        break;
                    }
                }
            }
            j++;
        }
        if(!it_indicator_t.line){
            string exception_info = it_indicator_t.line_begin + " is not find in file " + it_indicator_t.cfile;
            throw exception_info;
        }
    }

    stringstream ss_line_content(it_line_content);
    word_count = 0;
    while(ss_line_content >> it_word){
        ++word_count;
    }
    if(it_indicator_t.column > word_count){
        string exception_info = "column " + to_string(it_indicator_t.column) + " is not find in file " + it_indicator_t.cfile + ", line " + to_string(it_indicator_t.line) + " total column is " + to_string(word_count);
        throw exception_info;
    }

    seq_option.formated.push_back(it_indicator_t);
    if(it_indicator_t.position == "f"){
        combine_delim = ":";
        line_combine = to_string(it_indicator_t.line) + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column);
        full_combine = it_indicator_t.cfile + combine_delim + line_combine + combine_delim + "c";
        if(it_indicator_t.metric == "d"){
            (*seq_option.delta_hierarchy)[it_indicator_t.cfile].push_back(line_combine);
        }else{
            (*seq_option.cumulate_hierarchy)[it_indicator_t.cfile].push_back(line_combine);
        }
        (*seq_option.sys_hierarchy_line)[it_indicator_t.cfile][it_indicator_t.line][it_indicator_t.column] = it_indicator_t.metric;
        (*seq_option.sys_hierarchy_dtype_line)[it_indicator_t.cfile][it_indicator_t.line][it_indicator_t.column] = it_indicator_t.dtype;
    }else{
        if(it_indicator_t.line_begin.find(':') != string::npos){
            combine_delim = "|";
        }else{
            combine_delim = ":";
        }
        begin_combine = it_indicator_t.line_begin + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column);
        full_combine = it_indicator_t.cfile + combine_delim + begin_combine + combine_delim + "c";
        if(it_indicator_t.metric == "d"){
            (*seq_option.delta_hierarchy)[it_indicator_t.cfile].push_back(begin_combine);
        }else{
            (*seq_option.cumulate_hierarchy)[it_indicator_t.cfile].push_back(begin_combine);
        }
        (*seq_option.sys_hierarchy_begin)[it_indicator_t.cfile][it_indicator_t.line_begin][it_indicator_t.column] = it_indicator_t.metric;
        (*seq_option.sys_hierarchy_dtype_begin)[it_indicator_t.cfile][it_indicator_t.line_begin][it_indicator_t.column] = it_indicator_t.dtype;
    }
    (*seq_option.sys_dtype)[full_combine] = it_indicator_t.dtype;
}

void InitFormat(SeqOptions &seq_option){
    unordered_map<string, string> file_src;
    unordered_map<string, vector<string>> file_infos;
    string it_indicator;
    string it_cell;
    indicator_t it_indicator_t;
    istringstream ss_indicator;
    ifstream if_file;
    string line_contents;
    int colon_num = -1;
    string indicator_group_key;
    bool indicator_group_match;

    for(auto pair_group_files: (*seq_option.group_files)) {
        if(0 != seq_option.uts_release.find(pair_group_files.first, 0)){
            continue;
        }
        for(auto sys_file : (pair_group_files.second)){
            if((*seq_option.sys_files).find(sys_file.first) == (*seq_option.sys_files).end()){
                (*seq_option.sys_files)[sys_file.first] = sys_file.second;
            }
        }
    }
    for(auto pair_group_files: (*seq_option.group_files)) {
        if(pair_group_files.first == "default"){
            for(auto sys_file : (pair_group_files.second)){
                if((*seq_option.sys_files).find(sys_file.first) == (*seq_option.sys_files).end()){
                    (*seq_option.sys_files)[sys_file.first] = sys_file.second;
                }
            }
            break;
        }
    }

    string alias_copy = "";
    list<int> it_lines = {};
    list<int> it_columns = {};
    int it_line = 0;
    int it_column = 0;
    bool indicator_ignore;
    string it_original_alias;
    string it_kv_str;
    string it_key_str;
    string it_value_str;
    int last_slash_pos;
    int equal_pos;
    char indicator_delim;
    auto i_element = seq_option.formats.cbegin();
    for(;i_element != seq_option.formats.cend(); ++i_element){
        it_indicator = (*i_element);

        indicator_delim = ':';
        if(it_indicator.find('|') != string::npos){
            indicator_delim = '|';
        }
        
        it_lines.clear();
        it_columns.clear();
        indicator_ignore          = false;
        it_original_alias         = "";
        it_indicator_t.cfile      = "";
        it_indicator_t.line_begin = "";
        it_indicator_t.line       = 0;
        it_indicator_t.column     = 0;
        it_indicator_t.width      = 10;
        it_indicator_t.metric     = "c";
        it_indicator_t.position   = "f";
        it_indicator_t.dtype      = "int";
        it_indicator_t.gzip       = 0;
        it_indicator_t.alias      = it_indicator;
        if(it_indicator.find('=') != string::npos){
            istringstream ss_indicator(it_indicator);
            while(getline(ss_indicator, it_kv_str, indicator_delim)){
                equal_pos = it_kv_str.find("=");
                if(equal_pos == string::npos){
                    string exception_info = "part of indicator " + it_kv_str + " not include equal mark =";
                    throw exception_info;
                }
                it_key_str = it_kv_str.substr(0, equal_pos);
                it_value_str = it_kv_str.substr(equal_pos+1);
                if(it_key_str == "width"){
                    it_indicator_t.width = stoi(it_value_str);
                }else if(it_key_str == "line"){
                    it_lines = ParseParam(it_value_str);
                }else if(it_key_str == "column"){
                    if(it_value_str == "-1"){
                        indicator_ignore = true;
                        break;
                    }else{ 
                        it_columns = ParseParam(it_value_str);
                    }
                }else if(it_key_str == "gzip"){
                    if(it_value_str == "true"){
                        it_indicator_t.gzip = 1;
                    }
                }else if(it_key_str == "cfile"){
                    it_indicator_t.cfile = it_value_str;
                }else if(it_key_str == "line_begin"){
                    it_indicator_t.line_begin = it_value_str;
                }else if(it_key_str == "position"){
                    it_indicator_t.position = it_value_str;
                }else if(it_key_str == "metric"){
                    it_indicator_t.metric = it_value_str;
                }else if(it_key_str == "dtype"){
                    it_indicator_t.dtype = it_value_str;
                }else if(it_key_str == "src_path"){
                    it_indicator_t.src_path = it_value_str;
                }else if(it_key_str == "alias"){
                    it_indicator_t.alias = it_value_str;
                }else{
                    string exception_info = "unrecognized key: " + it_key_str;
                    throw exception_info;
                }
            }

            if(it_indicator_t.cfile == "" && it_indicator_t.src_path != ""){
                last_slash_pos = it_indicator_t.src_path.rfind("/", it_indicator_t.src_path.size());
                it_indicator_t.cfile = it_indicator_t.src_path.substr(last_slash_pos + 1);
            }else if(it_indicator_t.cfile != "" && it_indicator_t.src_path == ""){
                it_indicator_t.src_path= "/proc/" + it_indicator_t.cfile;
            }
        }else{
            colon_num = count(it_indicator.begin(), it_indicator.end(), indicator_delim);
            if(colon_num == 0){                                                  // config define indicator, such as dirty,mlocked.
                indicator_group_match = false;
                for(auto pair_indicator_group : (*seq_option.sys_indicator)){
                    indicator_group_key = pair_indicator_group.first;
                    if(0 == seq_option.uts_release.find(indicator_group_key, 0)){
                        if((*seq_option.sys_indicator)[indicator_group_key].find(it_indicator) 
                            != (*seq_option.sys_indicator)[indicator_group_key].end()){
                            indicator_group_match = true;
                            break;
                        }
                    }
                }
                if(!indicator_group_match){
                    indicator_group_key = "default";
                    if((*seq_option.sys_indicator)[indicator_group_key].find(it_indicator) 
                        == (*seq_option.sys_indicator)[indicator_group_key].end()){
                        string exception_info = "indicator " + it_indicator + " is not defined.";
                        throw exception_info;
                    }
                }
                it_indicator_t = (*seq_option.sys_indicator)[indicator_group_key][it_indicator];
            }else if(colon_num == 2){                                            // custom define, such as buddyinfo:Normal:5.
                istringstream ss_indicator(it_indicator);
                getline(ss_indicator, it_indicator_t.cfile, indicator_delim);
                getline(ss_indicator, it_cell, indicator_delim);
                if(is_number(it_cell)){
                    it_indicator_t.line = stoi(it_cell);
                }else{
                    it_indicator_t.line_begin = it_cell;
                    it_indicator_t.line = 0;
                }
                getline(ss_indicator, it_cell, indicator_delim);
                it_indicator_t.column = stoi(it_cell);
            }else if(colon_num == 3){
                istringstream ss_indicator(it_indicator);
                getline(ss_indicator, it_indicator_t.cfile, indicator_delim);
                getline(ss_indicator, it_cell, indicator_delim);
                if(is_number(it_cell)){
                    it_indicator_t.line = stoi(it_cell);
                }else{
                    it_indicator_t.line_begin = it_cell;
                    it_indicator_t.line = 0;
                }
                getline(ss_indicator, it_cell, indicator_delim);
                it_indicator_t.column = stoi(it_cell);
                getline(ss_indicator, it_indicator_t.metric, indicator_delim);
            }else if(colon_num == 4){
                istringstream ss_indicator(it_indicator);
                getline(ss_indicator, it_indicator_t.cfile, indicator_delim);
                getline(ss_indicator, it_cell, indicator_delim);
                if(is_number(it_cell)){
                    it_indicator_t.line = stoi(it_cell);
                }else{
                    it_indicator_t.line_begin = it_cell;
                    it_indicator_t.line = 0;
                }
                getline(ss_indicator, it_cell, indicator_delim);
                it_indicator_t.column = stoi(it_cell);
                getline(ss_indicator, it_indicator_t.metric, indicator_delim);
                getline(ss_indicator, it_indicator_t.position, indicator_delim);
            }else{
                string exception_info = "indicator " + it_indicator + " is not correct.";
                throw exception_info;
            }
            if(it_indicator_t.line){
                it_lines.push_back(it_indicator_t.line);
            }
            if(it_indicator_t.column){
                it_columns.push_back(it_indicator_t.column);
            }
        }
        it_indicator_t.indicator = it_indicator;

        if(indicator_ignore){
            continue;
        }
        if(it_indicator_t.cfile == ""){
            string exception_info = "field cfile is not set in indicator " + it_indicator;
            throw exception_info;
        }
        if(it_columns.empty()){
            string exception_info = "field column is not set in indicator " + it_indicator;
            throw exception_info;
        }
        if(!(it_indicator_t.metric == "c" || it_indicator_t.metric == "d")){
            string exception_info = "indicator metric " + it_indicator_t.metric + " is not correct..";
            throw exception_info;
        }
        if(!(it_indicator_t.dtype == "int" || it_indicator_t.dtype == "float" || it_indicator_t.dtype == "string")){
            string exception_info = "indicator dtype " + it_indicator_t.dtype + " is not correct..";
            throw exception_info;
        }
        if(!(it_indicator_t.position == "f" || it_indicator_t.position == "a")){
            string exception_info = "indicator position " + it_indicator_t.position + " is not correct...";
            throw exception_info;
        }
        if(!it_lines.empty() && it_indicator_t.line_begin != ""){
            string exception_info = "line and line_begin cannot be set at the same time.";
            throw exception_info;
        }
        if(it_lines.empty() && it_indicator_t.line_begin == ""){
            string exception_info = "Neither line_begin nor line is set.";
            throw exception_info;
        }
        if(!it_lines.empty() && it_indicator_t.position == "a"){
            string exception_info = "auto position is not match with line row.";
            throw exception_info;
        }
        if(it_indicator_t.dtype == "string" && it_indicator_t.metric == "d"){
            string exception_info = "string dtype is not support d(delta) metric.";
            throw exception_info;
        }
        if(it_lines.size() > 1 && it_indicator_t.alias.find("{line}") == string::npos){
            string exception_info = "line placeholder is not set in alias.";
            throw exception_info;
        }
        if(it_columns.size() > 1 && it_indicator_t.alias.find("{column}") == string::npos){
            string exception_info = "column placeholder is not set in alias.";
            throw exception_info;
        }

        if(it_lines.empty() && it_indicator_t.alias.find("{line}") != string::npos){
            string exception_info = "the line placeholder in alias is redundant";
            throw exception_info;
        }
        if(it_columns.empty() && it_indicator_t.alias.find("{column}") != string::npos){
            string exception_info = "the column placeholder in alias is redundant.";
            throw exception_info;
        }

        if(seq_option.sys_files->find(it_indicator_t.cfile) == seq_option.sys_files->end()){
            (*seq_option.sys_files)[it_indicator_t.cfile].src_path = it_indicator_t.src_path;
            (*seq_option.sys_files)[it_indicator_t.cfile].gzip     = it_indicator_t.gzip;
        }

        if(file_infos.find(it_indicator_t.cfile) == file_infos.end()){
            if_file.open((*seq_option.sys_files)[it_indicator_t.cfile].src_path, ios_base::in);
            if(!if_file.is_open()){
                if(errno & EACCES){
                    string exception_info = it_indicator_t.src_path + " file is permission denied.";
                    throw exception_info;
                }else{
                    string exception_info = it_indicator_t.src_path + " file is not exist.";
                    throw exception_info;
                }
            }
            file_infos[it_indicator_t.cfile] = {};
            while(if_file.good()){
                getline(if_file, line_contents);
                if(line_contents.empty()){
                    break;
                }
                file_infos[it_indicator_t.cfile].push_back(line_contents); 
            }
            if_file.close();
            
            (*seq_option.delta_hierarchy)[it_indicator_t.cfile] = {};
            (*seq_option.cumulate_hierarchy)[it_indicator_t.cfile] = {};
        }
        
        alias_copy = it_indicator_t.alias;
        if(it_lines.empty()){
            auto j_element = it_columns.cbegin();
            for(;j_element != it_columns.cend(); ++j_element){
                it_column = (*j_element);
                it_indicator_t.alias = alias_copy;
                if(it_indicator_t.alias.find("{column}") != string::npos){
                    it_indicator_t.alias.replace(it_indicator_t.alias.find("{column}"), 8, to_string(it_column));
                }
                it_indicator_t.column = it_column;
                InitIndicator(seq_option, it_indicator_t, file_infos);        
            }
        }else{
            auto j_element = it_lines.cbegin();
            for(;j_element != it_lines.cend(); ++j_element){
                it_line = (*j_element);
                
                auto k_element = it_columns.cbegin();
                for(;k_element != it_columns.cend(); ++k_element){
                    it_column= (*k_element);

                    it_indicator_t.alias = alias_copy;
                    if(it_indicator_t.alias.find("{line}") != string::npos){
                        it_indicator_t.alias.replace(it_indicator_t.alias.find("{line}"), 6, to_string(it_line));
                    }
                    it_indicator_t.line = it_line;
                    if(it_indicator_t.alias.find("{column}") != string::npos){
                        it_indicator_t.alias.replace(it_indicator_t.alias.find("{column}"), 8, to_string(it_column));
                    }
                    it_indicator_t.column = it_column;

                    InitIndicator(seq_option, it_indicator_t, file_infos);        
                }
            }
        }
    }
}

/**********************************************************************************
 *                                                                                *
 *                               proc part                                        * 
 *                                                                                *
 **********************************************************************************/

int SprocCompare(const sproc_t &itema, const sproc_t &itemb, const pair<string,string> &key){
    int ret;
    if(key.first.empty()){
        throw ssar_cli::THROW_KEY_EMPTY;
    TIME_KEY_COMPARE(etime, etimes);
    TIME_KEY_COMPARE(cputime, time);
    TIME_KEY_COMPARE(cpu_utime, utime);
    TIME_KEY_COMPARE(cpu_stime, stime);
    KEY_COMPARE(s);
    KEY_COMPARE(tid);
    KEY_COMPARE(ppid);
    KEY_COMPARE(pgid);
    KEY_COMPARE(sid);
    KEY_COMPARE(nlwp);
    KEY_COMPARE(tgid);
    KEY_COMPARE(pid);
    KEY_COMPARE(psr);
    KEY_COMPARE(prio);
    KEY_COMPARE(nice);
    KEY_COMPARE(rtprio);
    KEY_COMPARE(sched);
    KEY_COMPARE(cls);
    KEY_COMPARE(flags);
    KEY_COMPARE(vcswch);
    KEY_COMPARE(ncswch);
    KEY_COMPARE(vcswchs);
    KEY_COMPARE(ncswchs);
    KEY_COMPARE(utime);
    KEY_COMPARE(stime);
    KEY_COMPARE(cutime);
    KEY_COMPARE(cstime);
    KEY_COMPARE(start_time);
    KEY_COMPARE(etimes);
    KEY_COMPARE(tty);
    KEY_COMPARE(tpgid);
    KEY_COMPARE(pcpu);
    KEY_COMPARE(rss); 
    KEY_COMPARE(shr); 
    KEY_COMPARE(alarm);
    KEY_COMPARE(size);
    KEY_COMPARE(resident);
    KEY_COMPARE(share);
    KEY_COMPARE(trs);
    KEY_COMPARE(lrs);
    KEY_COMPARE(drs);
    KEY_COMPARE(dt);
    KEY_COMPARE(vsize);
    KEY_COMPARE(rss_rlim);
    KEY_COMPARE(min_flt);
    KEY_COMPARE(maj_flt);
    KEY_COMPARE(cmin_flt);
    KEY_COMPARE(cmaj_flt);
    KEY_COMPARE(cmd);
    KEY_COMPARE(fullcmd);
    KEY_COMPARE(cmdline);
    KEY_COMPARE(time);
    KEY_COMPARE(realtime);
    KEY_COMPARE(time_dlt);
    KEY_COMPARE(utime_dlt);
    KEY_COMPARE(stime_dlt);
    KEY_COMPARE(pcpu);
    KEY_COMPARE(pucpu);
    KEY_COMPARE(pscpu);
    KEY_COMPARE(maj_dlt);
    KEY_COMPARE(min_dlt);
    KEY_COMPARE(rss_dlt);
    KEY_COMPARE(shr_dlt);
    KEY_COMPARE(start_datetime);
    KEY_COMPARE(begin_datetime);
    KEY_COMPARE(finish_datetime);
    KEY_COMPARE(collect_datetime);
    KEY_COMPARE(b);
    }else{
        throw ssar_cli::THROW_COMPARE_KEY + key.first;
    }
    return ret;
}

int ReadProcFileData(SeqOptions &seq_option, string &it_path, string &addon_path, list<sproc_t> &it_sproc_t, int pid){
    igzfstream it_file;               // ifstream      
    igzfstream addon_file;
    bool finded = false; 
    it_sproc_t.clear();
    string addon_contents;
    unordered_map<int, aproc_t> all_aproc_t;

    if(!addon_path.empty()){
        addon_file.open(addon_path, ios_base::in);
        while(addon_file.good()){
            aproc_t item_aproc_t;

            getline(addon_file, addon_contents);
            if(addon_contents.empty()){
                break;
            }
            istringstream iaddon_contents(addon_contents);
            iaddon_contents >> item_aproc_t.tgid;

            iaddon_contents.ignore();
            item_aproc_t.cmdline = addon_contents.substr(iaddon_contents.tellg());
            iaddon_contents >> item_aproc_t.fullcmd;
            all_aproc_t[item_aproc_t.tgid] = item_aproc_t;
        }
        addon_file.close();
    }

    it_file.open(it_path, ios_base::in);
    if(!it_file.good()){
        return 1;
    }
    string file_contents;
    while(it_file.good()){
        sproc_t item_sproc_t;
        // start_time, tgid, ppid, pgid, sid, tpgid, nlwp, psr, s, etimes, flags, sched, nice, rtprio, prio, nvcsw, nivcsw utime, stime, size, rss, shr, maj_flt, min_flt, cmd, cmdline
        getline(it_file, file_contents);
        if(file_contents.empty() || finded){
            break;
        }
        istringstream ifile_contents(file_contents);
        ifile_contents >> item_sproc_t.start_time;
        ifile_contents >> item_sproc_t.tgid;
        if(pid > 0){
            if(item_sproc_t.tgid == pid){
                finded = true;
            }else{
                continue;
            }
        }
        ifile_contents >> item_sproc_t.ppid;
        ifile_contents >> item_sproc_t.pgid;
        ifile_contents >> item_sproc_t.sid;
        ifile_contents >> item_sproc_t.tpgid;
        ifile_contents >> item_sproc_t.nlwp;
        ifile_contents >> item_sproc_t.psr;
        ifile_contents >> item_sproc_t.s;
        ifile_contents >> item_sproc_t.etimes;
        ifile_contents >> item_sproc_t.flags;
        ifile_contents >> item_sproc_t.sched;
        ifile_contents >> item_sproc_t.nice;
        ifile_contents >> item_sproc_t.rtprio;
        ifile_contents >> item_sproc_t.prio;
        ifile_contents >> item_sproc_t.vcswch;
        ifile_contents >> item_sproc_t.ncswch;
        ifile_contents >> item_sproc_t.utime;
        ifile_contents >> item_sproc_t.stime;
        ifile_contents >> item_sproc_t.size;
        ifile_contents >> item_sproc_t.rss;
        ifile_contents >> item_sproc_t.shr;
        ifile_contents >> item_sproc_t.maj_flt;
        ifile_contents >> item_sproc_t.min_flt;
        ifile_contents >> item_sproc_t.cmd;

        if(all_aproc_t.find(item_sproc_t.tgid) == all_aproc_t.cend()){
            item_sproc_t.fullcmd = "-";
            item_sproc_t.cmdline = "-";
        }else{
            item_sproc_t.fullcmd = all_aproc_t[item_sproc_t.tgid].fullcmd;
            item_sproc_t.cmdline = all_aproc_t[item_sproc_t.tgid].cmdline;
        }

        item_sproc_t.pid = item_sproc_t.tgid;
        item_sproc_t.time = item_sproc_t.utime + item_sproc_t.stime;
        item_sproc_t.etime = FmtTime(item_sproc_t.etimes);
        item_sproc_t.cputime = FmtTime(item_sproc_t.time);
        item_sproc_t.cpu_utime = FmtTime(item_sproc_t.utime);
        item_sproc_t.cpu_stime = FmtTime(item_sproc_t.stime);
        item_sproc_t.cls = sched2cls[item_sproc_t.sched];

        chrono::system_clock::time_point it_time_point_start = chrono::system_clock::from_time_t(item_sproc_t.start_time);
        item_sproc_t.start_datetime = FmtDatetime(it_time_point_start);
        item_sproc_t.begin_datetime = FmtDatetime(seq_option.time_point_begin_real);
        item_sproc_t.finish_datetime = FmtDatetime(seq_option.time_point_finish_real);
 
        item_sproc_t.record_type = 1001;
        it_sproc_t.push_back(item_sproc_t);
    }
    it_file.close();

    if(it_sproc_t.size()){
        return 0;
    }else{
        return 2;
    }
}

void ReportProc(SeqOptions &seq_option){
    // record_type:  
    //      1001  this record is exist.  
    //      1003  this record is not exist.
    //      1004  this record is exist, but previous record is not exist.
    //      1011  this record is exist, but pid not exist in record.
    //      1012  this is before_first record.
    //      1013  this is after_last record.

    string full_sresar_suffix = sresar_suffix;
    if(!seq_option.proc_gzip_disable){
        full_sresar_suffix = sresar_suffix + ".gz";
    }
    string full_cmdline_suffix = cmdline_suffix;
    if(!seq_option.proc_gzip_disable){
        full_cmdline_suffix = cmdline_suffix + ".gz";
    }

    SreProc it_sre_proc;
    it_sre_proc.SetSuffix(full_sresar_suffix);
    it_sre_proc.SetDataPath(seq_option.data_path);
    int read_file_ret;
    int data_hour_path_ret;
    chrono::system_clock::time_point it_time_point = seq_option.time_point_finish;
    chrono::system_clock::time_point time_point_real = seq_option.time_point_finish;
    chrono::system_clock::time_point time_point_after_last   = chrono::time_point<chrono::system_clock>{};
    chrono::system_clock::time_point time_point_before_first = chrono::time_point<chrono::system_clock>{};
    chrono::system_clock::time_point start_decide_time_point = chrono::time_point<chrono::system_clock>{};
    list<sproc_t> it_list_sproc_t;
    list<sproc_t> it_sproc_t;
    sproc_t item_sproc_t;

    item_sproc_t.start_time = 0;
    for(;;){
        if(it_time_point < seq_option.time_point_begin){
            break;
        }
        if(it_time_point < start_decide_time_point - chrono::duration<int>(seq_option.intervals)){
            break;
        }
        
        data_hour_path_ret = it_sre_proc.MakeDataHourPath(it_time_point);
        if(0 != data_hour_path_ret){
            item_sproc_t.record_type      = 1003;
            item_sproc_t.collect_time     = chrono::system_clock::to_time_t(it_time_point);
            item_sproc_t.collect_datetime = FmtDatetime(it_time_point);
            it_sproc_t.push_front(item_sproc_t);    
            it_time_point = it_time_point - chrono::duration<int>(seq_option.intervals);
            continue;
        }
        string it_path  = it_sre_proc.GetDataHourPath();
        time_point_real = it_sre_proc.GetDataTimePointReal();

        string addon_path = "";
        if(seq_option.has_cmdline){
            SreProc addon_sre_proc;
            addon_sre_proc.EnableAdjust();
            addon_sre_proc.SetAdjustUnit(1);
            addon_sre_proc.SetAdjustForward(true);
            addon_sre_proc.SetSuffix(full_cmdline_suffix);
            addon_sre_proc.SetDataPath(seq_option.data_path);
            if(0 == addon_sre_proc.MakeDataHourPath(time_point_real)){
                addon_path = addon_sre_proc.GetDataHourPath();
            }
        }

        read_file_ret = -1;
        read_file_ret = ReadProcFileData(seq_option, it_path, addon_path, it_list_sproc_t, seq_option.pid);
        if(2 == read_file_ret){
            if(chrono::time_point<chrono::system_clock>{} == start_decide_time_point){
                time_point_after_last = it_time_point;
                it_time_point = it_time_point - chrono::duration<int>(seq_option.intervals);
                continue;
            }else{
                time_point_before_first = it_time_point;
                break;
            }
        }else if(0 != read_file_ret){
            item_sproc_t.record_type      = 1003;
            item_sproc_t.collect_time     = chrono::system_clock::to_time_t(it_time_point);
            item_sproc_t.collect_datetime = FmtDatetime(it_time_point);
            it_sproc_t.push_front(item_sproc_t);    
            it_time_point = it_time_point - chrono::duration<int>(seq_option.intervals);
            continue;
        }

        item_sproc_t = it_list_sproc_t.front();
        if(chrono::time_point<chrono::system_clock>{} != start_decide_time_point 
            && start_decide_time_point != chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(item_sproc_t.start_time-seq_option.intervals))){
            break;
        }
        if(chrono::time_point<chrono::system_clock>{} == start_decide_time_point){
            start_decide_time_point=chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(item_sproc_t.start_time-seq_option.intervals));
        }
        item_sproc_t.collect_time     = it_sre_proc.GetDataTimestamp();
        item_sproc_t.collect_datetime = FmtDatetime(time_point_real);
        item_sproc_t.begin_datetime   = FmtDatetime(seq_option.time_point_begin);
        item_sproc_t.finish_datetime  = FmtDatetime(seq_option.time_point_finish);
        it_sproc_t.push_front(item_sproc_t);
        it_time_point = it_time_point - chrono::duration<int>(seq_option.intervals);
    }

    list<sproc_t> fill_sproc_t;
    sproc_t previous_sproc_t;
    sproc_t current_sproc_t;

    unsigned long long vcswchs_dlt;
    unsigned long long ncswchs_dlt;
    previous_sproc_t.record_type = 1003;
    list<sproc_t>::const_iterator i_element = it_sproc_t.cbegin();
    for(;i_element != it_sproc_t.cend(); ++i_element){
        current_sproc_t = (*i_element);        
       
        if(time_point_after_last != chrono::time_point<chrono::system_clock>{} 
            && chrono::system_clock::from_time_t(current_sproc_t.collect_time) > time_point_after_last){
            break;
        }
 
        if(1001 != current_sproc_t.record_type){
            previous_sproc_t = current_sproc_t;
            fill_sproc_t.push_back(current_sproc_t);
            continue;
        }
        if(1003 == previous_sproc_t.record_type){
            current_sproc_t.record_type = 1004;
            previous_sproc_t = current_sproc_t;
            fill_sproc_t.push_back(current_sproc_t);
            continue;
        }

        current_sproc_t.realtime  = current_sproc_t.collect_time - previous_sproc_t.collect_time;
        current_sproc_t.rss_dlt   = current_sproc_t.rss          - previous_sproc_t.rss;
        current_sproc_t.shr_dlt   = current_sproc_t.shr          - previous_sproc_t.shr;
        current_sproc_t.maj_dlt   = current_sproc_t.maj_flt      - previous_sproc_t.maj_flt;
        current_sproc_t.min_dlt   = current_sproc_t.min_flt      - previous_sproc_t.min_flt;
        current_sproc_t.utime_dlt = current_sproc_t.utime        - previous_sproc_t.utime;
        current_sproc_t.stime_dlt = current_sproc_t.stime        - previous_sproc_t.stime;
        current_sproc_t.time_dlt  = current_sproc_t.utime_dlt    + current_sproc_t.stime_dlt;
        current_sproc_t.pucpu     = float(current_sproc_t.utime_dlt) / float(current_sproc_t.realtime);
        current_sproc_t.pscpu     = float(current_sproc_t.stime_dlt) / float(current_sproc_t.realtime);
        current_sproc_t.pcpu      = current_sproc_t.pucpu        + current_sproc_t.pscpu;
        vcswchs_dlt               = current_sproc_t.vcswch       - previous_sproc_t.vcswch;
        ncswchs_dlt               = current_sproc_t.ncswch       - previous_sproc_t.ncswch;
        current_sproc_t.vcswchs   = int(float(vcswchs_dlt)/float(current_sproc_t.realtime));
        current_sproc_t.ncswchs   = int(float(ncswchs_dlt)/float(current_sproc_t.realtime));

        previous_sproc_t = current_sproc_t;
        fill_sproc_t.push_back(current_sproc_t);
    }

    if(chrono::time_point<chrono::system_clock>{} != time_point_before_first){
        item_sproc_t.collect_datetime = FmtDatetime(time_point_before_first);
        item_sproc_t.record_type = 1012;
        fill_sproc_t.push_front(item_sproc_t);
    }
    if(chrono::time_point<chrono::system_clock>{} != time_point_after_last
        && it_time_point + chrono::duration<int>(seq_option.intervals) < time_point_after_last){
        item_sproc_t.collect_datetime = FmtDatetime(time_point_after_last);
        item_sproc_t.record_type = 1013;
        fill_sproc_t.push_back(item_sproc_t);
    }

    if(seq_option.api){
        DisplayProcJson(seq_option, fill_sproc_t);    
    }else{
        DisplayProcShell(seq_option, fill_sproc_t);    
    }
}

void MakeRangeIndicators(list<sproc_t> &it_list_sproc_t,unordered_map<int,sproc_t> &begin_sproc_t,list<sproc_t> &it_sproc_t,int it_count){
    unsigned long long vcswchs_dlt;
    unsigned long long ncswchs_dlt;
    list<sproc_t>::const_iterator j_element = it_list_sproc_t.cbegin();
    for(;j_element != it_list_sproc_t.cend(); ++j_element){
        sproc_t item_sproc_t = (*j_element);
        auto i_pair_found = begin_sproc_t.find(item_sproc_t.tgid);
        item_sproc_t.realtime = it_count;
        if(i_pair_found == begin_sproc_t.cend() || item_sproc_t.start_time != begin_sproc_t.at(item_sproc_t.tgid).start_time){
            item_sproc_t.b       = '<';
            item_sproc_t.rss_dlt = item_sproc_t.rss;
            item_sproc_t.shr_dlt = item_sproc_t.shr;
            item_sproc_t.maj_dlt = item_sproc_t.maj_flt;
            item_sproc_t.min_dlt = item_sproc_t.min_flt;
            item_sproc_t.utime_dlt = item_sproc_t.utime;
            item_sproc_t.stime_dlt = item_sproc_t.stime;
            item_sproc_t.time_dlt = item_sproc_t.utime_dlt + item_sproc_t.stime_dlt;
            item_sproc_t.pucpu = float(item_sproc_t.utime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pscpu = float(item_sproc_t.stime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pcpu = item_sproc_t.pucpu + item_sproc_t.pscpu;
            item_sproc_t.vcswchs = item_sproc_t.vcswch;
            item_sproc_t.ncswchs = item_sproc_t.ncswch;
        }else if(item_sproc_t.utime < begin_sproc_t.at(item_sproc_t.tgid).utime 
            || item_sproc_t.stime < begin_sproc_t.at(item_sproc_t.tgid).stime){
            item_sproc_t.b       = '>';
            item_sproc_t.rss_dlt = item_sproc_t.rss;
            item_sproc_t.shr_dlt = item_sproc_t.shr;
            item_sproc_t.maj_dlt = item_sproc_t.maj_flt;
            item_sproc_t.min_dlt = item_sproc_t.min_flt;
            item_sproc_t.utime_dlt = item_sproc_t.utime;
            item_sproc_t.stime_dlt = item_sproc_t.stime;
            item_sproc_t.time_dlt = item_sproc_t.utime_dlt + item_sproc_t.stime_dlt;
            item_sproc_t.pucpu = float(item_sproc_t.utime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pscpu = float(item_sproc_t.stime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pcpu = item_sproc_t.pucpu + item_sproc_t.pscpu;
            item_sproc_t.vcswchs = item_sproc_t.vcswch;
            item_sproc_t.ncswchs = item_sproc_t.ncswch;
        }else{
            item_sproc_t.b       = '=';
            item_sproc_t.rss_dlt = item_sproc_t.rss     - begin_sproc_t.at(item_sproc_t.tgid).rss;
            item_sproc_t.shr_dlt = item_sproc_t.shr     - begin_sproc_t.at(item_sproc_t.tgid).shr;
            item_sproc_t.maj_dlt = item_sproc_t.maj_flt - begin_sproc_t.at(item_sproc_t.tgid).maj_flt;
            item_sproc_t.min_dlt = item_sproc_t.min_flt - begin_sproc_t.at(item_sproc_t.tgid).min_flt;
            item_sproc_t.utime_dlt = item_sproc_t.utime - begin_sproc_t.at(item_sproc_t.tgid).utime;
            item_sproc_t.stime_dlt = item_sproc_t.stime - begin_sproc_t.at(item_sproc_t.tgid).stime;
            item_sproc_t.time_dlt = item_sproc_t.utime_dlt + item_sproc_t.stime_dlt;
            item_sproc_t.pucpu = float(item_sproc_t.utime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pscpu = float(item_sproc_t.stime_dlt)/float(item_sproc_t.realtime);
            item_sproc_t.pcpu = item_sproc_t.pucpu + item_sproc_t.pscpu;
            vcswchs_dlt = item_sproc_t.vcswch - begin_sproc_t.at(item_sproc_t.tgid).vcswch;
            ncswchs_dlt = item_sproc_t.ncswch - begin_sproc_t.at(item_sproc_t.tgid).ncswch;
            item_sproc_t.vcswchs = int(float(vcswchs_dlt)/float(item_sproc_t.realtime));
            item_sproc_t.ncswchs = int(float(ncswchs_dlt)/float(item_sproc_t.realtime));
        } 
        it_sproc_t.push_back(item_sproc_t);
    }
}

void ReportProcs(SeqOptions &seq_option){
    string full_sresar_suffix = sresar_suffix;
    if(!seq_option.proc_gzip_disable){
        full_sresar_suffix = sresar_suffix + ".gz";
    }
    string full_cmdline_suffix = cmdline_suffix;
    if(!seq_option.proc_gzip_disable){
        full_cmdline_suffix = cmdline_suffix + ".gz";
    }

    list<sproc_t> begin_list_sproc_t;
    list<sproc_t> it_list_sproc_t;
    unordered_map<int, sproc_t> begin_sproc_t;
    list<sproc_t> it_sproc_t;
    SreProc it_sre_proc;
    it_sre_proc.SetSuffix(full_sresar_suffix);
    it_sre_proc.SetDataPath(seq_option.data_path);

    it_sre_proc.EnableAdjust();
    if(0 != it_sre_proc.MakeDataHourPath(seq_option.time_point_finish)){
        throw ssar_cli::THROW_FINISH_DATA_HOUR_PATH;
    }
    string finish_path                = it_sre_proc.GetDataHourPath();
    seq_option.time_point_finish_real = it_sre_proc.GetDataTimePointReal();    
    seq_option.adjust                 = it_sre_proc.GetAdjusted();
    it_sre_proc.DisableAdjust();

    chrono::system_clock::time_point it_time_point_begin = seq_option.time_point_begin;
    if(seq_option.adjust){
        it_time_point_begin = it_time_point_begin - chrono::duration<int>(60);
    }
    if(0 != it_sre_proc.MakeDataHourPath(it_time_point_begin)){
        throw ssar_cli::THROW_BEGIN_DATA_HOUR_PATH;
    }
    string begin_path = it_sre_proc.GetDataHourPath();
    seq_option.time_point_begin_real = it_sre_proc.GetDataTimePointReal();    
    seq_option.duration_real = chrono::duration_cast<chrono::seconds>(seq_option.time_point_finish_real - seq_option.time_point_begin_real);

    string finish_addon_path = "";
    string begin_addon_path = "";
    if(seq_option.has_cmdline){
        SreProc addon_sre_proc;
        addon_sre_proc.EnableAdjust();
        addon_sre_proc.SetAdjustUnit(1);
        addon_sre_proc.SetAdjustForward(true);
        addon_sre_proc.SetSuffix(full_cmdline_suffix);
        addon_sre_proc.SetDataPath(seq_option.data_path);
        if(0 == addon_sre_proc.MakeDataHourPath(seq_option.time_point_finish_real)){
            finish_addon_path = addon_sre_proc.GetDataHourPath();
        }
    }

    if(1 == ReadProcFileData(seq_option, begin_path, begin_addon_path, begin_list_sproc_t)){
        throw ssar_cli::THROW_BEGIN_DATA_FILE;
    }
    if(1 == ReadProcFileData(seq_option, finish_path, finish_addon_path,it_list_sproc_t)){
        throw ssar_cli::THROW_FINISH_DATA_FILE;
    }

    list<sproc_t>::const_iterator i_element = begin_list_sproc_t.cbegin();
    for(;i_element != begin_list_sproc_t.cend(); ++i_element){
        begin_sproc_t.insert(make_pair((*i_element).tgid, (*i_element)));
    }

    MakeRangeIndicators(it_list_sproc_t, begin_sproc_t, it_sproc_t, seq_option.duration_real.count());
   
    // sort it_sproc_t;
    it_sproc_t.sort([&](const sproc_t &itema, const sproc_t &itemb)->bool{
        auto i_element = seq_option.keys.cbegin();
        bool ret = false;
        for(; i_element != seq_option.keys.cend(); ++i_element){
            int greater = SprocCompare(itema, itemb, (*i_element));
            if(0 < greater){
                ret = true;
                break;
            }else if(0 > greater){
                ret = false;
                break;
            }
        }
        return ret;
    });

    if(seq_option.api){
        DisplayProcJson(seq_option, it_sproc_t);    
    }else{
        DisplayProcShell(seq_option, it_sproc_t);    
    }
}

void DisplayProcJson(SeqOptions &seq_option, list<sproc_t> &it_sproc_t){
    int i = 0;
    bool pollute = false;
    json it_json = json::array();
    list<sproc_t>::const_iterator i_element = it_sproc_t.cbegin();
    for(;i_element != it_sproc_t.cend(); ++i_element){
        pollute = false;
        json obj_json = json::object();
        auto j_element = seq_option.formats.cbegin();
        for(; j_element != seq_option.formats.cend(); ++j_element){
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            }else if ("collect_datetime" == (*j_element)){
                obj_json["collect_datetime"] = (*i_element).collect_datetime;
            }else if ("collect_time" == (*j_element)){
                obj_json["collect_time"] = (*i_element).collect_time;
            FORMAT_PERCENT2JSON(pcpu);
            FORMAT_PERCENT2JSON(pucpu);
            FORMAT_PERCENT2JSON(pscpu);
            FORMAT2JSON(s);
            FORMAT2JSON(tid);
            FORMAT2JSON(ppid);
            FORMAT2JSON(pgid);
            FORMAT2JSON(sid);
            FORMAT2JSON(nlwp);
            FORMAT2JSON(tgid);
            FORMAT2JSON(pid);
            FORMAT2JSON(psr);
            FORMAT2JSON(prio);
            FORMAT2JSON(nice);
            FORMAT2JSON(rtprio);
            FORMAT2JSON(sched);
            FORMAT2JSON(cls);
            FORMAT2JSON(flags);
            FORMAT2JSON(vcswch);
            FORMAT2JSON(ncswch);
            FORMAT2JSON(vcswchs);
            FORMAT2JSON(ncswchs);
            FORMAT2JSON(utime);
            FORMAT2JSON(stime);
            FORMAT2JSON(cutime);
            FORMAT2JSON(cstime);
            FORMAT2JSON(start_time);
            FORMAT2JSON(etimes);
            FORMAT2JSON(tty);
            FORMAT2JSON(tpgid);
            FORMAT2JSON(rss); 
            FORMAT2JSON(shr); 
            FORMAT2JSON(alarm);
            FORMAT2JSON(size);
            FORMAT2JSON(resident);
            FORMAT2JSON(share);
            FORMAT2JSON(trs);
            FORMAT2JSON(lrs);
            FORMAT2JSON(drs);
            FORMAT2JSON(dt);
            FORMAT2JSON(vsize);
            FORMAT2JSON(rss_rlim);
            FORMAT2JSON(min_flt);
            FORMAT2JSON(maj_flt);
            FORMAT2JSON(cmin_flt);
            FORMAT2JSON(cmaj_flt);
            FORMAT2JSON(cmd);
            FORMAT2JSON(fullcmd);
            FORMAT2JSON(cmdline);
            FORMAT2JSON(time);
            FORMAT2JSON(realtime);
            FORMAT2JSON(time_dlt);
            FORMAT2JSON(utime_dlt);
            FORMAT2JSON(stime_dlt);
            FORMAT2JSON(pucpu);
            FORMAT2JSON(pscpu);
            FORMAT2JSON(maj_dlt);
            FORMAT2JSON(min_dlt);
            FORMAT2JSON(rss_dlt);
            FORMAT2JSON(shr_dlt);
            FORMAT2JSON(etime);
            FORMAT2JSON(cputime);
            FORMAT2JSON(cpu_utime);
            FORMAT2JSON(cpu_stime);
            FORMAT2JSON(start_datetime);
            FORMAT2JSON(begin_datetime);
            FORMAT2JSON(finish_datetime);
            FORMAT2JSON(collect_datetime);
            FORMAT2JSON(b);
            FORMAT2JSON(record_type);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
        }
        if(seq_option.purify && pollute){
            continue;
        }
        it_json[i] = obj_json;
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
    cout << it_json.dump() << endl;
}

void DisplayProcShell(SeqOptions &seq_option, list<sproc_t> &it_sproc_t){
    int i = 0;
    bool pollute = false;
    list<sproc_t>::const_iterator i_element = it_sproc_t.cbegin();
    for(;i_element != it_sproc_t.cend(); ++i_element){
        if(i % AREA_LINE == 0 && !seq_option.noheaders && !pollute){
            stringstream it_shell;
            auto j_element = seq_option.formats.cbegin();
            for(; j_element != seq_option.formats.cend(); ++j_element){
                auto j_field_found = field_attribute.find(*j_element);
                if(j_field_found == field_attribute.cend()){
                    throw ssar_cli::THROW_NO_KEY + (*j_element);
                }
                string it_adjust = field_attribute.at((*j_element)).at("adjust");
                it_shell << resetiosflags(ios::adjustfield);
                if("left" == it_adjust){
                    it_shell << setiosflags(ios::left);
                }else{
                    it_shell << setiosflags(ios::right);
                }
                //it_shell << setfill('*');
                it_shell << setw(stoi(field_attribute.at((*j_element)).at("f_width"))) << (*j_element);
                it_shell << " ";
            }
            string it_shell_str = it_shell.str();
            cout << endl;
            cout << it_shell_str << endl;
        }

        pollute = false;
        stringstream it_shell;
        auto j_element = seq_option.formats.cbegin();
        for(; j_element != seq_option.formats.cend(); ++j_element){
            string it_field_format = "";
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            }else if ("collect_datetime" == (*j_element)){
                it_shell << setw(stoi(field_attribute.at((*j_element)).at("f_width")));
                it_shell << resetiosflags(ios::adjustfield);
                it_shell << setiosflags(ios::left);
                it_shell << (*i_element).collect_datetime;
            }else if ("collect_time" == (*j_element)){
                it_shell << setw(stoi(field_attribute.at((*j_element)).at("f_width")));
                it_shell << resetiosflags(ios::adjustfield);
                it_shell << setiosflags(ios::left);
                it_shell << (*i_element).collect_time;
            FORMAT_PERCENT2SHELL(pcpu);
            FORMAT_PERCENT2SHELL(pucpu);
            FORMAT_PERCENT2SHELL(pscpu);
            FORMAT2SHELL(s);
            FORMAT2SHELL(tid);
            FORMAT2SHELL(ppid);
            FORMAT2SHELL(pgid);
            FORMAT2SHELL(sid);
            FORMAT2SHELL(nlwp);
            FORMAT2SHELL(tgid);
            FORMAT2SHELL(pid);
            FORMAT2SHELL(psr);
            FORMAT2SHELL(prio);
            FORMAT2SHELL(nice);
            FORMAT2SHELL(rtprio);
            FORMAT2SHELL(sched);
            FORMAT2SHELL(cls);
            FORMAT2SHELL(flags);
            FORMAT2SHELL(vcswch);
            FORMAT2SHELL(ncswch);
            FORMAT2SHELL(vcswchs);
            FORMAT2SHELL(ncswchs);
            FORMAT2SHELL(utime);
            FORMAT2SHELL(stime);
            FORMAT2SHELL(cutime);
            FORMAT2SHELL(cstime);
            FORMAT2SHELL(start_time);
            FORMAT2SHELL(etimes);
            FORMAT2SHELL(tty);
            FORMAT2SHELL(tpgid);
            FORMAT2SHELL(rss); 
            FORMAT2SHELL(shr); 
            FORMAT2SHELL(alarm);
            FORMAT2SHELL(size);
            FORMAT2SHELL(resident);
            FORMAT2SHELL(share);
            FORMAT2SHELL(trs);
            FORMAT2SHELL(lrs);
            FORMAT2SHELL(drs);
            FORMAT2SHELL(dt);
            FORMAT2SHELL(vsize);
            FORMAT2SHELL(rss_rlim);
            FORMAT2SHELL(min_flt);
            FORMAT2SHELL(maj_flt);
            FORMAT2SHELL(cmin_flt);
            FORMAT2SHELL(cmaj_flt);
            FORMAT2SHELL(cmd);
            FORMAT2SHELL(fullcmd);
            FORMAT2SHELL(cmdline);
            FORMAT2SHELL(time);
            FORMAT2SHELL(realtime);
            FORMAT2SHELL(time_dlt);
            FORMAT2SHELL(utime_dlt);
            FORMAT2SHELL(stime_dlt);
            FORMAT2SHELL(pucpu);
            FORMAT2SHELL(pscpu);
            FORMAT2SHELL(maj_dlt);
            FORMAT2SHELL(min_dlt);
            FORMAT2SHELL(rss_dlt);
            FORMAT2SHELL(shr_dlt);
            FORMAT2SHELL(etime);
            FORMAT2SHELL(cputime);
            FORMAT2SHELL(cpu_utime);
            FORMAT2SHELL(cpu_stime);
            FORMAT2SHELL(start_datetime);
            FORMAT2SHELL(begin_datetime);
            FORMAT2SHELL(finish_datetime);
            FORMAT2SHELL(b);
            FORMAT2SHELL(record_type);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
            it_shell << " ";
        }
        if(seq_option.purify && pollute){
            continue;
        }
        cout << it_shell.str() << endl;
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
}

/**********************************************************************************
 *                                                                                *
 *                               load part                                        * 
 *                                                                                *
 **********************************************************************************/

int ReadStackFileData(SeqOptions &seq_option, list<load2p_t> &it_list_stack_t){
    string it_path = GetFilePath(seq_option.time_point_collect, seq_option.data_path, "stack");
    ifstream it_file;
    it_file.open(it_path, ios_base::in);
    if(!it_file.is_open()){
        chrono::system_clock::time_point adjust_time_point = seq_option.time_point_collect + chrono::duration<int>(1);  // adjust 1s.
        it_path = GetFilePath(adjust_time_point, seq_option.data_path, "stack");
        it_file.open(it_path, ios_base::in);
        if(!it_file.is_open()){
            return -1;
        }
    }

    string file_contents;
    load2p_t item_stack_t;
    int comma_pos;
    while(it_file.good()){
        getline(it_file, file_contents);
        if(file_contents.empty()){
            break;
        }
        istringstream ifile_contents(file_contents);
        ifile_contents >> item_stack_t.tid;
        ifile_contents >> item_stack_t.nwchan;
        ifile_contents >> item_stack_t.stackinfo;
        item_stack_t.full_stackinfo = item_stack_t.nwchan + ";" + item_stack_t.stackinfo;
 
        comma_pos = item_stack_t.stackinfo.find(',');
        if(comma_pos == string::npos){
            item_stack_t.wchan = item_stack_t.stackinfo;
        }else{
            item_stack_t.wchan = item_stack_t.stackinfo.substr(0, comma_pos);
        }

        it_list_stack_t.push_back(item_stack_t);
    }
    it_file.close();

    return 0;
}

int ReadLoadrdFileData(SeqOptions &seq_option, list<load2p_t> &it_list_load2p_t){
    string it_path = GetFilePath(seq_option.time_point_collect, seq_option.data_path, "loadrd");
    ifstream it_file;
    it_file.open(it_path, ios_base::in);
    if(!it_file.is_open()){
        chrono::system_clock::time_point adjust_time_point = seq_option.time_point_collect + chrono::duration<int>(1);  // adjust 1s.
        it_path = GetFilePath(adjust_time_point, seq_option.data_path, "loadrd");
        it_file.open(it_path, ios_base::in);
        if(!it_file.is_open()){
            return -1;
        }
    }

    string file_contents;
    load2p_t item_load2p_t;
    while(it_file.good()){
        getline(it_file, file_contents);
        if(file_contents.empty()){
            break;
        }
        istringstream ifile_contents(file_contents);
        ifile_contents >> item_load2p_t.s;
        ifile_contents >> item_load2p_t.tid;
        ifile_contents >> item_load2p_t.tgid;
        ifile_contents >> item_load2p_t.psr;
        ifile_contents >> item_load2p_t.prio;
        if(!(ifile_contents >> item_load2p_t.cmd)){
            item_load2p_t.cmd = "";
        }
        item_load2p_t.pid = item_load2p_t.tgid;
        
        it_list_load2p_t.push_back(item_load2p_t);
    }
    it_file.close();

    return 0;
}

int ReadLoad5sFileData(SeqOptions &seq_option, chrono::system_clock::time_point &it_time_point, list<load5s_t> &it_list_load5s_t){
    string it_datetime_point = FmtDatetime(it_time_point, "%Y%m%d%H");
    string it_path = seq_option.data_path + it_datetime_point + "/" + it_datetime_point + "_load5s";
    
    ifstream it_file;
    it_list_load5s_t.clear();
    it_file.open(it_path, ios_base::in);
    if(!it_file.is_open()){
        return -1;
    }
    string file_contents;
    load5s_t item_load5s_t;
    while(it_file.good()){
        getline(it_file, file_contents);
        if(file_contents.empty()){
            break;
        }
        istringstream ifile_contents(file_contents);
        ifile_contents >> item_load5s_t.collect_time;
        ifile_contents >> item_load5s_t.threads;
        ifile_contents >> item_load5s_t.runq;
        ifile_contents >> item_load5s_t.load1;
        ifile_contents >> item_load5s_t.load5s;
        ifile_contents >> item_load5s_t.load5s_type;
        ifile_contents >> item_load5s_t.load5s_state;
        if(!(ifile_contents >> item_load5s_t.zoom_state)){
            item_load5s_t.zoom_state = 0;
        }

        it_list_load5s_t.push_back(item_load5s_t);
    }
    it_file.close();

    return 0;
}

void ReportLoad2p(SeqOptions &seq_option){
    list<load2p_t> it_list_loadrd_t;
    if(ReadLoadrdFileData(seq_option, it_list_loadrd_t) == -1){
        string exception_info = "ReadLoadrdFileData failed. Make sure the param -c <collect time> is correct, act field is not -.";
        throw DataException(exception_info);
    }
    list<load2p_t> it_list_stack_t;
    if(ReadStackFileData(seq_option, it_list_stack_t) == -1){
        string exception_info = "ReadStackFileData failed.";
        throw DataException(exception_info);
    }

    string full_sresar_suffix = sresar_suffix;
    if(!seq_option.proc_gzip_disable){
        full_sresar_suffix = sresar_suffix + ".gz";
    }
    string full_cmdline_suffix = cmdline_suffix;
    if(!seq_option.proc_gzip_disable){
        full_cmdline_suffix = cmdline_suffix + ".gz";
    }

    SreProc it_sre_proc;
    it_sre_proc.SetSuffix(full_sresar_suffix);
    it_sre_proc.SetDataPath(seq_option.data_path);
    SreProc addon_sre_proc;
    addon_sre_proc.EnableAdjust();
    addon_sre_proc.SetAdjustUnit(1);
    addon_sre_proc.SetAdjustForward(true);
    addon_sre_proc.SetSuffix(full_cmdline_suffix);
    addon_sre_proc.SetDataPath(seq_option.data_path);

    chrono::system_clock::time_point it_time_point_collect = seq_option.time_point_collect;
    list<sproc_t> collect_list_sproc_t;
    if(it_sre_proc.MakeDataHourPath(it_time_point_collect) == 0){
        string collect_path = it_sre_proc.GetDataHourPath();
        string addon_path = "";
        if(0 == addon_sre_proc.MakeDataHourPath(it_sre_proc.GetDataTimePointReal())){
            addon_path = addon_sre_proc.GetDataHourPath();
        }
        if(1 == ReadProcFileData(seq_option, collect_path, addon_path, collect_list_sproc_t)){
            //string exception_info = "ReadProcFileDat e failed.";
        }
    }

    unordered_map<int, sproc_t> collect_sproc_t;
    list<sproc_t>::const_iterator i_element = collect_list_sproc_t.cbegin();
    for(;i_element != collect_list_sproc_t.cend(); ++i_element){
        collect_sproc_t.insert(make_pair((*i_element).tgid, (*i_element)));
    }

    chrono::system_clock::time_point alternative_time_point_collect = seq_option.time_point_collect + chrono::duration<int>(60);
    unordered_map<int, sproc_t> alternative_collect_sproc_t;
    if(0 == it_sre_proc.MakeDataHourPath(alternative_time_point_collect)){
        string alternative_collect_path = it_sre_proc.GetDataHourPath();
        list<sproc_t> alternative_collect_list_sproc_t;
        string alternative_addon_path = "";
        if(0 == addon_sre_proc.MakeDataHourPath(it_sre_proc.GetDataTimePointReal())){
            alternative_addon_path = addon_sre_proc.GetDataHourPath();
        }
        if(1 == ReadProcFileData(seq_option, alternative_collect_path, alternative_addon_path, alternative_collect_list_sproc_t)){
            i_element = alternative_collect_list_sproc_t.cbegin();
            for(;i_element != alternative_collect_list_sproc_t.cend(); ++i_element){
                alternative_collect_sproc_t.insert(make_pair((*i_element).tgid, (*i_element)));
            }

        }
    }

    unordered_map<int,load2p_t> it_map_loadrd_t;
    map<int, load2p_t>  psr_counts;
    map<int, load2p_t>  loadr_counts;
    map<int, load2p_t>  loadd_counts;
    int                 loadd_number = 0;
    list<load2p_t>      it_loadrd_t;
    load2p_t            item_loadrd_t;
    list<load2p_t>::const_iterator l_element = it_list_loadrd_t.cbegin();
    for(;l_element != it_list_loadrd_t.cend(); ++l_element){
        item_loadrd_t = (*l_element);
        if(collect_sproc_t.find(l_element->tgid) != collect_sproc_t.cend()){
            item_loadrd_t.cmdline = collect_sproc_t[l_element->tgid].cmdline;
        }else if(alternative_collect_sproc_t.find(l_element->tgid) != alternative_collect_sproc_t.cend()){
            item_loadrd_t.cmdline = alternative_collect_sproc_t[l_element->tgid].cmdline;
        }else{
            item_loadrd_t.cmdline = "-";
        }

        if(l_element->s == "R"){
            if(psr_counts.find(l_element->psr) == psr_counts.cend()){
                psr_counts[l_element->psr].count = 1;
                psr_counts[l_element->psr].psr   = l_element->psr;
            }else{
                psr_counts[l_element->psr].count = psr_counts[l_element->psr].count + 1;
            }
            if(loadr_counts.find(l_element->tgid) == loadr_counts.cend()){
                loadr_counts[l_element->tgid].count   = 1;
                loadr_counts[l_element->tgid].s       = "R";
                loadr_counts[l_element->tgid].pid     = l_element->tgid;
                loadr_counts[l_element->tgid].cmd     = item_loadrd_t.cmd;
                loadr_counts[l_element->tgid].cmdline = item_loadrd_t.cmdline;
            }else{
                loadr_counts[l_element->tgid].count   = loadr_counts[l_element->tgid].count + 1;
            }
        }else{
            loadd_number++;
            if(loadd_counts.find(l_element->tgid) == loadd_counts.cend()){
                loadd_counts[l_element->tgid].count   = 1;
                loadd_counts[l_element->tgid].s       = "D";
                loadd_counts[l_element->tgid].pid     = l_element->tgid;
                loadd_counts[l_element->tgid].cmd     = item_loadrd_t.cmd;
                loadd_counts[l_element->tgid].cmdline = item_loadrd_t.cmdline;
            }else{
                loadd_counts[l_element->tgid].count   = loadd_counts[l_element->tgid].count + 1;
            }
        }
        it_loadrd_t.push_back(item_loadrd_t); 
        it_map_loadrd_t[item_loadrd_t.tid] = item_loadrd_t;
    }

    list<load2p_t>        it_stack_t;
    map<string, load2p_t> stackinfo_counts;
    load2p_t              item_stack_t;
    list<load2p_t>::const_iterator s_element = it_list_stack_t.cbegin();
    for(;s_element != it_list_stack_t.cend(); ++s_element){
        item_stack_t = (*s_element);
        if(stackinfo_counts.find(s_element->full_stackinfo) == stackinfo_counts.cend()){
            stackinfo_counts[s_element->full_stackinfo].count     = 1;
            stackinfo_counts[s_element->full_stackinfo].full_stackinfo = s_element->full_stackinfo;
            stackinfo_counts[s_element->full_stackinfo].wchan = s_element->wchan;
            stackinfo_counts[s_element->full_stackinfo].nwchan = s_element->nwchan;
            stackinfo_counts[s_element->full_stackinfo].stackinfo = s_element->stackinfo;
        }else{
            stackinfo_counts[s_element->full_stackinfo].count = stackinfo_counts[s_element->full_stackinfo].count + 1;
        }
        if(loadd_number < 100){
            stackinfo_counts[s_element->full_stackinfo].s_unit = "number";
        }else{
            stackinfo_counts[s_element->full_stackinfo].s_unit = "percent";
        }

        if(it_map_loadrd_t.find(s_element->tid) != it_map_loadrd_t.cend()){
            item_stack_t.pid = it_map_loadrd_t[s_element->tid].pid;
            item_stack_t.cmd = it_map_loadrd_t[s_element->tid].cmd;
        }else{
            item_stack_t.pid = 0;
            item_stack_t.cmd = "-";
        }
        it_stack_t.push_back(item_stack_t); 
    }

    list<load2p_t>  it_loadr; 
    list<load2p_t>  it_loadd; 
    list<load2p_t>  it_psr; 
    list<load2p_t>  it_stackinfo;
    transform(loadr_counts.begin(), loadr_counts.end(), back_inserter(it_loadr), [](pair<int, load2p_t> i){return i.second;});
    transform(loadd_counts.begin(), loadd_counts.end(), back_inserter(it_loadd), [](pair<int, load2p_t> i){return i.second;});
    transform(psr_counts.begin(),   psr_counts.end(),   back_inserter(it_psr),   [](pair<int, load2p_t> i){return i.second;});
    transform(stackinfo_counts.begin(),stackinfo_counts.end(),back_inserter(it_stackinfo),[](pair<string, load2p_t> i){return i.second;});

    it_loadr.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.count > itemb.count){
            return true;
        }else{
            return false;
        }
    });
    it_loadd.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.count > itemb.count){
            return true;
        }else{
            return false;
        }
    });
    it_psr.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.count > itemb.count){
            return true;
        }else{
            return false;
        }
    });
    it_stackinfo.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.count > itemb.count){
            return true;
        }else{
            return false;
        }
    });
    it_loadrd_t.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.tid > itemb.tid){
            return true;
        }else{
            return false;
        }
    });
    it_stack_t.sort([&](const load2p_t &itema, const load2p_t &itemb)->bool{
        if(itema.stackinfo > itemb.stackinfo){
            return true;
        }else{
            return false;
        }
    });

    if(seq_option.api){
        unordered_set<string> formats_set(begin(seq_option.formats),end(seq_option.formats));
        json it_json = json::object();
        auto search_format = formats_set.find("loadr");
        if(search_format != formats_set.cend()){
            it_json["loadr"] = DisplayLoad2pJson(seq_option, load2p_loadpid_formats, it_loadr);
        }
        search_format = formats_set.find("loadd");
        if(search_format != formats_set.cend()){
            it_json["loadd"] = DisplayLoad2pJson(seq_option, load2p_loadpid_formats, it_loadd);
        }
        search_format = formats_set.find("psr");
        if(search_format != formats_set.cend()){
            it_json["psr"] = DisplayLoad2pJson(seq_option, load2p_psr_formats, it_psr);
        }
        search_format = formats_set.find("stackinfo");
        if(search_format != formats_set.cend()){
            it_json["stackinfo"] = DisplayLoad2pJson(seq_option, load2p_stackinfo_formats, it_stackinfo);
        }
        search_format = formats_set.find("loadrd");
        if(search_format != formats_set.cend()){
            it_json["loadrd"] = DisplayLoad2pJson(seq_option, load2p_loadrd_formats, it_loadrd_t);
        }
        search_format = formats_set.find("stack");
        if(search_format != formats_set.cend()){
            it_json["stack"] = DisplayLoad2pJson(seq_option, load2p_stack_formats, it_stack_t);
        }
        cout << it_json.dump() << endl;
    }else{
        unordered_set<string> formats_set(begin(seq_option.formats),end(seq_option.formats));
        auto search_format = formats_set.find("loadr");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_loadpid_formats, it_loadr);
        }
        search_format = formats_set.find("loadd");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_loadpid_formats, it_loadd);
        }
        search_format = formats_set.find("psr");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_psr_formats, it_psr);
        }
        search_format = formats_set.find("stackinfo");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_stackinfo_formats, it_stackinfo);
        }
        search_format = formats_set.find("loadrd");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_loadrd_formats, it_loadrd_t);
        }
        search_format = formats_set.find("stack");
        if(search_format != formats_set.cend()){
            DisplayLoad2pShell(seq_option, load2p_stack_formats, it_stack_t);
        }
    }
}

bool load5s_comparator(load5s_t first, load5s_t second){ 
    return first.collect_time < second.collect_time; 
}

void ReportLoad5s(SeqOptions &seq_option){
    int read_file_ret;
    chrono::system_clock::time_point it_time_point = seq_option.time_point_finish;
    list<load5s_t> it_list_load5s_t;
    list<load5s_t> it_load5s_t;
    for(;;){
        if(chrono::time_point_cast<chrono::hours>(it_time_point) < chrono::time_point_cast<chrono::hours>(seq_option.time_point_begin)){
            break;
        }
        read_file_ret = ReadLoad5sFileData(seq_option, it_time_point, it_list_load5s_t);
        if(read_file_ret != -1){
            it_load5s_t.merge(it_list_load5s_t, load5s_comparator);
        }
        it_time_point = it_time_point - chrono::duration<int>(60 * 60);
    }

    int data_hour_path_ret;
    active_t it_act;
    Load5s it_load5s;
    it_load5s.SetDataPath(seq_option.data_path);
    it_load5s.SetSuffix("loadrd");

    chrono::system_clock::time_point collect_time_point;
    load5s_t current_load5s_t;
    list<load5s_t> fill_load5s_t;
    list<load5s_t>::const_iterator i_element = it_load5s_t.cbegin();
    for(;i_element != it_load5s_t.cend(); ++i_element){
        current_load5s_t = (*i_element);
        collect_time_point = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::from_time_t(current_load5s_t.collect_time));
        if(collect_time_point < seq_option.time_point_begin){
           continue;
        }else if(collect_time_point > seq_option.time_point_finish){
           break;
        }
        if(seq_option.yes && !i_element->load5s_state){
           continue;
        }
        if(seq_option.zoom && !i_element->zoom_state){
           continue;
        }
        if(i_element->load5s_state){
            if(it_load5s.GetActiveCounts(i_element->collect_time, it_act) == -1){
                current_load5s_t.actr = 0;
                current_load5s_t.actd = 0;
            }else{
                current_load5s_t.actr = it_act.actr;
                current_load5s_t.actd = it_act.actd;
            }
            current_load5s_t.act = current_load5s_t.actr + current_load5s_t.actd;
            if(current_load5s_t.load5s){
                current_load5s_t.act_rto = (double)current_load5s_t.act/(double)current_load5s_t.load5s;
            }else{
                current_load5s_t.act_rto = 0;
            }
            if(current_load5s_t.runq){
                current_load5s_t.actr_rto = (double)current_load5s_t.actr/(double)current_load5s_t.runq;
            }else{
                current_load5s_t.actr_rto = 0;
            }
        }
        current_load5s_t.collect_datetime = FmtDatetime(collect_time_point);
        if(current_load5s_t.load5s_type == 5){
            current_load5s_t.stype = "5s";
        }else{
            current_load5s_t.stype = "1m";
        }
        if(current_load5s_t.load5s_state == 2){
            current_load5s_t.sstate = "Y";
        }else{
            current_load5s_t.sstate = "N";
        }
        if(current_load5s_t.zoom_state == 3){
            current_load5s_t.zstate = "Z";
        }else{
            current_load5s_t.zstate = "U";
        }

        fill_load5s_t.push_back(current_load5s_t);     
    }

    if(seq_option.api){
        DisplayLoad5sJson(seq_option, fill_load5s_t);    
    }else{
        DisplayLoad5sShell(seq_option, fill_load5s_t);    
    }
}

void DisplayLoad5sShell(SeqOptions &seq_option, list<load5s_t> &it_load5s_t){
    int i = 0;
    list<load5s_t>::const_iterator i_element = it_load5s_t.cbegin();
    for(;i_element != it_load5s_t.cend(); ++i_element){
        if(i % AREA_LINE == 0 && !seq_option.noheaders){
            stringstream it_shell;
            auto j_element = seq_option.formats.cbegin();
            for(; j_element != seq_option.formats.cend(); ++j_element){
                auto j_field_found = load5s_field_attribute.find(*j_element);
                if(j_field_found == load5s_field_attribute.cend()){
                    throw ssar_cli::THROW_NO_KEY + (*j_element);
                }
                string it_adjust = load5s_field_attribute.at((*j_element)).at("adjust");
                it_shell << resetiosflags(ios::adjustfield);
                if("left" == it_adjust){
                    it_shell << setiosflags(ios::left);
                }else{
                    it_shell << setiosflags(ios::right);
                }
                it_shell << setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width"))) << (*j_element);
                it_shell << " ";
            }
            string it_shell_str = it_shell.str();
            cout << endl;
            cout << it_shell_str << endl;
        }

        stringstream it_shell;
        auto j_element = seq_option.formats.cbegin();
        for(; j_element != seq_option.formats.cend(); ++j_element){
            string it_field_format = "";
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            }else if ("collect_datetime" == (*j_element)){
                it_shell << setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width")));
                it_shell << resetiosflags(ios::adjustfield);
                it_shell << setiosflags(ios::left);
                it_shell << (*i_element).collect_datetime;
            }else if ("collect_time" == (*j_element)){
                it_shell << setw(stoi(load5s_field_attribute.at((*j_element)).at("f_width")));
                it_shell << resetiosflags(ios::adjustfield);
                it_shell << setiosflags(ios::left);
                it_shell << (*i_element).collect_time;
            FORMAT_LOAD5S2SHELL(threads);
            FORMAT_LOAD5S2SHELL(load1);
            FORMAT_LOAD5S2SHELL(load5s);
            FORMAT_LOAD5S2SHELL(stype);
            FORMAT_LOAD5S2SHELL(sstate);
            FORMAT_LOAD5S2SHELL(zstate);
            FORMAT_LOAD5S2SHELL(act);
            FORMAT_LOAD5S2SHELL(runq);
            FORMAT_LOAD5S2SHELL(actr);
            FORMAT_LOAD5S2SHELL(actd);
            FORMAT_LOAD5S_PERCENT2SHELL(act_rto);
            FORMAT_LOAD5S_PERCENT2SHELL(actr_rto);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
            it_shell << " ";
        }
        cout << it_shell.str() << endl;
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
}

void DisplayLoad5sJson(SeqOptions &seq_option, list<load5s_t> &it_load5s_t){
    int i = 0;
    json it_json = json::array();
    list<load5s_t>::const_iterator i_element = it_load5s_t.cbegin();
    for(;i_element != it_load5s_t.cend(); ++i_element){
        auto j_element = seq_option.formats.cbegin();
        for(; j_element != seq_option.formats.cend(); ++j_element){
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            }else if ("collect_datetime" == (*j_element)){
                it_json[i]["collect_datetime"] = (*i_element).collect_datetime;
            }else if ("collect_time" == (*j_element)){
                it_json[i]["collect_time"] = (*i_element).collect_time;
            FORMAT_LOAD5S2JSON(threads);
            FORMAT_LOAD5S2JSON(load1);
            FORMAT_LOAD5S2JSON(load5s);
            FORMAT_LOAD5S2JSON(stype);
            FORMAT_LOAD5S2JSON(sstate);
            FORMAT_LOAD5S2JSON(zstate);
            FORMAT_LOAD5S2JSON(act);
            FORMAT_LOAD5S2JSON(runq);
            FORMAT_LOAD5S2JSON(actr);
            FORMAT_LOAD5S2JSON(actd);
            FORMAT_LOAD5S_PERCENT2JSON(act_rto);
            FORMAT_LOAD5S_PERCENT2JSON(actr_rto);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
        }
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
    cout << it_json.dump() << endl;
}

void DisplayLoad2pShell(SeqOptions &seq_option, list<string> it_formats, list<load2p_t> &it_load2p_data){
    int i = 0;
    list<load2p_t>::const_iterator i_element = it_load2p_data.cbegin();
    for(;i_element != it_load2p_data.cend(); ++i_element){
        if(i == 0 && !seq_option.noheaders){
            stringstream it_shell;
            it_shell << endl << endl;
            auto j_element = it_formats.cbegin();
            for(; j_element != it_formats.cend(); ++j_element){
                auto j_field_found = load2p_field_attribute.find(*j_element);
                if(j_field_found == load2p_field_attribute.cend()){
                    throw ssar_cli::THROW_NO_KEY + (*j_element);
                }
                string it_adjust = load2p_field_attribute.at((*j_element)).at("adjust");
                it_shell << resetiosflags(ios::adjustfield);
                if("left" == it_adjust){
                    it_shell << setiosflags(ios::left);
                }else{
                    it_shell << setiosflags(ios::right);
                }
                it_shell << setw(stoi(load2p_field_attribute.at((*j_element)).at("f_width"))) << (*j_element);
                it_shell << " ";
            }
            string it_shell_str = it_shell.str();
            cout << it_shell_str << endl;
        }

        stringstream it_shell;
        auto j_element = it_formats.cbegin();
        for(; j_element != it_formats.cend(); ++j_element){
            string it_field_format = "";
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            FORMAT_LOAD2P2SHELL(count);
            FORMAT_LOAD2P2SHELL(pid);
            FORMAT_LOAD2P2SHELL(cmd);
            FORMAT_LOAD2P2SHELL(cmdline);
            FORMAT_LOAD2P2SHELL(s);
            FORMAT_LOAD2P2SHELL(tid);
            FORMAT_LOAD2P2SHELL(tgid);
            FORMAT_LOAD2P2SHELL(psr);
            FORMAT_LOAD2P2SHELL(prio);
            FORMAT_LOAD2P2SHELL(wchan);
            FORMAT_LOAD2P2SHELL(nwchan);
            FORMAT_LOAD2P2SHELL(stackinfo);
            FORMAT_LOAD2P2SHELL(full_stackinfo);
            FORMAT_LOAD2P2SHELL(s_unit);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
            it_shell << " ";
        }
        cout << it_shell.str() << endl;
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
}

json DisplayLoad2pJson(SeqOptions &seq_option, list<string> &it_formats, list<load2p_t> &it_load2p_data){
    int i = 0;
    json it_json = json::array();
    list<load2p_t>::const_iterator i_element = it_load2p_data.cbegin();
    for(;i_element != it_load2p_data.cend(); ++i_element){
        auto j_element = it_formats.cbegin();
        for(; j_element != it_formats.cend(); ++j_element){
            if((*j_element).empty()){
                throw ssar_cli::THROW_KEY_EMPTY;
            FORMAT_LOAD2P2JSON(count);
            FORMAT_LOAD2P2JSON(pid);
            FORMAT_LOAD2P2JSON(cmd);
            FORMAT_LOAD2P2JSON(cmdline);
            FORMAT_LOAD2P2JSON(s);
            FORMAT_LOAD2P2JSON(tid);
            FORMAT_LOAD2P2JSON(tgid);
            FORMAT_LOAD2P2JSON(psr);
            FORMAT_LOAD2P2JSON(prio);
            FORMAT_LOAD2P2JSON(wchan);
            FORMAT_LOAD2P2JSON(nwchan);
            FORMAT_LOAD2P2JSON(stackinfo);
            FORMAT_LOAD2P2JSON(full_stackinfo);
            FORMAT_LOAD2P2JSON(s_unit);
            }else{
                throw ssar_cli::THROW_NO_KEY + (*j_element);
            }
        }
        i++;
        if(seq_option.lines > 0 && i >= seq_option.lines){
            break;
        }
    }
    return it_json;
}

/**********************************************************************************
 *                                                                                *
 *                               sys part                                         * 
 *                                                                                *
 **********************************************************************************/

void DisplayLiveSysJson(SeqOptions &seq_option, int k, chrono::system_clock::time_point it_time_point_collect, bool first_combine, unordered_map<string, string> &it_combines){
    indicator_t it_indicator_t;
    string combine_key;
    string combine_delim;
    bool pollute = false;

    json obj_json = json::object();
    stringstream ss_json_field;
    obj_json["collect_datetime"] = FmtDatetime(it_time_point_collect);
    auto m_element = seq_option.formated.cbegin();
    for(; m_element != seq_option.formated.cend(); ++m_element){
        it_indicator_t = (*m_element);
        ss_json_field.str("");
        if(first_combine && it_indicator_t.metric == "d"){
            pollute = true;
            ss_json_field << "-";
        }else{
            if(it_indicator_t.position == "a"){
                if(it_indicator_t.line_begin.find(':') != string::npos){
                    combine_delim = "|";
                }else{
                    combine_delim = ":";
                }
                combine_key = it_indicator_t.cfile + combine_delim + it_indicator_t.line_begin + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
            }else{
                combine_delim = ":";
                combine_key = it_indicator_t.cfile + combine_delim + to_string(it_indicator_t.line) + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
            }
            ss_json_field << it_combines[combine_key];
        }
        obj_json[it_indicator_t.alias] = ss_json_field.str();
    }
    if(seq_option.purify && pollute){
        return;
    }
    cout << obj_json.dump() << endl;
}

void DisplayLiveSysShell(SeqOptions &seq_option, int k, chrono::system_clock::time_point it_time_point_collect, bool first_combine, unordered_map<string, string> &it_combines){
    indicator_t it_indicator_t;
    string combine_key;
    string combine_delim;
    bool pollute = false;

    if(k % AREA_LINE == 0 && !seq_option.noheaders){
        stringstream it_shell;
        it_shell << resetiosflags(ios::adjustfield);
        it_shell << setiosflags(ios::left);
        //it_shell << setfill('*');
        it_shell << setw(19) << "collect_datetime";
        it_shell << " ";

        auto h_element = seq_option.formated.cbegin();
        for(; h_element != seq_option.formated.cend(); ++h_element){
            it_indicator_t = (*h_element);
            it_shell << resetiosflags(ios::adjustfield);
            it_shell << setiosflags(ios::right);
            //it_shell << setfill('*');
            if(it_indicator_t.metric == "d"){
                it_shell << setw(it_indicator_t.width - 2);
            }else{
                it_shell << setw(it_indicator_t.width);
            }
            it_shell << it_indicator_t.alias;
            if(it_indicator_t.metric == "d"){
                it_shell << "/s";
            }
            it_shell << " ";
        }
        string it_shell_str = it_shell.str();
        cout << endl;
        cout << it_shell_str << endl;
    }

    stringstream it_shell;
    it_shell << resetiosflags(ios::adjustfield);
    it_shell << setiosflags(ios::left);
    //it_shell << setfill('*');
    it_shell << setw(19) << FmtDatetime(it_time_point_collect);
    it_shell << " ";
    auto m_element = seq_option.formated.cbegin();
    for(; m_element != seq_option.formated.cend(); ++m_element){
        it_indicator_t = (*m_element);
        it_shell << resetiosflags(ios::adjustfield);
        it_shell << setiosflags(ios::right);
        //it_shell << setfill('*');
        it_shell << setw(it_indicator_t.width);
        if(first_combine && it_indicator_t.metric == "d"){
            pollute = true;
            it_shell << "-";
        }else{
            if(it_indicator_t.position == "a"){
                if(it_indicator_t.line_begin.find(':') != string::npos){
                    combine_delim = "|";
                }else{
                    combine_delim = ":";
                }
                combine_key = it_indicator_t.cfile + combine_delim + it_indicator_t.line_begin + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
            }else{
                combine_delim = ":";
                combine_key = it_indicator_t.cfile + combine_delim + to_string(it_indicator_t.line) + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
            }
            it_shell << it_combines[combine_key];
        }
        it_shell << " ";
    }
    if(seq_option.purify && pollute){
        return;
    }
    string it_shell_str = it_shell.str();
    cout << it_shell_str << endl;
}

void ReportLiveSys(SeqOptions &seq_option){
    vector<string> sys_hierarchy_file;
    transform(seq_option.sys_hierarchy_line->begin(),  seq_option.sys_hierarchy_line->end(),  back_inserter(sys_hierarchy_file), [](pair<string, map<int,    map<int, string>>> i){return i.first;});
    transform(seq_option.sys_hierarchy_begin->begin(), seq_option.sys_hierarchy_begin->end(), back_inserter(sys_hierarchy_file), [](pair<string, map<string, map<int, string>>> i){return i.first;});
    if(sys_hierarchy_file.empty()){
        string exception_info = "indicator is empty.";
        throw exception_info;
    }
    RemoveDuplicateVectorElement(sys_hierarchy_file);

    unordered_map<string, shared_ptr<ifstream>> file_stream;
    string it_cfile;
    for(string it_cfile : sys_hierarchy_file){
        shared_ptr<ifstream> if_filp = std::make_shared<ifstream>();
        if_filp->open( (*(seq_option.sys_files))[it_cfile].src_path, ios_base::in );
        if(!if_filp->is_open()){
            string exception_info = "open file failed: " + it_cfile + "=>" + (*(seq_option.sys_files))[it_cfile].src_path;
            throw exception_info;
        }
        file_stream[it_cfile] = if_filp;
    }

    string line_content;
    string it_dtype;
    int k = 0;
    int i;
    int j;
    int max_line;
    int max_column;
    string it_cell;
    chrono::system_clock::time_point it_time_point_collect;
    chrono::duration<int> timespan(seq_option.intervals);
    string delta_combine;
    string cumulate_combine;
    unordered_map<string, long long>   previous_combines_long;
    unordered_map<string, long long>   current_combines_long;
    unordered_map<string, long double> previous_combines_float;
    unordered_map<string, long double> current_combines_float;
    unordered_map<string, string>      item_combines;
    bool first_combine = true;
    string combine_delim;
    string last_file_begin;
    int match_file_begin;
    float delta_item;
    stringstream ss_item;
    ss_item << fixed << setprecision(2);

    for(;;){
        it_time_point_collect = chrono::system_clock::now();
        if(it_time_point_collect > seq_option.time_point_finish){
            break;
        }

        for(pair<string, map<int, map<int, string>>> pair_file : *(seq_option.sys_hierarchy_line)){
            it_cfile = pair_file.first;

            if(file_stream.find(it_cfile) == file_stream.cend()){
                string exception_info = it_cfile + "is not exist in file_stream";
                throw exception_info;
            }
            shared_ptr<ifstream> if_filp = file_stream[it_cfile];
            if_filp->clear(); 
            if_filp->seekg(0, ios::beg);

            map<int, map<int, string>>::const_iterator i_element = pair_file.second.cend();            
            --i_element;
            max_line = i_element->first;
            i_element = pair_file.second.cbegin();
            i = 1;
            while(if_filp->good()){
                getline(*if_filp, line_content);
                if(i > max_line || line_content.empty()){
                    break;
                }
                if(i == (*i_element).first){
                    istringstream iline_content(line_content);
                    map<int, string>::const_iterator j_element = (*i_element).second.cend();
                    --j_element;
                    max_column = j_element->first;
                    j_element = (*i_element).second.cbegin();
                    j = 1;
                    while(iline_content >> it_cell){
                        if(j > max_column){
                            break;
                        }
                        if(j == (*j_element).first){
                            ss_item.str("");
                            combine_delim = ":";
                            cumulate_combine = it_cfile + combine_delim + to_string(i) + combine_delim + "f" + combine_delim + to_string(j) + combine_delim + "c";

                            it_dtype = (*seq_option.sys_hierarchy_dtype_line)[it_cfile][i][j];
                            if(it_dtype == "int"){
                                current_combines_long[cumulate_combine] = stoll(it_cell);
                            }else if(it_dtype == "float"){
                                current_combines_float[cumulate_combine] = stold(it_cell);
                            }else{
                                ss_item << it_cell;
                                item_combines[cumulate_combine] = ss_item.str();
                            }
                            if((*j_element).second == "d"){
                                delta_combine = it_cfile + combine_delim + to_string(i) + combine_delim + "f" + combine_delim + to_string(j) + combine_delim + "d";
                                if(it_dtype == "int"){
                                    delta_item = float(current_combines_long[cumulate_combine] - previous_combines_long[cumulate_combine])/float(seq_option.intervals);
                                }else if(it_dtype == "float"){
                                    delta_item = float(current_combines_float[cumulate_combine] - previous_combines_float[cumulate_combine])/float(seq_option.intervals);
                                }
                                delta_item = (delta_item > 0) ? delta_item : 0;
                                ss_item << delta_item;
                                item_combines[delta_combine] = ss_item.str();
                            }else{
                                if(it_dtype == "int"){
                                    ss_item << current_combines_long[cumulate_combine];
                                }else if(it_dtype == "float"){
                                    ss_item << current_combines_float[cumulate_combine];
                                }
                                item_combines[cumulate_combine] = ss_item.str();
                            }
                            ++j_element;
                        }
                        j++;
                    }
                    ++i_element;
                }
                i++;
            }
        }

        for(pair<string, map<string, map<int, string>>> pair_file : *(seq_option.sys_hierarchy_begin)){
            it_cfile = pair_file.first;

            if(file_stream.find(it_cfile) == file_stream.cend()){
                string exception_info = it_cfile + "is not exist in file_stream";
                throw exception_info;
            }
            shared_ptr<ifstream> if_filp = file_stream[it_cfile];
            if_filp->clear(); 
            if_filp->seekg(0, ios::beg);

            set<string> it_file_begins;
            transform(pair_file.second.cbegin(), pair_file.second.cend(), inserter(it_file_begins,it_file_begins.begin()),[](const pair<string, map<int, string>> i){return i.first;});

            while(if_filp->good()){
                getline(*if_filp, line_content);
                if(it_file_begins.empty() || line_content.empty()){
                    break;
                }
                int line_begin_pos;
                for(string it_file_begin : it_file_begins){
                    last_file_begin = it_file_begin;
                    match_file_begin = false;
                    line_begin_pos = line_content.find(it_file_begin + " ", 0);
                    if(line_begin_pos != string::npos){                                         // must plus a space
                        match_file_begin = true;
                        map<int, string>::const_iterator j_element = pair_file.second[it_file_begin].cend();
                        --j_element;
                        max_column = j_element->first;
                        j_element = pair_file.second[it_file_begin].cbegin();

                        j = 1;
                        istringstream iline_content(line_content);
                        while(iline_content >> it_cell){
                            if(j > max_column){
                                break;
                            }
                            if(j == (*j_element).first){
                                ss_item.str("");
                                if(it_file_begin.find(':') != string::npos){
                                    combine_delim = "|";
                                }else{
                                    combine_delim = ":";
                                }
                                cumulate_combine = it_cfile + combine_delim + it_file_begin + combine_delim + "a" + combine_delim + to_string(j) + combine_delim + "c";

                                it_dtype = (*seq_option.sys_hierarchy_dtype_begin)[it_cfile][it_file_begin][j];
                                if(it_dtype == "int"){
                                    current_combines_long[cumulate_combine] = stoll(it_cell);
                                }else if(it_dtype == "float"){
                                    current_combines_float[cumulate_combine] = stold(it_cell);
                                }else{
                                    ss_item << it_cell;
                                    item_combines[cumulate_combine] = ss_item.str();
                                }

                                if((*j_element).second == "d"){
                                    delta_combine = it_cfile + combine_delim + it_file_begin + combine_delim + "a" + combine_delim + to_string(j) + combine_delim + "d";
                                    if(it_dtype == "int"){
                                        delta_item = float(current_combines_long[cumulate_combine] - previous_combines_long[cumulate_combine])/float(seq_option.intervals);
                                    }else if(it_dtype == "float"){
                                        delta_item = float(current_combines_float[cumulate_combine] - previous_combines_float[cumulate_combine])/float(seq_option.intervals);
                                    }
                                    delta_item = (delta_item > 0) ? delta_item : 0;
                                    ss_item << delta_item;
                                    item_combines[delta_combine] = ss_item.str();
                                }else{
                                    if(it_dtype == "int"){
                                        ss_item << current_combines_long[cumulate_combine];
                                    }else if(it_dtype == "float"){
                                        ss_item << current_combines_float[cumulate_combine];
                                    }
                                    item_combines[cumulate_combine] = ss_item.str();
                                }
                                ++j_element;
                            }
                            j++;
                        }
                        break;
                    }
                }
                if(match_file_begin){
                    it_file_begins.erase(last_file_begin);
                }
            }
            if(!it_file_begins.empty()){
                string exception_info = "line_begin ";
                for(string it_file_begin : it_file_begins){
                    exception_info += it_file_begin + " ";
                }
                exception_info += "is not find in file " + it_cfile + ".";
                throw exception_info;
            }
        }

        if(seq_option.api){
            DisplayLiveSysJson(seq_option, k, it_time_point_collect, first_combine, item_combines);
        }else{
            DisplayLiveSysShell(seq_option, k, it_time_point_collect, first_combine, item_combines);
        }

        previous_combines_long  = current_combines_long;
        previous_combines_float = current_combines_float;
        first_combine = false;
        this_thread::sleep_for(timespan);
        k++;
    }
    for(pair<string, shared_ptr<ifstream>> if_pair_filp : file_stream){
        if_pair_filp.second->close();
    }
}

int ReadSysFileLineData(SeqOptions &seq_option, string &it_path, string &it_file, const map<int, map<int, string>> &file_hierarchy, map<int, map<int, string>> &file_hierarchy_dtype, unordered_map<string, long long> &it_combine, unordered_map<string, long double> &it_combine_float, unordered_map<string, string> &it_combine_string){
    igzfstream if_file;
    bool finded = false;
    
    if_file.open(it_path, ios_base::in);
    if(!if_file.good()){
        return 1;
    }

    string combine_delim = ":";
    string line_content;
    string it_cell;
    string cumulate_combine;
    int i;
    int j;
    int max_line;
    int max_column;

    map<int, map<int, string>>::const_iterator i_element = file_hierarchy.cend();
    --i_element;
    max_line = i_element->first;
    i_element = file_hierarchy.cbegin();
    i = 1;
    while(if_file.good()){
        getline(if_file, line_content);
        if(i > max_line || line_content.empty()){
            break;
        }
        if(i == (*i_element).first){
            istringstream iline_content(line_content);
            map<int, string>::const_iterator j_element = (*i_element).second.cend();
            --j_element;
            max_column = j_element->first;
            j_element = (*i_element).second.cbegin();
            j = 1;
            while(iline_content >> it_cell){
                if(j > max_column){
                    break;
                }
                if(j == (*j_element).first){
                    cumulate_combine = it_file + combine_delim + to_string(i) + combine_delim + "f" + combine_delim + to_string(j) + combine_delim + "c";
                    if((file_hierarchy_dtype)[i][j] == "int"){
                        it_combine[cumulate_combine] = stoll(it_cell);
                    }else if((file_hierarchy_dtype)[i][j] == "float"){
                        it_combine_float[cumulate_combine] = stold(it_cell);
                    }else{
                        it_combine_string[cumulate_combine] = it_cell;
                    }
                    ++j_element;
                }
                j++;
            }
            ++i_element;
        }
        i++;
    }
    if_file.close();

    return 0;
}

int ReadSysFileBeginData(SeqOptions &seq_option, string &it_path, string &it_file, const map<string, map<int, string>> &file_hierarchy, map<string, map<int, string>> &file_hierarchy_dtype, unordered_map<string, long long> &it_combine, unordered_map<string, long double> &it_combine_float, unordered_map<string, string> &it_combine_string){
    igzfstream if_file;
    bool finded = false;
    map<string, map<int, string>> it_file_hierarchy = file_hierarchy;
    
    if_file.open(it_path, ios_base::in);
    if(!if_file.good()){
        return 1;
    }
    
    string combine_delim;
    string line_content;
    string it_cell;
    string cumulate_combine;
    int j;
    int max_column;
    
    set<string> it_file_begins;
    transform(file_hierarchy.cbegin(), file_hierarchy.cend(), inserter(it_file_begins,it_file_begins.begin()),[](const pair<string, map<int, string>> i){return i.first;});
    
    while(if_file.good()){
        getline(if_file, line_content);
        if(line_content.empty()){
            break;
        }
        
        int line_begin_pos;
        for(string it_file_begin : it_file_begins){
            line_begin_pos = line_content.find(it_file_begin + " ", 0);
            if(line_begin_pos != string::npos){                                         // must plus a space
                map<int, string>::const_iterator j_element = it_file_hierarchy[it_file_begin].cend();
                --j_element;
                max_column = j_element->first;
                j_element = it_file_hierarchy[it_file_begin].cbegin();

                j = 1;
                istringstream iline_content(line_content);
                while(iline_content >> it_cell){
                    if(j > max_column){
                        break;
                    }
                    if(j == (*j_element).first){
                        if(it_file_begin.find(':') != string::npos){
                            combine_delim = "|";
                        }else{
                            combine_delim = ":";
                        }
                        cumulate_combine = it_file + combine_delim + it_file_begin + combine_delim + "a" + combine_delim + to_string(j) + combine_delim +"c";
                        if((file_hierarchy_dtype)[it_file_begin][j] == "int"){
                            it_combine[cumulate_combine] = stoll(it_cell);
                        }else if((file_hierarchy_dtype)[it_file_begin][j] == "float"){
                            it_combine_float[cumulate_combine] = stold(it_cell);
                        }else{
                            it_combine_string[cumulate_combine] = it_cell;
                        }
                        ++j_element;
                    }
                    j++;
                }
            }
        }
    }


    if_file.close();

    return 0;
}

void DisplaySysShell(SeqOptions &seq_option, list<unordered_map<string, long long>> &it_ctrls, unordered_map<long,unordered_map<string, string>> &fill_combines){
    indicator_t it_indicator_t;
    int i = 0;

    bool pollute = false;
    chrono::system_clock::time_point it_time_point_collect;
    unordered_map<string, long long> current_ctrls;
    unordered_map<string, string> current_combines;
    string combine_key;
    string line_key;
    string combine_delim;
    list<unordered_map<string, long long>>::const_iterator i_element = it_ctrls.cbegin();
    for(;i_element != it_ctrls.cend(); ++i_element){
        current_ctrls = (*i_element);
        current_combines = fill_combines[current_ctrls["collect_time"]];

        if(i % AREA_LINE == 0 && !seq_option.noheaders && !pollute){
            stringstream it_shell;
            it_shell << resetiosflags(ios::adjustfield);
            it_shell << setiosflags(ios::left);
            //it_shell << setfill('*');
            it_shell << setw(19) << "collect_datetime";
            it_shell << " ";

            auto h_element = seq_option.formated.cbegin();
            for(; h_element != seq_option.formated.cend(); ++h_element){
                it_indicator_t = (*h_element);
                it_shell << resetiosflags(ios::adjustfield);
                it_shell << setiosflags(ios::right);
                //it_shell << setfill('*');
                if(it_indicator_t.metric == "d"){
                    it_shell << setw(it_indicator_t.width - 2); 
                }else{
                    it_shell << setw(it_indicator_t.width); 
                }
                it_shell << it_indicator_t.alias;
                if(it_indicator_t.metric == "d"){
                    it_shell << "/s";
                }
                it_shell << " ";
            }
            string it_shell_str = it_shell.str();
            cout << endl;
            cout << it_shell_str << endl;
        }

        pollute = false;
        stringstream it_shell;
        it_shell << resetiosflags(ios::adjustfield);
        it_shell << setiosflags(ios::left);
        //it_shell << setfill('*');
        it_time_point_collect = chrono::system_clock::from_time_t(current_ctrls["collect_time"]);
        it_shell << setw(19) << FmtDatetime(it_time_point_collect);
        it_shell << " ";
        auto m_element = seq_option.formated.cbegin();
        for(; m_element != seq_option.formated.cend(); ++m_element){
            it_indicator_t = (*m_element);
            it_shell << resetiosflags(ios::adjustfield);
            it_shell << setiosflags(ios::right);
            //it_shell << setfill('*');
            it_shell << setw(it_indicator_t.width);
            if(current_ctrls["record_type"] == 1005){
                pollute = true;
                it_shell << "*";
            }else if(current_ctrls["record_type"] == 1003){
                pollute = true;
                it_shell << "-";
            }else if(current_ctrls["record_type"] == 1004){
                if(it_indicator_t.metric == "c"){
                    if(it_indicator_t.position == "f"){
                        line_key = to_string(it_indicator_t.line);
                        combine_delim = ":";
                    }else{
                        line_key = it_indicator_t.line_begin;
                        if(it_indicator_t.line_begin.find(':') != string::npos){
                            combine_delim = "|";
                        }else{
                            combine_delim = ":";
                        }
                    }
                    combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                    it_shell << current_combines[combine_key];
                }else{
                    pollute = true;
                    it_shell << "-";
                }
            }else if(current_ctrls["record_type"] == 1001){
                if(current_ctrls["record_type" + it_indicator_t.cfile] == 1001){
                    if(it_indicator_t.position == "f"){
                        line_key = to_string(it_indicator_t.line);
                        combine_delim = ":";
                    }else{
                        line_key = it_indicator_t.line_begin;
                        if(it_indicator_t.line_begin.find(':') != string::npos){
                            combine_delim = "|";
                        }else{
                            combine_delim = ":";
                        }
                    }
                    combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                    if(it_indicator_t.metric == "c"){
                        it_shell << current_combines[combine_key];
                    }else{
                        it_shell << current_combines[combine_key];
                    }
                }else if(current_ctrls["record_type" + it_indicator_t.cfile] == 1004){
                    if(it_indicator_t.metric == "c"){
                        if(it_indicator_t.position == "f"){
                            line_key = to_string(it_indicator_t.line);
                            combine_delim = ":";
                        }else{
                            line_key = it_indicator_t.line_begin;
                            if(it_indicator_t.line_begin.find(':') != string::npos){
                                combine_delim = "|";
                            }else{
                                combine_delim = ":";
                            }
                        }
                        combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                        it_shell << current_combines[combine_key];
                    }else{
                        pollute = true;
                        it_shell << "-";
                    }
                }else{
                    pollute = true;
                    it_shell << "-";
                }
            }
            it_shell << " ";
        }
        if(seq_option.purify && pollute){
            continue;
        }
        string it_shell_str = it_shell.str();
        cout << it_shell_str << endl;
        i++;
    }
}

void DisplaySysJson(SeqOptions &seq_option, list<unordered_map<string, long long>> &it_ctrls, unordered_map<long,unordered_map<string, string>> &fill_combines){
    indicator_t it_indicator_t;
    json it_json = json::array();

    int i = 0;
    bool pollute = false;
    chrono::system_clock::time_point it_time_point_collect;
    unordered_map<string, long long> current_ctrls;
    unordered_map<string, string> current_combines;
    string combine_key;
    string line_key;
    string combine_delim;
    list<unordered_map<string, long long>>::const_iterator i_element = it_ctrls.cbegin();
    for(;i_element != it_ctrls.cend(); ++i_element){
        current_ctrls = (*i_element);
        current_combines = fill_combines[current_ctrls["collect_time"]];

        pollute = false;
        json obj_json = json::object();
        stringstream ss_json_field;
        it_time_point_collect = chrono::system_clock::from_time_t(current_ctrls["collect_time"]);
        obj_json["collect_datetime"] = FmtDatetime(it_time_point_collect);

        auto m_element = seq_option.formated.cbegin();
        for(; m_element != seq_option.formated.cend(); ++m_element){
            it_indicator_t = (*m_element);
            stringstream ss_json_field;
            if(current_ctrls["record_type"] == 1005){
                pollute = true;
                ss_json_field << "*";
            }else if(current_ctrls["record_type"] == 1003){
                pollute = true;
                ss_json_field << "-";
            }else if(current_ctrls["record_type"] == 1004){
                if(it_indicator_t.metric == "c"){
                    if(it_indicator_t.position == "f"){
                        line_key = to_string(it_indicator_t.line);
                        combine_delim = ":";
                    }else{
                        line_key = it_indicator_t.line_begin;
                        if(it_indicator_t.line_begin.find(':') != string::npos){
                            combine_delim = "|";
                        }else{
                            combine_delim = ":";
                        }
                    }
                    combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                    ss_json_field << current_combines[combine_key];
                }else{
                    pollute = true;
                    ss_json_field << "-";
                }
            }else if(current_ctrls["record_type"] == 1001){
                if(current_ctrls["record_type" + it_indicator_t.cfile] == 1001){
                    if(it_indicator_t.position == "f"){
                        line_key = to_string(it_indicator_t.line);
                        combine_delim = ":";
                    }else{
                        line_key = it_indicator_t.line_begin;
                        if(it_indicator_t.line_begin.find(':') != string::npos){
                            combine_delim = "|";
                        }else{
                            combine_delim = ":";
                        }
                    }
                    combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                    if(it_indicator_t.metric == "c"){
                        ss_json_field << current_combines[combine_key];
                    }else{
                        ss_json_field << current_combines[combine_key];
                    }
                }else if(current_ctrls["record_type" + it_indicator_t.cfile] == 1004){
                    if(it_indicator_t.metric == "c"){
                        if(it_indicator_t.position == "f"){
                            line_key = to_string(it_indicator_t.line);
                            combine_delim = ":";
                        }else{
                            line_key = it_indicator_t.line_begin;
                            if(it_indicator_t.line_begin.find(':') != string::npos){
                                combine_delim = "|";
                            }else{
                                combine_delim = ":";
                            }
                        }
                        combine_key = it_indicator_t.cfile + combine_delim + line_key + combine_delim + it_indicator_t.position + combine_delim + to_string(it_indicator_t.column) + combine_delim + it_indicator_t.metric;
                        ss_json_field << current_combines[combine_key];
                    }else{
                        pollute = true;
                        ss_json_field << "-";
                    }
                }else{
                    pollute = true;
                    ss_json_field << "-";
                }
            }
            obj_json[it_indicator_t.alias] = ss_json_field.str();
        }
        if(seq_option.purify && pollute){
            continue;
        }
        it_json[i] = obj_json;
        i++;
    }

    cout << it_json.dump() << endl;
}

void ReportSys(SeqOptions &seq_option){
    // record_type:  
    //      1001  this record is exist.  
    //      1003  this record is not exist.
    //      1004  this record is exist, but previous record is not exist.
    //      1005  this record is not exist, because system is reboot.
    //      1011  this record is exist, but pid not exist in record.
    //      1012  this is before_first record.
    //      1013  this is after_last record.

    vector<string> sys_hierarchy_file;
    transform(seq_option.sys_hierarchy_line->begin(),  seq_option.sys_hierarchy_line->end(),  back_inserter(sys_hierarchy_file), [](pair<string, map<int,    map<int, string>>> i){return i.first;});
    transform(seq_option.sys_hierarchy_begin->begin(), seq_option.sys_hierarchy_begin->end(), back_inserter(sys_hierarchy_file), [](pair<string, map<string, map<int, string>>> i){return i.first;});
    if(sys_hierarchy_file.empty()){
        string exception_info = "indicator is empty.";
        throw exception_info;
    }
    RemoveDuplicateVectorElement(sys_hierarchy_file);

    string it_cfile = "";
    string it_full_suffix = "";
    int read_file_line_ret;
    int read_file_begin_ret;
    int data_hour_path_ret;
    bool break_time = true;
    chrono::system_clock::time_point time_point_real;
    chrono::system_clock::time_point unified_time_point_real;

    string combine_delim;    
    string cumulate_combine;
    long it_collect_time;
    unordered_map<string, long long>                        item_ctrl;
    unordered_map<string, long long>                        item_combine_long;
    unordered_map<string, long long>                        item_combine_long_line;
    unordered_map<string, long long>                        item_combine_long_begin;
    unordered_map<string, long double>                      item_combine_float;
    unordered_map<string, long double>                      item_combine_float_line;
    unordered_map<string, long double>                      item_combine_float_begin;
    unordered_map<string, string>                           item_combine_string;
    unordered_map<string, string>                           item_combine_string_line;
    unordered_map<string, string>                           item_combine_string_begin;
    list<unordered_map<string, long long>>                  list_ctrl;
    unordered_map<long, unordered_map<string, long long>>   list_combine_long;
    unordered_map<long, unordered_map<string, long double>> list_combine_float;
    unordered_map<string, string>                           fill_items;
    unordered_map<long, unordered_map<string, string>>      fill_combines;
    float delta_item;
    string it_dtype;
    stringstream ss_item;
    ss_item << fixed << setprecision(2);

    SreProc it_sre_proc;
    it_sre_proc.SetDataPath(seq_option.data_path);
    it_sre_proc.SetSuffix(*(sys_hierarchy_file.cbegin()));

    it_sre_proc.EnableAdjust();
    if(0 == it_sre_proc.MakeDataHourPath(seq_option.time_point_finish)){
        seq_option.time_point_finish_real = it_sre_proc.GetDataTimePointReal();
        seq_option.adjust                 = it_sre_proc.GetAdjusted();
    }else{
        seq_option.time_point_finish_real = seq_option.time_point_finish;
        seq_option.adjust                 = true;
    }
    it_sre_proc.DisableAdjust();

    chrono::system_clock::time_point it_time_point_begin = seq_option.time_point_begin;
    seq_option.time_point_begin_real = it_time_point_begin;
    if(seq_option.adjust){
        seq_option.time_point_begin_real = it_time_point_begin - chrono::duration<int>(60);
    }

    chrono::system_clock::time_point it_time_point = seq_option.time_point_finish_real;
    for(;;){
        if(it_time_point < seq_option.time_point_begin_real){
            break;
        }
        
        fill_items.clear();    
        item_combine_long.clear();
        item_combine_float.clear();
        item_combine_string.clear();
        item_ctrl.clear();
        break_time                = true;
        item_ctrl["record_type"]  = 1003;
        item_ctrl["collect_time"] = chrono::system_clock::to_time_t(it_time_point);
        time_point_real           = chrono::time_point<chrono::system_clock>{};
        unified_time_point_real   = chrono::time_point<chrono::system_clock>{};

        for(string it_cfile : sys_hierarchy_file){
            if((*(seq_option.sys_files))[it_cfile].gzip){
                it_full_suffix = it_cfile + ".gz";
            }else{
                it_full_suffix = it_cfile;
            }
            it_sre_proc.SetSuffix(it_full_suffix);
            data_hour_path_ret = it_sre_proc.MakeDataHourPath(it_time_point);
            if(0 != data_hour_path_ret){
                item_ctrl["record_type" + it_cfile] = 1003;
                continue;
            }
            string it_path  = it_sre_proc.GetDataHourPath();
            time_point_real = it_sre_proc.GetDataTimePointReal();
            if(unified_time_point_real != chrono::time_point<chrono::system_clock>{} && time_point_real != unified_time_point_real){
                string exception_info = "file real time is not unified.";
                throw exception_info;
            }
            unified_time_point_real = time_point_real;
           
            if((*seq_option.sys_hierarchy_line).find(it_cfile) != (*seq_option.sys_hierarchy_line).cend()){
                read_file_line_ret = -1;
                item_combine_long_line.clear();
                read_file_line_ret = ReadSysFileLineData(seq_option, it_path, it_cfile, (*seq_option.sys_hierarchy_line)[it_cfile], (*seq_option.sys_hierarchy_dtype_line)[it_cfile], item_combine_long_line, item_combine_float_line, item_combine_string_line);
                if(0 != read_file_line_ret){
                    item_ctrl["record_type" + it_cfile] = 1003;
                    continue;
                }
                item_combine_long.insert(item_combine_long_line.begin(), item_combine_long_line.end());
                item_combine_float.insert(item_combine_float_line.begin(), item_combine_float_line.end());
                item_combine_string.insert(item_combine_string_line.begin(), item_combine_string_line.end());
            }
            if((*seq_option.sys_hierarchy_begin).find(it_cfile) != (*seq_option.sys_hierarchy_begin).cend()){
                read_file_begin_ret = -1;
                item_combine_long_begin.clear();
                read_file_begin_ret = ReadSysFileBeginData(seq_option, it_path, it_cfile, (*seq_option.sys_hierarchy_begin)[it_cfile], (*seq_option.sys_hierarchy_dtype_begin)[it_cfile], item_combine_long_begin, item_combine_float_begin, item_combine_string_begin);
                if(0 != read_file_begin_ret){
                    item_ctrl["record_type" + it_cfile] = 1003;
                    continue;
                }
                item_combine_long.insert(item_combine_long_begin.begin(), item_combine_long_begin.end());
                item_combine_float.insert(item_combine_float_begin.begin(), item_combine_float_begin.end());
                item_combine_string.insert(item_combine_string_begin.begin(), item_combine_string_begin.end());
            }

            break_time = false;
            item_ctrl["record_type" + it_cfile] = 1001;
            item_ctrl["record_type"]  = 1001;
            it_collect_time = it_sre_proc.GetDataTimestamp();
            item_ctrl["collect_time"] = it_collect_time;
        }

        list_ctrl.push_front(item_ctrl);
        it_time_point = it_time_point - chrono::duration<int>(seq_option.intervals);

        if(break_time){
            continue;
        }

        auto k_element = seq_option.cumulate_hierarchy->cbegin();
        for(; k_element != seq_option.cumulate_hierarchy->cend(); ++k_element){
            it_cfile = k_element->first;
            auto m_element = k_element->second.cbegin();
            for(; m_element != k_element->second.cend(); ++m_element){
                if((*m_element).find('|') == string::npos){
                    combine_delim = ":";
                }else{
                    combine_delim = "|";
                }
                cumulate_combine = it_cfile + combine_delim + (*m_element) + combine_delim + "c";

                ss_item.str("");

                it_dtype = (*seq_option.sys_dtype)[cumulate_combine];
                if(it_dtype == "int"){
                    ss_item << item_combine_long[cumulate_combine];
                }else if(it_dtype == "float"){
                    ss_item << item_combine_float[cumulate_combine];
                }else{
                    ss_item << item_combine_string[cumulate_combine];
                }

                fill_items[cumulate_combine] = ss_item.str();
            }
        }

        list_combine_long[it_collect_time]  = item_combine_long;
        list_combine_float[it_collect_time] = item_combine_float;
        fill_combines[it_collect_time]      = fill_items;

    }

    string delta_combine = "";
    list<unordered_map<string, long long>> fill_ctrl;
    unordered_map<string, long long>       previous_ctrl;
    unordered_map<string, long long>       current_ctrl;
    unordered_map<string, long long>       previous_combine_long;
    unordered_map<string, long long>       current_combine_long;
    unordered_map<string, long double>     previous_combine_float;
    unordered_map<string, long double>     current_combine_float;

    previous_ctrl["record_type"] = 1002;
    previous_ctrl["collect_time"] = chrono::system_clock::to_time_t(it_time_point);
    vector<chrono::system_clock::time_point>::const_iterator j_element = seq_option.reboot_times->cbegin();
    vector<chrono::system_clock::time_point>::const_iterator j_element_end = seq_option.reboot_times->cend();
    j_element_end--;
    list<unordered_map<string, long long>>::const_iterator i_element = list_ctrl.cbegin();
    for(;i_element != list_ctrl.cend(); ++i_element){
        current_ctrl = (*i_element);
        it_collect_time = current_ctrl["collect_time"];
        current_combine_long = list_combine_long[it_collect_time];
        current_combine_float = list_combine_float[it_collect_time];
        if(j_element != seq_option.reboot_times->cend()){
            while((*j_element) <= chrono::system_clock::from_time_t(previous_ctrl["collect_time"])){
                if(j_element == j_element_end){
                    break;
                }
                j_element++;
            }
            if((*j_element) > chrono::system_clock::from_time_t(previous_ctrl["collect_time"]) && (*j_element) <= chrono::system_clock::from_time_t(current_ctrl["collect_time"])){
                current_ctrl["record_type"] = 1005;
            }
        }
        if(1001 != current_ctrl["record_type"]){
            fill_ctrl.push_back(current_ctrl);
            previous_ctrl = current_ctrl;
            previous_combine_long = current_combine_long;
            previous_combine_float = current_combine_float;
            continue;
        }
        if(1003 == previous_ctrl["record_type"] || 1002 == previous_ctrl["record_type"]){
            current_ctrl["record_type"] = 1004;
            fill_ctrl.push_back(current_ctrl);

            previous_ctrl = current_ctrl;
            previous_combine_long = current_combine_long;
            previous_combine_float = current_combine_float;
            continue;
        }
        current_ctrl["realtime"] = current_ctrl["collect_time"] - previous_ctrl["collect_time"];
       
        fill_items = fill_combines[it_collect_time];
        auto k_element = seq_option.delta_hierarchy->cbegin();
        for(; k_element != seq_option.delta_hierarchy->cend(); ++k_element){
            it_cfile = k_element->first;
            if(current_ctrl["record_type" + it_cfile] == 1001){
                if(previous_ctrl["record_type" + it_cfile] == 1003){
                    current_ctrl["record_type" + it_cfile] = 1004;
                }else{
                    auto m_element = k_element->second.cbegin();
                    for(; m_element != k_element->second.cend(); ++m_element){
                        if((*m_element).find('|') == string::npos){
                            combine_delim = ":";
                        }else{
                            combine_delim = "|";
                        }

                        cumulate_combine = it_cfile + combine_delim + (*m_element) + combine_delim + "c";
                        delta_combine    = it_cfile + combine_delim + (*m_element) + combine_delim + "d";

                        ss_item.str("");
                        it_dtype = (*seq_option.sys_dtype)[cumulate_combine];
                        if(it_dtype == "int"){
                            delta_item = float(current_combine_long[cumulate_combine]-previous_combine_long[cumulate_combine])/float(current_ctrl["realtime"]);
                        }else if(it_dtype == "float"){
                            delta_item = float(current_combine_float[cumulate_combine]-previous_combine_float[cumulate_combine])/float(current_ctrl["realtime"]);
                        }else{
                        }

                        delta_item = (delta_item > 0) ? delta_item : 0;
                        ss_item << delta_item;
                        fill_items[delta_combine] = ss_item.str();
                    }
                }
            }
        }

        fill_ctrl.push_back(current_ctrl);
        fill_combines[it_collect_time] = fill_items;
        previous_ctrl = current_ctrl;
        previous_combine_long = current_combine_long;
        previous_combine_float = current_combine_float;
    }
    
    if(seq_option.api){
        DisplaySysJson(seq_option, fill_ctrl, fill_combines);
    }else{
        DisplaySysShell(seq_option, fill_ctrl, fill_combines);    
    }
}

/**********************************************************************************
 *                                                                                *
 *                               main part                                        * 
 *                                                                                *
 **********************************************************************************/

void ActivityReporter(SeqOptions &seq_option){
    if(seq_option.version){
        cout << "ssar " << VERSION << endl;  
        exit(0);
    }

    try{
        InitOptions(seq_option);
        InitEnv(seq_option);
    }catch(const string exp){
        cout << "Exception: " << exp << endl;  
        exit(200);
    }catch(exception &e){ 
        cout << "Exception Caught: " << e.what() << endl; 
        exit(200);
    }

    if("procs" == seq_option.src){
        try{
            ReportProcs(seq_option);
        }catch(const string exp){
            cout << "Exception: " << exp << endl;  
            exit(201);
        }catch(exception &e) { 
            cout << "Exception Caught: " << e.what() << endl; 
            exit(201);
        } 
    }else if("proc" == seq_option.src){
        try{
            ReportProc(seq_option);
        }catch(const string exp){
            cout << "Exception: " << exp << endl;  
            exit(202);
        }catch(exception &e) { 
            cout << "Exception Caught: " << e.what() << endl; 
            exit(202);
        } 
    }else if("load5s" == seq_option.src){
        try{
            ReportLoad5s(seq_option);
        }catch(const string exp){
            cout << "Exception: " << exp << endl;  
            exit(203);
        }catch(exception &e) { 
            cout << "Exception Caught: " << e.what() << endl; 
            exit(203);
        } 
    }else if("load2p" == seq_option.src){
        try{
            ReportLoad2p(seq_option);
        }catch(const string exp){
            cout << "Exception: " << exp << endl;  
            exit(204);
        }catch(DataException &e){
            if(seq_option.api){
                cout << "[]"<< endl;  
            }else{
                cout << e.what() << endl;  
            }
            exit(199);
        }catch(exception &e){
            cout << "Exception Caught: " << e.what() << endl; 
            exit(204);
        }
    }else if("sys" == seq_option.src){
        try{
            InitFormat(seq_option);
            if(seq_option.live_mode){
                ReportLiveSys(seq_option);
            }else{
                ReportSys(seq_option);
            }
        }catch(const string exp){
            cout << "Exception: " << exp << endl;  
            exit(205);
        }catch(exception &e) { 
            cout << "Exception Caught: " << e.what() << endl; 
            exit(205);
        } 
    }else{
        cout << "src is not recognized." << endl;
        exit(206);
    }
}

int main(int argc, char *argv[]){
    map<string, list<string>>                                        sys_view;
    unordered_map<string, cfile_t>                                   sys_files;
    map<string, unordered_map<string, cfile_t>, greater<string>>     group_files;
    map<string, unordered_map<string, indicator_t>, greater<string>> sys_indicator;
    map<string, map<int, map<int, string>>>                          sys_hierarchy_line;
    map<string, map<int, map<int, string>>>                          sys_hierarchy_dtype_line;
    map<string, map<string, map<int, string>>>                       sys_hierarchy_begin;
    map<string, map<string, map<int, string>>>                       sys_hierarchy_dtype_begin;
    map<string, vector<string>>                                      delta_hierarchy;
    map<string, vector<string>>                                      cumulate_hierarchy;
    unordered_map<string, string>                                    sys_dtype;
    vector<chrono::system_clock::time_point>                         reboot_times;
    
    SeqOptions opts = SeqOptions();
    opts.sys_view                  = &sys_view;
    opts.sys_files                 = &sys_files;
    opts.group_files               = &group_files;
    opts.sys_indicator             = &sys_indicator;
    opts.sys_hierarchy_line        = &sys_hierarchy_line;
    opts.sys_hierarchy_begin       = &sys_hierarchy_begin;
    opts.sys_hierarchy_dtype_line  = &sys_hierarchy_dtype_line;
    opts.sys_hierarchy_dtype_begin = &sys_hierarchy_dtype_begin;
    opts.delta_hierarchy           = &delta_hierarchy;
    opts.cumulate_hierarchy        = &cumulate_hierarchy;
    opts.sys_dtype                 = &sys_dtype;
    opts.reboot_times              = &reboot_times;
    
    string ssar_conf_str = "/etc/ssar/ssar.conf";
    shared_ptr<table> ssar_config;
    try{
        ssar_config = parse_file(ssar_conf_str);
    }catch(const parse_exception& e){
        opts.ssar_conf = false;
    }
    if(ssar_config && ssar_config->contains("main")){
        shared_ptr<table> main_ini = ssar_config->get_table("main");
        if(main_ini->contains_qualified("work_path")){
            opts.work_path = main_ini->get_qualified("work_path")->as<string>()->get();
        }
    }
    if(ssar_config && ssar_config->contains("proc")){
        shared_ptr<table> proc_ini = ssar_config->get_table("proc");
        if(proc_ini->contains_qualified("proc_gzip_disable")){
            auto it_proc_gzip_disable = proc_ini->get_as<bool>("proc_gzip_disable");
            if(it_proc_gzip_disable && (*it_proc_gzip_disable)){
                opts.proc_gzip_disable = true;
            }
        }
    }
    
    string sys_conf_str = "/etc/ssar/sys.conf";
    shared_ptr<table> sys_config;
    try{
        sys_config = parse_file(sys_conf_str);
    }catch(const parse_exception& e){
        opts.sys_conf = false;
    }
   
    cfile_t it_cfile_t; 
    string sys_file_key = "";
    string sys_file_value = "";
    int slash_pos = 0;
    int last_slash_pos = 0;
    if(sys_config && sys_config->contains("file")){
        shared_ptr<table> file_ini = sys_config->get_table("file");

        for(pair<string, shared_ptr<base>> pair_file_group : *(file_ini.get())){
            string file_group_key = pair_file_group.first;
            shared_ptr<table_array> file_group_value = pair_file_group.second->as_table_array();

            sys_file_key = "";
            sys_file_value = "";
            slash_pos = 0;
            last_slash_pos = 0;
 
            for(const auto it_file_t : *(file_group_value.get())){
                auto it_src_path = it_file_t->get_as<string>("src_path");
                slash_pos = (*it_src_path).find("/");
                if(slash_pos == string::npos){
                    sys_file_value = "/proc/"+(*it_src_path);
                    it_cfile_t.src_path= "/proc/"+(*it_src_path);
                }else{
                    sys_file_value = *it_src_path;
                    it_cfile_t.src_path= *it_src_path;
                }
                auto it_cfile = it_file_t->get_as<string>("cfile");
                if(it_cfile){
                    sys_file_key = *it_cfile;
                    it_cfile_t.cfile = *it_cfile;
                }else{
                    last_slash_pos = sys_file_value.rfind("/",sys_file_value.size());
                    sys_file_key = sys_file_value.substr(last_slash_pos + 1);
                    it_cfile_t.cfile = sys_file_value.substr(last_slash_pos + 1);
                }
                auto it_gzip = it_file_t->get_as<bool>("gzip");
                it_cfile_t.gzip = 0;
                if(it_gzip && (*it_gzip)){
                    it_cfile_t.gzip = 1;
                }

                (*(opts.group_files))[file_group_key][sys_file_key] = it_cfile_t;
            }
        }
    }
    
    if(sys_config && sys_config->contains("view")){
        shared_ptr<table> view_ini = sys_config->get_table("view");
        for(pair<string, shared_ptr<base>> pair_view : *(view_ini.get())){
            string view_key = pair_view.first;
            sys_view[view_key] = {};
            shared_ptr<cpptoml::array> view_value = pair_view.second->as_array();
            for(vector<shared_ptr<base>>::iterator view_iterator = view_value->begin(); view_iterator != view_value->end(); ++view_iterator){
                sys_view[view_key].push_back((*view_iterator)->as<string>()->get());
            }
        }
    }
    if(sys_config && sys_config->contains("indicator")){
        shared_ptr<table> indicator_ini = sys_config->get_table("indicator");

        for(pair<string, shared_ptr<base>> pair_indicator_group : *(indicator_ini.get())){
            string indicator_group_key = pair_indicator_group.first;
            shared_ptr<table> indicator_group_value = pair_indicator_group.second->as_table();

            for(pair<string, shared_ptr<base>> pair_indicator : *(indicator_group_value.get())){
                string indicator_key = pair_indicator.first;
                sys_indicator[indicator_group_key][indicator_key] = indicator_t();
                sys_indicator[indicator_group_key][indicator_key].cfile       = "";
                sys_indicator[indicator_group_key][indicator_key].line_begin = "";
                sys_indicator[indicator_group_key][indicator_key].line       = 0;
                sys_indicator[indicator_group_key][indicator_key].column     = 0;
                sys_indicator[indicator_group_key][indicator_key].width      = 10;
                sys_indicator[indicator_group_key][indicator_key].metric     = "c";
                sys_indicator[indicator_group_key][indicator_key].position   = "f";
                sys_indicator[indicator_group_key][indicator_key].dtype      = "int";
                sys_indicator[indicator_group_key][indicator_key].alias      = indicator_key;
                shared_ptr<cpptoml::table> indicator_value = pair_indicator.second->as_table();
                for(pair<string, shared_ptr<base>> pair_indicator_t : *(indicator_value.get())){
                    string indicator_t_key = pair_indicator_t.first;
                    shared_ptr<base> indicator_t_value = pair_indicator_t.second;
    
                    if(shared_ptr<value<string>> str_value = indicator_t_value->as<string>()){
                        if(indicator_t_key == "cfile"){
                            sys_indicator[indicator_group_key][indicator_key].cfile = (*str_value).get();
                        }else if(indicator_t_key == "line_begin"){
                            sys_indicator[indicator_group_key][indicator_key].line_begin = (*str_value).get();
                        }else if(indicator_t_key == "metric"){
                            sys_indicator[indicator_group_key][indicator_key].metric = (*str_value).get();
                        }else if(indicator_t_key == "position"){
                            sys_indicator[indicator_group_key][indicator_key].position = (*str_value).get();
                        }else if(indicator_t_key == "dtype"){
                            sys_indicator[indicator_group_key][indicator_key].dtype = (*str_value).get();
                        }else{
                            cout << "indicator_t key " << indicator_key << " is not exist." << endl;
                            exit(200);
                        }
                    }else if(shared_ptr<value<int64_t>> int_value = indicator_t_value->as<int64_t>()){
                        if(indicator_t_key == "line"){
                            sys_indicator[indicator_group_key][indicator_key].line = (*int_value).get();
                        }else if(indicator_t_key == "column"){
                            sys_indicator[indicator_group_key][indicator_key].column = (*int_value).get();
                        }else if(indicator_t_key == "width"){
                            sys_indicator[indicator_group_key][indicator_key].width = (*int_value).get();
                        }else{
                            cout << "indicator_t key " << indicator_key << " is not exist." << endl;
                            exit(200);
                        }
                    }else{
                        cout << "indicator_t key " << indicator_key << " type is not correct...." << endl;
                        exit(200);
                    }
                }
                if(sys_indicator[indicator_group_key][indicator_key].cfile.empty()){
                    cout << "indicator_t key " << indicator_key << " file value is not set." << endl;
                    exit(200);
                }
                if(!sys_indicator[indicator_group_key][indicator_key].column){
                    cout << "indicator_t key " << indicator_key << " column value is not set." << endl;
                    exit(200);
                }
                if(!sys_indicator[indicator_group_key][indicator_key].line && sys_indicator[indicator_group_key][indicator_key].line_begin.empty()){
                    cout << "indicator_t key line and line_begin value is always not set." << endl;
                    exit(200);
                }
            }
        }
    }

    CLI::App cli_global{};
    cli_global.get_formatter()->column_width(40);

    CLI::Option* cli_sys_version_option   = cli_global.add_flag("--version,-V", opts.version, "show the version of ssar.");
    CLI::Option* cli_sys_api_option       = cli_global.add_flag("--api", opts.api, "If selected, output is json format, otherwise shell format.");
    CLI::Option* cli_sys_finish_option    = cli_global.add_option("--finish,-f", opts.finish, "Assign the output datetime, allow history datetime. default value is current datetime.");
    CLI::Option* cli_sys_range_option     = cli_global.add_option("--range,-r", opts.range, "Range from begin to finish, default 300 minutes.");
    CLI::Option* cli_sys_begin_option     = cli_global.add_option("--begin,-b", opts.begin, "Assign the compare datetime by the finish")->excludes(cli_sys_range_option);
    CLI::Option* cli_sys_interval_option  = cli_global.add_option("--interval,-i", opts.interval, "interval, default 10 minutes.");
    CLI::Option* cli_sys_noheaders_option = cli_global.add_flag("--no-headers,-H", opts.noheaders, "Disable the header info.")->excludes(cli_sys_api_option);
    CLI::Option* cli_sys_purify_option    = cli_global.add_flag("--purify,-P", opts.purify, "Hide the - * < > = no number data info.");
    CLI::Option* cli_sys_format_option    = cli_global.add_option("--format,-o", opts.format, "User-defined format. syntax is field1,field2");
    CLI::Option* cli_sys_extra_option     = cli_global.add_option("--extra,-O", opts.extra, "User-defined extra format. syntax is field1,field2")->excludes(cli_sys_format_option);
    CLI::Option* cli_sys_path_option      = cli_global.add_flag("--path", opts.work_path, "specify a dir path as input");
    for(auto it_view_pair : (*opts.sys_view)){
        cli_global.add_flag("--"+it_view_pair.first,opts.views[it_view_pair.first],"Display user define format "+it_view_pair.first)->excludes(cli_sys_format_option);
    }
    cli_global.footer(ssar_cli::DEFAULT_OPTIONS_HELP);

    // ssar procs --finish 2021-05-18T12:34:56
    auto cli_procs = cli_global.add_subcommand("procs", "all process subcommand.");
    CLI::Option* cli_global_api_option = cli_procs->add_flag("--api", opts.api, "If selected, output is json format, otherwise shell format.");
    cli_procs->add_option("--finish,-f", opts.finish,"Assign the output datetime, allow history datetime. default value is current datetime.");
    CLI::Option* cli_global_range_option = cli_procs->add_option("--range,-r", opts.range, "Range from begin to finish, default 1 minutes.");
    cli_procs->add_option("--begin,-b", opts.begin, "Assign the compare datetime by the finish")->excludes(cli_global_range_option);
    cli_procs->add_option("--key,--sort,-k", opts.key, "Specify sorting order. Sorting syntax is [+|-]key[,[+|-]key[,...]]");
    cli_procs->add_option("--lines,-l", opts.lines, "Limit the output.");
    cli_procs->add_flag("--no-headers,-H", opts.noheaders, "Disable the header info.")->excludes(cli_global_api_option);
    CLI::Option* cli_global_format_option = cli_procs->add_option("--format,-o", opts.format, "User-defined format. syntax is field1,field2");
    cli_procs->add_option("--extra,-O", opts.extra, "User-defined extra format. syntax is field1,field2")->excludes(cli_global_format_option);
    cli_procs->add_flag("--cpu", opts.cpu, "Display cpu-oriented format.")->excludes(cli_global_format_option);
    cli_procs->add_flag("--mem", opts.mem, "Display virtual memory format.")->excludes(cli_global_format_option);
    cli_procs->add_flag("--job", opts.job, "Display job control format")->excludes(cli_global_format_option);
    cli_procs->add_flag("--sched", opts.sched, "Display sched format")->excludes(cli_global_format_option);
    cli_procs->add_flag("--path", opts.work_path, "specify a dir path as input");
    cli_procs->footer(ssar_cli::PROCS_OPTIONS_HELP);
    cli_procs->callback([&](){ 
        if(!cli_procs->get_subcommands().size()){
            opts.src = "procs";
            ActivityReporter(opts);
        }
    });
 
    // ssar proc --pid 1 --finish 2021-05-18T12:34:56
    auto cli_proc = cli_global.add_subcommand("proc", "single process subcommand.");
    CLI::Option* cli_proc_api_option = cli_proc->add_flag("--api", opts.api, "If selected, output is json format, otherwise shell format.");
    cli_proc->add_option("--pid,-p", opts.pid, "process id.")->required();
    cli_proc->add_option("--finish,-f", opts.finish, "Assign the output datetime, allow history datetime. default value is current datetime.");
    CLI::Option* cli_proc_range_option = cli_proc->add_option("--range,-r", opts.range, "Range from begin to finish, default 300 minutes.");
    cli_proc->add_option("--begin,-b", opts.begin, "Assign the compare datetime by the finish")->excludes(cli_proc_range_option);
    cli_proc->add_option("--interval,-i", opts.interval, "interval, default 10 minutes..");
    cli_proc->add_flag("--no-headers,-H", opts.noheaders, "Disable the header info.")->excludes(cli_proc_api_option);
    cli_proc->add_flag("--purify,-P", opts.purify, "Hide the - * < > = no number data info.");
    CLI::Option* cli_proc_format_option = cli_proc->add_option("--format,-o", opts.format, "User-defined format. syntax is field1,field2");
    cli_proc->add_option("--extra,-O", opts.extra, "User-defined extra format. syntax is field1,field2")->excludes(cli_proc_format_option);
    cli_proc->add_flag("--cpu", opts.cpu, "Display cpu-oriented format.")->excludes(cli_proc_format_option);
    cli_proc->add_flag("--mem", opts.mem, "Display virtual memory format.")->excludes(cli_proc_format_option);
    cli_proc->add_flag("--path", opts.work_path, "specify a dir path as input");
    cli_proc->footer(ssar_cli::PROC_OPTIONS_HELP);
    cli_proc->callback([&](){ 
        if(!cli_proc->get_subcommands().size()){
            opts.src = "proc";
            ActivityReporter(opts);
        }
    });
    
    // ssar load5s --finish 2021-05-18T12:34:56
    auto cli_load5s = cli_global.add_subcommand("load5s", "load list of five second precision sudcommand.");
    CLI::Option* cli_load5s_api_option = cli_load5s->add_flag("--api", opts.api,"If selected, output is json format, otherwise shell format.");
    cli_load5s->add_option("--finish,-f",opts.finish,"Assign the output datetime, allow history datetime. default value is current datetime.");
    CLI::Option* cli_load5s_range_option = cli_load5s->add_option("--range,-r", opts.range, "Range from begin to finish, default 5 minutes.");
    cli_load5s->add_option("--begin,-b", opts.begin, "Assign the compare datetime by the finish")->excludes(cli_load5s_range_option);
    cli_load5s->add_flag("--no-headers,-H", opts.noheaders, "Disable the header info.")->excludes(cli_load5s_api_option);
    CLI::Option* cli_load5s_yes_option = cli_load5s->add_flag("--yes,-y", opts.yes, "Just display Y sstate collect datetime.");
    cli_load5s->add_flag("--zoom,-z", opts.zoom, "Just display loadrd detail collect datetime.")->excludes(cli_load5s_yes_option);
    cli_load5s->add_flag("--path", opts.work_path, "specify a dir path as input");
    cli_load5s->footer(ssar_cli::LOAD5S_OPTIONS_HELP);
    cli_load5s->callback([&](){
        if(!cli_load5s->get_subcommands().size()){
            opts.src = "load5s";
            ActivityReporter(opts);
        }
    });
    
    // ssar load2p --collect 2021-05-18T12:34:56
    auto cli_load2p = cli_global.add_subcommand("load2p", "Load to process subcommand.");
    CLI::Option* cli_load2p_api_option = cli_load2p->add_flag("--api", opts.api,"If selected, output is json format, otherwise shell format.");
    cli_load2p->add_option("--collect,-c", opts.collect, "collect datetime.")->required();
    cli_load2p->add_option("--lines,-l", opts.lines, "Limit the output.");
    cli_load2p->add_flag("--no-headers,-H", opts.noheaders, "Disable the header info.")->excludes(cli_load2p_api_option);
    cli_load2p->add_flag("--loadr", opts.loadr, "Display loadr format.");
    cli_load2p->add_flag("--loadd", opts.loadd, "Display loadd format.");
    cli_load2p->add_flag("--psr", opts.psr, "Display psr format.");
    cli_load2p->add_flag("--stackinfo", opts.stackinfo, "Display stackinfo format.");
    cli_load2p->add_flag("--loadrd", opts.loadrd, "Display loadrd format.");
    cli_load2p->add_flag("--stack", opts.stack, "Display stack format.");
    cli_load2p->add_flag("--path", opts.work_path, "specify a dir path as input");
    cli_load2p->footer(ssar_cli::LOAD2P_OPTIONS_HELP);
    cli_load2p->callback([&](){
        if(!cli_load2p->get_subcommands().size()){
            opts.src = "load2p";
            ActivityReporter(opts);
        }
    });

    CLI11_PARSE(cli_global, argc, argv);

    if("sys" == opts.src && !cli_global.get_subcommands().size()){                            // ssar --finish 2021-05-18T12:34:56
        ActivityReporter(opts);
    }

    return 0;
}
