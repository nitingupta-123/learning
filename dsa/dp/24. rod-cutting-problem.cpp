// User function Template for C++

/*
https://www.geeksforgeeks.org/problems/rod-cutting0840/1

https://www.youtube.com/watch?v=mO8XpGoJwuo&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=26



*/


class Solution {
  public:
  
  /*
   converted the problem in 0/1 unbounded knapsack problem
  */
    int fn(int i, int rodLen,vector<int> &wt,vector<int> &price,vector<vector<int>>&dp){
        
        if(i==0){
            return (rodLen/wt[i])*price[i];
        }
        
        if(dp[i][rodLen]!=-1){
            return dp[i][rodLen];
        }
        
        
        int pick = 0;
        
        if(rodLen>=wt[i]){
            pick = price[i] + fn(i,rodLen-wt[i],wt,price,dp);
        }
        
        int notPick = fn(i-1,rodLen,wt,price,dp);
        
        return dp[i][rodLen]= max(notPick, pick);
    
    }
    int cutRod(vector<int> &price) {
        // code here
        int n = price.size();
        int rodLen = n;
        vector<int> wt(n,0);
        
        for(int i=0;i<n;i++){
            wt[i] = i+1;
        }
        vector<vector<int>>dp(n,vector<int>(n+1,-1));
        return fn(n-1, rodLen,wt, price,dp);
    }
};