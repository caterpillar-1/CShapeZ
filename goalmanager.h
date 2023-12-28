#ifndef GOALMANAGER_H
#define GOALMANAGER_H

#include <QObject>
#include "item.h"

class GoalManager : public QObject {
  Q_OBJECT
public:
  GoalManager();
  GoalManager(QDataStream &in);

  // serialize
  void save(QDataStream &out);
  void init();

public slots:
  void receiveItem(const Item *item);

signals:
  void updateGoal(int problemSet, int task, int received, int required, const QPicture *icon);
  void enhanceChange();
  void moneyChange(int delta);
  void mapConstructEvent();

private:
  void advance(); // Having received a correct item. What's next?
  bool inRange();
  void updateIcon();

private:
  int problemSet, task;
  int received, required;

  const Mine *ref;
  QPicture *icon;

  using Task = std::tuple<type_t, shape_t, trait_t, int>;
  using ProblemSet = std::vector<Task>;

  static const std::vector<ProblemSet> levels;
};

#endif // GOALMANAGER_H
