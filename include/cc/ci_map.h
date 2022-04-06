
#pragma once
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <string_view>
namespace cc { struct ci_hash { size_t operator()(std::string key) const { std::size_t seed = 0;  for (auto c : key) { boost::hash_combine(seed, c);  } return seed; } }; struct ci_key_eq { bool operator()(std::string l, std::string r) const { return boost::iequals(l, r); } }; using ci_map = std::unordered_multimap<std::string, std::string, ci_hash, ci_key_eq>;}