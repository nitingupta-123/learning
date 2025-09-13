

/*

https://www.youtube.com/watch?v=-zI4mrF2Pb4&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=27&t=689s

https://www.naukri.com/code360/problems/print-longest-common-subsequence_8416383?leftPanelTabValue=PROBLEM
*/


string findLCS(int n, int m,string &s1, string &s2){
	// Write your code here.

	vector<vector<int>>dp(n+1,vector<int>(m+1,0));

	for(int i=0;i<=n;i++){
		for(int j=0;j<=m;j++){

			if(i==0 || j==0){
				dp[i][j] =0;
			}

			else if(s1[i-1]==s2[j-1]){
				dp[i][j] = 1+ dp[i-1][j-1];
			}
			else{
				dp[i][j] = max(dp[i-1][j],dp[i][j-1]);
			}

		}
	}

	for(int i=0;i<=n;i++){
		for(int j=0;j<=m;j++){

			// cout<<dp[i][j]<<" ";

		}//cout<<endl;
	}
	int i = n;
	int j = m;

	/*

	    c b b c a d
	  0 0 0 0 0 0 0 
	a 0 0 0 0 0 1 1 
	b 0 0 1 1 1 1 1 
	a 0 0 1 1 1 2 2 
	b 0 0 1 2 2 2 2 
	a 0 0 1 2 2 3 3 
	

	*/
	string ans(dp[i][j],'$');
	int ind = dp[i][j] - 1;

	while(i>0 && j>0){

		if(s1[i-1]==s2[j-1]){
			ans[ind] = s1[i-1];
			ind--;
			j--;i--;
		}
		else if(dp[i-1][j]>dp[i][j-1]){
			i--;
		}else{
			j--;
		}
	}

	return ans;


}