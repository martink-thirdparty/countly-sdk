/*
 CountlyEventQueue.cpp
 CountlyCpp
 
 Created by Benoit Girard on 26/10/14.
 
 The MIT License (MIT)
 
 Copyright (c) 2015 Kontrol SAS (tanker.io)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */


#include "CountlyEventQueue.h"

#include <assert.h>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#include "Countly.h"

using std::string;

namespace CountlyCpp {

CountlyEventQueue::CountlyEventQueue() {
  _path = "";
#ifndef NOSQLITE
  _sqlHandler = NULL;
#else
  _evtIdCounter = 0;
#endif
#ifndef _WIN32
  pthread_mutexattr_t mutAttr;
  pthread_mutexattr_init(&mutAttr);
  pthread_mutex_init(&_lock, &mutAttr);
#else
  _lock = CreateMutex(NULL, false, NULL);
#endif
}

CountlyEventQueue::~CountlyEventQueue() {
#ifndef NOSQLITE
  if (_sqlHandler)
    sqlite3_close(_sqlHandler);
#endif
#ifndef _WIN32
  pthread_mutex_destroy(&_lock);
#else
  CloseHandle(_lock);
#endif
}

void CountlyEventQueue::SetPath(string path) {
  _path = path;
}

void CountlyEventQueue::Lock() {
#ifndef _WIN32
  pthread_mutex_lock(&_lock);
#else
  WaitForSingleObject(_lock, INFINITE);
#endif
}

void CountlyEventQueue::Unlock() {
#ifndef _WIN32
  pthread_mutex_unlock(&_lock);
#else
  ReleaseMutex(_lock);
#endif
}

bool CountlyEventQueue::LoadDb() {
  assert(_path.size());
  Lock();

#ifndef NOSQLITE
  if (_sqlHandler) {
    Unlock();
    return true;
  }

  string fullpath = _path + string("countly.sqlite");
  if (sqlite3_open(fullpath.c_str(), &_sqlHandler) != SQLITE_OK) {
    Unlock();
    return false;
  }
  assert(sqlite3_threadsafe());

  char* zErrMsg = NULL;
  string req = "CREATE TABLE IF NOT EXISTS events (evtid INTEGER PRIMARY KEY, event TEXT)";  // NOLINT(whitespace/line_length)
  unsigned int code = sqlite3_exec(_sqlHandler, req.c_str(), NULL, 0, &zErrMsg);
  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
      Unlock();
      return false;
    }
  }

  req = "CREATE TABLE IF NOT EXISTS settings (deviceid TEXT)";
  code = sqlite3_exec(_sqlHandler, req.c_str(), NULL, 0, &zErrMsg);
  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
      Unlock();
      return false;
    }
  }
#else
  if (_deviceid.length() > 0) {
    Unlock();
    return true;
  }

  string deviceid;
  string fullpath = _path + string("countly.deviceid");
  std::fstream fs;
  fs.open(fullpath.c_str(), std::fstream::in);
  getline(fs, deviceid);
  fs.close();
  if (deviceid.length() == 0) {
    fs.open(fullpath.c_str(), std::fstream::out);
    deviceid = MakeDeviceId();
    fs << deviceid << std::endl;
    fs.close();
  }
  _deviceid = deviceid;
#endif

  Unlock();
  return true;
}

bool CountlyEventQueue::RecordEvent(string key, int count) {
  std::stringstream json;
  json << "{\n";
  json << "  \"timestamp\": \"" << std::dec
    << Countly::GetTimestamp() << "\",\n";
  json << "  \"key\": \"" << key << "\",\n";
  json << "  \"count\": " << std::dec << count << "\n";
  json << "}";
  return AddEvent(json.str());
}

bool CountlyEventQueue::RecordEvent(string key, int count, double sum) {
  std::stringstream json;
  json << "{\n";
  json << "  \"timestamp\": \"" << std::dec
    << Countly::GetTimestamp() << "\",\n";
  json << "  \"key\": \"" << key << "\",\n";
  json << "  \"count\": " << std::dec << count << ",\n";
  json << "  \"sum\": \"" << std::dec << sum << "\"\n";
  json << "}";
  return AddEvent(json.str());
}

bool CountlyEventQueue::RecordEvent(string key,
  std::map<string, string> segmentation, int count
) {
  std::stringstream json;
  std::map<string, string>::iterator it;
  json << "{\n";
  json << "  \"timestamp\": \"" << std::dec
    << Countly::GetTimestamp() << "\",\n";
  json << "  \"key\": \"" << key << "\",\n";
  json << "  \"count\": " << std::dec << count << ",\n";

  json << "  \"segmentation\": {\n";

  it = segmentation.begin();
  while (it != segmentation.end()) {
    json << "    \"" << it->first << "\": \"" << it->second
         << "\"" << (it == segmentation.end() ? "":",") << "\n";
    it++;
  }
  json << "  }\n";
  json << "}";
  return AddEvent(json.str());
}

bool CountlyEventQueue::RecordEvent(string key,
  std::map<string, string> segmentation, int count, double sum
) {
  std::stringstream json;
  std::map<string, string>::iterator it;

  json << "{\n";
  json << "  \"timestamp\": \"" << std::dec
    << Countly::GetTimestamp() << "\",\n";
  json << "  \"key\": \"" << key << "\",\n";
  json << "  \"count\": " << std::dec << count << ",\n";
  json << "  \"sum\": \"" << std::dec << sum << "\",\n";
  json << "  \"segmentation\": {\n";

  it = segmentation.begin();
  while (it != segmentation.end()) {
    json << "    \"" << it->first << "\": \"" << it->second
         << "\"" << (it == segmentation.end() ? "":",") << "\n";
    it++;
  }
  json << "  }\n";
  json << "}";
  return AddEvent(json.str());
}

bool CountlyEventQueue::AddEvent(string json) {
  if (!LoadDb())
    return false;
  Lock();

#ifndef NOSQLITE
  char* zErrMsg = NULL;
  string req = "INSERT INTO events (event) VALUES('" + json +"')";
  unsigned int code = sqlite3_exec(_sqlHandler, req.c_str(), NULL, 0, &zErrMsg);

  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
      Unlock();
      return false;
    }
  }
#else
  EventsItem item;
  item.evtId = _evtIdCounter++;
  item.json = json;
  _events.push_back(item);
#endif

  Unlock();
  return true;
}

string CountlyEventQueue::MakeDeviceId() {
  std::stringstream UDID;
  uint64_t seed = Countly::GetTimestamp();
  srand((unsigned int) seed);
  for (int i = 0; i < 5; i++)  // 5 * 8 = 40
    UDID << std::setfill('0') << std::setw(8)
         << std::hex << rand();  // NOLINT(runtime/threadsafe_fn)
  return UDID.str();
}

string CountlyEventQueue::GetDeviceId() {
  string deviceid;
  LoadDb();
  Lock();

#ifndef NOSQLITE
  char* zErrMsg = NULL;
  char** pazResult;
  int rows, nbCols;

  // Read deviceid from settings
  string req = "SELECT deviceid FROM settings";
  unsigned int code = sqlite3_get_table(_sqlHandler, req.c_str(),
    &pazResult, &rows, &nbCols, &zErrMsg);
  if ((code == SQLITE_OK) && (rows)) {
    deviceid = pazResult[1];
    sqlite3_free_table(pazResult);
    Unlock();
    return deviceid;
  }
  sqlite3_free_table(pazResult);
  Unlock();

  deviceid = MakeDeviceId();
  req = "INSERT INTO settings (deviceid) VALUES('" + deviceid + "')";
  Lock();
  code = sqlite3_exec(_sqlHandler, req.c_str(), NULL, 0, &zErrMsg);
  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
    }
  }
#else
  deviceid = _deviceid;
#endif

  Unlock();
  return deviceid;
}

int CountlyEventQueue::Count() {
  int ret = 0;
  LoadDb();
  Lock();

#ifndef NOSQLITE
  char* zErrMsg = NULL;
  char** pazResult;
  int rows, nbCols;

  string req = "SELECT COUNT(*) FROM events";
  unsigned int code = sqlite3_get_table(_sqlHandler, req.c_str(),
    &pazResult, &rows, &nbCols, &zErrMsg);
  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
    }
    Unlock();
    return 0;
  }

  if (rows != 0)
    ret = atoi(pazResult[1]);

  sqlite3_free_table(pazResult);
#else
  ret = _events.size();
#endif

  Unlock();
  return ret;
}

string CountlyEventQueue::PopEvent(int* evtId, size_t offset) {
  string ret;
  LoadDb();
  Lock();

#ifndef NOSQLITE
  char* zErrMsg = NULL;
  char** pazResult;
  int rows, nbCols;
  *evtId = -1;

  char req[64];
  snprintf(req, "SELECT evtid, event FROM events LIMIT 1 OFFSET %d", offset);
  unsigned int code = sqlite3_get_table(_sqlHandler, req,
    &pazResult, &rows, &nbCols, &zErrMsg);
  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
    }
    Unlock();

    return "";
  }

  if (!rows) {
    Unlock();
    return "";
  }

  // pasResult[0] : "evtid"
  // pasResult[1] : "event"
  *evtId = atoi(pazResult[2]);
  ret = pazResult[3];
  sqlite3_free_table(pazResult);
#else
  if (offset >= _events.size()) {
    *evtId = -1;
    Unlock();
    return "";
  }
  *evtId = _events[offset].evtId;
  ret = _events[offset].json;
#endif

  Unlock();
  return ret;
}

void CountlyEventQueue::ClearEvent(int evtId) {
  LoadDb();
  Lock();

#ifndef NOSQLITE
  stringstream req;
  req  << "DELETE FROM events WHERE evtid=" << dec << evtId;

  char* zErrMsg = NULL;
  unsigned int code = sqlite3_exec(_sqlHandler, req.str().c_str(),
    NULL, 0, &zErrMsg);

  if (code != SQLITE_OK) {
    if ((code == SQLITE_CORRUPT) || (code == SQLITE_IOERR_SHORT_READ) ||
        (code == SQLITE_IOERR_WRITE) || (code == SQLITE_IOERR)) {
      sqlite3_close(_sqlHandler);
      _sqlHandler = NULL;
    }
  }
#else
  assert(_events.size() > 0);
  assert(_events.front().evtId == evtId);
  _events.pop_front();
#endif

  Unlock();
}

}  // namespace CountlyCpp
