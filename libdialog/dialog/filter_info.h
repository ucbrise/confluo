#ifndef DIALOG_FILTER_INFO_H_
#define DIALOG_FILTER_INFO_H_

namespace dialog {

struct filter_info {
  uint32_t filter_id;
  char filter_expr[1024];

  void set_expression(const std::string& expr) {
    strcpy(filter_expr, expr.c_str());
    filter_expr[expr.length()] = '\0';
  }
};

}

#endif /* DIALOG_FILTER_INFO_H_ */
