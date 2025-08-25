#ifndef ACTOR_H_
#define ACTOR_H_

#include "seriko.h"
#include "surface.h"

class Seriko;

class Actor {
    private:
        int id_;
        Animation anim_;
        Pattern pattern_;
        int index_;
        int wait_;
        bool active_;
        bool loop0_;
        Seriko *parent_;
    public:
        Actor(const int id, const Animation &anim, Seriko *parent);
        ~Actor() {}
        void activate(From from);
        bool active() const {
            return active_;
        }
        void inactivate() {
            active_ = false;
            pattern_ = {Method::Overlay, -1, 0, 0, 0, 0, {}};
        }
        const Pattern &currentPattern() const;
        void update(int elapsed);
        const std::unordered_set<Interval> &interval() const {
            return anim_.interval;
        }
        const std::vector<Pattern> &patterns() const {
            return anim_.pattern;
        }
};

#endif // ACTOR_H_
