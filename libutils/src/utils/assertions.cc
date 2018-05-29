#include "assertions.h"

namespace utils {
namespace assert {

assertion::assertion(const char *expr, const char *file, int line)
    : expr_(expr),
      file_(file),
      line_(line) {
}

assertion::~assertion() noexcept(false) {
  if (!msg_.str().empty())
    msg_ << ": ";

  std::string expr_str(expr_);
  if (expr_str == "false" || expr_str == "FALSE" || expr_str == "0")
    msg_ << "Unreachable code assertion";
  else
    msg_ << "Assertion '" << expr_str << "'";

  msg_ << " failed, file '" << file_ << "' at line " << line_;
  throw assertion_failure_exception(msg_.str());
}

}
}