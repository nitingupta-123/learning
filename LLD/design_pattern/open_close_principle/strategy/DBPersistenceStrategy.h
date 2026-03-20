#pragma once
#include <iostream>
#include "../shoppingCart.h"
using namespace std;

class DBPersistenceStrategy {

    public:
    virtual void save(ShoppingCart const&cart) = 0;
    virtual ~DBPersistenceStrategy(){

    }
};