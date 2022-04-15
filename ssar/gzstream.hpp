#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <zlib.h>
#include <string.h>

using namespace std;
bool isGzipFileName(string file_name){
    int file_size = file_name.size();
    if(file_size > 3){
        if(".gz" == file_name.substr(file_size - 3, 3)){
            return true;
        }
    }
    return false;
}

class gzstreambuf: public streambuf {
public:
    static const int bufferSize = 47 + 256;                    // size of data buff
    gzFile file;                                               // file handle for compressed file
    char   buffer[bufferSize];                                 // data buffer
    char   opened;                                             // open/close state of stream

    gzstreambuf() : opened(0) {
        this->setg(this->buffer + 4, this->buffer + 4, this->buffer + 4); // setg(gfirst, gnext, glast), beginning of putback area
    }
    gzstreambuf(const char* name) : gzstreambuf(){
        this->gzsbopen(name);
    }

    int is_open(){
        return this->opened;
    }
    gzstreambuf* gzsbopen(const char* name){
        if(this->is_open()){
            return (gzstreambuf*) 0;
        }
        this->file = gzopen(name, "rb");
        if (this->file == 0){
            return (gzstreambuf*) 0;
        }
        this->opened = 1;
        return this;
    }

    gzstreambuf* gzsbclose(){
        if(this->is_open()) {
            this->opened = 0;
            if (gzclose(this->file) == Z_OK){                   // Z_OK indicates that a gzip stream was completed on the last gzread
                return this;
            }
        }
        return (gzstreambuf*) 0;
    }

    int gzsbseek(streamoff off, ios::seekdir way){
        int res = gzseek(this->file, off, way);
        if(-1 == res){
            ostringstream oss;
            oss << (filebuf *) this;
            res = gzseek(this->file, off, way);
        }
        return res;
    }
    streampos gzsbtell(){
        return (streampos)gztell(this->file);
    }

    int underflow(){                                           // only used for input buffer
        if (this->gptr() && (this->gptr() < this->egptr())){
            return *reinterpret_cast<unsigned char *>(this->gptr());
        }

        if (!this->opened){
            return EOF;
        }
        int n_putback = this->gptr() - this->eback();
        if (n_putback > 4){
            n_putback = 4;
        }
        memmove(this->buffer + (4 - n_putback), this->gptr() - n_putback, n_putback);

        int num = gzread(this->file, this->buffer + 4, bufferSize - 4);
        if (num <= 0){                                         // ERROR or EOF
            return EOF;
        }

        // reset buffer pointers
        this->setg(this->buffer + (4 - n_putback), this->buffer + 4, this->buffer + 4 + num);          // beginning of putback area, read position. end of buffer

        // return next character
        return *reinterpret_cast<unsigned char *>(this->gptr());
    }

    ~gzstreambuf(){
        if(this->is_open()){
            this->gzsbclose();
        }
    }
};


class igzfstream: public ifstream {
    gzstreambuf gz_buf;
public:
    igzfstream(){}
    void open(const string &name, ios::openmode open_mode = ios::in) {
        if(isGzipFileName(name)){
            gzstreambuf* res = gz_buf.gzsbopen(name.c_str());
            ios::rdbuf(&gz_buf);
            if(!res){
                ios::setstate(ios::failbit|ios::badbit);
            }
        }else{
            ifstream::open(name, ios::in);
        }
    }
    streambuf* rdbuf(){
        return istream::rdbuf();
    }
    igzfstream& seekg(streamoff off, ios::seekdir way){
        if(gz_buf.is_open()){
            gz_buf.gzsbseek(off, way);
        }else{
            ifstream::seekg(off, way);
        }
        return *this;
    }
    streampos tellg(){
        if(gz_buf.is_open()){
            return gz_buf.gzsbtell();
        }else{
            return ifstream::tellg();
        }
    }
};
