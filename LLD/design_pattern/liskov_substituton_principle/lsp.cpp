#include<iostream>
#include<vector>
#include<stdexcept>
using namespace std;

class NonWithdrawableAccount{
    
    public:
    
    void virtual deposit(double amount) =0;
    
    virtual ~NonWithdrawableAccount(){}
    
};


class WithdrawableAccount: public NonWithdrawableAccount{
    public:
    void virtual withdraw(double amount)=0;

    virtual ~WithdrawableAccount(){}
};

class SavingsAccount: public WithdrawableAccount{

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

class CurrentAccount: public WithdrawableAccount{
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

class FixedDepositAccount: public NonWithdrawableAccount{
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
};

class BankClient{
    private:
    vector<NonWithdrawableAccount*> nonWithdrawableAccounts;
    vector<WithdrawableAccount*> withdrawableAccounts;

    public:
    BankClient(vector<NonWithdrawableAccount*> nonWithdrawableAccounts, vector<WithdrawableAccount*> withdrawableAccounts){
        this->nonWithdrawableAccounts = nonWithdrawableAccounts;
        this->withdrawableAccounts = withdrawableAccounts;
    }

    void processTransaction(){

        for(WithdrawableAccount* account : this->withdrawableAccounts){
            try{

                account->withdraw(50);
                account->deposit(100);
            }catch(const logic_error& e){
                cout << e.what() << endl;
            }
        }
        
        for(NonWithdrawableAccount* account : this->nonWithdrawableAccounts){
            try{
                account->deposit(100);
            }catch(const logic_error& e){
                cout << e.what() << endl;
            }
        }

    }
};


int main(){
    vector<NonWithdrawableAccount*> nonWithdrawableAccounts;
    vector<WithdrawableAccount*> withdrawableAccounts;

    nonWithdrawableAccounts.push_back(new FixedDepositAccount(1000));
    withdrawableAccounts.push_back(new SavingsAccount(1000));
    withdrawableAccounts.push_back(new CurrentAccount(1000));
    
    BankClient bankClient(nonWithdrawableAccounts, withdrawableAccounts);
    bankClient.processTransaction();
    return 0;
}