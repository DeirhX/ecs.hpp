/*******************************************************************************
 * This file is part of the "https://github.com/blackmatov/ecs.hpp"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"

#include "ecs.hpp"
namespace ecs = ecs_hpp;

namespace
{
    struct position_c {
        int x{0};
        int y{0};

        position_c() = default;
        position_c(int nx, int ny) : x(nx), y(ny) {}
    };

    struct velocity_c {
        int x{0};
        int y{0};

        velocity_c() = default;
        velocity_c(int nx, int ny) : x(nx), y(ny) {}
    };

    bool operator==(const position_c& l, const position_c& r) noexcept {
        return l.x == r.x
            && l.y == r.y;
    }

    bool operator==(const velocity_c& l, const velocity_c& r) noexcept {
        return l.x == r.x
            && l.y == r.y;
    }

    struct mult_indexer {
        template < typename T >
        std::size_t operator()(const T& v) const noexcept {
            return static_cast<std::size_t>(v * 2);
        }
    };

    struct position_c_indexer {
        std::size_t operator()(const position_c& v) const noexcept {
            return static_cast<std::size_t>(v.x);
        }
    };
}

TEST_CASE("detail") {
    SECTION("get_type_id") {
        using namespace ecs::detail;
        REQUIRE(type_family<position_c>::id() == 1u);
        REQUIRE(type_family<position_c>::id() == 1u);

        REQUIRE(type_family<velocity_c>::id() == 2u);
        REQUIRE(type_family<velocity_c>::id() == 2u);

        REQUIRE(type_family<position_c>::id() == 1u);
        REQUIRE(type_family<velocity_c>::id() == 2u);
    }
    SECTION("tuple_tail") {
        using namespace ecs::detail;
        {
            REQUIRE(tuple_tail(std::make_tuple(1, 2, 3)) == std::make_tuple(2, 3));
            REQUIRE(tuple_tail(std::make_tuple(2, 3)) == std::make_tuple(3));
            REQUIRE(tuple_tail(std::make_tuple(3)) == std::make_tuple());
        }
        {
            const auto t1 = std::make_tuple(1);
            const auto t2 = std::make_tuple(1, 2);
            const auto t3 = std::make_tuple(1, 2, 3);
            REQUIRE(tuple_tail(t1) == std::make_tuple());
            REQUIRE(tuple_tail(t2) == std::make_tuple(2));
            REQUIRE(tuple_tail(t3) == std::make_tuple(2, 3));
        }
    }
    SECTION("tuple_contains") {
        using namespace ecs::detail;
        {
            REQUIRE_FALSE(tuple_contains(std::make_tuple(), nullptr));
            REQUIRE_FALSE(tuple_contains(std::make_tuple(1), 0));
            REQUIRE_FALSE(tuple_contains(std::make_tuple(1), 2));
            REQUIRE(tuple_contains(std::make_tuple(1), 1));
            REQUIRE(tuple_contains(std::make_tuple(1,2,3), 1));
            REQUIRE(tuple_contains(std::make_tuple(1,2,3), 2));
            REQUIRE(tuple_contains(std::make_tuple(1,2,3), 3));
            REQUIRE_FALSE(tuple_contains(std::make_tuple(1,2,3), 0));
            REQUIRE_FALSE(tuple_contains(std::make_tuple(1,2,3), 4));
        }
    }
    SECTION("entity_id") {
        using namespace ecs::detail;
        {
            REQUIRE(entity_id_index(entity_id_join(10u, 20u)) == 10u);
            REQUIRE(entity_id_version(entity_id_join(10u, 20u)) == 20u);
            REQUIRE(upgrade_entity_id(entity_id_join(10u, 20u)) == entity_id_join(10u, 21u));
            REQUIRE(upgrade_entity_id(entity_id_join(0u, 1023u)) == entity_id_join(0u, 0u));
            REQUIRE(upgrade_entity_id(entity_id_join(1u, 1023u)) == entity_id_join(1u, 0u));
            REQUIRE(upgrade_entity_id(entity_id_join(2048u, 1023u)) == entity_id_join(2048u, 0u));
        }
    }
    SECTION("sparse_set") {
        using namespace ecs::detail;
        {
            sparse_set<unsigned, mult_indexer> s{mult_indexer{}};

            REQUIRE(s.empty());
            REQUIRE_FALSE(s.size());
            REQUIRE_FALSE(s.has(42u));
            REQUIRE(s.find(42u) == s.end());
            REQUIRE_FALSE(s.find_dense_index(42u).second);
            REQUIRE_THROWS(s.get_dense_index(42u));

            REQUIRE(s.insert(42u));

            REQUIRE_FALSE(s.empty());
            REQUIRE(s.size() == 1u);
            REQUIRE(s.has(42u));
            REQUIRE_FALSE(s.has(84u));

            REQUIRE(s.find(42u) == s.begin());
            REQUIRE(s.find_dense_index(42u).second);
            REQUIRE(s.find_dense_index(42u).first == 0u);
            REQUIRE(s.get_dense_index(42u) == 0u);

            s.clear();

            REQUIRE(s.empty());
            REQUIRE_FALSE(s.size());
            REQUIRE_FALSE(s.has(42u));

            REQUIRE(s.insert(84u));
            REQUIRE_FALSE(s.insert(84u));

            REQUIRE(s.has(84u));
            REQUIRE_FALSE(s.unordered_erase(42u));
            REQUIRE(s.unordered_erase(84u));
            REQUIRE_FALSE(s.has(84u));
            REQUIRE(s.empty());
            REQUIRE_FALSE(s.size());

            s.insert(42u);
            s.insert(84u);

            REQUIRE(s.has(42u));
            REQUIRE(s.has(84u));
            REQUIRE(s.size() == 2u);
            REQUIRE(s.find_dense_index(42u).second);
            REQUIRE(s.find_dense_index(42u).first == 0u);
            REQUIRE(s.find_dense_index(84u).second);
            REQUIRE(s.find_dense_index(84u).first == 1u);
            REQUIRE(s.get_dense_index(42u) == 0u);
            REQUIRE(s.get_dense_index(84u) == 1u);

            REQUIRE(s.unordered_erase(42u));

            REQUIRE_FALSE(s.has(42u));
            REQUIRE(s.has(84u));
            REQUIRE(s.size() == 1u);
            REQUIRE(s.find_dense_index(84u).second);
            REQUIRE(s.find_dense_index(84u).first == 0u);
            REQUIRE_THROWS(s.get_dense_index(42u));
            REQUIRE(s.get_dense_index(84u) == 0u);
        }
        {
            sparse_set<position_c, position_c_indexer> s{position_c_indexer()};
            REQUIRE(s.insert(position_c(1,2)));
            REQUIRE_FALSE(s.insert(position_c(1,2)));
            REQUIRE(s.has(position_c(1,2)));
            REQUIRE(s.emplace(3,4));
            REQUIRE(s.has(position_c(3,4)));
            REQUIRE(s.get_dense_index(position_c(1,2)) == 0);
            REQUIRE(s.get_dense_index(position_c(3,4)) == 1);
            REQUIRE(s.find_dense_index(position_c(1,2)).first == 0);
            REQUIRE(s.find_dense_index(position_c(3,4)).first == 1);
            REQUIRE(s.find_dense_index(position_c(1,2)).second);
            REQUIRE(s.find_dense_index(position_c(3,4)).second);
            REQUIRE(s.unordered_erase(position_c(1,2)));
            REQUIRE(s.get_dense_index(position_c(3,4)) == 0);
        }
    }
    SECTION("sparse_map") {
        using namespace ecs::detail;
        {
            struct obj_t {
                int x;
                obj_t(int nx) : x(nx) {}
            };

            sparse_map<unsigned, obj_t> m;

            REQUIRE(m.empty());
            REQUIRE_FALSE(m.size());
            REQUIRE_FALSE(m.has(42u));
            REQUIRE_THROWS(m.get(42u));
            REQUIRE_THROWS(as_const(m).get(42u));
            REQUIRE_FALSE(m.find(42u));
            REQUIRE_FALSE(as_const(m).find(42u));

            {
                obj_t o{21u};
                REQUIRE(m.insert(21u, o));
                REQUIRE(m.insert(42u, obj_t{42u}));
                REQUIRE(m.emplace(84u, 84u));
            }

            {
                obj_t o{21u};
                REQUIRE_FALSE(m.insert(21u, o));
                REQUIRE_FALSE(m.insert(42u, obj_t{42u}));
                REQUIRE_FALSE(m.emplace(84u, 84u));
            }

            REQUIRE_FALSE(m.empty());
            REQUIRE(m.size() == 3u);
            REQUIRE(m.has(21u));
            REQUIRE(m.has(42u));
            REQUIRE(m.has(84u));
            REQUIRE_FALSE(m.has(11u));
            REQUIRE_FALSE(m.has(25u));
            REQUIRE_FALSE(m.has(99u));

            REQUIRE(m.get(21u).x == 21u);
            REQUIRE(m.get(42u).x == 42u);
            REQUIRE(m.get(84u).x == 84u);
            REQUIRE(as_const(m).get(84u).x == 84u);
            REQUIRE_THROWS(m.get(11u));
            REQUIRE_THROWS(m.get(25u));
            REQUIRE_THROWS(m.get(99u));
            REQUIRE_THROWS(as_const(m).get(99u));

            REQUIRE(m.find(21u)->x == 21u);
            REQUIRE(m.find(42u)->x == 42u);
            REQUIRE(m.find(84u)->x == 84u);
            REQUIRE(as_const(m).find(84u)->x == 84u);
            REQUIRE_FALSE(m.find(11u));
            REQUIRE_FALSE(m.find(25u));
            REQUIRE_FALSE(m.find(99u));
            REQUIRE_FALSE(as_const(m).find(99u));

            REQUIRE(m.unordered_erase(42u));
            REQUIRE_FALSE(m.unordered_erase(42u));

            REQUIRE(m.has(21u));
            REQUIRE_FALSE(m.has(42u));
            REQUIRE(m.has(84u));
            REQUIRE(m.size() == 2u);

            m.clear();
            REQUIRE(m.empty());
            REQUIRE_FALSE(m.size());
            REQUIRE_FALSE(m.has(21u));
            REQUIRE_FALSE(m.has(42u));
            REQUIRE_FALSE(m.has(84u));
        }
        {
            struct obj_t {
                int x;
                obj_t(int nx) : x(nx) {}
            };

            sparse_map<position_c, obj_t, position_c_indexer> s{position_c_indexer()};
            REQUIRE(s.insert(position_c(1,2), obj_t{1}));
            REQUIRE_FALSE(s.insert(position_c(1,2), obj_t{1}));
            REQUIRE(s.has(position_c(1,2)));
            REQUIRE(s.emplace(position_c(3,4), obj_t{3}));
            REQUIRE(s.has(position_c(3,4)));
            REQUIRE(s.get(position_c(1,2)).x == 1);
            REQUIRE(s.get(position_c(3,4)).x == 3);
            REQUIRE(s.find(position_c(1,2))->x == 1);
            REQUIRE(s.find(position_c(3,4))->x == 3);
            REQUIRE(s.find(position_c(1,2)));
            REQUIRE(s.find(position_c(3,4)));
            REQUIRE(s.unordered_erase(position_c(1,2)));
            REQUIRE(s.get(position_c(3,4)).x == 3);
        }
    }
}

TEST_CASE("registry") {
    SECTION("entities") {
        {
            ecs::registry w;
            ecs::entity e1{w};
            ecs::entity e2 = e1;
            e2 = e1;

            ecs::const_entity ce1{w};
            ecs::const_entity ce2 = ce1;
            ce2 = ce1;

            ecs::const_entity ce3 = e1;
            ce3 = e2;
        }
        {
            ecs::registry w;

            ecs::entity e1{w};
            ecs::entity e2{w};
            ecs::const_entity e3{w};

            REQUIRE(e1 == e2);
            REQUIRE(e2 == e3);

            REQUIRE_FALSE(e1 != e2);
            REQUIRE_FALSE(e2 != e3);

            REQUIRE_FALSE(w.alive_entity(e1));
            REQUIRE_FALSE(w.alive_entity(e2));
            REQUIRE_FALSE(w.alive_entity(e3));

            REQUIRE_FALSE(w.destroy_entity(e1));
            REQUIRE_FALSE(w.destroy_entity(e2));
        }
        {
            ecs::registry w;

            ecs::entity e1 = w.create_entity();
            ecs::entity e2 = w.create_entity();
            ecs::const_entity e3 = w.create_entity();
            ecs::entity ee3 = w.wrap_entity(e3);

            REQUIRE(e1 != e2);
            REQUIRE(e2 != e3);
            REQUIRE_FALSE(e3 != ee3);

            REQUIRE_FALSE(e1 == e2);
            REQUIRE_FALSE(e2 == e3);
            REQUIRE(e3 == ee3);

            REQUIRE(w.alive_entity(e1));
            REQUIRE(w.alive_entity(e2));
            REQUIRE(w.alive_entity(e3));
            REQUIRE(w.alive_entity(ee3));

            REQUIRE(w.destroy_entity(e1));
            REQUIRE_FALSE(w.alive_entity(e1));
            REQUIRE(w.alive_entity(e2));

            REQUIRE(w.destroy_entity(e2));
            REQUIRE_FALSE(w.alive_entity(e1));
            REQUIRE_FALSE(w.alive_entity(e2));

            REQUIRE(w.destroy_entity(ee3));
            REQUIRE_FALSE(w.alive_entity(e3));
            REQUIRE_FALSE(w.alive_entity(ee3));

            REQUIRE_FALSE(w.destroy_entity(e1));
            REQUIRE_FALSE(w.destroy_entity(e2));
            REQUIRE_FALSE(w.destroy_entity(ee3));
        }
        {
            ecs::registry w;
            using namespace ecs::detail;

            const auto e1 = w.create_entity();

            w.destroy_entity(e1);
            const auto e2 = w.create_entity();
            REQUIRE(e1 != e2);
            REQUIRE(entity_id_index(e1.id()) == entity_id_index(e2.id()));
            REQUIRE(entity_id_version(e1.id()) + 1 == entity_id_version(e2.id()));

            w.destroy_entity(e2);
            const auto e3 = w.create_entity();
            REQUIRE(e3 != e2);
            REQUIRE(entity_id_index(e2.id()) == entity_id_index(e3.id()));
            REQUIRE(entity_id_version(e2.id()) + 1 == entity_id_version(e3.id()));
        }
        {
            ecs::registry w;
            using namespace ecs::detail;

            auto e = w.create_entity();
            const auto e_id = e.id();
            for ( std::size_t i = 0; i < entity_id_version_mask; ++i ) {
                e.destroy();
                e = w.create_entity();
                REQUIRE(entity_id_version(e_id) != entity_id_version(e.id()));
            }
            // entity version wraps around
            e.destroy();
            e = w.create_entity();
            REQUIRE(entity_id_version(e_id) == entity_id_version(e.id()));
        }
        {
            ecs::registry w;
            using namespace ecs::detail;

            for ( std::size_t i = 0; i < entity_id_index_mask; ++i ) {
                w.create_entity();
            }
            // entity index overflow
            REQUIRE_THROWS_AS(w.create_entity(), std::logic_error);
        }
    }
    SECTION("components") {
        {
            ecs::registry w;
            ecs::entity e1 = w.create_entity();
            ecs::entity e2 = w.create_entity();

            {
                REQUIRE(w.wrap_component<position_c>(e1) == ecs::component<position_c>(e1));
                REQUIRE_FALSE(w.wrap_component<position_c>(e1) == ecs::component<position_c>(e2));
            }
            {
                const ecs::registry& ww = w;
                REQUIRE(ww.wrap_component<position_c>(e1) == ecs::component<position_c>(e1));
                REQUIRE_FALSE(ww.wrap_component<position_c>(e1) == ecs::component<position_c>(e2));
            }

            {
                ecs::component<position_c> c1{e1};
                REQUIRE_FALSE(c1.exists());
            }
        }
        {
            ecs::registry w;
            ecs::const_entity e1 = w.create_entity();
            ecs::const_entity e2 = w.create_entity();

            {
                REQUIRE(w.wrap_component<position_c>(e1) == ecs::const_component<position_c>(e1));
                REQUIRE_FALSE(w.wrap_component<position_c>(e1) == ecs::const_component<position_c>(e2));
            }
        }
        {
            ecs::registry w;
            ecs::entity e1 = w.create_entity();

            ecs::component<position_c> c1 = w.wrap_component<position_c>(e1);
            ecs::const_component<position_c> c2 = w.wrap_component<position_c>(e1);
            REQUIRE(c1 == c2);
            REQUIRE_FALSE(c1 != c2);

            REQUIRE(c1.owner() == e1);
            REQUIRE(c2.owner() == e1);

            REQUIRE_FALSE(c1.exists());
            REQUIRE_FALSE(c2.exists());
            REQUIRE_FALSE(c1.find());
            REQUIRE_FALSE(c2.find());
            REQUIRE_THROWS_AS(c1.get(), std::logic_error);
            REQUIRE_THROWS_AS(c2.get(), std::logic_error);

            REQUIRE(c1.assign(4,2));

            REQUIRE(c1.exists());
            REQUIRE(c2.exists());
            REQUIRE(c1.find()->x == 4);
            REQUIRE(c1.find()->y == 2);
            REQUIRE(c2.find()->x == 4);
            REQUIRE(c2.find()->y == 2);
            REQUIRE(c1.get().x == 4);
            REQUIRE(c1.get().y == 2);
            REQUIRE(c2.get().x == 4);
            REQUIRE(c2.get().y == 2);

            REQUIRE(c1.assign(2,4));

            REQUIRE(c1.find()->x == 2);
            REQUIRE(c1.find()->y == 4);
            REQUIRE(c2.find()->x == 2);
            REQUIRE(c2.find()->y == 4);
            REQUIRE(c1.get().x == 2);
            REQUIRE(c1.get().y == 4);
            REQUIRE(c2.get().x == 2);
            REQUIRE(c2.get().y == 4);

            REQUIRE(c1.remove());

            REQUIRE_FALSE(c1.exists());
            REQUIRE_FALSE(c2.exists());
            REQUIRE_FALSE(c1.find());
            REQUIRE_FALSE(c2.find());
            REQUIRE_THROWS_AS(c1.get(), std::logic_error);
            REQUIRE_THROWS_AS(c2.get(), std::logic_error);

            REQUIRE_FALSE(c1.remove());
        }
    }
    SECTION("component_assigning") {
        {
            ecs::registry w;
            ecs::entity e1 = w.create_entity();

            {
                REQUIRE_FALSE(w.exists_component<position_c>(e1));
                REQUIRE_FALSE(w.exists_component<velocity_c>(e1));
                REQUIRE_FALSE(w.component_count<position_c>());
                REQUIRE_FALSE(w.entity_component_count<position_c>(e1));

                REQUIRE(w.assign_component<position_c>(e1));

                REQUIRE(w.exists_component<position_c>(e1));
                REQUIRE_FALSE(w.exists_component<velocity_c>(e1));
                REQUIRE(w.component_count<position_c>() == 1u);
                REQUIRE(w.component_count<velocity_c>() == 0u);
                REQUIRE(w.entity_component_count<position_c>(e1) == 1u);

                REQUIRE(w.assign_component<velocity_c>(e1));
                REQUIRE(w.component_count<position_c>() == 1u);
                REQUIRE(w.component_count<velocity_c>() == 1u);
                REQUIRE(w.entity_component_count<position_c>(e1) == 2u);

                REQUIRE(w.exists_component<position_c>(e1));
                REQUIRE(w.exists_component<velocity_c>(e1));

                REQUIRE(w.remove_all_components(e1) == 2u);

                REQUIRE_FALSE(w.exists_component<position_c>(e1));
                REQUIRE_FALSE(w.exists_component<velocity_c>(e1));
                REQUIRE_FALSE(w.component_count<position_c>());
                REQUIRE_FALSE(w.component_count<velocity_c>());
                REQUIRE_FALSE(w.entity_component_count<position_c>(e1));
            }

            {
                REQUIRE_FALSE(e1.exists_component<position_c>());
                REQUIRE_FALSE(e1.exists_component<velocity_c>());

                REQUIRE(e1.assign_component<position_c>());

                REQUIRE(e1.exists_component<position_c>());
                REQUIRE_FALSE(e1.exists_component<velocity_c>());

                REQUIRE(e1.assign_component<velocity_c>());

                REQUIRE(e1.exists_component<position_c>());
                REQUIRE(e1.exists_component<velocity_c>());

                REQUIRE(e1.destroy());

                REQUIRE_FALSE(e1.exists_component<position_c>());
                REQUIRE_FALSE(e1.exists_component<velocity_c>());
                REQUIRE_FALSE(w.component_count<position_c>());
                REQUIRE_FALSE(w.component_count<velocity_c>());
                REQUIRE_FALSE(w.entity_component_count<position_c>(e1));
            }
        }
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            REQUIRE(w.assign_component<position_c>(e1));
            REQUIRE(w.assign_component<velocity_c>(e1));

            REQUIRE(w.assign_component<position_c>(e2));
            REQUIRE(w.assign_component<velocity_c>(e2));

            REQUIRE(w.destroy_entity(e1));

            REQUIRE_FALSE(w.exists_component<position_c>(e1));
            REQUIRE_FALSE(w.exists_component<velocity_c>(e1));

            REQUIRE(w.exists_component<position_c>(e2));
            REQUIRE(w.exists_component<velocity_c>(e2));
        }
        {
            ecs::registry w;
            auto e1 = w.create_entity();
            REQUIRE(e1.destroy());
            REQUIRE_FALSE(e1.assign_component<position_c>());
            REQUIRE_FALSE(w.exists_component<position_c>(e1));
        }
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            REQUIRE(w.assign_component<position_c>(e1));

            REQUIRE(w.assign_component<position_c>(e2));
            REQUIRE(w.assign_component<velocity_c>(e2));

            REQUIRE(e1.destroy());
        }
    }
    SECTION("component_accessing") {
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            REQUIRE_FALSE(e1.find_component<position_c>());
            REQUIRE_FALSE(e2.find_component<velocity_c>());

            e1.assign_component<position_c>(1, 2);
            e2.assign_component<velocity_c>(3, 4);

            REQUIRE(e1.get_component<position_c>().x == 1);
            REQUIRE(e1.get_component<position_c>().y == 2);

            REQUIRE(e2.get_component<velocity_c>().x == 3);
            REQUIRE(e2.get_component<velocity_c>().y == 4);

            REQUIRE_THROWS_AS(e1.get_component<velocity_c>(), std::logic_error);
            REQUIRE_THROWS_AS(e2.get_component<position_c>(), std::logic_error);
        }
        {
            ecs::registry w;

            const auto e1 = w.create_entity();
            const auto e2 = w.create_entity();

            REQUIRE_FALSE(e1.find_component<position_c>());
            REQUIRE_FALSE(e2.find_component<velocity_c>());

            w.assign_component<position_c>(e1, 1, 2);
            w.assign_component<velocity_c>(e2, 3, 4);

            REQUIRE(e1.find_component<position_c>()->y == 2);
            REQUIRE(e2.find_component<velocity_c>()->y == 4);

            {
                const ecs::registry& ww = w;
                REQUIRE(ww.get_component<position_c>(e1).x == 1);
                REQUIRE(ww.get_component<position_c>(e1).y == 2);

                REQUIRE(ww.get_component<velocity_c>(e2).x == 3);
                REQUIRE(ww.get_component<velocity_c>(e2).y == 4);

                REQUIRE_THROWS_AS(ww.get_component<velocity_c>(e1), std::logic_error);
                REQUIRE_THROWS_AS(ww.get_component<position_c>(e2), std::logic_error);
            }
        }
        {
            ecs::registry w;
            auto e1 = w.create_entity();
            e1.assign_component<position_c>(1, 2);
            e1.assign_component<position_c>(3, 4);
            REQUIRE(e1.get_component<position_c>().x == 3);
            REQUIRE(e1.get_component<position_c>().y == 4);
        }
        {
            ecs::registry w;

            auto e1 = w.create_entity();

            REQUIRE(e1.find_components<>() ==
                std::make_tuple());
            REQUIRE(e1.find_components<velocity_c>() ==
                std::make_tuple<velocity_c*>(nullptr));
            REQUIRE(e1.find_components<position_c, velocity_c>() ==
                std::make_tuple<position_c*, velocity_c*>(nullptr, nullptr));

            REQUIRE(e1.get_components<>() == std::make_tuple());
            REQUIRE_THROWS(e1.get_components<velocity_c>());
            REQUIRE_THROWS(e1.get_components<position_c, velocity_c>());

            {
                const auto ee1 = e1;

                REQUIRE(ee1.find_components<>() ==
                    std::make_tuple());
                REQUIRE(ee1.find_components<velocity_c>() ==
                    std::make_tuple<velocity_c*>(nullptr));
                REQUIRE(ee1.find_components<position_c, velocity_c>() ==
                    std::make_tuple<position_c*, velocity_c*>(nullptr, nullptr));

                REQUIRE(ee1.get_components<>() == std::make_tuple());
                REQUIRE_THROWS(ee1.get_components<velocity_c>());
                REQUIRE_THROWS(ee1.get_components<position_c, velocity_c>());
            }

            e1.assign_component<velocity_c>(3, 4);

            REQUIRE(e1.find_components<velocity_c>() ==
                std::make_tuple<velocity_c*>(e1.find_component<velocity_c>()));
            REQUIRE(e1.find_components<position_c, velocity_c>() ==
                std::make_tuple<position_c*, velocity_c*>(nullptr, e1.find_component<velocity_c>()));

            REQUIRE(e1.get_components<velocity_c>() ==
                std::make_tuple<velocity_c&>(e1.get_component<velocity_c>()));
            REQUIRE_THROWS(e1.get_components<position_c, velocity_c>());

            {
                const auto ee1 = e1;

                REQUIRE(ee1.find_components<velocity_c>() ==
                    std::make_tuple<velocity_c*>(e1.find_component<velocity_c>()));
                REQUIRE(ee1.find_components<position_c, velocity_c>() ==
                    std::make_tuple<position_c*, velocity_c*>(nullptr, e1.find_component<velocity_c>()));

                REQUIRE(ee1.get_components<velocity_c>() ==
                    std::make_tuple<const velocity_c&>(ee1.get_component<velocity_c>()));
                REQUIRE_THROWS(ee1.get_components<position_c, velocity_c>());
            }

            e1.assign_component<position_c>(1, 2);

            auto p = e1.get_components<position_c, velocity_c>();
            std::get<0>(p).x = 10;
            std::get<1>(p).x = 30;
            REQUIRE(e1.get_component<position_c>().x == 10);
            REQUIRE(e1.get_component<velocity_c>().x == 30);

            auto p2 = e1.find_components<position_c, velocity_c>();
            std::get<0>(p2)->y = 20;
            std::get<1>(p2)->y = 40;
            REQUIRE(e1.get_component<position_c>().y == 20);
            REQUIRE(e1.get_component<velocity_c>().y == 40);
        }
    }
    SECTION("for_each_entity") {
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            {
                ecs::entity_id acc1 = 0;
                w.for_each_entity([&acc1](const ecs::entity& e){
                    acc1 += e.id();
                });
                REQUIRE(acc1 == e1.id() + e2.id());
            }
            {
                const ecs::registry& ww = w;
                ecs::entity_id acc1 = 0;
                ww.for_each_entity([&acc1](const ecs::const_entity& e){
                    acc1 += e.id();
                });
                REQUIRE(acc1 == e1.id() + e2.id());
            }
        }
    }
    SECTION("for_each_component") {
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            e1.assign_component<position_c>(1, 2);
            e1.assign_component<velocity_c>(3, 4);
            e2.assign_component<position_c>(5, 6);
            e2.assign_component<velocity_c>(7, 8);

            {
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                w.for_each_component<position_c>([&acc1, &acc2](ecs::entity_id id, position_c& p){
                    acc1 += id;
                    acc2 += p.x;
                });
                REQUIRE(acc1 == e1.id() + e2.id());
                REQUIRE(acc2 == 6);
            }

            {
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                w.for_each_component<position_c>([&acc1, &acc2](ecs::entity e, position_c& p){
                    acc1 += e.id();
                    acc2 += p.x;
                });
                REQUIRE(acc1 == e1.id() + e2.id());
                REQUIRE(acc2 == 6);
            }

            {
                const ecs::registry& ww = w;
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                ww.for_each_component<position_c>([&acc1, &acc2](ecs::const_entity e, const position_c& p){
                    acc1 += e.id();
                    acc2 += p.x;
                });
                REQUIRE(acc1 == e1.id() + e2.id());
                REQUIRE(acc2 == 6);
            }
        }
        {
            ecs::registry w;

            {
                auto e1 = w.create_entity();
                auto e2 = w.create_entity();

                e1.destroy();
                e2.destroy();
            }

            auto e3 = w.create_entity();
            auto e4 = w.create_entity();

            e3.assign_component<position_c>(1, 2);
            e4.assign_component<position_c>(3, 4);

            {
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                w.for_each_component<position_c>([&acc1, &acc2](ecs::entity e, position_c& p){
                    acc1 += e.id();
                    acc2 += p.x;
                });
                REQUIRE(acc1 == e3.id() + e4.id());
                REQUIRE(acc2 == 4);
            }
        }
    }
    SECTION("for_joined_components") {
        {
            ecs::registry w;

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();
            auto e3 = w.create_entity();
            auto e4 = w.create_entity();
            w.create_entity();

            e1.assign_component<position_c>(1, 2);
            e1.assign_component<velocity_c>(3, 4);
            e2.assign_component<position_c>(5, 6);
            e2.assign_component<velocity_c>(7, 8);

            e3.assign_component<position_c>(100, 500);
            e4.assign_component<velocity_c>(500, 100);

            {
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                w.for_joined_components<position_c, velocity_c>([&acc1, &acc2](
                    ecs::entity_id id, const position_c& p, const velocity_c& v)
                {
                    acc1 += id;
                    acc2 += p.x + v.x;
                });
                REQUIRE(acc1 == e1.id() + e2.id());
                REQUIRE(acc2 == 16);
            }

            {
                ecs::entity_id acc1 = 0;
                int acc2 = 0;
                w.for_joined_components<position_c, velocity_c>([&acc1, &acc2](
                    ecs::entity e, const position_c& p, const velocity_c& v)
                {
                    acc1 += e.id();
                    acc2 += p.x + v.x;
                });
                REQUIRE(acc1 == e1.id() + e2.id());
                REQUIRE(acc2 == 16);
            }

            {
                const ecs::registry& ww = w;
                {
                    ecs::entity_id acc1 = 0;
                    int acc2 = 0;
                    ww.for_joined_components<position_c, velocity_c>([&acc1, &acc2](
                        ecs::entity_id id, const position_c& p, const velocity_c& v)
                    {
                        acc1 += id;
                        acc2 += p.x + v.x;
                    });
                    REQUIRE(acc1 == e1.id() + e2.id());
                    REQUIRE(acc2 == 16);
                }
                {
                    ecs::entity_id acc1 = 0;
                    int acc2 = 0;
                    ww.for_joined_components<position_c, velocity_c>([&acc1, &acc2](
                        ecs::const_entity e, const position_c& p, const velocity_c& v)
                    {
                        acc1 += e.id();
                        acc2 += p.x + v.x;
                    });
                    REQUIRE(acc1 == e1.id() + e2.id());
                    REQUIRE(acc2 == 16);
                }
            }
        }
        {
            ecs::registry w;
            auto e1 = w.create_entity();
            e1.assign_component<position_c>(1, 2);
            w.for_joined_components<position_c, velocity_c>([](
                ecs::entity, const position_c&, const velocity_c&)
            {
            });
        }
    }
    SECTION("systems") {
        {
            class movement_system : public ecs::system {
            public:
                void process(ecs::registry& owner) override {
                    owner.for_joined_components<position_c, velocity_c>([](
                        ecs::entity, position_c& p, const velocity_c& v)
                    {
                        p.x += v.x;
                        p.y += v.y;
                    });
                }
            };

            ecs::registry w;
            w.add_system<movement_system>(0);

            auto e1 = w.create_entity();
            auto e2 = w.create_entity();

            e1.assign_component<position_c>(1, 2);
            e1.assign_component<velocity_c>(3, 4);
            e2.assign_component<position_c>(5, 6);
            e2.assign_component<velocity_c>(7, 8);

            w.process_all_systems();

            REQUIRE(e1.get_component<position_c>().x == 1 + 3);
            REQUIRE(e1.get_component<position_c>().y == 2 + 4);

            REQUIRE(e2.get_component<position_c>().x == 5 + 7);
            REQUIRE(e2.get_component<position_c>().y == 6 + 8);
        }
        {
            class system_n : public ecs::system {
            public:
                system_n(int& i, int n) : i_(i), n_(n) {}
                void process(ecs::registry&) override {
                    i_ += n_;
                }
            private:
                int& i_;
                int n_;
            };

            int i = 0;
            ecs::registry w;
            w.add_system<system_n>(20, std::ref(i), 2);
            w.add_system<system_n>(10, std::ref(i), 1);
            REQUIRE(i == 0);
            w.process_all_systems();
            REQUIRE(i == 3);
            w.process_systems_below(10);
            REQUIRE(i == 4);
            w.process_systems_above(20);
            REQUIRE(i == 6);
            w.process_systems_below(20);
            REQUIRE(i == 9);
            w.process_systems_above(10);
            REQUIRE(i == 12);

            w.process_systems_below(9);
            w.process_systems_above(21);
            REQUIRE(i == 12);

            w.process_systems_in_range(0, 9);
            w.process_systems_in_range(11, 19);
            w.process_systems_in_range(21, 30);
            REQUIRE(i == 12);

            w.process_systems_in_range(0, 10);
            REQUIRE(i == 13);
            w.process_systems_in_range(10, 19);
            REQUIRE(i == 14);
            w.process_systems_in_range(10, 20);
            REQUIRE(i == 17);
            w.process_systems_in_range(20, 30);
            REQUIRE(i == 19);
            w.process_systems_in_range(10, 20);
            REQUIRE(i == 22);
            w.process_systems_in_range(0, 30);
            REQUIRE(i == 25);
        }
    }
}

TEST_CASE("example") {
    struct position_component {
        float x;
        float y;
        position_component(float nx, float ny)
        : x(nx), y(ny) {}
    };

    struct velocity_component {
        float dx;
        float dy;
        velocity_component(float ndx, float ndy)
        : dx(ndx), dy(ndy) {}
    };

    class movement_system : public ecs_hpp::system {
    public:
        void process(ecs_hpp::registry& owner) override {
            owner.for_joined_components<
                position_component,
                velocity_component
            >([](const ecs_hpp::entity&, position_component& p, const velocity_component& v) {
                p.x += v.dx;
                p.y += v.dy;
            });
        }
    };

    class gravity_system : public ecs_hpp::system {
    public:
        gravity_system(float gravity)
        : gravity_(gravity) {}

        void process(ecs_hpp::registry& owner) override {
            owner.for_each_component<
                velocity_component
            >([this](const ecs_hpp::entity&, velocity_component& v) {
                v.dx += gravity_;
                v.dy += gravity_;
            });
        }
    private:
        float gravity_;
    };

    ecs_hpp::registry world;
    world.add_system<movement_system>(0);
    world.add_system<gravity_system>(1, 9.8f);

    auto entity_one = world.create_entity();
    world.assign_component<position_component>(entity_one, 4.f, 2.f);
    world.assign_component<velocity_component>(entity_one, 10.f, 20.f);

    auto entity_two = world.create_entity();
    entity_two.assign_component<position_component>(4.f, 2.f);
    entity_two.assign_component<velocity_component>(10.f, 20.f);

    world.process_all_systems();
}
