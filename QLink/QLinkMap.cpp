//
// Created by 黄喆敏 on 2021/11/1.
//

#include "QLinkMap.h"
#include "Constant.h"
#include <QDebug>
#include <ctime>
#include <cstdlib>
#include <QPixmap>
#include <QGridLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iostream>

QLinkMap::QLinkMap()
{
    for (int i = 0; i < Constant::boxType; i++) {
        icons[i] = new QImage();
        QString filename = ":/res/box" + QString::number(i + 1) + ".png";
        icons[i]->load(filename);
    }
}

void QLinkMap::generate(int h, int w)
{
    this->h = h;
    this->w = w;
    qDebug() << "qLinkMap generate called";

    clear();
    layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    map.resize(h);
    status.resize(h);
    boxes.resize(h);
    for (int i = 0; i < h; i++) {
        map[i].resize(w);
        boxes[i].resize(w);
        status[i].resize(w);
    }
    generateMap();

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            qDebug() << "start init linkbox" << i << "," << j;
            boxes[i][j] = new QLinkBox(h, w, map[i][j], icons[map[i][j]]);
            layout->addWidget(boxes[i][j]->getLabel(), i, j);
            status[i][j] = Constant::BoxStatus::NOT_SELECTED;
//            qDebug() << "add widget completed";
        }
    }
}

void QLinkMap::generateMap()
{
    std::vector<int> tmp;
    int remain = w * h;
    for (int i = 0; i <= Constant::boxType - 2; i++) {
        int num = (w * h) / Constant::boxType;
        qDebug() << "QLinkMap::generateMap called" << i << " " << num;
        if (num % 2 == 1) {
            if (i % 2 == 1) {
                num--;
            } else {
                num++;
            }
        }
        for (int j = 1; j <= num; j++) {
            tmp.push_back(i);
        }
        remain -= num;
    }
    for (int j = 1; j <= remain; j++) {
        tmp.push_back(Constant::boxType - 1);
    }
    random_shuffle(tmp.begin(), tmp.end());

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            map[i][j] = tmp[i * w + j];
        }
    }

}

void QLinkMap::clear()
{
    delete layout;
    layout = nullptr;
    int p = boxes.size();
    for (int i = 0; i < p; i++) {
        int q = boxes[i].size();
        for (int j = 0; j < q; j++) {
            delete boxes[i][j];
            boxes[i][j] = nullptr;
        }
    }
}

QLinkMap::~QLinkMap()
{
    clear();
}

bool QLinkMap::checkPoint(int x, int y)
{
    if (x < 0 || x >= h || y < 0 || y >= w) {
        return false;
    }
    return true;
}


QGridLayout *QLinkMap::getLayout()
{
    return layout;
}

void QLinkMap::setBoxStatus(int x, int y, Constant::BoxStatus p)
{
    if (!checkPoint(x, y)) return;
    status[x][y] = p;
    if (p == Constant::BoxStatus::SELECTED_BY_FIRST) {
        boxes[x][y]->select(0);
    } else if (p == Constant::BoxStatus::SELECTED_BY_SECOND) {
        boxes[x][y]->select(1);
    } else if (p == Constant::BoxStatus::NOT_SELECTED) {
        boxes[x][y]->deSelect();
    } else {
//        boxes[x][y]->clear();
        boxes[x][y]->deSelect();
        map[x][y] = -1;
    }
}

Constant::BoxStatus QLinkMap::getBoxStatus(int x, int y)
{
    if (!checkPoint(x, y)) return Constant::BoxStatus::EMPTY;
    return status[x][y];
}


int QLinkMap::checkRemove(int fromX, int fromY, int toX, int toY)
{
    if (!checkPoint(fromX, fromY) || !checkPoint(toX, toY)) return 0;
    if (status[fromX][fromY] == Constant::BoxStatus::EMPTY || status[fromX][fromY] == Constant::BoxStatus::EMPTY)
        return 0;
    if (map[fromX][fromY] != map[toX][toY]) return 0;
    bool found = false;

    std::vector<int> dir;
    if (fromX < toX) {
        dir = {1, 2, 3, 0};
    } else if (fromX > toX) {
        dir = {0, 1, 2, 3};
    } else {
        if (fromY < toY) {
            dir = {3, 0, 1, 2};
        } else {
            dir = {2, 0, 1, 3};
        }
    }

    for (int i = 0; i < 4; i++) {
        route.clear();
        route.push_back(qMakePair(fromX, fromY));
        if (search(fromX, fromY, toX, toY, 0, dir[i])) {
            found = true;
            route.push_back(qMakePair(toX, toY));
            generatePath();
            break;
        }
    }
    if (!found) return 0;
    int score = 0;
    if (map[fromX][fromY] >= 0 && map[fromX][fromY] < Constant::boxType) {
        score = Constant::boxScore[map[fromX][fromY]];
    }
    map[fromX][fromY] = -1;
    map[toX][toY] = -1;
    return score;
}

bool QLinkMap::checkPointOut(int x, int y)
{
    if (x < -1 || x > h || y < -1 || y > w) return false;
    return true;
}

bool QLinkMap::search(int nowX, int nowY, int toX, int toY, int corner, int d)
{
    qDebug() << "QLinkMap::search" << nowX << " " << nowY << " " << toX << " " << toY << " " << corner << " " << d;
    if (nowX == toX && nowY == toY) return true;
    if (!checkPointOut(nowX, nowY)) return false;
    if (d > 4 || d < 0 || corner > 2) return false;

    int dx = direction[d][0];
    int dy = direction[d][1];
    bool f = checkPoint(nowX + dx, nowY + dy);
    if (nowX + dx == toX && nowY + dy == toY) return true;
    if ((f && map[nowX + dx][nowY + dy] == -1) || (!f && checkPointOut(nowX + dx, nowY + dy))) {   //如果按照当前方向可以走,继续搜索
        route.push_back(qMakePair(nowX + dx, nowY + dy));
        bool tmp = search(nowX + dx, nowY + dy, toX, toY, corner, d);
        if (tmp) return true;
        route.pop_back();
    }

    if (corner < 2) {
        for (int i = 0; i < 4; i++) {
            if (d == i) continue;
            dx = direction[i][0];
            dy = direction[i][1];
            if (nowX + dx == toX && nowY + dy == toY) return true;
            if (checkPoint(nowX + dx, nowY + dy) && map[nowX + dx][nowY + dy] != -1) continue; //除了起点和终点,中间经过的点应该为empty
            if (!checkPointOut(nowX + dx, nowY + dy)) continue;

            route.push_back(qMakePair(nowX + dx, nowY + dy));
            bool tmp = search(nowX + dx, nowY + dy, toX, toY, corner + 1, i);
            if (tmp) return true;
            route.pop_back();
        }
    }
    return false;
}

bool QLinkMap::checkSolvable()
{
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (map[i][j] == -1) continue;
            for (int p = i; p < h; p++) {
                for (int q = 0; q < w; q++) {
                    if ((i == p && j == q) || map[p][q] == -1 || map[i][j] != map[p][q]) continue;
                    qDebug() << "QLinkMap::checkSolvable:" << fromX << " " << fromY << " " << toX << " " << toY;
                    for (int dir = 0; dir < 4; dir++) {
                        if (search(i, j, p, q, 0, dir)) {
                            {
                                fromX = i;
                                fromY = j;
                                toX = p;
                                toY = q;
                                qDebug() << "QLinkMap::checkSolvable find:" << fromX << " " << fromY << " " << toX
                                         << " " << toY;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

QJsonObject QLinkMap::convertToJson()
{
    QJsonObject object;

    h = boxes.size();
    if (h <= 0) return object;
    w = boxes[0].size();

    object.insert("h", h);
    object.insert("w", w);

    QJsonArray mapArray;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            mapArray.append(map[i][j]);
        }
    }
    object.insert("map", QJsonValue(mapArray));

    QJsonArray statusArray;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            statusArray.append(status[i][j]);
        }
    }
    object.insert("status", QJsonValue(statusArray));
    return object;
}

bool QLinkMap::recoverFromJson(QJsonObject object)
{
    qDebug() << "QLinkMap::recoverFromJson called";
    QJsonValue value = object.value("h");
    if (value.isDouble()) {
        h = value.toVariant().toInt();
    } else {
        return false;
    }

    value = object.value("w");
    if (value.isDouble()) {
        w = value.toVariant().toInt();
    } else {
        return false;
    }

    map.clear();
    status.clear();
    boxes.clear();
    map.resize(h);
    status.resize(h);
    boxes.resize(h);
    for (int i = 0; i < h; i++) {
        map[i].resize(w);
        boxes[i].resize(w);
        status[i].resize(w);
    }

    if (layout != nullptr) {
        delete layout;
        layout = nullptr;
    }
    layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);


    value = object.value("map");
    if (value.isArray()) {
        QJsonArray mapArray = value.toArray();
        if (mapArray.size() < h * w) return false;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                map[i][j] = mapArray.at(i * w + j).toInt();
                if (map[i][j] >= Constant::boxType) return false;
                if (boxes[i][j] != nullptr) {
                    delete boxes[i][j];
                    boxes[i][j] = nullptr;
                }
                qDebug() << "create box:" << i << " " << j << " " << map[i][j];
                if (map[i][j] <= -1) {
                    boxes[i][j] = new QLinkBox(h, w, map[i][j]);
                } else {
                    boxes[i][j] = new QLinkBox(h, w, map[i][j], icons[map[i][j]]);
                }
            }
        }
    } else {
        return false;
    }

    value = object.value("status");
    if (value.isArray()) {
        QJsonArray statusArray = value.toArray();
        if (statusArray.size() < h * w) return false;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                status[i][j] = (Constant::BoxStatus) (statusArray.at(i * w + j).toInt());
                layout->addWidget(boxes[i][j]->getLabel(), i, j);
                if (status[i][j] == Constant::BoxStatus::EMPTY) {
                    boxes[i][j]->clear();
                } else if (status[i][j] == Constant::BoxStatus::SELECTED_BY_FIRST) {
                    boxes[i][j]->select(0);
                } else if (status[i][j] == Constant::BoxStatus::SELECTED_BY_SECOND) {
                    boxes[i][j]->select(1);
                }
            }
        }
    } else {
        return false;
    }
    return true;
}

void QLinkMap::shuffle()
{
    if (map.size() < h || map.size() <= 0) return;
    if (map[0].size() < w || map[0].size() <= 0) return;
    std::vector<int> tmp;
    tmp.clear();
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (map[i][j] >= Constant::boxType) return;
            tmp.push_back(map[i][j]);
        }
    }

    random_shuffle(tmp.begin(), tmp.end());
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            map[i][j] = tmp[i * w + j];
            if (map[i][j] == -1) {
                status[i][j] = Constant::BoxStatus::EMPTY;
                boxes[i][j]->clear();
            } else {
                status[i][j] = Constant::BoxStatus::NOT_SELECTED;
                boxes[i][j]->deSelect();
                boxes[i][j]->changeType(icons[map[i][j]]);
            }
        }
    }
}

void QLinkMap::hint()
{
    if (checkSolvable()) {
        boxes[fromX][fromY]->hint();
        boxes[toX][toY]->hint();
        hintFromX = fromX;
        hintFromY = fromY;
        hintToX = toX;
        hintToY = toY;
    }
}

bool QLinkMap::isHint(int fromX, int fromY, int toX, int toY)
{
    if (hintFromX == fromX && hintFromY == fromY && hintToX == toX && hintToY == toY) {
        return true;
    }
    if (hintFromX == toX && hintFromY == toY && hintToX == fromX && hintToY == fromY) {
        return true;
    }
    if (map[hintFromX][hintFromY] == -1 &&
        ((hintToX == toX && hintToY == toY) || (hintToX == fromX && hintToY == fromY))) {
        return true;
    }
    if (map[hintToX][hintToY] == -1 &&
        ((hintFromX == toX && hintFromY == toY) || (hintFromX == fromX && hintFromY == fromY))) {
        return true;
    }
    return false;
}

bool QLinkMap::checkFinish()
{
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (map[i][j] != -1) {
                return false;
            }
        }
    }
    return true;
}

void QLinkMap::clearBox(int x, int y)
{
    if (x < 0 || x >= h || y < 0 || y >= w) return;
    boxes[x][y]->clear();
}

QVector <QLineF> QLinkMap::getPath()
{
    QVector <QLineF> tmp = path;
    path.clear();
    return tmp;
}

void QLinkMap::generatePath()
{
    int size = route.size();
    if (size < 2) return;
    int from = 0;
    std::vector<bool> tmp(size);
    for (int i = 0; i < size; i++) tmp[i] = true;
    for (int i = 2; i < size - 1; i++) {
        for (int j = 0; j < i; j++) {
            if (route[i].first == route[j].first && route[i].second == route[j].second) {
                for (int k = j + 1; k <= i; k++) {
                    tmp[i] = false;
                }
                break;
            }
        }
    }
    int dx = route[1].first - route[0].first, dy = route[1].second - route[1].second;

    for (int i = 2; i < size; i++) {
        if (!tmp[i]) continue;
        int dx1 = route[i].first - route[i - 1].first;
        int dy1 = route[i].second - route[i - 1].second;
        if (dx != dx1 || dy != dy1) {
            path.push_back(QLineF(getPos(route[from].first, route[from].second),
                                  getPos(route[i - 1].first, route[i - 1].second)));
            dx = dx1;
            dy = dy1;
            from = i - 1;
        }
    }
    path.push_back(QLineF(getPos(route[from].first, route[from].second),
                          getPos(route[size - 1].first, route[size - 1].second)));
}

QPointF QLinkMap::getPos(int x, int y)
{
    double p = (double) Constant::mapSize / h * x;
    double q = (double) Constant::mapSize / h * y;
    if (x < 0) p += 50;
    if (y < 0) q += 50;
//    qDebug() << "getPos called, p=" << p + Constant::dx << " q=" << q + Constant::dy;
    return QPointF(q + Constant::dy - w * 5, p + Constant::dx);
}

//剩余没有被选中的
int QLinkMap::remainNumber()
{
    int res=0;
    int p=status.size();
    if(p<=0) return 0;
    int q=status[0].size();
    for(int i=0;i<p;i++) {
        for(int j=0;j<q;j++) {
            if(status[i][j]==Constant::BoxStatus::NOT_SELECTED) res++;
        }
    }
    qDebug()<<"QLinkMap::remainNumber called res="<<res;
    return res;
}

void QLinkMap::deHint() {
    if(checkPoint(hintFromX,hintFromY)) {
        if(status[hintFromX][hintFromY]==Constant::BoxStatus::NOT_SELECTED) {
            boxes[hintFromX][hintFromY]->deSelect();
        }
    }
    if(checkPoint(hintToX,hintToY)) {
        if(status[hintToX][hintToY]==Constant::BoxStatus::NOT_SELECTED) {
            boxes[hintToX][hintToY]->deSelect();
        }
    }
}
