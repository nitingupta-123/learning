class Solution {
  public:
  
  /*
  https://www.youtube.com/watch?v=sdE0A2Oxofw&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=9

  https://www.geeksforgeeks.org/problems/number-of-paths0926/1
  */
    int recursiveAndMemoizedSol(int m,int n,vector<vector<int>>&dp){
        
        if(m<0 || n<0) return 0;
        if(m==0 && n==0) return 1;
        if(dp[m][n]!=-1)return dp[m][n];
        int downWays = recursiveAndMemoizedSol(m-1,n,dp);
        int rightWays = recursiveAndMemoizedSol(m,n-1,dp);
        
        return dp[m][n] = downWays + rightWays;
    }
    
    
    int tabulationSol(int m, int n){
        vector<vector<int>>dp(m,vector<int>(n,0));
        
        
        for(int i=0;i<m;i++){
            dp[i][0] = 1;
        }
        
        for(int j=0;j<n;j++){
            dp[0][j] = 1; 
        }

        for(int i=1;i<m;i++){
            for(int j=1;j<n;j++){
                dp[i][j]=dp[i-1][j] + dp[i][j-1];
            }
        }
        return dp[m-1][n-1];
    }
    
    int tabulationSolSpaceOptimized(int m, int n){
        vector<int>currRow(n,0);
        vector<int>lastRow(n,0);
        
        for(int i=0;i<m;i++){
            for(int j=0;j<n;j++){
                
                if(i==0 &&j==0){
                    currRow[0]=1;
                }else{
                    int up =lastRow[j];
                    int down = j<1 ? 0: currRow[j-1];
                    currRow[j] = up + down;
                }
            }
            lastRow = currRow;
        }
        
        return currRow[n-1];
    }
    
    
    int numberOfPaths(int m, int n) {
        // Code Here
        // vector<vector<int>>dp(m+1,vector<int>(n+1,-1));
        // return recursiveAndMemoizedSol(m-1,n-1,dp);
        return tabulationSolSpaceOptimized(m, n);
    }
};
