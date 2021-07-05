#pragma once
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_map>
namespace crow {
  struct ci_hash {
	size_t operator()(const std::string& key) const {
	  std::size_t seed=0;
	  std::locale locale;
	  for (auto c:key) {
		boost::hash_combine(seed,std::toupper(c,locale));
	  }
	  return seed;
	}
	size_t operator()(const char*key) const {
	  std::size_t seed=0;
	  std::locale locale;
	  for (int i=0;key[i];++i) {
		boost::hash_combine(seed,std::toupper(key[i],locale));
	  }
	  return seed;
	}
  };
  struct ci_key_eq {
	bool operator()(const std::string& l,const std::string& r) const {
	  return boost::iequals(l,r);
	}
	bool operator()(const char*l,const char*r) const {
	  return boost::iequals(l,r);
	}
	bool operator()(const char*l,const std::string& r) const {
	  return boost::iequals(l,r);
	}
  };
  using ci_map=std::unordered_multimap<std::string,std::string,ci_hash,ci_key_eq>;
}
