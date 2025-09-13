/*

https://www.youtube.com/watch?v=xPBLEj41rFU&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=32

https://www.geeksforgeeks.org/problems/form-a-palindrome2544/1


*/

class Solution {
  public:
    int findMinInsertions(string &s) {
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
        
        return n-prev[m];
    }
};