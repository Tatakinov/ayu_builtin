#ifndef SERIKO_H_
#define SERIKO_H_

#include <iostream>
#include <queue>
#include <variant>
#include <vector>

#include "actor.h"
#include "character.h"
#include "element.h"
#include "surface.h"

class Actor;

struct ActorWithPriority {
    int id;
    int remain;
};

struct Compare {
    bool operator()(const ActorWithPriority &a, const ActorWithPriority &b) const {
        return a.remain > b.remain;
    }
};

struct CollisionInfo {
    int x, y;
    std::vector<Collision> list;
};

class Character;

class Seriko {
    private:
        int current_id_;
        std::unordered_map<int, Surface> surfaces_;
        std::unordered_map<int, Actor> actors_;
        std::chrono::system_clock::time_point prev_time_;
        std::priority_queue<ActorWithPriority, std::vector<ActorWithPriority>, Compare> process_;
        Character *parent_;
        std::unordered_map<int, bool> binds_;
        std::unordered_map<int, std::unordered_set<int>> bind_addids_;
        void update(bool change = false);
        void updateBind();
    public:
        Seriko(const std::unordered_map<int, Surface> &surfaces) : current_id_(-1), surfaces_(surfaces) {}
        ~Seriko() {}
        void setParent(Character *parent) {
            parent_ = parent;
        }
        void push(int id, int elapsed);
        bool active(int id);
        void activate(From from, int id, int elapsed);
        void inactivate(int id);
        std::vector<RenderInfo> get(int id);
        std::vector<RenderInfo> getElements(int id, std::unordered_set<int> done);
        std::vector<CollisionInfo> getCollision(int id);
        void bind(int id, bool enable);
        bool isBinding(int id);
};

#endif // SERIKO_H_
