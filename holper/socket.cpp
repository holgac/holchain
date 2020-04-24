#include "socket.h"
#include "string.h"
#include "exception.h"
#include "context.h"
#include "logger.h"
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

UnixSocket::UnixSocket(Context* context, const std::string& path)
  : path_(path), context_(context)
{
  socket_ = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (socket_ == -1) {
    THROW("Cannot create unix socket: %s", StringUtils::errorString().c_str());
  }
}

UnixSocket::UnixSocket(Context* context, int socket)
  : path_("Client socket"), socket_(socket), context_(context)
{
}

UnixSocket::UnixSocket(UnixSocket&& socket)
    : path_(socket.path_), socket_(socket.socket_), context_(socket.context_) {
  socket.socket_ = -1;
}

UnixSocket::~UnixSocket() {
  if (socket_ != -1) {
    close(socket_);
  }
}

void UnixSocket::connect() {
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  if (path_.size() >= sizeof(addr.sun_path)) {
    THROW("Cannot create unix socket - path does not fit");
  }
  memcpy(addr.sun_path, path_.c_str(), path_.size() + 1);
  int res = ::connect(socket_, (struct sockaddr*)&addr, sizeof(addr));
  if (res == -1) {
    THROW("Cannot connect unix socket: %s", StringUtils::errorString().c_str());
  }
}

void UnixSocket::write(const std::string& data) {
  u64 datalen = (u64)data.size();
  ssize_t sentlen;
  sentlen = send(socket_, &datalen, sizeof(u64), 0);
  if(sentlen != sizeof(u64)) {
    THROW("Partial write (%u out of %u)",
        (unsigned)sentlen, (unsigned)(sizeof(u64)));
  }
  sentlen = send(socket_, data.c_str(), datalen, 0);
  if ((u64)sentlen != datalen) {
    THROW("Partial write (%u out of %llu)", (unsigned)sentlen, datalen);
  }
}

void UnixSocket::readRaw(u64 len, void* dest) {
  u64 total_read = 0;
  char* bytes = (char*)dest;
  while (total_read < len) {
    int res = recv(socket_, bytes + total_read, len - total_read, MSG_WAITALL);
    if (res < 0) {
      THROW("Cannot read from unix socket: %s",
          StringUtils::errorString().c_str());
    }
    total_read += res;
  }
}

std::string UnixSocket::read() {
  u64 length;
  readRaw(sizeof(u64), (void*)&length);
  char* buf = new char[length];
  readRaw(length, (void*)buf);
  std::string st(buf, length);
  delete[] buf;
  return st;
}

void UnixSocket::serve(std::function<void(std::unique_ptr<UnixSocket>)> fn) {
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  if (path_.size() >= sizeof(addr.sun_path)) {
    THROW("Cannot create unix socket - path does not fit");
  }
  memcpy(addr.sun_path, path_.c_str(), path_.size() + 1);
  if (0 != bind(socket_, (struct sockaddr*)&addr, sizeof(addr))) {
    THROW("Bind failed: %s", StringUtils::errorString().c_str());
  }
  if (0 != listen(socket_, 10)) {
    THROW("Listen failed: %s", StringUtils::errorString().c_str());
  }
  while (true) {
    int fd = accept(socket_, NULL, NULL);
    if (fd < 0) {
      context_->logger->logErrno("Socket accept failed");
    }
    fn(std::move(std::make_unique<UnixSocket>(context_, fd)));
  }
}
TimePoint UnixSocket::ctime() {
  return ctime_;
}
