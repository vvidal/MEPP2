#ifndef DialogProcessing1_H
#define DialogProcessing1_H
////////////////////////////////////////////////////////////////////////////////
#include <QDialog>
////////////////////////////////////////////////////////////////////////////////
namespace Ui {
class DialogProcessing1;
}
////////////////////////////////////////////////////////////////////////////////
namespace FEVV {

class DialogProcessing1 : public QDialog
{
  Q_OBJECT

public:
  explicit DialogProcessing1(QWidget *parent = 0);
  ~DialogProcessing1();

  void setProcess(double x, double y, double z);
  void getProcess(double &x, double &y, double &z);

private:
  Ui::DialogProcessing1 *ui;
};

} // namespace FEVV

////////////////////////////////////////////////////////////////////////////////
#endif // DialogProcessing1_H
