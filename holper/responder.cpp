#include "responder.h"
#include "logger.h"

void Responder::handleMessage(std::unique_ptr<ResponderArgs> msg) {
  std::unique_ptr<Request> request = std::move(msg->request);
  context_->logger->info() << "Responder responding to " << request->id();
  request->profiler().event("Received by Responder");
  request->response().set("response", msg->response.Move());
  request->sendResponse(msg->code);
}
