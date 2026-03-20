#pragma once
#include "DBPersistenceStrategy.h"
#include "../shoppingCart.h"
#include <iostream>
using namespace std;

class Redis: public DBPersistenceStrategy{
    public:
    void save(const ShoppingCart& cart){      
        std::cout << "Saving to Redis: "  << std::endl;
    }
};