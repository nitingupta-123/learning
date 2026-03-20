#pragma once
#include <iostream>
#include "product.h"

using namespace std;

class ShoppingCart{

    private:
    vector<Product*> products;
    public:
    void setProducts(Product* product){
        products.push_back(product);
    }


    vector<Product*> getProducts() const {
        return products;
    }

    double getTotalPrice() const {
        double totalPrice = 0;
        for(Product* product : products){
            totalPrice += product->getPrice();
        }
        cout << "Total price: " << totalPrice << endl;
        return totalPrice;
    }

    
};