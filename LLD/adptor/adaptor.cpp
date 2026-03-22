#include<iostream>
using namespace std;


struct MoralisResponse{
    string message;
    int httpStatus;
};

struct Response{
    string msg;
    int status;
};

class ApiResponse{
    public:
    virtual Response getApiResponse()=0;
};


class ApiResponseAdaptor{
    private: 
    Response resp;
    public:
    Response convertResponse(MoralisResponse &mResp){
        
        this->resp.msg = mResp.message;
        this->resp.status = mResp.httpStatus;
        return resp;
    }

};

class MoralisApiResponse:public ApiResponse {

    private:
    MoralisResponse response;
    ApiResponseAdaptor *adp=nullptr;

    public:
    MoralisApiResponse(ApiResponseAdaptor *adp) {
        this->adp = adp;
    }

    ~MoralisApiResponse(){
        delete adp;
    }

    MoralisResponse fetchApiData(){
        

        this->response.httpStatus = 200;
        this->response.message = "Data fetch sucessfully!!";

        return this->response;

    }

    Response getApiResponse(){
        fetchApiData();
        return adp->convertResponse(this->response);
    }

};




int main(){
    ApiResponseAdaptor* adp = new ApiResponseAdaptor();
    ApiResponse *resp = new MoralisApiResponse(adp);    
    Response data= resp->getApiResponse();

    cout<<data.msg<<" "<<data.status<<endl;
    delete resp;


}