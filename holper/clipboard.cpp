#include "clipboard.h"
#include "holper.h"
#include "command.h"
#include "context.h"
#include "request.h"
#include "filesystem.h"
#include "workpool.h"
#include "subprocess.h"
#include "logger.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>

class GeneratePasswordAction : public Action
{
  const std::string kLower = "abcdefghijklmnopqrstuvwxyz";
  const std::string kUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string kNumeric = "1234567890";
  const std::string kSpecial = "!@#$%^&*()_+-=";
public:
  GeneratePasswordAction(Context* context)
      : Action(context) {}
  void spec(ParamSpec& spec) const override {
    auto minmaxspec = [&](const char* name) {
      spec.param<int>(St::fmt("min_%s", name), "", 
          St::fmt("minimum number of %s chars", name));
      spec.param<int>(St::fmt("max_%s", name), "", 
          St::fmt("maximum number of %s chars", name));
    };
    minmaxspec("lower");
    minmaxspec("upper");
    minmaxspec("numeric");
    minmaxspec("special");
    spec
      .param<int>("length", "length", "password length")
      .key("length", 1, 1);
  }
  rapidjson::Value actOn(Work* work) const override {
    auto& params = work->parameters();
    auto getordefault = [&](const char* name, int mindef, int maxdef) {
      std::pair<int,int> vals;
      auto val = params.get<int>(St::fmt("min_%s", name));
      if (!val) {
        vals.first = mindef;
      } else {
        vals.first = *val;
      }
      val = params.get<int>(St::fmt("max_%s", name));
      if (!val) {
        vals.second = maxdef;
      } else {
        vals.second = *val;
      }
      if (vals.second < vals.first) {
        vals.first = vals.second;
      }
      // since we don't decrement max when we pick from min pool
      vals.second -= vals.first;
      return vals;
    };
    int len = *params.get<int>("length");
    auto lower = getordefault("lower", 1, len);
    auto upper = getordefault("upper", 1, len);
    auto numeric = getordefault("numeric", 1, len);
    auto special = getordefault("special", 1, len);
    std::vector<char> result;
    result.reserve(len);

    std::vector<std::pair<std::pair<int,int>, std::string>> limit_alpha_pairs{
      {lower, kLower},
      {upper, kUpper},
      {numeric, kNumeric},
      {special, kSpecial}
    };

    auto getmin = [](auto& limit) -> int& { return limit.first; };
    auto getmax = [](auto& limit) -> int& { return limit.second; };
    auto calc_size_for = []
      (const auto& getter, auto& limit, const auto& alpha) -> int {
        if (getter(limit)) {
          return alpha.size();
        }
        return 0;
    };
    auto process_if_match = [&result]
      (int& pick, auto& getter, auto& limit, const auto& alpha) {
        if(pick >= 0 && getter(limit)) {
          if (pick < (int)alpha.size()) {
            result.push_back(alpha[pick]);
            getter(limit)--;
          }
          pick -= alpha.size();
        }
    };
    auto iterate = [&](auto& getter) {
      int len = 0;
      for (auto& [limit, alpha] : limit_alpha_pairs) {
        len += calc_size_for(getter, limit, alpha);
      }
      unsigned rand;
      Fs::urandom(4, &rand);
      int pick = rand % len;
      for (auto& [limit, alpha] : limit_alpha_pairs) {
        process_if_match(pick, getter, limit, alpha);
      }
    };

    int minlens = lower.first + upper.first + numeric.first + special.first;
    if (minlens > len) {
      context_->logger->warn("Given min lengths exceed password length");
    }
    for (int i=0; i<len; ++i) {
      if (i < minlens) {
        // first add min amount of everything
        iterate(getmin);
      } else {
        iterate(getmax);
      }
    }
    std::string password(len, '\0');
    for (int i=0; i<len; ++i) {
      unsigned rand;
      Fs::urandom(4, &rand);
      int pick = rand % result.size();
      password[i] = result[pick];
      result.erase(result.begin() + pick);
    }
    Subprocess sp({"/usr/bin/xsel", "--input", "--clipboard"});
    sp.write(password);
    {
      std::string o, e;
      sp.finish(o,e);
    }
    return rapidjson::Value("success");
  }
};

void ClipboardCommandGroup::initializeCommand(Context* context,
    Command* command) {
  (*command)
    .setName("clipboard").setName("clip")
    .setDescription("System control");
  (*command->addChild())
    .setName("genpwd").setName("password")
    .setDescription("Generates a password and stores in clipboard")
    .makeAction<GeneratePasswordAction>(context);
}
