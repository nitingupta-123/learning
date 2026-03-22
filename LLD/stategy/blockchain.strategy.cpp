#include<iostream>
using namespace std;



class SignatureAlgo{
    public:
    virtual string sign(string walletAddress, string nonce)=0;
    virtual ~SignatureAlgo(){

    }
};


class Wallet{
    public:
    virtual void setWalletAddress(string walletAddress)=0;
    virtual string getWalletAddress()=0;
    virtual ~Wallet(){
        
    }
};



class Metamask:public Wallet{
    public:
    string walletAddress;

    Metamask(string walletAddress){
        this->setWalletAddress(walletAddress);
    }

    void setWalletAddress(string walletAddress){
        this->walletAddress = walletAddress;
    }

    string getWalletAddress(){
        return this->walletAddress;
    }
};


class Pera:public Wallet{
    public:
    string walletAddress;
    void setWalletAddress(string walletAddress){
        this->walletAddress = walletAddress;
    }

    string getWalletAddress(){
        return this->walletAddress;
    }
};






class EthereumSigner: public SignatureAlgo{
    public:
    string sign(string walletAddress, string nonce){
        return "EthereumSignerSignature"+walletAddress+nonce;
    }
};


class PolygonSigner: public SignatureAlgo{
    public:
    string sign(string walletAddress, string nonce){
        return "PolygonSignerSignature"+walletAddress+nonce;
    }
};


class Blockchain{

    public:
        SignatureAlgo *sa;
        Wallet *w;
        string blockchainName;
    
    ~Blockchain(){
        delete sa;
        delete w;
    }


    void setBlockChain(string blockchainName){
        this->blockchainName = blockchainName;
    }

    string getBlockchain(){
        return this->blockchainName;
    }


    string signSignature(string walletAddress, string nonce){
        return sa->sign(walletAddress,nonce);
    }

    string getWalletAddress(){
        return w->getWalletAddress();
    }

    void setSignatureAlgo(SignatureAlgo *algo){
        this->sa = algo;
    }

    void setWallet(Wallet *wallet){
        this->w = wallet;
    } 
    
};


class Ethereum: public Blockchain{
    public:
    Ethereum(){
        this->blockchainName = "ethereum";
    }
};

class BlockChainFactory{
    public:
    static SignatureAlgo* SignerFactory(string blockchain){
        if (blockchain == "ethereum") {
            return new EthereumSigner();
        }else if(blockchain == "polygon"){
            return new PolygonSigner();
        }
        else{
            throw logic_error("wrong blockchain");
        }
    }
};




int main(){

    string walletAddress = "0xksjdfbkjfbeqd";
    string nonce = "ascqw123e2";

    Blockchain *eth = new Ethereum();
    Wallet *w =  new Metamask(walletAddress);

    eth->setSignatureAlgo(BlockChainFactory::SignerFactory(eth->getBlockchain()));
    eth->setWallet(w);

    string signature= eth->signSignature(walletAddress,nonce);

    cout<<signature<<" "<<walletAddress<<endl;


    eth->setSignatureAlgo(BlockChainFactory::SignerFactory("polygon"));
    string sig2 = eth->signSignature(walletAddress, nonce);

    cout<<signature<<" "<<walletAddress<<endl;



    delete eth;

}



