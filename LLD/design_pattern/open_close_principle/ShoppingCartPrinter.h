#pragma once
#include "shoppingCart.h"
#include "product.h"
#include <iostream>
using namespace std;

class ShoppingCartPrinter{

    public:
    void printInvoice(const ShoppingCart& cart){
        cout << "Printing invoice for cart: " << endl;
        for(Product* product : cart.getProducts()){
            cout << product->getName() << " - " << product->getPrice() << endl;
        }
        cout << "Total price: " << cart.getTotalPrice() << endl;
    }

};