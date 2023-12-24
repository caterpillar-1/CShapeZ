#include "goalmanager.h"

GoalManager::GoalManager()
    : problemSet(0), task(0), received(0),
      required(std::get<3>(levels[problemSet][task])), icon(nullptr) {
  auto &[type, shape, trait, num] = levels[problemSet][task];
  ref = getMine(type, shape, R0, trait);

  updateIcon();
}

GoalManager::GoalManager(QDataStream &in) : icon(nullptr) {
  in >> problemSet >> task >> received;
  auto &[type, shape, trait, num] = levels[problemSet][task];
  required = num;
  assert(inRange());
  ref = getMine(type, shape, R0, trait);

  updateIcon();
}

void GoalManager::save(QDataStream &out) {
  out << problemSet << task << received;
}

void GoalManager::receiveItem(const Item *item) {
  const Task &t = levels[problemSet][task];
  auto [type, shape, trait, num] = t;
  if (auto dut = dynamic_cast<const Mine *>(item)) {
    if (*dut == *ref) {
      advance();
    }
  }
}

void GoalManager::advance() {
  assert(inRange());

  bool sync = false;
  received++;
  if (received == required) {
    received = 0;
    task++;
    sync = true;
  }

  if (task == levels[problemSet].size()) {
    task = 0;
    problemSet++;
  }

  if (problemSet == levels.size()) {
    problemSet = 0;
  }

  if (sync) {
    auto &[type, shape, trait, num] = levels[problemSet][task];
    required = num;
    if (ref) {
      delete ref;
    }
    ref = getMine(type, shape, R0, trait);
    updateIcon();
  }

  emit updateGoal(problemSet, task, received, required, icon);
}

bool GoalManager::inRange()
{
  // sanity check
  if (!(0 <= problemSet && problemSet < levels.size())) {
    return false;
  }
  auto &p = levels[problemSet];
  if (!(0 <= task && task < p.size())) {
    return false;
  }
  if (!(0 <= received && received < required)) {
    return false;
  }
  return true;
}

void GoalManager::updateIcon()
{
  if (icon != nullptr) {
    delete icon;
  }
  icon = new QPicture();
  QPainter painter(icon);

  assert(ref);
  ref->paint(&painter);
}

void GoalManager::init()
{
  emit updateGoal(problemSet, task, received, required, icon);
}

const std::vector<GoalManager::ProblemSet> GoalManager::levels = {
    {
        // problem set 0: Miner, Belt and Cutter
        {SQUARE, FULL, BLACK, 20},
        {ROUND, FULL, BLACK, 30},
        {SQUARE, HALF, BLACK, 50},
    },
    {
        // problem set 1: Mixer, Rotater
        {ROUND, FULL, RED, 20},
        {SQUARE, QUARTER, BLUE, 40},
        {SQUARE, HALF, BLUE, 80},
    },
    {
        // problem set 2: Nothing new
        {ROUND, HALF, BLACK, 10},
        {SQUARE, QUARTER, BLUE, 20},
        {ROUND, FULL, RED, 80},
    },
};
