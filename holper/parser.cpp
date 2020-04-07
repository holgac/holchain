#include "parser.h"
#include "logger.h"
#include "commandmanager.h"
#include "workpool.h"
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
void Parser::handleMessage(std::unique_ptr<ParserArgs> msg) {
  context_->logger->log(Logger::INFO, "Reading from request %d", msg->request->id());
  std::stringstream request_message;
  while (true) {
    char buf[1024];
    int res = recv(msg->request->socket(), buf, 1024, 0);
    request_message << std::string(buf, res);
    if (res < 0) {
      char errbuf[1024];
      strerror_r(errno, errbuf, 1024);
      context_->logger->log(Logger::ERROR,
          "recv failed: %s", errbuf);
      close(msg->request->socket());
      return;
    }
    if (res == 0) {
      break;
    }
  }
  context_->logger->log(Logger::INFO, "Request %d received %s (%d)",
      msg->request->id(), request_message.str().c_str(),
      (int)request_message.tellp());
  boost::property_tree::ptree tree;
  try {
    boost::property_tree::read_json(request_message, tree);
  } catch (boost::property_tree::json_parser::json_parser_error& _) {
    context_->logger->log(Logger::ERROR, "Json parsing failed");
    msg->request->response()
      .set("response", std::string("Malformed json"))
      .set("code", -1);
    msg->request->respond();
    return;
  }
  std::vector<std::string> args;
  for (auto& val: tree.get_child("")) {
    args.push_back(val.second.get_value<std::string>());
  }

  auto opt = options();
  auto popt = positionalOptions();
  boost::program_options::variables_map vm;
  auto parser = boost::program_options::command_line_parser(args);
  parser.options(opt);
  parser.positional(popt);
  try {
    boost::program_options::store(parser.run(), vm);
  } catch (boost::program_options::unknown_option& e) {
    context_->logger->log(Logger::ERROR, "Invalid command: %s\n", e.what());
    msg->request->response()
      .set("response", std::string("Invalid command: ") + e.what())
      .set("code", -1);
    msg->request->respond();
    return;
  }
  boost::program_options::notify(vm);
  if (vm.count("schedule")) {
    // TODO: move down
    msg->request->response()
      .set("response", "Scheduled commands not yet implemented")
      .set("code", -1);
    msg->request->respond();
    return;
  }
  if (vm.count("command")) {
    args = vm["command"].as<std::vector<std::string>>();
  } else {
    args.clear();
  }

  msg->request->setRawArgs(std::move(args));
  context_->commandManager->resolveRequest(msg->request.get());
  int id = msg->request->id();
  if (msg->request->resolved()) {
    context_->workPool->sendMessage(new WorkPoolArgs(std::move(msg->request)));
    context_->logger->log(Logger::INFO, "Sent request %d to work pool", id);
    return;
  }
  // TODO: worker threads should send help msg, not parser
  context_->logger->log(Logger::INFO, "Could not resolve request %d", id);
  auto cmd = msg->request->command();
  if (cmd) {
    msg->request->response()
      .set("response", cmd->helpMessage())
      .set("code", 0);
    msg->request->respond();
  } else {
    std::stringstream ss;
    ss << opt << std::endl << context_->commandManager->rootCommand()->helpMessage();
    msg->request->response()
      .set("response", ss.str())
      .set("code", 0);
    msg->request->respond();
  }
}
