#pragma once
#include <sstream>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "exception.h"
#include "string.h"

// TODO: find a better place for these
class OnExit {
  std::function<void()> fn_;
public:
  OnExit(std::function<void()> fn) : fn_(fn) {}
  ~OnExit() { fn_(); }
};

class OnExitLine {
public:
  OnExit operator<<(std::function<void()> fn) {
    return OnExit(fn);
  }
};

#define ON_EXIT const auto RANDOMNAME(onexit) = OnExitLine() << [&]()

class Subprocess {
  int in_ = -1, out_ = -1, err_ = -1;
  pid_t pid_;
  void readFromPipe(std::stringstream& ss, int fd) {
    while(true) {
      char buf[1024];
      ssize_t sz = ::read(fd, buf, 1024);
      if (sz < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
        THROW("read failed: %s", St::errorString().c_str());
      } else if (sz > 0) {
        ss.write(buf, sz);
      } else {
        return;
      }
    }
  }
public:
  ~Subprocess() {
    if (in_ != -1) {
      close(in_);
    }
    if (out_ != -1) {
      close(out_);
    }
    if (err_ != -1) {
      close(err_);
    }
  }
  // TODO: use context and get rid of throws
  // TODO: support spawn&disown for things like spotify
  Subprocess(const std::vector<std::string>& args) {
    const char** ptrs = new const char*[args.size() + 1];
    ON_EXIT { delete[] ptrs; };
    for(size_t i=0; i<args.size(); ++i) {
      ptrs[i] = args[i].c_str();
    }
    ptrs[args.size()] = NULL;
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    if (0 != pipe(stdin_pipe)) {
      THROW("pipe failed: %s", St::errorString().c_str());
    }
    if (0 != pipe2(stdout_pipe, O_NONBLOCK)) {
      close(stdin_pipe[0]);
      close(stdin_pipe[1]);
      THROW("pipe failed: %s", St::errorString().c_str());
    }
    if (0 != pipe2(stderr_pipe, O_NONBLOCK)) {
      close(stdin_pipe[0]);
      close(stdin_pipe[1]);
      close(stdout_pipe[0]);
      close(stdout_pipe[1]);
      THROW("pipe failed: %s", St::errorString().c_str());
    }

    pid_ = fork();
    if (pid_ < 0) {
      close(stdin_pipe[0]);
      close(stdin_pipe[1]);
      close(stdout_pipe[0]);
      close(stdout_pipe[1]);
      close(stderr_pipe[0]);
      close(stderr_pipe[1]);
      THROW("fork failed: %s", St::errorString().c_str());
    }
    if (pid_ == 0) {
      close(stdout_pipe[0]);
      close(stderr_pipe[0]);
      close(stdin_pipe[1]);
      dup2(stdin_pipe[0], 0);
      dup2(stdout_pipe[1], 1);
      dup2(stderr_pipe[1], 2);
      execv(ptrs[0], (char* const*)ptrs);
      THROW("exec failed: %s", St::errorString().c_str());
    }
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    in_ = stdin_pipe[1];
    out_ = stdout_pipe[0];
    err_ = stderr_pipe[0];
  }

  void write(const std::string& data) {
    size_t size = data.size();
    size_t wsize = ::write(in_, data.c_str(), size);
    if (size != wsize) {
      THROW("Write attempt failed (%ld/%ld)", wsize, size);
    }
    fsync(in_);
  }

  void finish(std::string& out, std::string& err) {
    close(in_);
    in_ = -1;
    std::stringstream oss, ess;
    pid_t w;
    int wstatus;
    do {
      w = waitpid(pid_, &wstatus, WUNTRACED | WCONTINUED);
      if (w == -1) {
        THROW("Waitpid failed: %s", St::errorString().c_str());
      }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    readFromPipe(oss, out_);
    readFromPipe(ess, err_);
    close(out_);
    out_ = -1;
    close(err_);
    err_ = -1;
    out = oss.str();
    err = ess.str();
  }
};
