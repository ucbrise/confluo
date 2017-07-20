#ifndef LIBDIALOG_DIALOG_DIALOG_STORE_H_
#define LIBDIALOG_DIALOG_DIALOG_STORE_H_

namespace dialog {

class dialog_store {
 public:
  dialog_store();

 private:
  // Manangement
  task_queue mgmt_queue_;
  task_worker mgmt_worker_;

  // Tables

};

}

#endif /* LIBDIALOG_DIALOG_DIALOG_STORE_H_ */
