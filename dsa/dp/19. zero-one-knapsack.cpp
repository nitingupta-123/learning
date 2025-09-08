// https://www.geeksforgeeks.org/problems/0-1-knapsack-problem0945/1

// https://youtu.be/GqOmJHQZivw?si=LkkfkJBkJjattJ-K

class Solution {
  public:
  
    /*
    <----------------------------------Memoization--------------------------------------->
    int fn(int i, vector<int> &val,vector<int> &wt, int W,vector<vector<int>>&dp){
        
        if(i==0){
            if(W>=wt[i])return val[i];
            return 0;
        }
        
        if(dp[i][W] != -1 ){
            return dp[i][W];
        }
        
        int pick = 0;
        
        if(wt[i]<=W){
            pick += val[i] + fn(i-1,val,wt,W-wt[i],dp);
        }
        
        int notPick = fn(i-1,val,wt,W,dp);
        
        return dp[i][W] = max(pick,notPick);
        
        
    }
    
    

    
    int knapsack(int W, vector<int> &val, vector<int> &wt) {
        // code here
        int n = val.size();
        vector<vector<int>>dp(n,vector<int>(W+1,-1));
        return fn(n-1,val,wt,W,dp);
    }
    */
    
    
    
    
    
    /*
    <----------------------------------Tabulation--------------------------------------->
    int knapsack(int W, vector<int> &val, vector<int> &wt) {
        // code here
        int n = val.size();
        vector<vector<int>>dp(n,vector<int>(W+1,0));
        
        
        vector<int>prev(W+1,0);
        vector<int>curr(W+1,0);
        
        int i=0;
        
        for(int w=0;w<=W;w++){
            
            if(w>=wt[i]){
                prev[w] = val[i];
            }else{
                prev[w] = 0;
            }
        }
        
        
        
        
        for(i=1;i<n;i++){
            for(int w=0;w<=W;w++){
                
                int pick = 0;
        
                if(wt[i]<=w){
                    pick += val[i] + prev[w-wt[i]];
                }
        
                int notPick = prev[w];
        
                curr[w] = max(pick,notPick);
         
            }
            prev = curr;
        }
        return prev[W];
        
    }
        */
        
    int knapsack(int W, vector<int> &val, vector<int> &wt) {
        // code here
        int n = val.size();

        
        vector<int>prev(W+1,0);

        int i=0;
        
        for(int w=0;w<=W;w++){
            
            if(w>=wt[i]){
                prev[w] = val[i];
            }else{
                prev[w] = 0;
            }
        }
        
        
        
        
        for(i=1;i<n;i++){
            for(int w=W;w>=0;w--){
                
                int pick = 0;
        
                if(wt[i]<=w){
                    pick += val[i] + prev[w-wt[i]];
                }
        
                int notPick = prev[w];
        
                prev[w] = max(pick,notPick);
         
            }
        }
        return prev[W];
        
    }
        
        
};