/******************************************************************************

    GPLv3 License
    Copyright (c) 2023 Aria Janke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#include "../../test-helpers.hpp"

#include "../../../src/map-director/map-loader-task/StateMachineDriver.hpp"

class Animal {
public:
    virtual ~Animal() {}

    virtual const char * speak() const = 0;
};

template <typename T>
class InstCounted {
public:
    InstCounted() { ++s_inst_count; }

    InstCounted(const InstCounted &) { ++s_inst_count; }

    InstCounted(InstCounted &&) { ++s_inst_count; }

    ~InstCounted() { --s_inst_count; }

    static int instance_count()
        { return s_inst_count; }

    static void reset_instance_count()
        { s_inst_count = 0; }

private:
    static int s_inst_count;
};

template <typename T>
int InstCounted<T>::s_inst_count = 0;

class Cat final : public Animal, public InstCounted<Cat> {
public:
    const char * speak() const final { return "nya"; }
};

class Dog final : public Animal, public InstCounted<Dog> {
public:
    static constexpr const char * k_dog_speach = "browf browf";

    const char * speak() const final { return k_dog_speach; }
};

class Elephant final : public Animal, public InstCounted<Elephant> {
public:
    static constexpr const char * k_elephant_speach = "*blows trunk*";

    Elephant() {}

    Elephant(const Elephant &) = delete;

    Elephant(Elephant && elephant):
        InstCounted<Elephant>(elephant) {}

    const char * speak() const final { return k_elephant_speach; }
};

class Sponge final : public Animal, public InstCounted<Sponge> {
public:
    static constexpr const char * k_sponge_speach = "*spongy silence*";

    Sponge() {}

    Sponge(const Sponge & sponge):
        InstCounted<Sponge>(sponge) {}

    Sponge(Sponge &&) = delete;

    const char * speak() const final { return k_sponge_speach; }
};

template <typename HeadType, typename ... Types>
void reset_all_instance_counts_of() {
    InstCounted<HeadType>::reset_instance_count();
    if constexpr (sizeof...(Types) > 0)
        reset_all_instance_counts_of<Types...>();
}

void reset_all_instance_counts() {
    reset_all_instance_counts_of<Cat, Dog, Elephant, Sponge>();
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
using TestStateDriver = StateMachineDriver<Animal, Cat, Dog, Elephant, Sponge>;

// need to clean up test output (there are too many now o___o)
describe<TestStateDriver>("StateMachineDriver move semantics")([] {
    mark_it("Stateless state machine is movable", [] {
        TestStateDriver driver;
        TestStateDriver other;
        other.set_current_state<Elephant>();
        other = std::move(driver);
        return test_that(InstCounted<Elephant>::instance_count() == 0);
    });
});

describe<TestStateDriver>("StateMachineDriver copy semantics")([] {
    mark_it("Stateless state machine is copyable", [] {
        TestStateDriver driver;
        TestStateDriver other;
        other.set_current_state<Elephant>();
        other = driver;
        return test_that(InstCounted<Elephant>::instance_count() == 0);
    });
});

describe<TestStateDriver>("StateMachineDriver::set_current_state")([] {
    mark_it("creates a new instance of the state", [] {
        Cat::reset_instance_count();
        TestStateDriver driver;
        driver.set_current_state<Cat>();
        return test_that(Cat::instance_count() == 1);
    }).
    mark_it("correctly cleans up instances", [] {
        Cat::reset_instance_count();
        {
        TestStateDriver driver;
        driver.set_current_state<Cat>();
        }
        return test_that(Cat::instance_count() == 0);
    }).
    mark_it("correctly cleans up previously set state", [] {
        reset_all_instance_counts();
        TestStateDriver driver;
        driver.set_current_state<Cat>();
        driver.set_current_state<Dog>();
        return test_that(Cat::instance_count() == 0);
    });
});

describe<RestrictedStateSwitcherComplete<Animal, Cat>>
    ("RestrictedStateSwitcherComplete::set_next_state")([]
{
    mark_it("destructs the next state if unused", [] {
        Cat::reset_instance_count();
        {
        TestStateDriver driver;
        driver.state_switcher().set_next_state<Cat>();
        }
        return test_that(Cat::instance_count() == 0);
    }).
    mark_it("correctly cleans up previously set state", [] {
        reset_all_instance_counts();
        TestStateDriver driver;
        auto switcher = driver.state_switcher();
        switcher.set_next_state<Cat>();
        switcher.set_next_state<Dog>();
        return test_that(Cat::instance_count() == 0);
    }).
    mark_it("correctly cleans up previously set state, "
            "with different switcher instances", []
    {
        reset_all_instance_counts();
        TestStateDriver driver;
        driver.state_switcher().set_next_state<Cat>();
        driver.state_switcher().set_next_state<Dog>();
        return test_that(Cat::instance_count() == 0);
    });
});

describe<TestStateDriver>("TestStateDriver::advance")([] {
    mark_it("cleans up the old state", [] {
        reset_all_instance_counts();
        TestStateDriver driver;
        driver.set_current_state<Cat>();
        driver.state_switcher().set_next_state<Dog>();
        driver.advance();
        return test_that(Cat::instance_count() == 0);
    }).
    mark_it("makes next state the current state", [] {
        reset_all_instance_counts();
        TestStateDriver driver;
        driver.set_current_state<Cat>();
        driver.state_switcher().set_next_state<Dog>();
        driver.advance();
        return test_that(driver->speak() == std::string{Dog::k_dog_speach});
    });
});

return [] {};

} ();
