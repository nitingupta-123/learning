#pragma once
#include "DBPersistenceStrategy.h"
#include "../shoppingCart.h"
#include <iostream>
using namespace std;

class MongoDB: public DBPersistenceStrategy{
    public:
    void save(const ShoppingCart& cart){
        std::cout << "Saving to MongoDB: " << std::endl;
    }
};