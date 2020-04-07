#pragma once
#include <memory>
#include <boost/program_options.hpp>
#include "request.h"
#include "thread.h"


struct ParserArgs {
  std::unique_ptr<Request> request;
  explicit ParserArgs(std::unique_ptr<Request>&& req)
      : request(std::move(req)) {}
};

class Parser : public Thread<ParserArgs>
{
private:
  boost::program_options::positional_options_description positionalOptions() const
  {
    boost::program_options::positional_options_description popt;
    popt.add("command", -1);
    return popt;
  }
  boost::program_options::options_description options() const {
    boost::program_options::options_description opts;
    opts.add_options()
      ("schedule", boost::program_options::value<int>(),
        "Schedule the command instead of running immediately")
      ("command", boost::program_options::value<std::vector<std::string>>(), "The actual command")
    ;
    return opts;
  }
public:
  explicit Parser(std::shared_ptr<Context> context) : Thread("Parser", context) {}
  void handleMessage(std::unique_ptr<ParserArgs> msg);
};
