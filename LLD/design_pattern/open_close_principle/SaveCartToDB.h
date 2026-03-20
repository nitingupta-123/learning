#pragma once
#include "shoppingCart.h"
#include "strategy/DBPersistenceStrategy.h"
#include <iostream>
using namespace std;

class SaveCartToDB{

    private:
    ShoppingCart* cart;
    
    public:
    SaveCartToDB(ShoppingCart* cart){
        this->cart = cart;
    }

    public:

    void saveToDB(DBPersistenceStrategy& dbPersistenceStrategy){
        dbPersistenceStrategy.save(*cart);
    }
};