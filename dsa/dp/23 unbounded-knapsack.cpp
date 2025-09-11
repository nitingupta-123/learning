// User function Template for C++

class Solution {
  public:

  /*

  https://www.geeksforgeeks.org/problems/knapsack-with-duplicate-items4201/1


  https://www.youtube.com/watch?v=OgvOZ6OrJoY&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=25

  


  */
  
  /*
  
    -------------------------------------Tabulation------------------------------------------
    int fn(int i, vector<int>& val, vector<int>& wt,int capacity,vector<vector<int>>&dp){
        
        if(i==0){
            if(capacity/wt[i]>0){
                return (capacity/wt[i])*val[i];
            }
            
            
            return 0;
        }
        
        if(dp[i][capacity]!=-1){
            return dp[i][capacity];
        }
        int pick = 0;
        
        if(capacity>=wt[i]){
            pick = val[i] + fn(i,val,wt,capacity-wt[i],dp);
        }
        
        int notPick = fn(i-1, val, wt,capacity,dp);
        
        return dp[i][capacity]=max(notPick, pick);
        
    }
  
    int knapSack(vector<int>& val, vector<int>& wt, int capacity) {
        // code here
        int n = val.size();
        
        vector<vector<int>>dp(n,vector<int>(capacity+1,-1));
        
        return fn(n-1,val,wt,capacity,dp);
    }
    
    */
    
    
    int knapSack(vector<int>& val, vector<int>& wt, int capacity) {
        // code here
        int n = val.size();
        
        // vector<vector<int>>dp(n,vector<int>(capacity+1,0));
        
        vector<int>prev(capacity+1,0);
        
        vector<int>curr(capacity+1,0);

        int i=0;
        
        for(int c =0;c<=capacity;c++){
            if(c/wt[i]>0){
                prev[c] = (c/wt[i])*val[i];
            }
        }
        
        for(int i=1;i<n;i++){
            for(int c =0;c<=capacity;c++){
                int pick = 0;
                if(c>=wt[i]){
                    pick = val[i] + curr[c-wt[i]];
                }
                int notPick = prev[c];
        
                curr[c]=max(notPick, pick);
            }
            prev = curr;
        }
        
        return prev[capacity];
    }
    
    
};