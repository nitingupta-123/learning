#pragma once
#include <iostream>
using namespace std;

class Product{
    private:
    string name;
    double price;

    public:
    Product(const string& name, double price){
        this->name = name;
        this->price = price;
    }

    string getName(){
        return this->name;
    }
    double getPrice(){
        return this->price;
    }

};