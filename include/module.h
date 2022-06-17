#pragma once
#include "cc/json.hh"
using namespace std;
static int RES_INIT = orm::InitializationOrm();

class User;
class List;

Class(User)
bool is; int age; double weight;
string name; bool state;
FUCKJSON(User, is, age, weight, name, state)

Class(List)
box<User> user;
vector<User> userList;
FUCKJSON(List, user, userList)

class Tab;
class Type;

Class(Type)
uint8_t id; text<10> language; double bigBlob;
box<Tab> tab;
FUCKJSON(Type, id, language, bigBlob, tab)

Class(Tab)
uint32_t id; bool ok; text<15> name; tm date = orm::now();
box<Type> type;
FUCKJSON(Tab, id, ok, name, date, type)
