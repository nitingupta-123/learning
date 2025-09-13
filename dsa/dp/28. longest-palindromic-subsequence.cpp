// User function Template for C++
#include<bits/stdc++.h>
class Solution {
  public:
  
          

    int longestPalinSubseq(string &s) {
        // code here
        string t = s;
        reverse(t.begin(),t.end());
        
        int n = s.size();
        int m = t.size();
        
        vector<int>curr(m+1,0),prev(m+1,0);
        
        for(int i=0;i<=n;i++){
            for(int j=0;j<=m;j++){
                
                if(i==0 || j==0){
                    prev[j] =0;
                }
                else if(s[i-1]==t[j-1]){
                    curr[j] = 1+ prev[j-1];
                }
                else{
                    curr[j] = max(prev[j],curr[j-1]);
                }
                
            }
            
            prev = curr;
        }
        
        return prev[m];
        
    }
    
    
    /*
    int longestPalinSubseq(string &s) {
        // code here
        string t = s;
        reverse(t.begin(),t.end());
        
        int n = s.size();
        int m = t.size();
        
        vector<vector<int>>dp(n+1,vector<int>(m+1,0));
        
        for(int i=0;i<=n;i++){
            for(int j=0;j<=m;j++){
                
                if(i==0 || j==0){
                    dp[i][j] =0;
                }
                else if(s[i-1]==t[j-1]){
                    dp[i][j] = 1+ dp[i-1][j-1];
                }
                else{
                    dp[i][j] = max(dp[i-1][j],dp[i][j-1]);
                }
                
            }
        }
        
        return dp[n][m];
        
    }
    */
};