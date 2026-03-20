#pragma once
#include <iostream>
#include "product.h"
#include "shoppingCart.h"
#include "ShoppingCartPrinter.h"
#include "strategy/DBPersistenceStrategy.h"
#include "strategy/MongoDB.h"
#include "strategy/FileSystem.h"
#include "strategy/Redis.h"
#include "SaveCartToDB.h"

using namespace std;




int main(){
    

    Product* product1 = new Product("Product 1", 100);
    Product* product2 = new Product("Product 2", 200);
    Product* product3 = new Product("Product 3", 300);

    ShoppingCart* cart = new ShoppingCart();
    cart->setProducts(product1);
    cart->setProducts(product2);
    cart->setProducts(product3);

    ShoppingCartPrinter* printer = new ShoppingCartPrinter();
    printer->printInvoice(*cart);

    SaveCartToDB* saveCartToDB = new SaveCartToDB(cart);

    DBPersistenceStrategy* mongoDB = new MongoDB();
    DBPersistenceStrategy* fileSystem = new FileSystem();
    DBPersistenceStrategy* redis = new Redis();

    saveCartToDB->saveToDB(*mongoDB);
    saveCartToDB->saveToDB(*fileSystem);
    saveCartToDB->saveToDB(*redis);

    delete printer;
    delete saveCartToDB;
    delete cart;
    delete mongoDB;
    delete fileSystem;
    delete redis;
    delete product1;
    delete product2;
    delete product3;
    return 0;
}