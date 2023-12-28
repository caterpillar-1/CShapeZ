#ifndef SHOP_H
#define SHOP_H

#include <QtWidgets>

class Shop : public QDialog
{
  Q_OBJECT
public:
  Shop(int &money, qreal &moneyRatio, qreal &itemRatio, int &nextW, int &nextH, QObject *parent = nullptr);

private slots:
  void handleMoneyRatio();
  void handleItemRatio();
  void handleNextMap();

private:
  int &money;
  qreal &moneyRatio;
  qreal &itemRatio;
  int &nextW, &nextH;

  QLabel *dMoneyRatio;
  QLabel *dItemRatio;
  QLabel *dNextMap;
};

#endif // SHOP_H
