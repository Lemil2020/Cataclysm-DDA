#ifndef CATA_TESTS_ACTIVITY_SCHEDULING_H
#define CATA_TESTS_ACTIVITY_SCHEDULING_H

#include "activity_actor.h"
#include "avatar.h"
#include "clone_ptr.h"

struct schedule {
    virtual void setup( avatar & ) const = 0;
    virtual void do_turn( avatar & ) const = 0;
    virtual bool instantaneous() const {
        return false;
    }
    time_duration interval = 5_minutes;

    virtual ~schedule() = default;
};

class activity_schedule : public schedule
{
        activity_id act;
        cata::clone_ptr<activity_actor> actor;

    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar &guy ) const override;

        activity_schedule( const activity_id &id, const time_duration ticks ) : act( id ) {
            interval = ticks;
        }
        activity_schedule( const activity_actor &assigned,
                           const time_duration ticks ) : actor( assigned.clone() ) {
            interval = ticks;
        }
};

class meal_schedule : public schedule
{
        itype_id food;

    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar & ) const override;
        bool instantaneous() const override;

        meal_schedule( const itype_id &eaten ) : food( eaten ) {};
};

class clear_guts : public schedule
{
    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar & ) const override;
        bool instantaneous() const override;
};

const clear_guts sched_clear_guts{};

class sleep_schedule : public schedule
{
    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar &guy ) const override;
};

class tasklist
{
        // The tasks we have yet to do, and how long
        // The first member is a pointer because we need to preserve the virtual
        // class that it's pointing to
        // It will _never_ be nullptr
        std::vector<std::pair<const schedule *, time_duration>> tasks;
        // How long we've been on the current task
        time_duration advanced = 0_seconds;
        // The current task's index
        size_t cursor = 0;

    public:
        const schedule &next_task() {
            // Uh oh! We ran out of tasks!
            if( cursor >= tasks.size() ) {
                debugmsg( "Requested task when none existed!" );
                if( tasks.empty() ) {
                    abort();
                }
                return *tasks[0].first;
            }
            return *tasks[cursor].first;
        }

        void advance( const time_duration how_long ) {
            advanced += how_long;
            if( advanced > tasks[cursor].second ) {
                advanced = 0_seconds;
                ++cursor;
            }
            // It's valid for us to finish our task and run out of them, putting
            // the cursor out of bounds. But it's definitely not valid to go further
            // (If we somehow made it here without complaints)
            if( cursor > tasks.size() ) {
                debugmsg( "Attempted to continue advancing once all tasks were finished!" );
                cursor = 0;
            }
        }

        void enschedule( const schedule &added, const time_duration how_long ) {
            tasks.insert( tasks.end(), { &added, how_long } );
        }

        void clear() {
            cursor = 0;
            advanced = 0_seconds;
            tasks.clear();
        }

        time_duration duration() {
            time_duration ret;
            for( const std::pair<const schedule *, time_duration> &cursor : tasks ) {
                ret += cursor.second;
            }
            return ret;
        }
};

struct weariness_events {

        struct weary_transition {
            int minutes = 0;
            int from = 0;
            int to = 0;
        };

        std::vector<weary_transition> transitions;

    public:
        void log( const int old_level, const int new_level, const time_duration when ) {
            weary_transition added;
            added.from = old_level;
            added.to = new_level;
            added.minutes = to_minutes<int>( when );

            transitions.insert( transitions.end(), added );
        }

        // Return the first time a transition between `from` and `to` occurs, in minutes
        // if around = 0_seconds or equivalent, otherwise return the time closest to around
        int transition_minutes( const int from, const int to,
                                const time_duration around ) const {
            int around_mins = to_minutes<int>( around );
            // first = change.minutes, second = difference from around_mins
            std::pair<int, int> ret = {INT_MAX, INT_MAX};
            for( const weary_transition &change : transitions ) {
                int diff = std::abs( change.minutes - around_mins );
                if( change.from == from && change.to == to && ret.second > diff ) {
                    ret = { change.minutes, diff };
                }
            }
            return ret.first;
        }

        std::string summarize() const {
            std::string buffer;
            for( const weary_transition &change : transitions ) {
                buffer += string_format( "Transition: Weariness from %d to %d at %d minutes\n", change.from,
                                         change.to, change.minutes );
            }
            return buffer;
        }

        bool empty() const {
            return transitions.empty();
        }
};

weariness_events do_activity( tasklist tasks );

#endif // CATA_TESTS_ACTIVITY_SCHEDULING_H
