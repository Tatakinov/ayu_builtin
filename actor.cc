#include "actor.h"

#include <cmath>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "logger.h"
#include "util.h"

namespace {
    const std::unordered_set<Interval> from_system = {
        Interval::Sometimes,
        Interval::Rarely,
        Interval::Random,
        Interval::Periodic,
        Interval::Always,
        Interval::Runonce,
    };
    const std::unordered_set<Interval> from_user = {
        Interval::Sometimes,
        Interval::Rarely,
        Interval::Random,
        Interval::Periodic,
        Interval::Always,
        Interval::Runonce,
        Interval::Never,
        Interval::YenE,
        Interval::Talk,
    };
    const std::unordered_set<Interval> from_yen_e = {
        Interval::YenE,
    };
    const std::unordered_set<Interval> from_talk = {
        Interval::Talk,
    };

    const std::unordered_set<Method> synthesis = {
        Method::Base,
        Method::Overlay,
        Method::OverlayFast,
        Method::OverlayMultiply,
        Method::Replace,
        Method::Interpolate,
        Method::Asis,
        Method::Bind,
        Method::Add,
        Method::Reduce,
    };
    int wait(int a, int b) {
        if (a >= b) {
            return a;
        }
        return util::random(a, b);
    }
}

Actor::Actor(const int id, const Animation &anim, Seriko *parent)
    : id_(id), anim_(anim), parent_(parent) {
    int total = 0;
    for (auto &p : anim_.pattern) {
        total += p.wait_max;
    }
    loop0_ = (anim_.interval.contains(Interval::Always) && total == 0);
    inactivate();
}

void Actor::activate(From from) {
    if (anim_.pattern.size() == 0) {
        Logger::log("0-sized pattern");
        return;
    }
    if (anim_.interval.contains(Interval::Bind) && !parent_->isBinding(id_)) {
        return;
    }
    bool start = false;
    switch (from) {
        case From::System:
            for (auto e : anim_.interval) {
                start = from_system.contains(e);
                if (start) {
                    break;
                }
            }
            break;
        case From::Seriko:
        case From::User:
            for (auto e : anim_.interval) {
                start = from_user.contains(e);
                if (start) {
                    break;
                }
            }
            break;
        case From::YenE:
            start = anim_.interval.contains(Interval::YenE);
            break;
        case From::Talk:
            start = anim_.interval.contains(Interval::Talk);
            break;
    }
    if (!start) {
        return;
    }
    inactivate();
    active_ = true;
    index_ = 0;
    auto &p = anim_.pattern[index_];
    wait_ = wait(p.wait_min, p.wait_max);
}

Pattern Actor::currentPattern() const {
    return pattern_;
}

void Actor::update(int elapsed, std::unordered_set<int> done) {
    if (!active_) {
        return;
    }
    if (wait_ > elapsed) {
        wait_ -= elapsed;
        return;
    }
    else {
        elapsed -= wait_;
        if (synthesis.contains(anim_.pattern[index_].method)) {
            pattern_ = anim_.pattern[index_];
        }
        else {
            auto &p = anim_.pattern[index_];
            switch (p.method) {
                case Method::Move:
                    // TODO stub
                    break;
                case Method::Insert:
                    // TODO stub
                    break;
                case Method::Start:
                    assert(p.ids.size() == 1);
                    parent_->activate(From::Seriko, p.ids[0], elapsed);
                    break;
                case Method::Stop:
                    assert(p.ids.size() == 1);
                    parent_->inactivate(p.ids[0]);
                    break;
                case Method::AlternativeStart:
                    parent_->activate(From::Seriko, util::random(0, p.ids.size()), elapsed);
                    break;
                case Method::AlternativeStop:
                    parent_->inactivate(util::random(0, p.ids.size()));
                    break;
                case Method::ParallelStart:
                    for (auto id : p.ids) {
                        parent_->activate(From::Seriko, id, elapsed);
                    }
                    break;
                case Method::ParallelStop:
                    for (auto id : p.ids) {
                        parent_->inactivate(id);
                    }
                    break;
                default:
                    break;
            }
        }
        index_++;
        if (index_ == anim_.pattern.size()) {
            double x;
            do {
                x = util::random();
            } while (x == 0);
            if (anim_.interval.contains(Interval::Always)) {
                index_ = 0;
                auto &p = anim_.pattern[index_];
                wait_ = wait(p.wait_min, p.wait_max);
            }
            else if (anim_.interval.contains(Interval::Sometimes)) {
                index_ = 0;
                wait_ = std::ceil(-log(2) / log(x)) * 1000;
            }
            else if (anim_.interval.contains(Interval::Rarely)) {
                index_ = 0;
                wait_ = std::ceil(-log(4) / log(x)) * 1000;
            }
            else if (anim_.interval.contains(Interval::Random)) {
                index_ = 0;
                wait_ = std::ceil(-log(anim_.interval_factor) / log(x)) * 1000;
            }
            else if (anim_.interval.contains(Interval::Periodic)) {
                index_ = 0;
                wait_ = anim_.interval_factor * 1000;
            }
            else {
                if (!anim_.interval.contains(Interval::Bind)) {
                    //inactivate();
                    active_ = false;
                }
                return;
            }
        }
        else {
            auto &p = anim_.pattern[index_];
            wait_ = wait(p.wait_min, p.wait_max);
        }
        if (loop0_) {
            elapsed--;
            if (elapsed < 0) {
                return;
            }
        }
        parent_->push(id_, elapsed);
    }
}
