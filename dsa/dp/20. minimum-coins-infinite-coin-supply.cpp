class Solution {
  public:
  
  /*
    int fn(int i, vector<int> &coins, int sum,vector<vector<int>> &dp){
        
        if(i == 0){
            if(sum % coins[0] == 0) return sum / coins[0];
            return 1e9;
        }

        
        if(dp[i][sum]!=-1)return dp[i][sum];
        
        
        int pick =1e9;
        
        if(sum>=coins[i]){
            pick = 1 + fn(i,coins,sum-coins[i],dp);
        }
        
        int notPick = fn(i-1,coins,sum,dp);
        
        
        return dp[i][sum]=min(notPick, pick);
    }
    
    
    int minCoins(vector<int> &coins, int sum) {
        // code here
        int n = coins.size();
        vector<vector<int>> dp(n,vector<int>(sum+1,-1));
        int ans= fn(n-1, coins, sum,dp);
        if(ans == 1e9)return -1;
        return ans;
        
    }
    
    */
    
    
    int minCoins(vector<int> &coins, int sum) {
        // code here
        int n = coins.size();
        
        vector<int>prev(sum+1,0), curr(sum+1,0);
        
        int i =0;
        
        for(int c = 0; c<=sum;c++){
            
            if(i == 0){
                if(c % coins[i] == 0){
                    prev[c] =  c / coins[i];
                } 
                else {
                    prev[c] =   1e9;
                }
            }
        }
        
        
        for(i=1;i<n;i++){
            for(int c =0;c<=sum;c++){
                int pick =1e9;
        
                if(c>=coins[i]){
                    pick = 1 + curr[c-coins[i]];
                }
                
                int notPick = prev[c];
        
                curr[c]=min(notPick, pick);
            }
            
            prev = curr;
        }
        
        if(prev[sum]==1e9)return -1;
        
        return prev[sum];
        
    }
    
    
    
};