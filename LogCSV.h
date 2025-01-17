#ifndef LogCSV_h
#define LogCSV_h

#include "Log.h"
#include <stdio.h>

/** Log packets into multiple CSV files, one for each apid. Documentation is in the form of column headers for each file.
 */
class LogCSV: public Log {
private:
  /** Number of APIDs allowed. The possible APIDs run from 0 to n_apid-1
   *  inclusive. APIDs equal or greater than n_apid will write off the
   *  end of at least one array. */
  static const int nApid=64;
  /** Under some conditions a packet is buffered into an in-memory array before
   *  it is written out. The condition is when we are documenting a packet for
   *  the first time. This packet must have a length less than or equal to
   *  this value or the code will write off the end of the array.
   */
  static const int bufSize=256;
  /** Pointer to the APID map.
   */
  char buf[bufSize];
  FILE* stream;
  FILE* fbuf; //Used to write to buf with fprintf
  int pktApid;
  bool hasDoc[nApid];
  bool firstField;
  void writeDoc(const char* fieldName) {
    if(!hasDoc[pktApid] && fieldName!=nullptr) {
      inDoc=true;
	  fprintf(stream,firstField?"%s":",%s",fieldName);
	}
  }
  bool inDoc;
public:
  /** Construct a LogCSV packet stream.
   * \param basename Base name (name of filename without path) to use for the
   *                 stream.
   * \param LbufEnabled Buffer enabled switch. If true (default), then the
   *                    normal stdio buffer is enabled, and the stream is only
   *                    flushed when the buffer is full. On a Pi, this is
   *                    after 4096 bytes. If false, then buffering is disabled
   *                    and packets are immediately written to the filesystem.
   */
  LogCSV(const char* filenames, bool bufEnabled=true);
  virtual ~LogCSV();
  virtual void start(int apid, const char* pktName=nullptr);
  virtual void write(int8_t      value,          const char* fieldName=nullptr) {write(( int32_t)value,fieldName);};
  virtual void write(int16_t     value,          const char* fieldName=nullptr) {write(( int32_t)value,fieldName);};
  virtual void write(int32_t     value,          const char* fieldName=nullptr) {writeDoc(fieldName);fprintf(fbuf,firstField?"%d":",%d",value);  firstField=false;};
  virtual void write(uint8_t     value,          const char* fieldName=nullptr) {write((uint32_t)value,fieldName);};
  virtual void write(uint16_t    value,          const char* fieldName=nullptr) {write((uint32_t)value,fieldName);};
  virtual void write(uint32_t    value,          const char* fieldName=nullptr) {writeDoc(fieldName);fprintf(fbuf,firstField?"%u":",%u",value);  firstField=false;};
  virtual void write(float       value,          const char* fieldName=nullptr) {write((double)value,fieldName);};
  virtual void write(double      value,          const char* fieldName=nullptr) {writeDoc(fieldName);fprintf(fbuf,firstField?"%f":",%f",value);  firstField=false;};
  virtual void write(const char* value, int len, const char* fieldName=nullptr);
  virtual void write(const char* value,          const char* fieldName=nullptr);
  virtual void end();
};

#endif /* LOG_H_ */
