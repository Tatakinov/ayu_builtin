#include "seriko.h"

#include <algorithm>
#include <iostream>

#include "logger.h"

void Seriko::update(bool change) {
    auto now = std::chrono::system_clock::now();
    int elapsed = (change) ? (0) : (std::chrono::duration_cast<std::chrono::milliseconds>(now - prev_time_).count());
    while (!process_.empty()) {
        process_.pop();
    }
    for (auto &[k, v] : actors_) {
        if (change) {
            v.activate(From::System);
        }
        push(k, elapsed);
    }
    while (!process_.empty()) {
        auto [k, t] = process_.top();
        process_.pop();
        actors_.at(k).update(t);
    }
    prev_time_ = now;
}

void Seriko::push(int id, int elapsed) {
    if (!actors_.contains(id)) {
        return;
    }
    auto &actor = actors_.at(id);
    if (!actor.active()) {
        return;
    }
    process_.push({id, elapsed});
}

bool Seriko::active(int id) {
    if (!actors_.contains(id)) {
        Logger::log("id: ", id, " not found");
        return false;
    }
    return actors_.at(id).active();
}

void Seriko::activate(From from, int id, int elapsed) {
    if (!actors_.contains(id)) {
        Logger::log("animation id: ", id , " not found");
        return;
    }
    auto &actor = actors_.at(id);
    if (actor.active()) {
        Logger::log("animation id: ", id , " already active");
        return;
    }
    actor.activate(from);
    push(id, elapsed);
}

void Seriko::inactivate(int id) {
    if (!actors_.contains(id)) {
        return;
    }
    auto &actor = actors_.at(id);
    actor.inactivate();
}

std::vector<RenderInfo> Seriko::get(int id) {
    if (!surfaces_.contains(id)) {
        return {};
    }
    std::vector<RenderInfo> ret;
    if (current_id_ != id) {
        current_id_ = id;
        if (!surfaces_.contains(id)) {
            return {};
        }
        auto &surface = surfaces_.at(id);
        actors_.clear();
        for (auto &[k, v] : surface.animation) {
            Actor actor = {k, v, this};
            actors_.emplace(k, actor);
        }
        updateBind();
        update(true);
    }
    else {
        update();
    }
    auto &surface = surfaces_.at(id);
    std::vector<int> list;
    list.reserve(std::max(surface.element.size(), actors_.size()));
    ret.reserve(surface.element.size());
    // TODO background
    for (auto &[k, _] : surface.element) {
        list.emplace_back(k);
    }
    std::sort(list.begin(), list.end());
    for (auto i : list) {
        ret.emplace_back(surface.element[i]);
    }
    list.clear();
    int allocate = ret.size();
    for (auto &[k, v] : actors_) {
        list.emplace_back(k);
        allocate += v.patterns().size();
    }
    ret.reserve(allocate);
    std::sort(list.begin(), list.end());
    std::unordered_set<int> done = {id};
    for (auto i : list) {
        auto &actor = actors_.at(i);
        auto &interval = actor.interval();
        if (interval.size() == 1 && interval.contains(Interval::Bind)) {
            if (isBinding(i)) {
                auto ps = actor.patterns();
                for (auto &p : ps) {
                    ElementWithChildren e = { p.method, p.x, p.y, getElements(p.id, done) };
                    ret.emplace_back(e);
                }
            }
        }
#if 0
        else if (actor.active()) {
            auto p = actor.currentPattern();
            ElementWithChildren e = { p.method, p.x, p.y, getElements(p.id, done) };
            ret.emplace_back(e);
        }
#else
        auto p = actor.currentPattern();
        ElementWithChildren e = { p.method, p.x, p.y, getElements(p.id, done) };
        ret.emplace_back(e);
#endif
    }
    return ret;
}

std::vector<RenderInfo> Seriko::getElements(int id, std::unordered_set<int> &done) {
    if (!surfaces_.contains(id)) {
        return {};
    }
    std::vector<RenderInfo> ret;
    auto &surface = surfaces_.at(id);
    done.emplace(id);
    // TODO background
    for (auto &[_, v] : surface.element) {
        ret.push_back(v);
    }
    std::vector<int> list;
    for (auto &[k, _] : surface.animation) {
        list.push_back(k);
    }
    std::sort(list.begin(), list.end());
    for (auto i : list) {
        auto &interval = surface.animation[i].interval;
        if (interval.size() == 1 && interval.contains(Interval::Bind)) {
            auto ps = surface.animation[i].pattern;
            for (auto &p : ps) {
                if (!done.contains(p.id)) {
                    ElementWithChildren e = { p.method, p.x, p.y, getElements(p.id, done) };
                    ret.emplace_back(e);
                }
            }
        }
    }
    return ret;
}

std::vector<CollisionInfo> Seriko::getCollision(int id) {
    if (!surfaces_.contains(id)) {
        return {};
    }
    if (current_id_ != id) {
        current_id_ = id;
        if (!surfaces_.contains(id)) {
            return {};
        }
        auto &surface = surfaces_.at(id);
        actors_.clear();
        for (auto &[k, v] : surface.animation) {
            Actor actor = {k, v, this};
            actors_.emplace(k, actor);
        }
        update(true);
    }
    else {
        update();
    }
    std::vector<CollisionInfo> ret;
    // TODO order
    // 当たり判定は定義順の逆順で走査するので判定式も逆
    auto comp = [](const Collision &a, const Collision &b) {
        return a.factor > b.factor;
    };
    auto &surface = surfaces_.at(id);
    // TODO background
    {
        CollisionInfo info = {0, 0, {}};
        for (auto &[_, v] : surface.collision) {
            info.list.push_back(v);
        }
        if (info.list.size() > 0) {
            std::sort(info.list.begin(), info.list.end(), comp);
            ret.push_back(info);
        }
    }
    std::vector<int> keys;
    for (auto &[k, _] : actors_) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    for (auto i : keys) {
        auto &actor = actors_.at(i);
        auto p = actor.currentPattern();
        int id = p.id;
        if (!surfaces_.contains(id)) {
            continue;
        }
        auto &s = surfaces_.at(id);
        CollisionInfo info = {p.x, p.y, {}};
        for (auto &[_, v] : s.collision) {
            info.list.push_back(v);
        }
        if (info.list.size() > 0) {
            std::sort(info.list.begin(), info.list.end(), comp);
            ret.push_back(info);
        }
    }
    // ここも逆順にする
    std::reverse(ret.begin(), ret.end());
    return ret;
}


void Seriko::bind(int id, bool enable) {
    if (!actors_.contains(id)) {
        return;
    }
    binds_[id] = enable;
    if (enable) {
        actors_.at(id).activate(From::System);
    }
    else {
        actors_.at(id).inactivate();
    }
    auto addids = parent_->getBindAddId(id);
    for (auto e : addids) {
        if (enable) {
            bind_addids_[e].emplace(id);
            bind(e, true);
        }
        else if (bind_addids_.contains(id)) {
            bind_addids_.at(e).erase(id);
            if (bind_addids_.at(e).size() == 0) {
                bind_addids_.erase(e);
                bind(e, false);
            }
        }
    }
}

bool Seriko::isBinding(int id) {
    if (bind_addids_.contains(id)) {
        return true;
    }
    return binds_[id];
}

void Seriko::updateBind() {
    for (auto &[k, _] : actors_) {
        if (!binds_.contains(k)) {
            binds_[k] = parent_->isBinding(k);
            bind(k, binds_.at(k));
        }
    }
}
