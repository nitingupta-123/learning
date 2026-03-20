#pragma once
#include<iostream>
#include<vector>
#include<stdexcept>
using namespace std;

class Account{
    
    public:
    
    void virtual deposit(double amount) =0;
    
    void virtual withdraw(double amount)=0;

    virtual ~Account(){}
    
};

class SavingsAccount: public Account{

    private:
    double balance;

    public:
    SavingsAccount(double balance){
        this->balance = balance;
    }

    double getBalance(){
        return balance;
    }

    void deposit (double amount){
        balance += amount;
        cout << "Depositing " << amount << " into savings account" << endl;
    }
    void withdraw(double amount){
        if(balance < amount){
            throw new logic_error("Insufficient balance");
        }
        balance -= amount;
        cout << "Withdrawing " << amount << " from savings account" << endl;
    }
};

class CurrentAccount: public Account{
    private:
    double balance;

    public:
    CurrentAccount(double balance){
        this->balance = balance;
    }

    void deposit(double amount){
        balance += amount;
        cout << "Depositing " << amount << " into current account" << endl;
    }   
    void withdraw(double amount){
        if(balance < amount){
            throw new logic_error("Insufficient balance");
        }
        balance -= amount;
        cout << "Withdrawing " << amount << " from current account" << endl;
    }
};

class FixedDepositAccount: public Account{
    private:
    double balance;

    public:
    FixedDepositAccount(double balance){
        this->balance = balance;
    }

    void deposit(double amount){
        balance += amount;
        cout << "Depositing " << amount << " into fixed deposit account" << endl;
    }
    void withdraw(double amount){
        throw new logic_error("Withdrawal is not allowed for fixed deposit account");
    }
};

class BankClient{
    private:
    vector<Account*> accounts;

    public:
    BankClient(vector<Account*> accounts){
        this->accounts = accounts;
    }

    void processTransaction(){
        for(Account* account : this->accounts){
            try{
                account->deposit(100);
                account->withdraw(50);
            }catch(logic_error& e){
                cout << e.what() << endl;
            }
        }
    }
};


int main(){
    vector<Account*> accounts;
    accounts.push_back(new SavingsAccount(1000));
    accounts.push_back(new CurrentAccount(1000));
    accounts.push_back(new FixedDepositAccount(1000));
    BankClient bankClient(accounts);
    bankClient.processTransaction();
    return 0;
}