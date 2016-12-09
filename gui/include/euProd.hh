#include "ui_euProd.h"
#include "eudaq/Producer.hh"
#include <QMainWindow>
#include <QMessageBox>

class ProducerGUI : public QMainWindow, public Ui::wndProd, public eudaq::Producer {
  Q_OBJECT
  Q_CLASSINFO("Author", "Emlyn Corrin")
  ;
public:
  ProducerGUI(const std::string & name, const std::string & runcontrol);
  void Exec() override final;
private slots:
  void on_btnTrigger_clicked();
};
