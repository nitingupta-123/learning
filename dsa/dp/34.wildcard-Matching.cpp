/*

https://leetcode.com/problems/wildcard-matching/

https://www.youtube.com/watch?v=ZmlQ3vgAOMo&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=35

*/

class Solution {
public:


    bool fn(int i,int j,string &s, string &p,vector<vector<int>>&dp){



        if(j==0 && i==0) return true;

        if(i==0){
            for(int k=j;k>=1;k--){
                if(p[k-1]!='*') return false;
            }
            return true;
        }

        if(j==0 || i==0) return false;

        if(dp[i][j]!=-1) return dp[i][j];
        if(p[j-1]=='*'){
            return dp[i][j] = fn(i-1,j-1,s,p,dp) || fn(i-1,j,s,p,dp) || fn(i,j-1,s,p,dp);
        }

        else if(p[j-1]=='?'){
            return dp[i][j] = fn(i-1,j-1,s,p,dp);
        }

        else if(p[j-1]==s[i-1]){
            return dp[i][j] = fn(i-1,j-1,s,p,dp);
        }
        return dp[i][j] = false;





    }


    bool isMatchMemo(string s, string p) {
        int n= s.size();
        int m = p.size();
        vector<vector<int>>dp(n+1,vector<int>(m+1,-1));
        return fn(n,m,s,p,dp);
    }

    bool isMatch(string s, string p) {
        int n= s.size();
        int m = p.size();
        vector<bool> curr(m+1,false),prev(m+1,false);
        int i=0;
        int j=0;

        

        prev[0] = true;


        if(i==0){
            for(int k=1;k<=m;k++){
               prev[k]= prev[k-1] && p[k-1]=='*';
            }
        }



        for(int i=1;i<=n;i++){
            for(int j=1;j<=m;j++){
                if(p[j-1]=='*'){
                    curr[j] = prev[j-1]|| prev[j] || curr[j-1];
                }

                else if(p[j-1]=='?'){
                     curr[j] = prev[j-1];
                }

                else if(p[j-1]==s[i-1]){
                     curr[j] = prev[j-1];
                }
                else{ 
                    curr[j] = false;
                }
            }

            prev = curr;
        }

        return prev[m];
    }
    
    bool isMatch2DArrayDp(string s, string p) {
        int n= s.size();
        int m = p.size();
        vector<vector<int>>dp(n+1,vector<int>(m+1,0));

        int i=0;
        int j=0;

        for(int i=0;i<=n;i++){
            dp[i][0]= false;
        }

         dp[0][0] = true;


        if(i==0){
             for(int k=1;k<=m;k++){
         dp[i][k]= dp[i][k-1] && p[k-1]=='*';
    }
        }



        for(int i=1;i<=n;i++){
            for(int j=1;j<=m;j++){
                if(p[j-1]=='*'){
                    dp[i][j] = dp[i-1][j-1]|| dp[i-1][j] || dp[i][j-1];
                }

                else if(p[j-1]=='?'){
                     dp[i][j] = dp[i-1][j-1];
                }

                else if(p[j-1]==s[i-1]){
                     dp[i][j] = dp[i-1][j-1];
                }
                else{ 
                    dp[i][j] = false;
                }
            }
        }

        return dp[n][m];
    }
};